#include "inject_offsets.c"
#include "data_interface.h"

#include <stdint.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winternl.h>
#include <TlHelp32.h>
#include <ntstatus.h>
#include <process.h>
#include <stdio.h>

__declspec(dllexport) char CONST_OFFSET_NAME[] = CONST_OFFSET_VALUE;
__declspec(dllexport) struct SC2Data g_sc2data = { 0 };

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

__declspec(dllexport)
VOID
WINAPI
hook_GetSystemTimePreciseAsFileTime(
    _Out_ LPFILETIME lpSystemTimeAsFileTime
);

HRESULT hook_EndScene(void* pDevice);

__declspec(dllexport) DECRYPT_DATA_TO_PASSED_BUFFER decrypt_data_to_passed_buffer = NULL;
__declspec(dllexport) GET_DECRYPTOR_THIS get_decryptor_this = NULL;
__declspec(dllexport) DECRYPT_DATA_ON_STACK decrypt_data_on_stack = NULL;

__declspec(dllexport) t_NtQueryInformationThread winapi_NtQueryInformationThread = NULL;

__declspec(dllexport) char* sc2_base_address = NULL;
__declspec(dllexport) size_t sc2_base_size = NULL;

#define WILDCARD_BYTE 0xAA

#define LEVEL_TRACE 5
#define LEVEL_DEBUG 4
#define LEVEL_INFO 3
#define LEVEL_WARNING 2
#define LEVEL_ERROR 1
void debug_print(int level, const char* format, ...) {
    if (level >= LEVEL_DEBUG)
        return;

    va_list args;
    va_start(args, format);

    char buf[2048];
    vsprintf(buf, format, args);

    //FILE* pFile = fopen("C:\\Users\\florian\\Downloads\\EfiGuard\\SC2.log", "a");
    //vfprintf(pFile, format, args);
    //fclose(pFile);
    OutputDebugStringA(buf);

    va_end(args);
}

// use '?' in mask for unspecified value, '\0' for end of matching, everything else to match the next byte
char* FindPattern(const char* mem, size_t mem_len, const char* pattern, const char* mask) {
    const char* p1 = mem;
    size_t pattern_len = strlen(mask);

    while (mem_len > 0 && pattern_len <= mem_len) {
        mem_len--;
        const char* p1Begin = p1, *p2 = pattern;
        const char* pMask = mask;

        while (*pMask && (*p1 == *p2 || *pMask == '?')) {
            p1++;
            p2++;
            pMask++;
        }
        if (*pMask == '\0')
            return (char*) p1Begin;
        p1 = p1Begin + 1;
    }
    return NULL;
}

// https://www.unknowncheats.me/forum/c-and-c-/61169-module-size.html
#define MODINFO_BASE 0
#define MODINFO_SIZE 1
size_t GetModuleInfo(DWORD processID, char* module, int info) {
    HANDLE hSnap;
    MODULEENTRY32 xModule;
    hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processID);
    xModule.dwSize = sizeof(MODULEENTRY32);
    if (Module32First(hSnap, &xModule)) {
        if (!strncmp((char*)xModule.szModule, module, 8)) {
            CloseHandle(hSnap);
            if (info == MODINFO_BASE)
                return (size_t) xModule.modBaseAddr;
            if (info == MODINFO_SIZE)
                return xModule.modBaseSize;
        }
        while (Module32Next(hSnap, &xModule)) {
            if (!strncmp((char*)xModule.szModule, module, 8)) {
                CloseHandle(hSnap);
                if (info == MODINFO_BASE)
                    return (size_t)xModule.modBaseAddr;
                if (info == MODINFO_SIZE)
                    return xModule.modBaseSize;
            }
        }
    }
    CloseHandle(hSnap);
    return 0;
}

const void* get_base() {
    #ifdef _WIN64
    PPEB peb = (PPEB)__readgsqword(0x60);
    #else
    PPEB peb = (PPEB)__readfsdword(0x30);    
    #endif // _WIN64
    const void* base_address = peb->Reserved3[1];

    return base_address;
}

