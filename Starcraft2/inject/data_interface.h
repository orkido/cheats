// This file defines the interface exported to the external tool

#ifndef DATA_INTERFACE_H
#define DATA_INTERFACE_H

#include <stdint.h>

struct Player {
    uint64_t address;

    int32_t camera_yaw;
    int32_t camera_pitch;
    uint64_t camera_location;

    char name[128];
    uint32_t minerals;
    uint32_t vespene;
    uint32_t resource1;
    uint32_t resource2;
    uint32_t supply;
};

enum Team {
    Self = 0,
    Ally = 1,
    Enemy = 3,
    Neutral = 2,
};

struct Unit {
    uint64_t address;
    uint32_t index;
    uint32_t index_unknown;
    float position_x;
    float position_y;
    float position_z;
    int32_t position_unknown1;
    int32_t position_unknown2;
    int32_t position_unknown3;
    int32_t position_unknown4;
    uint32_t health;
    uint32_t shields;
    uint32_t energy;
    enum Team team;
    uint8_t unknown_player_id;
    uint8_t owner_player_id;
    uint8_t interesting_value;
    uint8_t interesting_value2;
    uint64_t player_visible_num;
    uint32_t player_id;
    uint8_t control_type;
    uint8_t amount_units_attacking_self;
};

struct SC2Data {
    uint8_t ingame;
    uint32_t total_mapsize_x;
    uint32_t total_mapsize_y;
    uint32_t playable_mapsize_x_min;
    uint32_t playable_mapsize_x_max;
    uint32_t playable_mapsize_y_min;
    uint32_t playable_mapsize_y_max;
    uint64_t camera_bounds_address;
    uint8_t local_player_index;
    uint8_t overwrite_local_player_index;
    uint32_t players_length;
    // Player index 16 is neutral
    struct Player players[17];
    uint32_t units_length;
    struct Unit units[1 << 16];
};

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)

#define GLOBAL_SC2DATA_NAME g_sc2data

#define CONST_OFFSET_NAME search_string
#define CONST_OFFSET_VALUE "You must REPLACE this with a long unique C-string (no null-bytes) by simply pressing some random buttons on your keyboard. WARNING: Do NOT just append or insert something, do a full replacement!"

#endif
