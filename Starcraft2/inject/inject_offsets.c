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
    char pad_0x0024[32];
    unsigned __int8 owner_player_id;
    unsigned __int8 control_type;
    char pad_0x0042[14];
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
    char gap8AF[64];
    struct DT_Race* race_struct;
    char gap8f8[9];
    char gap901[364];
    int supply_max_cap;
    char gapA78[1447];
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
race_id // plain; 1 = Protoss, 2 = Terran, 3 = Zerg

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
// add rsp, 0x48
// ret
const char* fn_call_wrapper_pattern = "\xE8\x00\x00\x00\x00\x48\x83\xC4\x48\xC3";
const char* fn_call_wrapper_mask = "x????xxxxx";
void* fn_call_wrapper = NULL;

typedef uint8_t(__stdcall* FN_LOCAL_PLAYER_INDEX) ();
FN_LOCAL_PLAYER_INDEX fn_local_player_index = NULL;

// If player_list is NULL then the global player list is resolved in the function.
// This function cannot be used from outside of the main Starcraft 2 code section due to protection.
typedef struct DT_Player* (__fastcall* FN_PLAYER_GET) (uint8_t player_index, char* player_list);
FN_PLAYER_GET fn_player_get = NULL;

typedef char* (__fastcall* FN_PLAYER_GLOBAL_LIST) ();
FN_PLAYER_GLOBAL_LIST fn_player_global_list = NULL;

// This function returns a pointer to an unknown 8 bytes value. After those 8 bytes the player name follows in C-string format
typedef char* (__fastcall* FN_PLAYER_GET_NAME) (struct DT_Player* player);
FN_PLAYER_GET_NAME fn_player_get_name = NULL;

// This function returns a pointer to an unknown 8 bytes value. After those 8 bytes the player clan tag follows in C-string format
typedef char* (__fastcall* FN_PLAYER_GET_CLANTAG) (struct DT_Player* player);
FN_PLAYER_GET_CLANTAG fn_player_get_clantag = NULL;

// The return color seems to be a ARGB value
typedef uint32_t* (__fastcall* FN_PLAYER_GET_COLOR) (uint32_t* ARGB_color_buffer_output, uint8_t player_index);
FN_PLAYER_GET_COLOR fn_player_get_color = NULL;

typedef int32_t(__fastcall* FN_PLAYER_CAMERA_PITCH) (uint32_t player_index);
FN_PLAYER_CAMERA_PITCH fn_player_camera_pitch = NULL;

typedef int32_t(__fastcall* FN_PLAYER_CAMERA_YAW) (uint32_t player_index);
FN_PLAYER_CAMERA_YAW fn_player_camera_yaw = NULL;

typedef uint64_t(__fastcall* FN_PLAYER_CAMERA_LOCATION) (uint32_t player_index);
FN_PLAYER_CAMERA_LOCATION fn_player_camera_location = NULL;

typedef uint32_t(__fastcall* FN_PLAYER_CAMERA_GET_DISTANCE) (uint32_t player_index);
FN_PLAYER_CAMERA_GET_DISTANCE fn_player_camera_get_distance = NULL;

typedef struct DT_MapSize*(__fastcall* FN_PLAYER_GET_CAMERA_BOUNDS) (uint8_t player_index);
FN_PLAYER_GET_CAMERA_BOUNDS fn_player_get_camera_bounds = NULL;

// resource_selection must be in range [0; 3]
typedef int32_t(__fastcall* FN_PLAYER_GET_RESOURCES) (struct DT_Player* player, uint64_t resource_selection);
FN_PLAYER_GET_RESOURCES fn_player_get_resources = NULL;

typedef int32_t(__fastcall* FN_PLAYER_RACESTRUCT_GET_RACE) (struct DT_Race** race);
FN_PLAYER_RACESTRUCT_GET_RACE fn_player_racestruct_get_race = NULL;

typedef uint32_t* (__fastcall* FN_PLAYER_SUPPLY_CAP_DECRYPT) (struct DT_Player* player, uint32_t* output);
FN_PLAYER_SUPPLY_CAP_DECRYPT fn_player_supply_cap_decrypt = NULL;

typedef struct DT_MapSize* (__fastcall* FN_MAP_X_Y_MIN_MAX) ();
FN_MAP_X_Y_MIN_MAX fn_map_x_y_min_max = NULL;

// This returns a unit object. unit_list must be (sc2_base + units_list) and unit_index a value below
// *(sc2_base + units_list_length)
typedef struct DT_Unit* (__fastcall* FN_UNIT_GET_LIST) (char* unit_list, uint32_t unit_index);
FN_UNIT_GET_LIST fn_unit_get = NULL;

