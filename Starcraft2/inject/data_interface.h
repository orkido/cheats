// This file defines the interface exported to the external tool

#ifndef DATA_INTERFACE_H
#define DATA_INTERFACE_H

#include <stdint.h>

struct Player {
    uint32_t index;
    char name[128];
    uint32_t minerals;
    uint32_t vespene;
    uint32_t supply;
};

struct Unit {
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
};

struct SC2Data {
    uint32_t mapsize_x;
    uint32_t mapsize_y;
    uint8_t local_player_index;
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
