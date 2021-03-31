#include "Starcraft2.hpp"
#include "Process.hpp"
#include "Offsets.hpp"

#include "inject/data_interface.h"

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

void update_overlay(struct SC2Data sc2_data);

void starcraft2(void* driverctl, uint32_t pid) {
	DriverControl* _driverctl = (DriverControl*)driverctl;

	auto dll_path =  Utils::GetExeDirectory() + L"\\starcraft2_inject.dll";
	uint8_t target_ptr_size = 8; // 64-bit target

	// Parse the DLL file for exported function offsets
	// Relative addresses to dll base
	ptr_t entryPoint = 0;
	ptr_t search_string_offset = 0;
	ptr_t sc2_data_offset = 0;
	ptr_t asm_wrap_NtQueryInformationThread_offset = 0;
	ptr_t NtQueryInformationThread_address = 0;
	{
		auto img = blackbone::pe::PEImage();
		img.Load(dll_path);
		entryPoint = img.entryPoint(img.imageBase());
		entryPoint -= img.imageBase();

		pe::vecExports exports;
		img.GetExports(exports);
		for (const auto& _export : exports) {
			// C export
			if (_export.name.find(STRINGIFY(CONST_OFFSET_NAME)) != std::string::npos)
				search_string_offset = _export.RVA;
			// C export
			if (_export.name.find(STRINGIFY(GLOBAL_SC2DATA_NAME)) != std::string::npos)
				sc2_data_offset = _export.RVA;
			// C export
			if (_export.name.find("asm_wrap_NtQueryInformationThread", 0) != std::string::npos)
				asm_wrap_NtQueryInformationThread_offset = _export.RVA;
		}
		if (!search_string_offset || !sc2_data_offset || !asm_wrap_NtQueryInformationThread_offset)
			exit(1);
	}

	NTSTATUS status = 0;
	Process proc;
	status = proc.Attach(pid);
	if (!NT_SUCCESS(status))
		exit(1);

	ptr_t func_NtQueryInformationThread = proc.modules().GetMainModule()->baseAddress + Offsets::func_NtQueryInformationThread;
	status = _driverctl->ReadMem(pid, func_NtQueryInformationThread, target_ptr_size, &NtQueryInformationThread_address);
	if (!NT_SUCCESS(status) || !NtQueryInformationThread_address)
		exit(1);

	// Inject if not already injected and set injected_dll_base
	ptr_t injected_dll_base = 0;
	{
		PatternSearch ps(CONST_OFFSET_VALUE);
		std::vector<ptr_t> results;

		ps.SearchRemoteWhole(proc, false, 0, results);

		if (results.empty()) {
			status = _driverctl->MmapDll(pid, dll_path, KMmapFlags::KNoExecution);
			// status = _driverctl->MmapDll(pid, dll_path, KMmapFlags::KNoFlags | KMmapFlags::KNoExceptions | KMmapFlags::KNoTLS | KMmapFlags::KNoSxS | KMmapFlags::KNoThreads);
			// status = _driverctl->InjectDll(pid, std::wstring(L"\\DosDevices\\") + dll_path, InjectType::IT_Apc);
			// status = _driverctl->InjectDll(pid, std::wstring(L"\\DosDevices\\") + dll_path, InjectType::IT_Thread);

			if (!NT_SUCCESS(status))
				exit(1);

			ps.SearchRemoteWhole(proc, false, 0, results);

			if (results.empty())
				exit(1);
		}
		ptr_t search_string_address = results.front();
		injected_dll_base = search_string_address - search_string_offset;
	}

	// Redirect execution to our injected NtQueryInformationThread stub
	{
		ptr_t asm_wrap_NtQueryInformationThread_address = injected_dll_base + asm_wrap_NtQueryInformationThread_offset;

		// Assert the anti-anti-cheat
		uint8_t first_byte_asm_wrap;
		uint8_t first_byte_NtQueryInformationThread;
		_driverctl->ReadMem(pid, asm_wrap_NtQueryInformationThread_address, 1, &first_byte_asm_wrap);
		_driverctl->ReadMem(pid, NtQueryInformationThread_address, 1, &first_byte_NtQueryInformationThread);
		if (first_byte_asm_wrap != first_byte_NtQueryInformationThread)
			exit(1);

		// Redirect execution
		status = _driverctl->WriteMem(pid, func_NtQueryInformationThread, target_ptr_size, &asm_wrap_NtQueryInformationThread_address);

		if (!NT_SUCCESS(status))
			exit(1);
	}

	status = proc.Detach();
	if (!NT_SUCCESS(status))
		exit(1);

	printf("sc2_data %p\n", (char*) injected_dll_base + sc2_data_offset);

	ptr_t SC2Data_address = injected_dll_base + sc2_data_offset;
	struct SC2Data sc2_data;

	// Attach to the process
	GameProcess process{ driverctl, pid };
	if (process.process_base_exe == 0) {
		printf(GAME_NAME "(%u) Access denied, did you implement EAC bypass?\n", pid);
		return;
	}
	// Check if the offsets are valid for this game version
	if (process.check_version(Offsets::timestamp, Offsets::checksum)) {
		// The heart of the cheat is simple, repeat until the process dies
		uint64_t last_update = 0;
		while (process.heartbeat() && starcraft2_running) {
			uint64_t curr_time = static_cast<uint64_t>(get_time() * 1000.0);
			uint64_t time_diff = curr_time - last_update;
			if (time_diff >= starcraft2_config_refresh_rate) {
				last_update = curr_time;

				if (process.read(SC2Data_address, sc2_data)) {
					update_overlay(sc2_data);
				}
			} else {
				sleep(starcraft2_config_refresh_rate - time_diff);
			}
		}
	}
}

void update_overlay(struct SC2Data sc2_data) {

}

int main_starcraft2(void* driverctl) {
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
				starcraft2(driverctl, entry.id);
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