// This returns 2 if one of the players is neutral (index == 16) or they are neither allies nor enemies. Else
// This returns 0 if index of player and other_player are equal (player is self). Else
// This returns 1 if player and other_player are in the same team. Else
// This returns 3 (they are enemies).
typedef int32_t(__fastcall* FN_IS_OWNER_ALLY_NEUTRAL_ENEMY) (uint8_t player_index, uint8_t other_player_index);
FN_IS_OWNER_ALLY_NEUTRAL_ENEMY fn_is_owner_ally_neutral_enemy = NULL;

// In all cases this returns the output parameter. The output parameter must be a valid pointer which is set as follows:
// Set selection parameter to get life, shield, energy or zero.
// selection = 0: output is set to the current life.
// selection = 1: output is set to the current shield.
// selection = 2: output is set to the current energy.
// else: output is set to zero.
typedef int32_t* (__fastcall* FN_READ_HEALTH_SHIELD_ENERGY) (struct DT_Unit* unit_ptr, uint32_t* output, int32_t selection);
FN_READ_HEALTH_SHIELD_ENERGY fn_read_health_shield_energy = NULL;

typedef int64_t(__fastcall* FN_ACCESS_LOCATION_BY_UNIT) (struct DT_Unit* unit_ptr, struct DT_VectorLocation* output);
FN_ACCESS_LOCATION_BY_UNIT fn_access_location_by_unit = NULL;


// win10_20h2, win10_21h2
const char* fn_EndScene_pattern_win10_20h2 = "\x40\x53\x48\x83\xEC\x40\x48\xC7\x44\x24\x00\x00\x00\x00\x00\x48\x8B"
                                             "\xD9\x48\x8B\xC1\x4C\x8D\x41\x08\x48\xF7\xD8\x48\x1B\xD2\x49\x23\xD0"
                                             "\x45\x33\xC0\x48\x8D\x4C\x24\x00\xE8\x00\x00\x00\x00\x90\x8B\x43\x4C"
                                             "\x83\xE0\x02\x84\xC0\x0F\x85\x00\x00\x00\x00";
const char* fn_EndScene_mask_win10_20h2    = "xxxxxxxxxx?????xxxxxxxxxxxxxxxxxxxxxxxxxx?x????xxxxxxxxxxx????";
const char* fn_EndScene_pattern_win10_19h1 = "\x40\x57\x48\x83\xEC\x40\x48\xC7\x44\x24\x00\x00\x00\x00\x00\x48\x89"
                                             "\x5C\x24\x00\x48\x8B\xF9\x48\x8B\xC1\x48\x8D\x51\x08";
const char* fn_EndScene_mask_win10_19h1    = "xxxxxxxxxx?????xxxx?xxxxxxxxxx";
// HRESULT
typedef long (*FN_END_SCENE) (void* pDevice);
FN_END_SCENE fn_EndScene = NULL;


