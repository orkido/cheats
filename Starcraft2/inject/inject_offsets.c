#include <stdint.h>

// Force unaligned struct layout because struct exactly matches the game memory offsets
#pragma pack(push, 1)

struct DT_Race
{
    int pad0;
    int pad4;
    int pad8;
    int padC;
    int pad10;
    int pad14;
    int pad18;
    int pad1C;
    int pad20;
    int pad24;
    int pad28;
    int pad2C;
    int pad30;
    int pad34;
    int pad38;
    int pad3C;
    int race_id;
    int pad44;
};

struct DT_UnitType
{
    char pad00[8];
    unsigned __int32 unit_type_id;
    char pad0C[20];
    char unit_name[32];
    char pad40[12];
};

struct DT_UnitTypeRef
{
    char pad00[16];
    struct DT_UnitType* unit_type;
};

struct DT_Unit
{
    unsigned __int32 index;
    char len[20];
    unsigned __int32 index_unknown;
    __int64 m_ChangesWhenMoving;
    char pad_0x0024[28];
    unsigned __int8 owner_player_id;
    unsigned __int8 control_type;
    char pad_0x0042[18];
    unsigned __int8 interesting_value_in_setOwner;
    unsigned __int8 interesting_value2_in_setOwner;
    char pad_0x0056[2];
    unsigned __int8 amount_units_attacking_self;
    char pad_0x0059[49];
    unsigned __int8 unknown_player_id;
    char pad_0x008B[69];
    __int64 m_ChangesWhenMoving2;
    char gapD8[88];
    unsigned __int32 player_id;
    char gap134[4];
    __int64 player_visible_index;
    char pad_0x0140[72];
    struct DT_UnitTypeRef* unit_type_ref;
    __int64 pad190;
    __int32 m_MissingHealth;
    __int32 m_Shields;
    __int32 m_Energy;
    __int32 m_MaxHealth;
    __int32 m_MaxShield;
    char pad_0x01AC[192];
};

struct DT_Player
{
    unsigned __int32 zero;
    char pad_0x0000[20];
    unsigned __int32 index_unknown;
    __int32 camera_target1;
    __int32 camera_target2;
    char pad_0x0024[28];
    unsigned __int8 control_player_id;
    char pad_0x0041[19];
    unsigned __int8 interesting_value;
    unsigned __int8 interesting_value2;
    char pad_0x0056[122];
    __int64 pad_D0;
    char gapD8[88];
    unsigned __int32 pad130;
    char gap134[4];
    __int64 player_visible_num;
    char pad_0x0140[88];
    __int32 pad_198;
    __int32 pad_19C;
    __int32 pad_1A0;
    __int32 pad_1A4;
    __int32 pad_1A8;
    char pad_0x01AC;
    char gap1AD[315];
    int supply_cap_crypt1;
    int supply_cap_crypt2;
    char gap2F0[1470];
    char player_id;
    char gap8AF[81];
    struct DT_Race* race_struct;
    char gap908[364];
    int supply_max_cap;
    char gap8fAF[1447];
};

struct DT_MapSize {
    uint32_t x_min;
    uint32_t y_min;
    uint32_t x_max;
    uint32_t y_max;
};

#pragma pack(pop)

/*
// DT_Race:
race_id // plain; 2 Terran, 3 Zerg

// DT_Unit: All except those marked with "plain" are encoded
index // plain
index_unknown // plain
unknown_player_id // plain
control_type // plain; values meaning: // 1: movable unit (incl. movable buildings), 0: buildings, dead units or resources, 2: constructing buildings, unit training buildings, researching buildings or mined out resources
interesting_value // plain
interesting_value2 // plain
unknown_player_id // plain; always 0x10
player_id // plain; all 8 bytes do some visibility stuff; // = 1 << playerId; maybe only in DT_Player != 0
player_visible_num // plain; first byte is the owner; all 8 bytes do some visibility stuff
m_Energy // plain


// DT_Player:
player_id // plain; matches the index
supply_max_cap // plain; multiplied by 4096

*/

