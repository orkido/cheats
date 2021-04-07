#include "Starcraft2.hpp"
#include "Process.hpp"
#include "Offsets.hpp"

#include "inject/data_interface.h"
#include "Overlay.hpp"

#include <BlackBone/PE/PEImage.h>
#include <BlackBone/Process/Process.h>
#include <BlackBone/DriverControl/DriverControl.h>
#include <BlackBone/Patterns/PatternSearch.h>

EXCEPTION_POINTERS;

using namespace blackbone;

#define GAME_NAME "Starcraft 2"
#define PROCESS_EXE_NAME "SC2_x64.exe"

namespace Starcraft2 {

uint32_t starcraft2_config_refresh_rate;
std::atomic<bool> starcraft2_running = false;

struct DllOffsets {
	ptr_t search_string_offset = 0;
	ptr_t sc2_data_offset = 0;
	ptr_t asm_wrap_NtQueryInformationThread_offset = 0;
};

struct DllOffsets dll_exports(std::wstring& dll_path) {
	// Parse the DLL file for exported function offsets
	// Relative addresses to dll base
	ptr_t entryPoint = 0;
	struct DllOffsets result;

	auto img = blackbone::pe::PEImage();
	img.Load(dll_path);
	entryPoint = img.entryPoint(img.imageBase());
	entryPoint -= img.imageBase();

	pe::vecExports exports;
	img.GetExports(exports);
	for (const auto& _export : exports) {
		// C export
		if (_export.name.find(STRINGIFY(CONST_OFFSET_NAME)) != std::string::npos)
			result.search_string_offset = _export.RVA;
		// C export
		if (_export.name.find(STRINGIFY(GLOBAL_SC2DATA_NAME)) != std::string::npos)
			result.sc2_data_offset = _export.RVA;
		// C export
		if (_export.name.find("asm_wrap_NtQueryInformationThread", 0) != std::string::npos)
			result.asm_wrap_NtQueryInformationThread_offset = _export.RVA;
	}
	if (!result.search_string_offset || !result.sc2_data_offset || !result.asm_wrap_NtQueryInformationThread_offset)
		exit(1);