// v5.0.8.86383
const uint64_t fn_access_health2_offset = 0x0087c740;
const uint64_t fn_access_location_by_unit_offset = 0x00693ce0;
const uint64_t fn_access_location_by_unit_plain_offset = 0x0265b4a0;
const uint64_t fn_access_location_by_unknown_unit_type_offset = 0x00693310;
const uint64_t fn_access_unit_visible_offset = 0x006946a0;
const uint64_t fn_alloc_memory_offset = 0x015b5ce0;
const uint64_t fn_calloc_memory_offset = 0x0023aa30;
const uint64_t fn_decrypt_location1_offset = 0x002b65b0;
const uint64_t fn_decrypt_location2_offset = 0x002b6660;
const uint64_t fn_decrypt_location3_offset = 0x002b6710;
const uint64_t fn_enc_multiplier_energy_offset = 0x0087eec0;
const uint64_t fn_enc_multiplier_life_offset = 0x006a30d0;
const uint64_t fn_enc_multiplier_shields_offset = 0x0069fb30;
const uint64_t fn_encrypt_location1_offset = 0x002b9600;
const uint64_t fn_encrypt_location10_offset = 0x002b99f0;
const uint64_t fn_encrypt_location11_offset = 0x002b9a60;
const uint64_t fn_encrypt_location12_offset = 0x002b9ad0;
const uint64_t fn_encrypt_location13_offset = 0x002b9b40;
const uint64_t fn_encrypt_location14_offset = 0x002b9bb0;
const uint64_t fn_encrypt_location15_offset = 0x002b9c20;
const uint64_t fn_encrypt_location16_offset = 0x002b9ca0;
const uint64_t fn_encrypt_location2_offset = 0x002b9670;
const uint64_t fn_encrypt_location3_offset = 0x002b96d0;
const uint64_t fn_encrypt_location4_offset = 0x002b9740;
const uint64_t fn_encrypt_location5_offset = 0x002b97b0;
const uint64_t fn_encrypt_location6_offset = 0x002b9810;
const uint64_t fn_encrypt_location7_offset = 0x002b9890;
const uint64_t fn_encrypt_location8_offset = 0x002b9900;
const uint64_t fn_encrypt_location9_offset = 0x002b9970;
const uint64_t fn_event_name_by_index_offset = 0x00785bd0;
const uint64_t fn_event_name_by_index2_offset = 0x00785a80;
const uint64_t fn_get_unit_by_unit_offset = 0x00663ad0;
const uint64_t fn_get_unit_color_visibility_controller_offset = 0x00dff4d0;
const uint64_t fn_get_unitprop_name_by_index_offset = 0x008ac910;
const uint64_t fn_glob_symbols_decrypt_offset = 0x0262d110;
const uint64_t fn_glob_symbols_decrypt2_offset = 0x02637b70;
const uint64_t fn_glob_symbols_decrypt3_offset = 0x0262ce50;
const uint64_t fn_is_owner_ally_neutral_enemy_offset = 0x0075f5f0;
const uint64_t fn_local_player_index_offset = 0x00a993d0;
const uint64_t fn_map_set_playable_region_for_each_player_offset = 0x00681690;
const uint64_t fn_map_x_y_min_max_offset = 0x00675ad0;
const uint64_t fn_mapsize_access_offset = 0x00814960;
const uint64_t fn_player_ai_control_allowed_offset = 0x02669870;
const uint64_t fn_player_camera_get_distance_offset = 0x00974e30;
const uint64_t fn_player_camera_location_offset = 0x009751b0;
const uint64_t fn_player_camera_maybe_location_offset = 0x00976130;
const uint64_t fn_player_camera_pitch_offset = 0x00974ff0;
const uint64_t fn_player_camera_yaw_offset = 0x00975d10;
const uint64_t fn_player_color_print_offset = 0x00c781b0;
const uint64_t fn_player_dump_name_clan_color_supply_race_offset = 0x00c3a370;
const uint64_t fn_player_get_offset = 0x0074cb80;
const uint64_t fn_player_get_0_offset = 0x0074d5b0;
const uint64_t fn_player_get_by_unit_owner_offset = 0x006a1f10;
const uint64_t fn_player_get_by_unit_owner_no_glob_player_list_offset = 0x006a1f30;
const uint64_t fn_player_get_camera_bounds_offset = 0x00675b00;
const uint64_t fn_player_get_clantag_offset = 0x0075a290;
const uint64_t fn_player_get_color_offset = 0x00ab2cf0;
const uint64_t fn_player_get_name_offset = 0x0075e680;
const uint64_t fn_player_get_resources_offset = 0x00685e80;
const uint64_t fn_player_global_list_offset = 0x0074dfe0;
const uint64_t fn_player_racestruct_get_race_offset = 0x01857560;
const uint64_t fn_player_set_minerals_and_gas_offset = 0x00758770;
const uint64_t fn_player_set_minerals_and_gas_wrapper_offset = 0x007583b0;
const uint64_t fn_player_start_location_offset = 0x02679a40;
const uint64_t fn_player_supply_cap_decrypt_offset = 0x006a1f80;
const uint64_t fn_print_int_prepared_buf_offset = 0x0023c750;
const uint64_t fn_print_int_to_string_offset = 0x0023c7f0;
const uint64_t fn_print_prepare_offset = 0x0023be60;
const uint64_t fn_print_prepared_and_filled_buf_offset = 0x009742a0;
const uint64_t fn_print_string_prepared_buf_offset = 0x0023d5b0;
const uint64_t fn_print_value_out_of_bounds_offset = 0x00824f90;
const uint64_t fn_read_health_shield_energy_offset = 0x008805c0;
const uint64_t fn_read_max_health_shield_energy_offset = 0x0087f030;
const uint64_t fn_render_access_offset = 0x00e607f0;
const uint64_t fn_resolve_GetSystemTimePreciseAsFileTime_offset = 0x0198fe40;
const uint64_t fn_set_is_offset = 0x00f51dc0;
const uint64_t fn_set_is_not_offset = 0x00f54ea0;
const uint64_t fn_something_is_owner_ally_enemy_offset = 0x00d4a6b0;
const uint64_t fn_something_itemcount_offset = 0x0269daf0;
const uint64_t fn_strncmp_offset = 0x01dfa430;
const uint64_t fn_thread_init_offset = 0x01e0b96c;
const uint64_t fn_unit_access_owner_and_unknown_offset = 0x00a50d60;
const uint64_t fn_unit_dump_owner_tag_relation_offset = 0x012fd2b0;
const uint64_t fn_unit_get_offset = 0x00662b30;
const uint64_t fn_unit_get_owner_offset = 0x00d6aea0;
const uint64_t fn_unit_get_visibility_offset = 0x00e083a0;
const uint64_t fn_unit_is_selected_by_player_offset = 0x009b4660;
const uint64_t fn_unit_item_part1_offset = 0x02647060;
const uint64_t fn_unit_item_part2_offset = 0x002fd5e0;
const uint64_t fn_unit_maybe_computer_controlled_offset = 0x00e006e0;
const uint64_t fn_unit_maybe_user_controlled_offset = 0x00e009c0;
const uint64_t fn_unit_set_owner_offset = 0x009be190;
const uint64_t fn_unit_struct_init_offset = 0x00960f60;
const uint64_t fn_update_energy_offset = 0x0087fc20;
const uint64_t fn_update_life_offset = 0x0087fd60;
const uint64_t fn_update_max_life_offset = 0x0087ffe0;
const uint64_t fn_update_shields_offset = 0x00880300;
const uint64_t fn_update_shields_max_offset = 0x008800a0;
const uint64_t fn_update_unit_offset = 0x009bf960;
const uint64_t glob_EndScene_pointer_offset = 0x043d7018;
const uint64_t glob_EventList_offset = 0x03911d20;
const uint64_t glob_UnitPropList_offset = 0x03913d60;
const uint64_t glob_fn_GetProcAddress_offset = 0x02d08cb8;
const uint64_t glob_fn_GetSystemTimePreciseAsFileTime_offset = 0x0394b9b8;
const uint64_t glob_fn_LoadLibrary_offset = 0x02d08c68;
const uint64_t glob_fn_NtQueryInformationThread_offset = 0x039ee7e0;
const uint64_t glob_ingame_offset = 0x03e922ea;
const uint64_t glob_mapsize_x_offset = 0x03d924e8;
const uint64_t glob_mapsize_y_offset = 0x03d924ec;
const uint64_t glob_player_clantag_offset = 0x03e92890;
const uint64_t glob_player_list_ally_neutral_enemy_offset = 0x03c31730;
const uint64_t glob_player_names_offset = 0x03e92b10;
const uint64_t glob_region_list_offset = 0x077bef90;
const uint64_t glob_sc2_code_length_offset = 0x02d1c8f8;
const uint64_t glob_sc2_code_length2_offset = 0x02d1c908;
const uint64_t glob_sc2_code_start_offset = 0x02d1c8f0;
const uint64_t glob_sc2_code_start2_offset = 0x02d1c900;
const uint64_t glob_symbol_names_offset = 0x077ae220;
const uint64_t glob_units_list_offset = 0x03b7de40;
const uint64_t glob_units_list_len_offset = 0x03b7de44;