struct DT_VectorLocation {
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t unknown1;
    int32_t unknown2;
    int32_t unknown3;
    int32_t unknown4;
};

// call [rax]
// add rsp, 0x28
// ret
const char* fn_call_wrapper_pattern = "\xE8\x00\x00\x00\x00\x48\x83\xC4\x48\xC3";
const char* fn_call_wrapper_mask = "x????xxxxx";
void* fn_call_wrapper = NULL;

const char* fn_local_player_index_pattern = "\x48\x83\xEC\x28\xE8\x00\x00\x00\x00\x84\xC0\x75\x33";
const char* fn_local_player_index_mask = "xxxxx????xxxx";
typedef uint8_t(__stdcall* FN_LOCAL_PLAYER_INDEX) ();
FN_LOCAL_PLAYER_INDEX fn_local_player_index = NULL;

// If player_list is NULL then the global player list is resolved in the function.
// This function cannot be used from outside of the main Starcraft 2 code section due to protection.
const char* fn_player_get_pattern = "\x40\x55\x48\x8B\xEC\x48\x83\xEC\x50\x4C\x8B\xCA\x44\x0F\xB6\xD1"
                                    "\x80\xF9\x10\x72\x08\x33\xC0\x48\x83\xC4\x50\x5D\xC3";
const char* fn_player_get_mask = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
typedef struct DT_Player* (__fastcall* FN_PLAYER_GET) (uint8_t player_index, char* player_list);
FN_PLAYER_GET fn_player_get = NULL;

const char* fn_player_global_list_pattern = "\x8B\x05\x00\x00\x00\x00\x8B\x0D\x00\x00\x00\x00\x05\x00\x00\x00"
                                            "\x00\x03\xC8\x8B\x05\x00\x00\x00\x00\x2B\x05\x00\x00\x00\x00\xF7"
                                            "\xD0\x89\x4C\x24\x08\x89\x44\x24\x0C\x48\x8B\x44\x24\x00\xC3";
const char* fn_player_global_list_mask = "xx????xx????x????xxxx????xx????xxxxxxxxxxxxxx?x";
typedef char* (__fastcall* FN_PLAYER_GLOBAL_LIST) ();
FN_PLAYER_GLOBAL_LIST fn_player_global_list = NULL;

// This function returns a pointer to an unknown 8 bytes value. After those 8 bytes the player name follows in C-string format
const char* fn_player_get_name_pattern = "\x0F\xB6\x81\x00\x00\x00\x00\x48\x8D\x0D\x92\x00\x00\x00\x48\x6B"
                                         "\xC0\x58\x48\x03\xC1\xC3";
const char* fn_player_get_name_mask = "xxx????xxxx???xxxxxxxx";
typedef char* (__fastcall* FN_PLAYER_GET_NAME) (struct DT_Player* player);
FN_PLAYER_GET_NAME fn_player_get_name = NULL;

// This function returns a pointer to an unknown 8 bytes value. After those 8 bytes the player clan tag follows in C-string format
const char* fn_player_get_clantag_pattern = "\x0F\xB6\x81\x00\x00\x00\x00\x48\x8D\x0C\x80\x48\x8D\x05\x2E\x00"
                                            "\x00\x00\x48\x8D\x04\xC8\xC3";
const char* fn_player_get_clantag_mask = "xxx????xxxxxxxx???xxxxx";
typedef char* (__fastcall* FN_PLAYER_GET_CLANTAG) (struct DT_Player* player);
FN_PLAYER_GET_CLANTAG fn_player_get_clantag = NULL;

// The return color seems to be a ARGB value
const char* fn_player_get_color_pattern = "\x48\x89\x5C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xF9"
                                          "\x0F\xB6\xDA\x48\x8D\x4C\x24\x00\xE8\x00\x00\x00\x00\xE8\x00\x00"
                                          "\x00\x00\x85\xC0";
