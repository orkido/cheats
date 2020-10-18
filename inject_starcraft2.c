#include <stdint.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winternl.h>
#include <TlHelp32.h>
#include <ntstatus.h>

__declspec(dllexport) char search_string[] = "alsfkdjwendibegniubvfdnkxc";

#define BUFSIZE 39 // written 4 byte integers in decrypt_data_to_passed_buffer()

__declspec(dllexport) uint64_t valid_threads = 0;
__declspec(dllexport) uint64_t invalid_threads = 0;


typedef int (__cdecl* DECRYPT_DATA_TO_PASSED_BUFFER) (void* _this, int* buffer);
typedef void* (__cdecl* GET_DECRYPTOR_THIS) (int32_t player_num, unsigned int _unknown);
typedef BYTE (__cdecl* DECRYPT_DATA_ON_STACK) (int32_t player_num);

typedef NTSTATUS
(NTAPI* t_NtQueryInformationThread) (
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);

__declspec(dllexport) DECRYPT_DATA_TO_PASSED_BUFFER decrypt_data_to_passed_buffer = NULL;
__declspec(dllexport) GET_DECRYPTOR_THIS get_decryptor_this = NULL;
__declspec(dllexport) DECRYPT_DATA_ON_STACK decrypt_data_on_stack = NULL;

__declspec(dllexport) t_NtQueryInformationThread winapi_NtQueryInformationThread = NULL;

NTSTATUS
NTAPI
hook_NtQueryInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);

#define WILDCARD_BYTE 0xAA

char* FindPattern(const char* mem, size_t mem_len, const char* pattern, size_t pattern_len, char wildcard) {
    const char* p1 = mem;
    while (mem_len > 0) {
        mem_len--;
        const char* p1Begin = p1, *p2 = pattern;
        size_t pattern_len_t = pattern_len;

        while (pattern_len_t > 0 && (*p1 == *p2 || *p2 == wildcard)) {
            p1++;
            p2++;
            pattern_len_t--;
        }
        if (pattern_len_t == 0)
            return p1Begin;
        p1 = p1Begin + 1;
    }
    return NULL;
}

// https://www.unknowncheats.me/forum/c-and-c-/61169-module-size.html
DWORD GetModuleSize(DWORD processID, char* module) {
    HANDLE hSnap;
    MODULEENTRY32 xModule;
    hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processID);
    xModule.dwSize = sizeof(MODULEENTRY32);
    if (Module32First(hSnap, &xModule)) {
        if (!strncmp((char*)xModule.szModule, module, 8)) {
            CloseHandle(hSnap);
            return (DWORD)xModule.modBaseSize;
        }
        while (Module32Next(hSnap, &xModule)) {
            if (!strncmp((char*)xModule.szModule, module, 8)) {
                CloseHandle(hSnap);
                return (DWORD)xModule.modBaseSize;
            }
        }
    }
    CloseHandle(hSnap);
    return 0;
}

const char* get_base() {
    const PPEB peb = (PPEB)(__readfsdword(0x30));
    const char* base_address = peb->Reserved3[1];

    return base_address;
}