#define THREAD_START_ADDRESS_ENABLED 0
#define WORKER_THREAD_ENABLED 0
#define HOOK_GetSystemTimePreciseAsFileTime_ENABLED 0
#define HOOK_EndScene_ENABLED 0
#define EXTERNAL_HOOK_EndScene_ENABLED 1

// Only required if HOOK_EndScene_ENABLED is enabled
// v5.0.8.86383
const uint32_t EndScene_level0 = 0x043d7018;
const uint32_t EndScene_level1 = 0x28;
const uint32_t EndScene_level2 = 0x0;
const uint32_t EndScene_level3 = 0x150; // 0x150 = 8 * 42 (42 is the vtable index of EndScene)


// Only required if THREAD_START_ADDRESS_ENABLED is enabled
// v5.0.7.84643
const uint32_t thread_start_address = 0x1E000FC;

// Only required if HOOK_GetSystemTimePreciseAsFileTime_ENABLED is enabled
// v5.0.8.86383
const uint32_t glob_fn_GetSystemTimePreciseAsFileTime = 0x0394b9b8;

// v5.0.8.86383
// unit index maximum 14-bits
const uint32_t units_list = 0x03b7de40;
const uint32_t units_list_length = 0x03b7de40 + 4; // units_list + 4
const uint32_t glob_ingame = 0x03e922ea; // bool
