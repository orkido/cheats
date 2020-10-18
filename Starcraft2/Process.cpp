#include "Process.hpp"
#include "Offsets.hpp"

#include <memory>
#include <cinttypes>

#include <Windows.h>
#include <winternl.h>

#include <vector>
#include <BlackBone/DriverControl/DriverControl.h>
using namespace blackbone;

#define GAME_NAME "Starcraft 2"
#define PROCESS_EXE_NAME "SC2_x64.exe"

namespace Starcraft2
{
static LARGE_INTEGER TIME_START;
static LARGE_INTEGER TIME_FREQ;

void init_time() {
	QueryPerformanceCounter(&TIME_START);
	QueryPerformanceFrequency(&TIME_FREQ);
}
double get_time() {
	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);
	return static_cast<double>(time.QuadPart - TIME_START.QuadPart) / static_cast<double>(TIME_FREQ.QuadPart);
}

void sleep(uint32_t ms) {
	SleepEx(ms, TRUE);
}

void mouse_move(int dx, int dy) {
	INPUT input{ INPUT_MOUSE };
	input.mi = MOUSEINPUT{ dx, dy, 0, MOUSEEVENTF_MOVE, 0, 0 };
	SendInput(1, &input, sizeof(INPUT));
}

//----------------------------------------------------------------

ProcessEnumerator::ProcessEnumerator() {
	using NtQuerySystemInformationFn = NTSTATUS WINAPI(IN SYSTEM_INFORMATION_CLASS, OUT PVOID, IN ULONG, OUT PULONG);
	const auto NtQuerySystemInformation = reinterpret_cast<NtQuerySystemInformationFn*>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQuerySystemInformation"));
	ULONG return_length;
	while (NtQuerySystemInformation(SystemProcessInformation, buffer.get(), static_cast<ULONG>(buffer_len), &return_length) < 0) {
		buffer = std::unique_ptr<uint8_t[]>(new uint8_t[return_length]);
		buffer_len = return_length;
	}
}
bool ProcessEnumerator::next(ProcessEntry& entry) {
	if (next_offset >= buffer_len) {
		return false;
	}
	const auto pi = reinterpret_cast<const SYSTEM_PROCESS_INFORMATION*>(buffer.get() + next_offset);
	next_offset += pi->NextEntryOffset != 0 ? pi->NextEntryOffset : buffer_len - next_offset;

	entry.id = static_cast<uint32_t>((size_t)pi->UniqueProcessId);
	memset(&entry.name, 0, sizeof(entry.name));
	memcpy(&entry.name, pi->ImageName.Buffer, min(pi->ImageName.Length, sizeof(entry.name) - 1));

	return true;
}

//----------------------------------------------------------------

GameProcess::GameProcess(void* driverctl, uint32_t pid) : pid(pid), _driverctl(driverctl) {
	printf(GAME_NAME "(%u) Attached!\n", pid);
	process_base_exe = get_module_base(L"" PROCESS_EXE_NAME);
	printf(GAME_NAME "(%u) 0x%" PRIx64 " " PROCESS_EXE_NAME "\n", pid, process_base_exe);
}
GameProcess::~GameProcess() {
	printf(GAME_NAME "(%u) Detached!\n", pid);
}

bool GameProcess::heartbeat() const {
	uint16_t dummy;
	return read(process_base_exe, dummy);
}

/*#include <Psapi.h>
const wchar_t* get_mapped_file_name(void* process, uint64_t address, void* buffer, size_t size) {
	if (K32GetMappedFileNameW(process, (LPVOID)address, (LPWSTR)buffer, (DWORD)size) == 0) {
		return nullptr;
	}
	return (const wchar_t*)buffer;
}*/

