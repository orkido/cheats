#pragma once

#include "GameContext.hpp"
#include <cstdint>

namespace Starcraft2
{
class Offsets {
public:
	static const uint32_t checksum = 0x03E00777;
	static const uint32_t timestamp = 0x0;

	static const uintptr_t gameinfo = 0xdeadbeef;
	static const uintptr_t local_player = 0x84848; // old *(gameinfo + local_player)
	static const uintptr_t local_player_index = 0x1E8; // old *(*(gameinfo + local_player) + local_player_index)
	static const uintptr_t character_info = 0x97758; // old *(gameinfo + character_info)

	// Offsets in player struct
	// "isme", eigene ID?
	static const uintptr_t entity_num = 0x0; // dump string name: "clientnum"
	static const uintptr_t info_valid = 0x4; // old
	static const uintptr_t message = 0x34;
	static const uintptr_t gamertag = 0x10;
	static const uintptr_t team = 0x14;
	static const uintptr_t name = 0x78;
	static const uintptr_t clan_tag = 0x80;
	static const uintptr_t xuid = 0x8C;
	static const uintptr_t origin = 0xDead;
	static const uintptr_t camera_angle = 0x9B8; // old Vector2



	/*
	client: x,y,z
	sub_7FF78818E490


	m_cameraPos
	m_cameraDir

	userlist
	user_list
	(entitylist)


	listkeys
	*/

	static const uintptr_t entity_list = 0x10B58C98;

	static void init(GameContext& context) {
	}
};
}