/*__declspec(dllexport) BOOL init_functions_old() {
    if (winapi_NtQueryInformationThread && decrypt_data_to_passed_buffer && get_decryptor_this && decrypt_data_on_stack)
        return TRUE;

    const char* base_address = (const char*)get_base();
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

    decrypt_data_to_passed_buffer = (DECRYPT_DATA_TO_PASSED_BUFFER) FindPattern(base_address, base_size, ps_decrypt_data_to_passed_buffer, sizeof(ps_decrypt_data_to_passed_buffer), WILDCARD_BYTE);
    get_decryptor_this = (GET_DECRYPTOR_THIS) FindPattern(base_address, base_size, ps_get_decryptor_this, sizeof(ps_get_decryptor_this), WILDCARD_BYTE);
    decrypt_data_on_stack = (DECRYPT_DATA_ON_STACK) FindPattern(base_address, base_size, ps_decrypt_data_on_stack, sizeof(ps_decrypt_data_on_stack), WILDCARD_BYTE);

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
}*/

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
    /*__asm {
        sub esp, 1024
        mov eax, player_num
        push eax
        mov stack_address, esp
        call decrypt_data_on_stack
        add esp, 4
        mov result, al
        add esp, 1024
    }*/

    stack_address += - 8 - 0x9C;

    memcpy(&data, (void*) stack_address, sizeof(data));

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

__declspec(dllexport) BOOL init_functions() {
    debug_print(LEVEL_TRACE, "init_functions()\n");

#define GLOBAL_ADDRESSES sc2_base_address && sc2_base_size && winapi_NtQueryInformationThread \
&& fn_local_player_index && fn_get_unit && fn_is_owner_ally_neutral_enemy \
&& fn_read_health_shield_energy && fn_access_location_by_unit
    if (GLOBAL_ADDRESSES)
        return TRUE;

    sc2_base_address = (const char*)get_base();
    sc2_base_size = GetModuleInfo(GetCurrentProcessId(), "SC2_x64.exe", MODINFO_SIZE);

    winapi_NtQueryInformationThread = (t_NtQueryInformationThread)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQueryInformationThread");

    fn_local_player_index = (FN_LOCAL_PLAYER_INDEX)FindPattern(sc2_base_address, sc2_base_size, fn_local_player_index_pattern, fn_local_player_index_mask);
    fn_get_unit = (FN_GET_UNIT_LIST)FindPattern(sc2_base_address, sc2_base_size, fn_get_unit_pattern, fn_get_unit_mask);
    fn_is_owner_ally_neutral_enemy = (FN_IS_OWNER_ALLY_NEUTRAL_ENEMY)FindPattern(sc2_base_address, sc2_base_size, fn_is_owner_ally_neutral_enemy_pattern, fn_is_owner_ally_neutral_enemy_mask);
    fn_read_health_shield_energy = (FN_READ_HEALTH_SHIELD_ENERGY)FindPattern(sc2_base_address, sc2_base_size, fn_read_health_shield_energy_pattern, fn_read_health_shield_energy_mask);
    fn_access_location_by_unit = (FN_ACCESS_LOCATION_BY_UNIT)FindPattern(sc2_base_address, sc2_base_size, fn_access_location_by_unit_pattern, fn_access_location_by_unit_mask);

    uintptr_t vtable_EndScene = NULL;
    if (HOOK_EndScene) {
        char* d3d9_base_address = GetModuleInfo(GetCurrentProcessId(), "d3d9.dll", MODINFO_BASE);
        char* d3d9_base_size = GetModuleInfo(GetCurrentProcessId(), "d3d9.dll", MODINFO_SIZE);
        debug_print(LEVEL_DEBUG, "d3d9.dll base 0x%p, base size 0x%p\n", d3d9_base_address, (void*)d3d9_base_size);
        if (d3d9_base_address && d3d9_base_size) {
            debug_print(LEVEL_TRACE, "Accessing pointer chain to find EndScene\n");
            // Alternative is to store pointer by using the vtable entry
            fn_EndScene = FindPattern(d3d9_base_address, d3d9_base_size, fn_EndScene_pattern, fn_EndScene_mask);
            vtable_EndScene = *(uintptr_t*)(sc2_base_address + EndScene_level0);
            vtable_EndScene = *(uintptr_t*)(vtable_EndScene + EndScene_level1);
            vtable_EndScene = *(uintptr_t*)(vtable_EndScene + EndScene_level2);
            vtable_EndScene = vtable_EndScene + EndScene_level3;
            debug_print(LEVEL_DEBUG, "EndScene vtable entry is at: 0x%p\n", vtable_EndScene);
        } else {
            debug_print(LEVEL_ERROR, "Failed to get d3d9.dll base address or size\n");
        }
    }

    BOOL success = GLOBAL_ADDRESSES;
    if (HOOK_EndScene) {
        success = success && fn_EndScene && vtable_EndScene;
        // Sanity check EndScene function pointer
        if (!vtable_EndScene)
            debug_print(LEVEL_ERROR, "Failed to find vtable entry, got NULL-poninter\n");
        else if (*(char**)vtable_EndScene != fn_EndScene) {
            debug_print(LEVEL_ERROR, "Sanity check EndScene failed: Expected: 0x%p, got 0x%p\n", fn_EndScene, *(char**)vtable_EndScene);
            success = FALSE;
        }
        debug_print(LEVEL_DEBUG, "fn_EndScene: 0x%p, vtable_EndScene: 0x%p\n", fn_EndScene, vtable_EndScene);
    }

    int log_level;
    if (success)
        log_level = LEVEL_DEBUG;
    else {
        log_level = LEVEL_ERROR;
        debug_print(LEVEL_ERROR, "init_functions(): Failed to collect offsets and data\n");
    }
    debug_print(log_level, "sc2_base_address = %p, sc2_base_size = %lu, winapi_NtQueryInformationThread = %p, fn_local_player_index = %p\n", sc2_base_address, sc2_base_size, winapi_NtQueryInformationThread, fn_local_player_index);
    debug_print(log_level, "fn_get_unit = %p, fn_is_owner_ally_neutral_enemy = %p, fn_read_health_shield_energy = %p, fn_access_location_by_unit = %p\n", fn_get_unit, fn_is_owner_ally_neutral_enemy, fn_read_health_shield_energy, fn_access_location_by_unit);

    // Install hooks as the last step because those run instantly after installation
    if (success) {
        // Install hooks
        if (HOOK_GetSystemTimePreciseAsFileTime_ENABLED) {
            debug_print(LEVEL_TRACE, "Installing GetSystemTimePreciseAsFileTime hook\n");
            *(char**)(sc2_base_address + func_GetSystemTimePreciseAsFileTime) = hook_GetSystemTimePreciseAsFileTime;
        }
        if (HOOK_EndScene) {
            debug_print(LEVEL_TRACE, "Installing EndScene hook\n");
            *(char**)vtable_EndScene = hook_EndScene;
        }
    }
    return success;
}

