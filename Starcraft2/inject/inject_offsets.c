#include <stdint.h>

// Force unaligned struct layout because struct exactly matches the game memory offsets
#pragma pack(push, 1)
// All except those marked with "plain" are encoded
struct DT_Unit
{
    unsigned __int32 index;
    char pad_0x0000[20];
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
    __int64 player_visible_num;
    char pad_0x0140[88];
    __int32 m_MissingHealth;
    __int32 m_Shields;
    __int32 m_Energy;
    __int32 m_MaxHealth;
    __int32 m_MaxShield;
    char pad_0x01AC[192];
};
#pragma pack(pop)

/*
// All except those marked with "plain" are encoded
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

const char* fn_local_player_index_pattern = "\x8B\x15\x00\x00\x00\x00\x03\x15\x00\x00\x00\x00\x8B\x0D\x00\x00"
                                            "\x00\x00\x33\x0D\x00\x00\x00\x00\x44\x8B\x05\x00\x00\x00\x00\x89"
                                            "\x54\x24\x08\x89\x4C\x24\x0C\x48\x8B\x44\x24\x00\x8B\x00\x41\x2B"
                                            "\xC0\xF7\xD0\xC1\xE8\x04\xA8\x01\x75\x2D\x89\x54\x24\x08\x89\x4C"
                                            "\x24\x0C\x48\x8B\x44\x24\x00\x8B\x00\x41\x2B\xC0\xF7\xD0\xC1\xE8"
                                            "\x05\xA8\x01\x75\x12\x89\x54\x24\x08\x89\x4C\x24\x0C\x48\x8B\x44"
                                            "\x24\x00\x0F\xB6\x40\x0C\xC3";
const char* fn_local_player_index_mask =    "xx????xx????xx????xx????xxx????xxxxxxxxxxxx?xxxxxxxxxxxxxxxxxxxx"
                                            "xxxxxx?xxxxxxxxxxxxxxxxxxxxxxxxxx?xxxxx";
typedef uint8_t(__stdcall* FN_LOCAL_PLAYER_INDEX) ();
FN_LOCAL_PLAYER_INDEX fn_local_player_index = NULL;


// This returns a unit object. unit_list must be (sc2_base + units_list) and unit_index a value below
// *(sc2_base + units_list_length)
const char* fn_get_unit_pattern = "\x40\x53\x48\x83\xEC\x20\x8B\xC2\x8B\xDA\x48\xC1\xE8\x04\x83\xE3"
                                  "\x0F\x48\x83\xC0\x17\x48\x8D\x14\xC1\x8B\x05\x00\x00\x00\x00\x83"
                                  "\xF8\x0F";
const char* fn_get_unit_mask = "xxxxxxxxxxxxxxxxxxxxxxxxxxx????xxx";
typedef struct DT_Unit* (__fastcall* FN_GET_UNIT_LIST) (char* unit_list, uint32_t unit_index);
FN_GET_UNIT_LIST fn_get_unit = NULL;

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


const char* fn_EndScene_pattern = "\x40\x57\x48\x83\xEC\x40\x48\xC7\x44\x24\x00\x00\x00\x00\x00\x48\x89"
                                  "\x5C\x24\x00\x48\x8B\xF9\x48\x8B\xC1\x48\x8D\x51\x08";
const char* fn_EndScene_mask = "xxxxxxxxxx?????xxxx?xxxxxxxxxx";
// HRESULT
typedef long (*FN_END_SCENE) (void* pDevice);
FN_END_SCENE fn_EndScene = NULL;

// v5.0.6.83830
const uint32_t EndScene_level0 = 0x43DD178;
const uint32_t EndScene_level1 = 0x28;
const uint32_t EndScene_level2 = 0x0;
const uint32_t EndScene_level3 = 0x150; // 0x150 = 8 * 42 (42 is the vtable index of EndScene)

#define THREAD_START_ADDRESS_ENABLED 0
#define WORKER_THREAD_ENABLED 0
#define HOOK_GetSystemTimePreciseAsFileTime_ENABLED 0
#define HOOK_EndScene 1

// Only required if THREAD_START_ADDRESS_ENABLED is enabled
// v5.0.6.83830
const uint32_t thread_start_address = 0x1E0E06C;

// Only required if HOOK_GetSystemTimePreciseAsFileTime_ENABLED is enabled
// v5.0.6.83830
const uint32_t func_GetSystemTimePreciseAsFileTime = 0x39519B8;

// v5.0.6.83830
// unit index maximum 14-bits
const uint32_t units_list = 0x3B83E00;
const uint32_t units_list_length = 0x3B83E00 + 4; // units_list + 4
const uint32_t players_list = 0x3C37720;
const uint32_t mapsize_x = 0x3D984E8;
const uint32_t mapsize_y = 0x3D984E8 + 4; // mapsize_x + 4
