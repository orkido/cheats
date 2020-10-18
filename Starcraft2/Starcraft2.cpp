#include "Starcraft2.hpp"
#include "Process.hpp"
#include "GameContext.hpp"
#include "Offsets.hpp"

#include <BlackBone/PE/PEImage.h>
#include <BlackBone/Process/Process.h>
#include <BlackBone/DriverControl/DriverControl.h>
#include <BlackBone/Patterns/PatternSearch.h>

EXCEPTION_POINTERS;

using namespace blackbone;

#define GAME_NAME "Starcraft 2"
#define PROCESS_EXE_NAME "SC2.exe"

namespace Starcraft2 {

uint32_t starcraft2_config_refresh_rate;
std::atomic<bool> starcraft2_running = false;

void starcraft2(void* driverctl, uint32_t pid) {
	DriverControl* _driverctl = (DriverControl*)driverctl;

	auto dll_path =  Utils::GetExeDirectory() + L"\\starcraft2_inject.dll";

	// Relative addresses to dll base
	ptr_t entryPoint = 0;
	ptr_t search_string_offset = 0;
	ptr_t hook_NtQueryInformationThread_offset = 0;
	ptr_t winapi_NtQueryInformationThread_offset = 0;
	ptr_t NtQueryInformationThread_address = 0;

	auto img = blackbone::pe::PEImage();
	img.Load(dll_path);
	entryPoint = img.entryPoint(img.imageBase());
	entryPoint -= img.imageBase();

	pe::vecExports exports;
	img.GetExports(exports);
	for (const auto& _export : exports) {
		// C++ export
		if (_export.name.find("search_string", 0) != std::string::npos)
			search_string_offset = _export.RVA;
		// C export
		if (_export.name.find("hook_NtQueryInformationThread", 0) != std::string::npos)
			hook_NtQueryInformationThread_offset = _export.RVA;
		// C++ export
		if (_export.name.find("winapi_NtQueryInformationThread", 0) != std::string::npos)
			winapi_NtQueryInformationThread_offset = _export.RVA;
	}

	NTSTATUS status = 0;
	Process proc;
	status = proc.Attach(pid);
	if (!NT_SUCCESS(status))
		exit(1);

	ptr_t hook_NtQueryInformationThread_target_ptr = proc.modules().GetMainModule()->baseAddress + 0x2d49884;
	status = _driverctl->ReadMem(pid, hook_NtQueryInformationThread_target_ptr, 4, &NtQueryInformationThread_address);
	if (!NT_SUCCESS(status))
		exit(1);

	if (!search_string_offset || !hook_NtQueryInformationThread_offset || !winapi_NtQueryInformationThread_offset || !NtQueryInformationThread_address)
		exit(1);

	PatternSearch ps("alsfkdjwendibegniubvfdnkxc");
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
	ptr_t injected_dll_base = results.front() - search_string_offset;
	ptr_t hook_NtQueryInformationThread_ptr = injected_dll_base + hook_NtQueryInformationThread_offset;

	// Init function pointers
	// _driverctl->WriteMem(pid, injected_dll_base + winapi_NtQueryInformationThread_offset, 4, &NtQueryInformationThread_address);

	status = _driverctl->WriteMem(pid, hook_NtQueryInformationThread_target_ptr, 4, &hook_NtQueryInformationThread_ptr);
	if (!NT_SUCCESS(status))
		exit(1);

	status = proc.Detach();
	if (!NT_SUCCESS(status))
		exit(1);

	return;

	// Attach to the process
	GameProcess process{ driverctl, pid };
	if (process.process_base_exe == 0) {
		printf(GAME_NAME "(%u) Access denied, did you implement EAC bypass?\n", pid);
		return;
	}
	// Check if the offsets are valid for this game version
	if (process.check_version(Offsets::timestamp, Offsets::checksum)) {
		// The heart of the cheat is simple, repeat until the process dies
		GameContext context(process);
		while (process.heartbeat() && starcraft2_running) {
			static uint64_t last_update = 0;
			uint64_t curr_time = static_cast<uint64_t>(get_time() * 1000.0);
			uint64_t time_diff = curr_time - last_update;
			if (time_diff >= starcraft2_config_refresh_rate) {
				last_update = curr_time;

				context.tick();
			} else {
				sleep(starcraft2_config_refresh_rate - time_diff);
			}
		}
	}
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