// Interval in ms
#define UPDATE_INTERVAL 5
BOOL update_data(BOOL update_finished) {
    debug_print(LEVEL_TRACE, "update_data(%d)\n", update_finished);
    static uint64_t last_read = 0;

    uint64_t cur_time = GetTickCount64();
    if (cur_time - last_read > UPDATE_INTERVAL) {
        if (update_finished)
            last_read = cur_time;
        return TRUE;
    }
    return FALSE;
}

__declspec(dllexport) void work() {
    debug_print(LEVEL_TRACE, "work()\n");

    if (!update_data(FALSE))
        return;

    for (uint32_t i = 0; i < *(uint32_t*)(sc2_base_address + units_list_length) && i < sizeof(g_sc2data.units) / sizeof(g_sc2data.units[0]); ++i) {
        debug_print(LEVEL_TRACE, "fn_get_unit()\n");
        struct DT_Unit* in_unit = fn_get_unit(sc2_base_address + units_list, i);
        struct Unit* out_unit = &g_sc2data.units[i];

        debug_print(LEVEL_TRACE, "in_unit->index[_unknown]\n");
        // i == out_unix->index >> 18
        // out_unix->index & 0x3FFF == 1
        out_unit->index = in_unit->index;
        out_unit->index_unknown = in_unit->index_unknown;

        debug_print(LEVEL_TRACE, "fn_read_health_shield_energy()\n");
        fn_read_health_shield_energy(in_unit, &out_unit->health, 0);
        fn_read_health_shield_energy(in_unit, &out_unit->shields, 1);
        fn_read_health_shield_energy(in_unit, &out_unit->energy, 2);
        out_unit->health >>= 12;
        out_unit->shields >>= 12;
        out_unit->energy >>= 12;

        debug_print(LEVEL_TRACE, "fn_access_location_by_unit()\n");
        struct DT_VectorLocation output;
        fn_access_location_by_unit(in_unit, &output);
        out_unit->position_x = output.x / 4096.0f;
        out_unit->position_y = output.y / 4096.0f;
        out_unit->position_z = output.z / 4096.0f;
        out_unit->position_unknown1 = output.unknown1;
        out_unit->position_unknown2 = output.unknown2;
        out_unit->position_unknown3 = output.unknown3;
        out_unit->position_unknown4 = output.unknown4;
    }

    debug_print(LEVEL_TRACE, "fn_local_player_index()\n");
    g_sc2data.local_player_index = fn_local_player_index();
    g_sc2data.units_length = *(uint32_t*)(sc2_base_address + units_list_length);

    g_sc2data.mapsize_x = *(uint32_t*)(sc2_base_address + mapsize_x);
    g_sc2data.mapsize_y = *(uint32_t*)(sc2_base_address + mapsize_y);

    update_data(TRUE);
}