	return result;
}

ptr_t inject(std::wstring& dll_path, void* driverctl, uint32_t pid) {
	// Inject DLL and hook WINAPI function

	struct DllOffsets dll_offsets = dll_exports(dll_path);
	DriverControl* _driverctl = (DriverControl*)driverctl;
	NTSTATUS status = 0;
	Process proc;
	status = proc.Attach(pid);
	if (!NT_SUCCESS(status))
		exit(1);

	// Dynamically resolved function address of NtQueryInformationThread is stored there
	ptr_t fn_NtQueryInformationThread = proc.modules().GetMainModule()->baseAddress + Offsets::func_NtQueryInformationThread;
	ptr_t NtQueryInformationThread_address;
	status = _driverctl->ReadMem(pid, fn_NtQueryInformationThread, proc.core().isWow64() ? 4 : 8, &NtQueryInformationThread_address);
	if (!NT_SUCCESS(status) || !NtQueryInformationThread_address)
		exit(1);

	ptr_t NtQueryInformationThread_ntdll_address;
	{
		auto result = proc.modules().GetNtdllExport("NtQueryInformationThread");
		if (!result.success())
			exit(1);
		NtQueryInformationThread_ntdll_address = result.result().procAddress;
	}

	// Compare the above resolved function pointer with the dynamically resolved address
	// If the DLL has already been injected or the offset is outdated, this is false
	bool function_hookable = NtQueryInformationThread_ntdll_address == NtQueryInformationThread_address;

	// Inject if we can hook the function and set injected_dll_base
	ptr_t injected_dll_base = 0;
	{
		if (function_hookable) {
			// There are multiple working methods to inject the DLL
			// proc.mmap().MapImage();
			// status = _driverctl->MmapDll(pid, dll_path, KMmapFlags::KNoThreads | KMmapFlags::KNoTLS);
			status = _driverctl->MmapDll(pid, dll_path, KMmapFlags::KNoExecution);
			// status = _driverctl->MmapDll(pid, dll_path, KMmapFlags::KNoFlags | KMmapFlags::KNoExceptions | KMmapFlags::KNoTLS | KMmapFlags::KNoSxS | KMmapFlags::KNoThreads);
			// status = _driverctl->InjectDll(pid, std::wstring(L"\\DosDevices\\") + dll_path, InjectType::IT_Apc);
			// status = _driverctl->InjectDll(pid, std::wstring(L"\\DosDevices\\") + dll_path, InjectType::IT_Thread);

			if (!NT_SUCCESS(status))
				exit(1);
		}

		// Search for binary pattern to find the base address of our DLL
		PatternSearch ps(CONST_OFFSET_VALUE);
		std::vector<ptr_t> results;
		ps.SearchRemoteWhole(proc, false, 0, results, 1);
		if (results.empty())
			// The NtQueryInformationThread offset must be wrong
			exit(1);
		ptr_t search_string_address = results.front();
		injected_dll_base = search_string_address - dll_offsets.search_string_offset;
	}

	// Hook execution to our injected NtQueryInformationThread stub if is hookable
	if (function_hookable) {
		ptr_t asm_wrap_NtQueryInformationThread_address = injected_dll_base + dll_offsets.asm_wrap_NtQueryInformationThread_offset;

		// Assert the anti-anti-cheat
		uint8_t first_byte_asm_wrap;
		uint8_t first_byte_NtQueryInformationThread;
		_driverctl->ReadMem(pid, asm_wrap_NtQueryInformationThread_address, 1, &first_byte_asm_wrap);
		_driverctl->ReadMem(pid, NtQueryInformationThread_address, 1, &first_byte_NtQueryInformationThread);
		if (first_byte_asm_wrap != first_byte_NtQueryInformationThread)
			exit(1);

		// Redirect execution
		status = _driverctl->WriteMem(pid, fn_NtQueryInformationThread, proc.core().isWow64() ? 4 : 8, &asm_wrap_NtQueryInformationThread_address);

		if (!NT_SUCCESS(status))
			exit(1);
	}

	status = proc.Detach();
	if (!NT_SUCCESS(status))
		exit(1);

	return injected_dll_base;
}

void starcraft2(void* driverctl, uint32_t pid, void* shared_ptr_overlay1, void* shared_ptr_overlay2) {
	DriverControl* _driverctl = (DriverControl*)driverctl;

	auto dll_path = Utils::GetExeDirectory() + L"\\starcraft2_inject.dll";

	ptr_t injected_dll_base = inject(dll_path, driverctl, pid);

	ptr_t SC2Data_address = injected_dll_base + dll_exports(dll_path).sc2_data_offset;
	printf("sc2_data %p\n", (char*) SC2Data_address);

	// Overlay work loop
	struct SC2Data* sc2_data = new struct SC2Data;

	// Attach to the process
	GameProcess process{ driverctl, pid };
	if (process.process_base_exe == 0) {
		printf(GAME_NAME "(%u) Access denied, did you implement EAC bypass?\n", pid);
		return;
	}
	// Check if the offsets are valid for this game version
	if (process.check_version(Offsets::timestamp, Offsets::checksum)) {
		Overlay sc2_overlay(shared_ptr_overlay1, shared_ptr_overlay2);

		// The heart of the cheat is simple, repeat until the process dies
		uint64_t last_update = 0;
		while (process.heartbeat() && starcraft2_running) {
			uint64_t curr_time = static_cast<uint64_t>(get_time() * 1000.0);
			uint64_t time_diff = curr_time - last_update;
			if (time_diff >= starcraft2_config_refresh_rate) {
				last_update = curr_time;

				// First read the units_length field because reading the whole data field takes a long time and wastes time
				if (process.read(SC2Data_address + ((ptr_t)&sc2_data->units_length - (ptr_t)sc2_data), sc2_data->units_length)) {
					if (process.read_raw(SC2Data_address, sc2_data, (ptr_t)&sc2_data->units[sc2_data->units_length] - (ptr_t)sc2_data)) {
						sc2_overlay.update_overlay(sc2_data);
					}
				}
			} else {
				sleep(starcraft2_config_refresh_rate - time_diff);
			}
		}
	}

	delete sc2_data;
}

int main_starcraft2(void* driverctl, void* shared_ptr_overlay1, void* shared_ptr_overlay2) {
	init_time();
	// Track the last attached process id to prevent reattaching accidentally
	uint32_t last_process_id = ~0U;
	while (starcraft2_running) {
		bool seen_last_process_id = false;
		ProcessEntry entry;
		for (ProcessEnumerator processes{}; processes.next(entry); ) {
			// Ignore the last process id until it has gone away
			if (entry.id == last_process_id) {
				seen_last_process_id = true;
				continue;
			}
			// Find the process
			if (!wcscmp(entry.name, L"" PROCESS_EXE_NAME)) {
				starcraft2(driverctl, entry.id, shared_ptr_overlay1, shared_ptr_overlay2);
				last_process_id = entry.id;
				seen_last_process_id = true;
				break;
			}
		}
		// Clear the last process id if it hasn't been seen
		if (!seen_last_process_id) {
			last_process_id = ~0;
		}
		// Wait before looking again
		sleep(100);
	}
	return 0;
}
}