__declspec(dllexport) BOOL init_functions() {
    if (winapi_NtQueryInformationThread && decrypt_data_to_passed_buffer && get_decryptor_this && decrypt_data_on_stack)
        return TRUE;

    const char* base_address = get_base();
    const size_t base_size = GetModuleSize(GetCurrentProcessId(), "SC2.exe");

    winapi_NtQueryInformationThread = (t_NtQueryInformationThread)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQueryInformationThread");

    // dec_buf: decrypt_data_to_passed_buffer
    // dec_this: get_decryptor_this

    char ps_decrypt_data_to_passed_buffer[] = {
        0x55, 0x8B, 0xEC, 0x56, 0x57, 0x8B, 0x7D, 0x08, 0x8B, 0xCF, 0x6A, 0x68,
        WILDCARD_BYTE, WILDCARD_BYTE, WILDCARD_BYTE, WILDCARD_BYTE, WILDCARD_BYTE,
        0x8B, 0x75, 0x0C, 0x8B, 0xCF, 0x6A, 0x69, 0x89, 0x06
    };
    char ps_get_decryptor_this[] = {
        0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x18, 0x80, 0x7D, 0x08, 0x10, 0x53, 0x56,
        0x57, 0x72, 0x09, 0x33, 0xC0, 0x5F, 0x5E, 0x5B, 0x8B, 0xE5, 0x5D, 0xC3,
        0x83, 0x7D, 0x0C, 0x00,
        WILDCARD_BYTE, WILDCARD_BYTE, WILDCARD_BYTE, WILDCARD_BYTE, WILDCARD_BYTE, WILDCARD_BYTE,
        0x7A, 0x6A, 0x86, 0xC9, 0x7B, 0x66, 0xC6, 0xC7, 0x18, 0xF6, 0xDB
    };
    char ps_decrypt_data_on_stack[] = {
        0x55, 0x8B, 0xEC, 0x81, 0xEC, 0xBC, 0x00, 0x00, 0x00, 0x56, 0x8B, 0x35,
        WILDCARD_BYTE, WILDCARD_BYTE, WILDCARD_BYTE, WILDCARD_BYTE, 0x33, 0x35, WILDCARD_BYTE, WILDCARD_BYTE, WILDCARD_BYTE, WILDCARD_BYTE, 0x8B, 0xCE,
        0x8B, 0x06, 0x8B, 0x80, 0x70, 0x01, 0x00, 0x00, 0xFF, 0xD0, 0x84, 0xC0
    };

    decrypt_data_to_passed_buffer = FindPattern(base_address, base_size, ps_decrypt_data_to_passed_buffer, sizeof(ps_decrypt_data_to_passed_buffer), WILDCARD_BYTE);
    get_decryptor_this = FindPattern(base_address, base_size, ps_get_decryptor_this, sizeof(ps_get_decryptor_this), WILDCARD_BYTE);
    decrypt_data_on_stack = FindPattern(base_address, base_size, ps_decrypt_data_on_stack, sizeof(ps_decrypt_data_on_stack), WILDCARD_BYTE);

    if (winapi_NtQueryInformationThread && decrypt_data_to_passed_buffer && get_decryptor_this && decrypt_data_on_stack) {
        return TRUE;
    } else {
        char print_buf[1000] = { 0 };

        sprintf_s(print_buf, sizeof(print_buf), "init_functions(): Failed: winapi_NtQueryInformationThread=%p decrypt_data_to_passed_buffer=%p get_decryptor_this=%p "
            "decrypt_data_on_stack=%p base_address=%p base_size=%u", winapi_NtQueryInformationThread, decrypt_data_to_passed_buffer, get_decryptor_this, decrypt_data_on_stack, base_address, base_size
        );
        OutputDebugStringA(print_buf);
        return FALSE;
    }
}

struct PLAYER_DATA {
    int32_t minerals;
    int32_t vespene;
    int32_t unknown02;
    int32_t unknown03;
    int32_t supply; // current supply used, does not included unfinished units
    int32_t unknown05;
    int32_t unknown06;
    int32_t unknown07;
    int32_t unknown08;
    int32_t unknown09;
    int32_t unknown10;
    int32_t unknown11;
    int32_t unknown12;
    int32_t unknown13;
    int32_t unknown14;
    int32_t unknown15;
    int32_t unknown16;
    int32_t unknown17;
    int32_t unknown18;
    int32_t unknown19;
    int32_t unknown20;
    int32_t unknown21;
    int32_t unknown22;
    int32_t unknown23;
    int32_t unknown24;
    int32_t unknown25;
    int32_t unknown26;
    int32_t unknown27;
    int32_t unknown28;
    int32_t unknown29;
    int32_t unknown30;
    int32_t unknown31;
    int32_t unknown32;
    int32_t unknown33;
    int32_t unknown34;
    int32_t unknown35;
    int32_t unknown36;
    int32_t unknown37;
    int32_t unknown38;
};

