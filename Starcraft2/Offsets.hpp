#pragma once

#include <cstdint>

namespace Starcraft2
{
class Offsets {
public:
	// v5.0.7.84643
	static const uint32_t func_NtQueryInformationThread = 0x39D57A0;
	static const uint32_t checksum = 0x03DF0D2F;
	static const uint32_t timestamp = 0x6067BEAC;

	// Offsets in player struct
	// "isme", eigene ID?
	/*static const uintptr_t entity_num = 0x0; // dump string name: "clientnum"
	static const uintptr_t info_valid = 0x4; // old
	static const uintptr_t message = 0x34;
	static const uintptr_t gamertag = 0x10;
	static const uintptr_t team = 0x14;
	static const uintptr_t name = 0x78;
	static const uintptr_t clan_tag = 0x80;
	static const uintptr_t xuid = 0x8C;
	static const uintptr_t origin = 0xDead;
	static const uintptr_t camera_angle = 0x9B8; // old Vector2

	static const uintptr_t entity_list = 0x10B58C98;
	*/
};
}