uint64_t GameProcess::get_module_base(const wchar_t* module_name) const {
	if (StrCmpW(L"" PROCESS_EXE_NAME, module_name)) {
		printf("get_module_base() not implemented for this parameter!\n");
	}

	DriverControl* driverctl = static_cast<DriverControl*>(_driverctl);
	std::vector<MEMORY_BASIC_INFORMATION64> result;
	driverctl->EnumMemoryRegions(pid, result);
	for (auto mbi : result) {
		uint64_t address = mbi.BaseAddress;
		if (true || mbi.State == MEM_COMMIT && mbi.Type == MEM_IMAGE) {
			IMAGE_DOS_HEADER dos_header;
			IMAGE_NT_HEADERS64 nt_headers;
			if (!(read(address, dos_header) && read(address + dos_header.e_lfanew, nt_headers))) {
				continue;
			}
			if (nt_headers.OptionalHeader.CheckSum != Offsets().checksum) {
				continue;
			}

			return address;
		}
	}
	printf("get_module_base(): base not found!\n");
	return 0;
}
bool GameProcess::read_raw(uint64_t address, void* buffer, size_t size) const {
	DriverControl* driverctl = static_cast<DriverControl*>(_driverctl);
	auto status = driverctl->ReadMem(pid, address, size, buffer);
	return NT_SUCCESS(status);
}
bool GameProcess::write_raw(uint64_t address, const void* buffer, size_t size) const {
	DriverControl* driverctl = static_cast<DriverControl*>(_driverctl);
	void* tmp_mem = new char[size];
	memcpy(tmp_mem, buffer, size);
	auto status = driverctl->WriteMem(pid, address, size, tmp_mem);
	return NT_SUCCESS(status);
}
bool GameProcess::check_version(uint32_t time_date_stamp, uint32_t checksum) const {
	// Sanity check the image base address...
	if (process_base_exe == 0 || (process_base_exe & 0xfff) != 0) {
		printf(GAME_NAME "(%u) Invalid image base: perhaps your bypass is incomplete.\n", pid);
		return false;
	}

	IMAGE_DOS_HEADER dos_header;
	IMAGE_NT_HEADERS64 nt_headers;
	if (!(read(process_base_exe, dos_header) && read(process_base_exe + dos_header.e_lfanew, nt_headers))) {
		printf(GAME_NAME "(%u) Error reading headers: incorrect image base, broken bypass or other issue!\n", pid);
		return false;
	}

	// Sanity check the image magic values...
	if (
		dos_header.e_magic != IMAGE_DOS_SIGNATURE ||
		nt_headers.Signature != IMAGE_NT_SIGNATURE ||
		nt_headers.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC
		) {
		printf(GAME_NAME "(%u) Incorrect magic values: the image base address is incorrect!\n", pid);
		return false;
	}

	// If TimeDateStamp and CheckSum match then our offsets are probably up-to-date
	// This can also happen if the base address points to the wrong image in memory
	if (nt_headers.FileHeader.TimeDateStamp == time_date_stamp && nt_headers.OptionalHeader.CheckSum == checksum) {
		return true;
	}
	printf(GAME_NAME "(%u) Gamedata mismatch! Please update the offsets.\n", pid);

	// Wait a minute to give the game a chance to decrypt itself
	printf(GAME_NAME "(%u) Proceeding to dump the game executable in ~10 seconds.\n", pid);
	sleep(1000 * 10);

	// Dump the game binary from memory
	const size_t target_len = nt_headers.OptionalHeader.SizeOfImage;
	auto target = std::unique_ptr<uint8_t[]>(new uint8_t[target_len]);
	printf(GAME_NAME "(%u) Dumping %llu bytes to 0x%llx\n", pid, target_len, target.get());
	int fail_cnt = 0;
	const size_t cunk_size = 0x1000;
	for (uintptr_t offset = 0, pSrc = process_base_exe, pDst = (uintptr_t)target.get(); offset < target_len; offset += cunk_size, pSrc += cunk_size, pDst += cunk_size) {
		bool result = read_raw(pSrc, (void*)pDst, cunk_size);
		if (!result)
			fail_cnt += 1;
	}
	printf("Copied data, fail counter: %d / %llu\n", fail_cnt, target_len / cunk_size);
	if (true || read_array(process_base_exe, target.get(), target_len)) {
		// Fixup section headers...
		auto pnt_headers = reinterpret_cast<PIMAGE_NT_HEADERS64>(target.get() + dos_header.e_lfanew);
		auto section_headers = reinterpret_cast<PIMAGE_SECTION_HEADER>(
			target.get() +
			static_cast<size_t>(dos_header.e_lfanew) +
			static_cast<size_t>(FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader)) +
			static_cast<size_t>(nt_headers.FileHeader.SizeOfOptionalHeader));
		for (size_t i = 0; i < nt_headers.FileHeader.NumberOfSections; i += 1) {
			auto& section = section_headers[i];
			// Rewrite the file offsets to the virtual addresses
			section.PointerToRawData = section.VirtualAddress;
			section.SizeOfRawData = section.Misc.VirtualSize;
			// Rewrite the base relocations to the ".reloc" section
			if (!memcmp(section.Name, ".reloc\0\0", 8)) {
				pnt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC] = {
					section.VirtualAddress,
					section.Misc.VirtualSize,
				};
			}
		}

		const auto dump_file = CreateFileW(L"" PROCESS_EXE_NAME ".dump", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_COMPRESSED, NULL);
		if (dump_file != INVALID_HANDLE_VALUE) {
			if (!WriteFile(dump_file, target.get(), static_cast<DWORD>(target_len), NULL, NULL)) {
				printf(GAME_NAME "(%u) Error writing r5apex.dump: %u\n", pid, GetLastError());
			}
			CloseHandle(dump_file);
		} else {
			printf(GAME_NAME "(%u) Error writing " PROCESS_EXE_NAME ".dump: %u\n", pid, GetLastError());
		}
		printf(GAME_NAME "(%u) Wrote " PROCESS_EXE_NAME ".dump!\n", pid);
	} else {
		printf(GAME_NAME "(%u) Error reading the image from memory!\n", pid);
	}

	return false;
}
}