const char* fn_player_get_color_mask = "xxxx?xxxx????xxxxxxxxxx?x????x????xx";
typedef uint32_t* (__fastcall* FN_PLAYER_GET_COLOR) (uint32_t* ARGB_color_buffer_output, uint8_t player_index);
FN_PLAYER_GET_COLOR fn_player_get_color = NULL;

const char* fn_player_camera_pitch_pattern = "\x48\x89\x5C\x24\x00\x55\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81"
                                             "\xEC\x00\x00\x00\x00\x8B\xD9\x83\xF9\x0F\x0F\x87\x00\x00\x00\x00"
                                             "\x80\x3D\x00\x00\x00\x00\x00\x74\x24";
const char* fn_player_camera_pitch_mask = "xxxx?xxxxx????xxx????xxxxxxx????xx?????xx";
typedef int32_t(__fastcall* FN_PLAYER_CAMERA_PITCH) (uint32_t player_index);
FN_PLAYER_CAMERA_PITCH fn_player_camera_pitch = NULL;

const char* fn_player_camera_yaw_pattern = "\x48\x89\x5C\x24\x00\x55\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81"
                                           "\xEC\x00\x00\x00\x00\x8B\xD9\x83\xF9\x0F\x0F\x87\x00\x00\x00\x00"
                                           "\x80\x3D\x00\x00\x00\x00\x00\x74\x33";
const char* fn_player_camera_yaw_mask = "xxxx?xxxxx????xxx????xxxxxxx????xx?????xx";
typedef int32_t(__fastcall* FN_PLAYER_CAMERA_YAW) (uint32_t player_index);
FN_PLAYER_CAMERA_YAW fn_player_camera_yaw = NULL;

const char* fn_player_camera_location_pattern = "\x40\x55\x53\x57\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81\xEC\x00"
                                                "\x00\x00\x00\x8B\xD9";
const char* fn_player_camera_location_mask = "xxxxxxxx????xxx????xx";
typedef uint64_t(__fastcall* FN_PLAYER_CAMERA_LOCATION) (uint32_t player_index);
FN_PLAYER_CAMERA_LOCATION fn_player_camera_location = NULL;

const char* fn_player_camera_distance_pattern = "\x48\x89\x5C\x24\x00\x55\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81"
                                                "\xEC\x00\x00\x00\x00\x8B\xD9\x83\xF9\x0F\x0F\x87\x00\x00\x00\x00"
                                                "\x80\x3D\x00\x00\x00\x00\x00\x74\x23";
const char* fn_player_camera_distance_mask = "xxxx?xxxxx????xxx????xxxxxxx????xx?????xx";
typedef uint32_t(__fastcall* FN_PLAYER_CAMERA_DISTANCE) (uint32_t player_index);
FN_PLAYER_CAMERA_DISTANCE fn_player_camera_distance = NULL;

const char* fn_player_get_camera_bounds_pattern = "\x8B\x05\x00\x00\x00\x00\x2B\x05\x00\x00\x00\x00\x2D\x00\x00\x00\x00\x89\x44\x24\x10\x8B\x05\x00\x00\x00\x00\x33\x05\x00\x00\x00\x00\x89\x44\x24\x14\x0F\xB6\xC1\x48\x83\xC0\x1F\x48\xC1\xE0\x04\x48\x03\x44\x24\x00\xC3";
const char* fn_player_get_camera_bounds_mask = "xx????xx????x????xxxxxx????xx????xxxxxxxxxxxxxxxxxxx?x";
typedef struct DT_MapSize*(__fastcall* FN_PLAYER_GET_CAMERA_BOUNDS) (uint8_t player_index);
FN_PLAYER_GET_CAMERA_BOUNDS fn_player_get_camera_bounds = NULL;

// resource_selection must be in range [0; 3]
const char* fn_player_get_resources_pattern = "\x8D\x42\x68\x4C\x8D\x04\xC1\x8B\x05\x00\x00\x00\x00";
const char* fn_player_get_resources_mask = "xxxxxxxxx????";
typedef int32_t(__fastcall* FN_PLAYER_GET_RESOURCES) (struct DT_Player* player, uint64_t resource_selection);
FN_PLAYER_GET_RESOURCES fn_player_get_resources = NULL;

