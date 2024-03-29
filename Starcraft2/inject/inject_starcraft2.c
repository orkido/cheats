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

typedef NTSTATUS
(NTAPI* t_NtQueryInformationThread) (
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);

// Declare functions
__declspec(dllexport)
VOID
WINAPI
hook_GetSystemTimePreciseAsFileTime(
    _Out_ LPFILETIME lpSystemTimeAsFileTime
);

__declspec(dllexport) DWORD WINAPI worker(LPVOID lpThreadParameter);

__declspec(dllexport) HRESULT hook_EndScene(void* pDevice);

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

__declspec(dllexport) BOOL init_functions() {
    debug_print(LEVEL_TRACE, "init_functions()\n");

#define GLOBAL_ADDRESSES sc2_base_address && sc2_base_size && winapi_NtQueryInformationThread \
&& fn_call_wrapper \
&& fn_local_player_index && fn_player_get && fn_player_global_list \
&& fn_player_get_name && fn_player_get_clantag && fn_player_get_color \
&& fn_player_camera_pitch && fn_player_camera_yaw && fn_player_camera_location && fn_player_camera_get_distance && fn_player_get_camera_bounds && fn_player_get_resources \
&& fn_player_supply_cap_decrypt && fn_player_racestruct_get_race \
&& fn_map_x_y_min_max \
&& fn_unit_get \
&& fn_is_owner_ally_neutral_enemy && fn_read_health_shield_energy && fn_access_location_by_unit

    if (GLOBAL_ADDRESSES)
        return TRUE;

    g_sc2data.overwrite_local_player_index = 0xFF;

    // Only try to resolve functions once
    static int retry_counter = 0;
    if (retry_counter >= 1)
        return FALSE;
    ++retry_counter;

    sc2_base_address = (const char*)get_base();
    sc2_base_size = GetModuleInfo(GetCurrentProcessId(), "SC2_x64.exe", MODINFO_SIZE);

    winapi_NtQueryInformationThread = (t_NtQueryInformationThread)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQueryInformationThread");

#define PATTERN_MATCH_SC2BASE(x) \
x = FindPattern(sc2_base_address, sc2_base_size, x##_pattern, x##_mask); \
debug_print(LEVEL_DEBUG, #x " = 0x%p\n", x)
#define STATIC_OFFSET_SC2BASE(x) x = sc2_base_address + x##_offset;

    PATTERN_MATCH_SC2BASE(fn_call_wrapper);
    STATIC_OFFSET_SC2BASE(fn_local_player_index);
    STATIC_OFFSET_SC2BASE(fn_player_get);
    STATIC_OFFSET_SC2BASE(fn_player_global_list);
    STATIC_OFFSET_SC2BASE(fn_player_get_name);
    STATIC_OFFSET_SC2BASE(fn_player_get_clantag);
    STATIC_OFFSET_SC2BASE(fn_player_get_color);
    STATIC_OFFSET_SC2BASE(fn_player_camera_pitch);
    STATIC_OFFSET_SC2BASE(fn_player_camera_yaw);
    STATIC_OFFSET_SC2BASE(fn_player_camera_location);
    STATIC_OFFSET_SC2BASE(fn_player_camera_get_distance);
    STATIC_OFFSET_SC2BASE(fn_player_get_camera_bounds);
    STATIC_OFFSET_SC2BASE(fn_player_get_resources);
    STATIC_OFFSET_SC2BASE(fn_player_supply_cap_decrypt);
    STATIC_OFFSET_SC2BASE(fn_player_racestruct_get_race);
    STATIC_OFFSET_SC2BASE(fn_map_x_y_min_max);
    STATIC_OFFSET_SC2BASE(fn_unit_get);
    STATIC_OFFSET_SC2BASE(fn_is_owner_ally_neutral_enemy);
    STATIC_OFFSET_SC2BASE(fn_read_health_shield_energy);
    STATIC_OFFSET_SC2BASE(fn_access_location_by_unit);

    char* d3d9_base_address = NULL;
    char* d3d9_base_size = NULL;

    BOOL success = GLOBAL_ADDRESSES;

    if (HOOK_EndScene_ENABLED || EXTERNAL_HOOK_EndScene_ENABLED) {
        d3d9_base_address = GetModuleInfo(GetCurrentProcessId(), "d3d9.dll", MODINFO_BASE);
        d3d9_base_size = GetModuleInfo(GetCurrentProcessId(), "d3d9.dll", MODINFO_SIZE);
        fn_EndScene = FindPattern(d3d9_base_address, d3d9_base_size, fn_EndScene_pattern_win10_20h2, fn_EndScene_mask_win10_20h2);
        if (!fn_EndScene)
            fn_EndScene = FindPattern(d3d9_base_address, d3d9_base_size, fn_EndScene_pattern_win10_19h1, fn_EndScene_mask_win10_19h1);

        success = success && fn_EndScene;
        if (!fn_EndScene)
            debug_print(LEVEL_ERROR, "Failed to find EndScene by pattern matching. Your system version might not be supported\n");
    }

    uintptr_t vtable_EndScene = NULL;
    if (HOOK_EndScene_ENABLED) {
        debug_print(LEVEL_DEBUG, "d3d9.dll base 0x%p, base size 0x%p\n", d3d9_base_address, (void*)d3d9_base_size);
        if (d3d9_base_address && d3d9_base_size) {
            debug_print(LEVEL_TRACE, "Accessing pointer chain to find EndScene\n");
            vtable_EndScene = *(uintptr_t*)(sc2_base_address + EndScene_level0);
            vtable_EndScene = *(uintptr_t*)(vtable_EndScene + EndScene_level1);
            vtable_EndScene = *(uintptr_t*)(vtable_EndScene + EndScene_level2);
            vtable_EndScene = vtable_EndScene + EndScene_level3;
            debug_print(LEVEL_DEBUG, "EndScene vtable entry is at: 0x%p\n", vtable_EndScene);
        } else {
            debug_print(LEVEL_ERROR, "Failed to get d3d9.dll base address or size\n");
        }

        success = success && vtable_EndScene;

        if (!vtable_EndScene)
            debug_print(LEVEL_ERROR, "Failed to find vtable entry, got NULL-poninter\n");
        else if (*(char**)vtable_EndScene != fn_EndScene) {
            // Sanity check EndScene function pointer failed
            debug_print(LEVEL_ERROR, "Sanity check EndScene failed: Expected: 0x%p, got 0x%p, maybe already hooked, d3d9.dll of your system not supported or game offsets outdated\n", fn_EndScene, *(char**)vtable_EndScene);
            success = FALSE;
        }
        debug_print(LEVEL_DEBUG, "fn_EndScene: 0x%p, vtable_EndScene: 0x%p\n", fn_EndScene, vtable_EndScene);
    }

    if (!success)
        debug_print(LEVEL_ERROR, "init_functions(): Failed to collect offsets and data\n");

    // Install hooks as the last step because those run instantly after installation
    if (success) {
        // Install hooks
        if (HOOK_GetSystemTimePreciseAsFileTime_ENABLED) {
            debug_print(LEVEL_TRACE, "Installing GetSystemTimePreciseAsFileTime hook\n");
            *(char**)(sc2_base_address + glob_fn_GetSystemTimePreciseAsFileTime) = hook_GetSystemTimePreciseAsFileTime;
        }
        if (HOOK_EndScene_ENABLED) {
            debug_print(LEVEL_TRACE, "Installing EndScene hook\n");
            *(char**)vtable_EndScene = hook_EndScene;
        }

        // Thread feature can be disabled
        static BOOL thread_started = FALSE;
        if (!thread_started && WORKER_THREAD_ENABLED) {
            thread_started = TRUE;
            HANDLE handle = CreateThread(NULL, 0, worker, &thread_started, 0, NULL);
            if (handle == NULL)
                debug_print(LEVEL_ERROR, "Failed to create thread: error %lu", GetLastError());
            else {
                debug_print(LEVEL_DEBUG, "Thread started\n");
                CloseHandle(handle);
            }
        }
    }
    g_sc2data.startup_done = !!success;
    return success;
}

// Interval in ms
#define UPDATE_INTERVAL 20
BOOL update_data(BOOL update_finished) {
    debug_print(LEVEL_TRACE, "update_data(%d)\n", update_finished);
    static uint64_t last_read = 0;

    uint64_t cur_time = GetTickCount64();
    if (update_finished) {
        last_read = cur_time;
        return FALSE;
    }

    return cur_time - last_read > UPDATE_INTERVAL;
}

// Interval in ms
#define UPDATE_INTERVAL_SLOW 1000
BOOL update_data_slow(BOOL update_finished) {
    debug_print(LEVEL_TRACE, "update_data_slow(%d)\n", update_finished);
    static uint64_t last_read = 0;

    uint64_t cur_time = GetTickCount64();
    if (update_finished) {
        last_read = cur_time;
        return FALSE;
    }

    return cur_time - last_read > UPDATE_INTERVAL_SLOW;
}

uint64_t call_in_main_section_context(uint64_t* func, uint64_t arg1, uint64_t arg2, uint64_t arg3);

struct DT_Player* fn_player_get_wrapper(uint8_t player_index, char* player_list) {
    return call_in_main_section_context(fn_player_get, player_index, player_list, NULL);
}

__declspec(dllexport) void work() {
    debug_print(LEVEL_TRACE, "work()\n");

    if (!update_data(FALSE) || !init_functions())
        return;

    BOOL update_slow = update_data_slow(FALSE);

    debug_print(LEVEL_TRACE, "glob_ingame\n");
    g_sc2data.ingame = *(uint8_t*)(sc2_base_address + glob_ingame);
    if (!g_sc2data.ingame || !g_sc2data.enabled) {
        uint8_t ingame = g_sc2data.ingame;
        memset(&g_sc2data, 0, sizeof(g_sc2data));
        g_sc2data.startup_done = 1;
        g_sc2data.ingame = ingame;
        g_sc2data.overwrite_local_player_index = 0xFF;
        return;
    }

    if (update_slow) {
        debug_print(LEVEL_TRACE, "fn_local_player_index()\n");
        if (g_sc2data.overwrite_local_player_index == 0xFF)
            g_sc2data.local_player_index = fn_local_player_index();
        else
            g_sc2data.local_player_index = g_sc2data.overwrite_local_player_index;
    }

    debug_print(LEVEL_TRACE, "fn_player_global_list()\n");
    char* player_list = fn_player_global_list();

    g_sc2data.players_length = 16; // TODO

    for (uint32_t i = 0; i < 16; ++i) {
        debug_print(LEVEL_TRACE, "fn_player_get()\n");
        struct DT_Player* in_player = fn_player_get_wrapper(i, player_list);
        struct Player* out_player = &g_sc2data.players[i];

        if (update_slow) {
            // NOTE: return value +8 in name source
            strncpy(out_player->name, fn_player_get_name(in_player) + 8, sizeof(out_player->name));
            strncpy(out_player->clantag, fn_player_get_clantag(in_player) + 8, sizeof(out_player->clantag));
            out_player->supply = 0;
            fn_player_supply_cap_decrypt(in_player, &out_player->supply_cap);
            out_player->supply_cap >>= 12;
            out_player->supply_cap_max = in_player->supply_max_cap >> 12;
            debug_print(LEVEL_TRACE, "fn_player_get_color()\n");
            fn_player_get_color(&out_player->color, i);

            out_player->address = in_player;
            out_player->id = in_player->player_id;

            debug_print(LEVEL_TRACE, "fn_player_get_resources()\n");
            out_player->minerals = fn_player_get_resources(in_player, 0);
            out_player->vespene = fn_player_get_resources(in_player, 1);
            out_player->resource1 = fn_player_get_resources(in_player, 2);
            out_player->resource2 = fn_player_get_resources(in_player, 3);

            debug_print(LEVEL_TRACE, "fn_is_owner_ally_neutral_enemy()\n");
            enum Team team = fn_is_owner_ally_neutral_enemy(g_sc2data.local_player_index, i);
            out_player->team = team;

            debug_print(LEVEL_TRACE, "fn_player_racestruct_get_race()\n");
            // TODO
            int race = 0; // fn_player_racestruct_get_race(&in_player->race_struct);
            if (race < RaceUnknown || race > RaceZerg)
                race = RaceUnknown;
            out_player->race = race;
        }

        debug_print(LEVEL_TRACE, "fn_player_camera_*()\n");
        out_player->camera_pitch = fn_player_camera_pitch(i);
        out_player->camera_yaw = fn_player_camera_yaw(i);
        out_player->camera_distance = fn_player_camera_get_distance(i);
        out_player->camera_location = 0; // TODO
    }

    for (uint32_t i = 0; i < *(uint32_t*)(sc2_base_address + units_list_length) && i < sizeof(g_sc2data.units) / sizeof(g_sc2data.units[0]); ++i) {
        debug_print(LEVEL_TRACE, "fn_unit_get()\n");
        struct DT_Unit* in_unit = fn_unit_get(sc2_base_address + units_list, i);
        struct Unit* out_unit = &g_sc2data.units[i];

        if (update_slow) {
            out_unit->address = in_unit;

            debug_print(LEVEL_TRACE, "in_unit->index[_unknown]\n");
            // i == out_unix->index >> 18
            // out_unix->index & 0x3FFF == 1
            out_unit->index = in_unit->index;
            out_unit->index_unknown = in_unit->index_unknown;

            if (in_unit->unit_type_ref && in_unit->unit_type_ref->unit_type) {
                strncpy(out_unit->unit_name, in_unit->unit_type_ref->unit_type->unit_name, sizeof(out_unit->unit_name));
                out_unit->unit_type_id = in_unit->unit_type_ref->unit_type->unit_type_id;
            }

            debug_print(LEVEL_TRACE, "fn_read_health_shield_energy()\n");
            fn_read_health_shield_energy(in_unit, &out_unit->health, 0);
            fn_read_health_shield_energy(in_unit, &out_unit->shields, 1);
            fn_read_health_shield_energy(in_unit, &out_unit->energy, 2);
            out_unit->health >>= 12;
            out_unit->shields >>= 12;
            out_unit->energy >>= 12;

            enum Team team = fn_is_owner_ally_neutral_enemy(g_sc2data.local_player_index, in_unit->owner_player_id);
            out_unit->team = team;

            out_unit->owner_player_id = in_unit->owner_player_id;
            out_unit->unknown_player_id = in_unit->unknown_player_id;
            out_unit->control_type = in_unit->control_type;
            out_unit->amount_units_attacking_self = in_unit->amount_units_attacking_self;
            out_unit->interesting_value = in_unit->interesting_value_in_setOwner;
            out_unit->interesting_value2 = in_unit->interesting_value2_in_setOwner;
            out_unit->player_id = in_unit->player_id;
            out_unit->player_visible_num = in_unit->player_visible_index;
        }

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

    debug_print(LEVEL_TRACE, "general details\n");
    g_sc2data.units_length = *(uint32_t*)(sc2_base_address + units_list_length);

    if (update_slow) {
        struct DT_MapSize* camera_bounds = fn_player_get_camera_bounds(16);
        g_sc2data.camera_bounds_address = camera_bounds;
        g_sc2data.playable_mapsize_x_max = camera_bounds->x_max; // - 7; // On both sides +7
        g_sc2data.playable_mapsize_x_min = camera_bounds->x_min; // - 7; // On both sides +7
        g_sc2data.playable_mapsize_y_max = camera_bounds->y_max; // - 3; // On both sides +3
        g_sc2data.playable_mapsize_y_min = camera_bounds->y_min; // - 3; // On both sides +3
        struct DT_MapSize* map_bounds = fn_map_x_y_min_max();
        g_sc2data.total_mapsize_x = map_bounds->x_max - map_bounds->x_min; // min. x is always 0
        g_sc2data.total_mapsize_y = map_bounds->y_max - map_bounds->y_min; // min. y is always 0
    }

    if (update_slow)
        update_data_slow(TRUE);

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

__declspec(dllexport) HRESULT hook_EndScene(void* pDevice) {
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

    // On error try to fix the error to not crash the program and still forward winapi call
    if (!init_functions()) {
        if (!winapi_NtQueryInformationThread)
            winapi_NtQueryInformationThread = (t_NtQueryInformationThread)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQueryInformationThread");
        if (!winapi_NtQueryInformationThread)
            return STATUS_INVALID_BUFFER_SIZE;
    }

    NTSTATUS status = winapi_NtQueryInformationThread(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);

    if (!NT_SUCCESS(status))
        return status;

#define ThreadQuerySetWin32StartAddress 9
    // This can be convenient to bypass debugger detection
    if (THREAD_START_ADDRESS_ENABLED && ThreadInformationClass == ThreadQuerySetWin32StartAddress) {
        char* expected_start_function = sc2_base_address + thread_start_address;
        if (*(char**)ThreadInformation != expected_start_function) {

            debug_print(LEVEL_DEBUG, "Invalid thread: %p, base: %p, thread-base: %p\n", *(char**)ThreadInformation, sc2_base_address, (*(char**)ThreadInformation) - sc2_base_address);
            
            *(void**)ThreadInformation = expected_start_function;
        }
    }
    return status;
}


BOOL WINAPI DllMain(HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserve)
{
    debug_print(LEVEL_TRACE, "DllMain()\n");

    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        init_functions();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}