__declspec(dllexport) int get_decrypted_data(int player_num, int* buf) {
    BYTE result;
    uintptr_t stack_address;
    struct PLAYER_DATA data;

    // asm stores stack address before call:
    // result = decrypt_data_on_stack(player_num);
    __asm {
        sub esp, 1024
        mov eax, player_num
        push eax
        mov stack_address, esp
        call decrypt_data_on_stack
        add esp, 4
        mov result, al
        add esp, 1024
    }

    stack_address += - 8 - 0x9C;

    memcpy(&data, stack_address, sizeof(data));

    char print_buf[1000] = { 0 };
    sprintf_s(print_buf, sizeof(print_buf), "Data: %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u "
        "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u", 
        data.minerals, data.vespene, data.unknown02, data.unknown03, data.supply, data.unknown05, data.unknown06,
        data.unknown07, data.unknown08, data.unknown09, data.unknown10, data.unknown11, data.unknown12, data.unknown13,
        data.unknown14, data.unknown15, data.unknown16, data.unknown17, data.unknown18, data.unknown19, data.unknown20,
        data.unknown21, data.unknown22, data.unknown23, data.unknown24, data.unknown25, data.unknown26, data.unknown27,
        data.unknown28, data.unknown29, data.unknown30, data.unknown31, data.unknown32, data.unknown33, data.unknown34,
        data.unknown35, data.unknown36, data.unknown37, data.unknown38);
    OutputDebugStringA(print_buf);

    return result;


    /*uint32_t* unlock_decryptor_this1 = (uint32_t*) (get_base() + 0x483f900);
    uint32_t* unlock_decryptor_this2 = (uint32_t*)(get_base() + 0x2dbd1a8);
    BYTE* get_decryptor_this_check_byte = (BYTE*) (get_base() + 0x314c1dd);

    uint32_t* unlock_decryptor_xor = *unlock_decryptor_this1 ^ *unlock_decryptor_this2;
    if (!unlock_decryptor_xor) {
        OutputDebugStringA("get_decrypted_data(): *unlock_decryptor_this1 ^ *unlock_decryptor_this2 == 0");
        return FALSE;
    }
    if (!*(BYTE*)(unlock_decryptor_xor + 0x188)) {
        OutputDebugStringA("get_decrypted_data(): *(unlock_decryptor_xor + 0x188) == 0");
        return FALSE;
    }

    if (!*get_decryptor_this_check_byte) {
        OutputDebugStringA("get_decrypted_data(): byte check failed");
        return FALSE;
    }

    OutputDebugStringA("get_decrypted_data(): running get_decryptor_this()");
    
    void* dec_this = NULL;
    __try {
        dec_this = get_decryptor_this(player_num, 0);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        unsigned long code = GetExceptionCode();
        char print_buf[1000] = { 0 };
        sprintf_s(print_buf, sizeof(print_buf), "get_decrypted_data(): exception code=%u", code);
        OutputDebugStringA(print_buf);
        return FALSE;
    }

    char print_buf[1000] = { 0 };
    sprintf_s(print_buf, sizeof(print_buf), "get_decrypted_data(): dec_this=%p", dec_this);
    OutputDebugStringA(print_buf);

    int result = 0;
    if (dec_this) {
        result = decrypt_data_to_passed_buffer(dec_this, buf);
        char print_buf[1000] = { 0 };
        sprintf_s(print_buf, sizeof(print_buf), "get_decrypted_data(): ok player_num=%p, result=%d", (char*)player_num, result);
        OutputDebugStringA(print_buf);
    } else {
        char print_buf[1000] = { 0 };
        sprintf_s(print_buf, sizeof(print_buf), "get_decrypted_data(): failed with player_num=%p", (char*)player_num);
        OutputDebugStringA(print_buf);
    }

    return result;*/
}

#define MAX_PLAYERS 0x10
int buf[BUFSIZE * MAX_PLAYERS];
volatile int _tmp;

__declspec(dllexport) void read_data() {
    int result = TRUE;
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        i = 1;
        result &= get_decrypted_data(i, &buf[i * BUFSIZE]);
        return;
    }
    _tmp = result;
}

__declspec(dllexport) DWORD WINAPI worker(LPVOID lpThreadParameter) {
    // std::string dbg_output = "worker()";
    // OutputDebugStringA(dbg_output.c_str());

    if (init_functions()) {
        while (TRUE) {
            static long last_read = 0;
            long cur_time = GetTickCount();
            if (cur_time - last_read > 10 * 1000) {
                OutputDebugStringA("Running read_data()");
                last_read = cur_time;
                read_data();
            }
            Sleep(1);
        }
    }
    exit(1);
    return FALSE;
}

__declspec(dllexport)
NTSTATUS
NTAPI
hook2_NtQueryInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength OPTIONAL
) {
    static BOOL thread_started = FALSE;
    if (!thread_started) {
        thread_started = TRUE;
        HANDLE handle = CreateThread(NULL, 0, worker, NULL, 0, NULL);
        CloseHandle(handle);
    }

    if (!init_functions())
        return STATUS_INVALID_BUFFER_SIZE;

    NTSTATUS status = winapi_NtQueryInformationThread(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);

    if (!NT_SUCCESS(status))
        return status;

#define ThreadQuerySetWin32StartAddress 9
    if (ThreadInformationClass == ThreadQuerySetWin32StartAddress) {
        const char* base_address = get_base();
        const char* expected_start_function = base_address + 0x19ca7a3;
        if (*(DWORD*)ThreadInformation != expected_start_function) {
            invalid_threads += 1;

            char print_buf[1000];
            sprintf_s(print_buf, sizeof(print_buf), "Invalid thread: %p, base: %p, thread-base: %p", (char*)*(DWORD*)ThreadInformation, base_address, (char*)*(DWORD*)ThreadInformation - base_address);
            OutputDebugStringA(print_buf);
            
            *(DWORD*)ThreadInformation = expected_start_function;
        } else {
            valid_threads += 1;
        }
    }
    return status;
}


BOOL WINAPI DllMain(HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserve)
{
    HANDLE handle = 0;
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        /*handle = CreateThread(nullptr, 0, worker, nullptr, 0, nullptr);
        if (handle) {
            CloseHandle(handle);
        } else {
            return FALSE;
        }
        break;*/
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}