// Multiple functions match this pattern, use the first match
const char* fn_player_supply_cap_pattern = "\x8B\x05\x00\x00\x00\x00\x4C\x8D\x81\x00\x00\x00\x00\x4C\x8D\x1D"
                                           "\x00\x00\x00\x00";
const char* fn_player_supply_cap_mask = "xx????xxx????xxx????";
typedef uint32_t* (__fastcall* FN_PLAYER_SUPPLY_CAP) (struct DT_Player* player, uint32_t* output);
FN_PLAYER_SUPPLY_CAP fn_player_supply_cap = NULL;

const char* fn_map_x_y_min_max_pattern = "\x8B\x05\x00\x00\x00\x00\x2B\x05\x00\x00\x00\x00\x2D\x00\x00\x00\x00\x89\x44\x24\x08\x8B\x05\x00\x00\x00\x00\x33\x05\x00\x00\x00\x00\x89\x44\x24\x0C\x48\x8B\x44\x24\x00\x48\x05\xD0\x00\x00\x00\xC3";
const char* fn_map_x_y_min_max_mask = "xx????xx????x????xxxxxx????xx????xxxxxxxx?xxxxxxx";
typedef struct DT_MapSize* (__fastcall* FN_MAP_X_Y_MIN_MAX) ();
FN_MAP_X_Y_MIN_MAX fn_map_x_y_min_max = NULL;

// This returns a unit object. unit_list must be (sc2_base + units_list) and unit_index a value below
// *(sc2_base + units_list_length)
const char* fn_unit_get_pattern = "\x40\x53\x48\x83\xEC\x20\x8B\xC2\x8B\xDA\x48\xC1\xE8\x04\x83\xE3"
                                  "\x0F\x48\x83\xC0\x00\x48\x8D\x14\xC1\x8B\x05\x00\x00\x00\x00\x83"
                                  "\xF8\x0F";
const char* fn_unit_get_mask = "xxxxxxxxxxxxxxxxxxxx?xxxxxx????xxx";
typedef struct DT_Unit* (__fastcall* FN_UNIT_GET_LIST) (char* unit_list, uint32_t unit_index);
FN_UNIT_GET_LIST fn_unit_get = NULL;

// This returns 2 if one of the players is neutral (index == 16) or they are neither allies nor enemies. Else
// This returns 0 if index of player and other_player are equal (player is self). Else
// This returns 1 if player and other_player are in the same team. Else
// This returns 3 (they are enemies).
const char* fn_is_owner_ally_neutral_enemy_pattern = "\x80\xF9\x10\x74\x42\x80\xFA\x10\x74\x3D\x3A\xD1\x75\x03"
                                                     "\x33\xC0\xC3";
const char* fn_is_owner_ally_neutral_enemy_mask = "xxxxxxxxxxxxxxxxx";
typedef int32_t(__fastcall* FN_IS_OWNER_ALLY_NEUTRAL_ENEMY) (uint8_t player_index, uint8_t other_player_index);
FN_IS_OWNER_ALLY_NEUTRAL_ENEMY fn_is_owner_ally_neutral_enemy = NULL;

// In all cases this returns the output parameter. The output parameter must be a valid pointer which is set as follows:
// Set selection parameter to get life, shield, energy or zero.
// selection = 0: output is set to the current life.
// selection = 1: output is set to the current shield.
// selection = 2: output is set to the current energy.
// else: output is set to zero.
const char* fn_read_health_shield_energy_pattern = "\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xFA\x45"
                                                   "\x85\xC0\x74\x46\x41\x83\xE8\x01";