__declspec(dllexport) DWORD WINAPI worker(LPVOID lpThreadParameter) {
    debug_print(LEVEL_TRACE, "worker()\n");
    debug_print(LEVEL_DEBUG, "g_sc2data = %p\n", &g_sc2data);

    if (init_functions()) {
        while (TRUE) {
            work();
            Sleep(UPDATE_INTERVAL);
        }
    }
    debug_print(LEVEL_ERROR, "worker(): init_functions() returned false\n");
    return FALSE;
}

__declspec(dllexport)
VOID
WINAPI
hook_GetSystemTimePreciseAsFileTime(
    _Out_ LPFILETIME lpSystemTimeAsFileTime
) {
    debug_print(LEVEL_TRACE, "hook_GetSystemTimePreciseAsFileTime()\n");
    work();
    GetSystemTimePreciseAsFileTime(lpSystemTimeAsFileTime);
}

HRESULT hook_EndScene(void* pDevice) {
    debug_print(LEVEL_TRACE, "hook_EndScene()\n");
    work();
    debug_print(LEVEL_TRACE, "fn_EndScene()\n");
    return fn_EndScene(pDevice);
}

__declspec(dllexport)
NTSTATUS
NTAPI
hook_NtQueryInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength OPTIONAL
) {
    debug_print(LEVEL_TRACE, "hook_NtQueryInformationThread()\n");

    static int retry_counter = 2;

    // First init_functions before creating the worker thread to mitigate parallel pattern matching
    // On error or if failed too often only forward winapi call
    if (retry_counter <= 0 || !init_functions()) {
        if (retry_counter > 0)
            --retry_counter;
        // On error, behave as usual to not crash the program
        if (!winapi_NtQueryInformationThread)
            winapi_NtQueryInformationThread = (t_NtQueryInformationThread)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQueryInformationThread");
        if (!winapi_NtQueryInformationThread)
            return STATUS_INVALID_BUFFER_SIZE;
        return winapi_NtQueryInformationThread(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);
    }

    // Thread feature is disabled
    static BOOL thread_started = WORKER_THREAD_ENABLED;
    if (!thread_started) {
        thread_started = TRUE;
        HANDLE handle = CreateThread(NULL, 0, worker, &thread_started, 0, NULL);
        if (handle == NULL)
            debug_print(LEVEL_ERROR, "Failed to create thread: error %lu", GetLastError());
        else {
            debug_print(LEVEL_DEBUG, "Thread started\n");
            CloseHandle(handle);
        }
    }

    NTSTATUS status = winapi_NtQueryInformationThread(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);

    if (!NT_SUCCESS(status))
        return status;

#define ThreadQuerySetWin32StartAddress 9
    // This feature can be disabled because we start the thread from inside the target program execution environment and therefore will have
    // the same thread start address that all the other threads have. But still, this can be convenient to bypass debugger detection.
    if (THREAD_START_ADDRESS_ENABLED && ThreadInformationClass == ThreadQuerySetWin32StartAddress) {
        char* expected_start_function = sc2_base_address + thread_start_address;
        if (*(char**)ThreadInformation != expected_start_function) {
            invalid_threads += 1;

            debug_print(LEVEL_DEBUG, "Invalid thread: %p, base: %p, thread-base: %p\n", *(char**)ThreadInformation, sc2_base_address, (*(char**)ThreadInformation) - sc2_base_address);
            
            *(void**)ThreadInformation = expected_start_function;
        } else {
            valid_threads += 1;
        }
    }
    return status;
}


BOOL WINAPI DllMain(HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserve)
{
    debug_print(LEVEL_TRACE, "DllMain()\n");
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