const char* fn_read_health_shield_energy_mask = "xxxx?xxxxxxxxxxxxxxxxx";
typedef int32_t* (__fastcall* FN_READ_HEALTH_SHIELD_ENERGY) (struct DT_Unit* unit_ptr, uint32_t* output, int32_t selection);
FN_READ_HEALTH_SHIELD_ENERGY fn_read_health_shield_energy = NULL;

// This has 2 results, we need the lower offset
const char* fn_access_location_by_unit_pattern = "\x48\x89\x5C\x24\x00\x48\x89\x7C\x24\x00\x55\x48\x8B\xEC"
                                                 "\x48\x83\xEC\x30\x33\xC0\x48\x8B\xFA\x48\x89\x45\x10";
const char* fn_access_location_by_unit_mask = "xxxx?xxxx?xxxxxxxxxxxxxxxxx";
typedef int64_t(__fastcall* FN_ACCESS_LOCATION_BY_UNIT) (struct DT_Unit* unit_ptr, struct DT_VectorLocation* output);
FN_ACCESS_LOCATION_BY_UNIT fn_access_location_by_unit = NULL;

const char* fn_EndScene_pattern_win10_20h2 = "\x40\x53\x48\x83\xEC\x40\x48\xC7\x44\x24\x00\x00\x00\x00\x00\x48\x8B"
                                             "\xD9\x48\x8B\xC1\x4C\x8D\x41\x08\x48\xF7\xD8\x48\x1B\xD2\x49\x23\xD0"
                                             "\x45\x33\xC0\x48\x8D\x4C\x24\x00\xE8\x00\x00\x00\x00\x90\x8B\x43\x4C"
                                             "\x83\xE0\x02\x84\xC0\x0F\x85\x00\x00\x00\x00";
const char* fn_EndScene_mask_win10_20h2 = "xxxxxxxxxx?????xxxxxxxxxxxxxxxxxxxxxxxxxx?x????xxxxxxxxxxx????";
const char* fn_EndScene_pattern_win10_19h1 = "\x40\x57\x48\x83\xEC\x40\x48\xC7\x44\x24\x00\x00\x00\x00\x00\x48\x89"
                                             "\x5C\x24\x00\x48\x8B\xF9\x48\x8B\xC1\x48\x8D\x51\x08";
const char* fn_EndScene_mask_win10_19h1 = "xxxxxxxxxx?????xxxx?xxxxxxxxxx";
// HRESULT
typedef long (*FN_END_SCENE) (void* pDevice);
FN_END_SCENE fn_EndScene = NULL;

const char* glob_fn_NtQueryInformationThread_pattern = "\x48\x8B\x0D\x00\x00\x00\x00\x48\x89\x44\x24\x00\x48\x85\xC9";
const char* glob_fn_NtQueryInformationThread_mask = "xxx????xxxx?xxx";
// IDA pattern: 48 8B 0D ? ? ? ? 48 89 44 24 ? 48 85 C9

// v5.0.7.84643
const uint32_t EndScene_level0 = 0x43BDC28;
const uint32_t EndScene_level1 = 0x28;
const uint32_t EndScene_level2 = 0x0;
const uint32_t EndScene_level3 = 0x150; // 0x150 = 8 * 42 (42 is the vtable index of EndScene)

#define THREAD_START_ADDRESS_ENABLED 0
#define WORKER_THREAD_ENABLED 0
#define HOOK_GetSystemTimePreciseAsFileTime_ENABLED 0
#define HOOK_EndScene 1

// Only required if THREAD_START_ADDRESS_ENABLED is enabled
// v5.0.7.84643
const uint32_t thread_start_address = 0x1E000FC;

// Only required if HOOK_GetSystemTimePreciseAsFileTime_ENABLED is enabled
// v5.0.7.84643
const uint32_t glob_fn_GetSystemTimePreciseAsFileTime = 0x39329B8;

// v5.0.7.84643
// unit index maximum 14-bits
const uint32_t units_list = 0x3B64E00;
const uint32_t units_list_length = 0x3B64E00 + 4; // units_list + 4
const uint32_t glob_ingame = 0x3E7905A; // bool
