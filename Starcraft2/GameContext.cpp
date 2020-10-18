#include "GameContext.hpp"
#include "Offsets.hpp"

namespace Starcraft2
{
GameContext::GameContext(GameProcess& process) : process(process), local_player_index(-1), base_address(0xDEADBEEF)
{
	for (int i = 0; i < 155; ++i) {
		uintptr_t address = 0xDEADBEEF;
		PlayerEntity player(address, i);
	}
	Offsets::init(*this);
}

GameContext::~GameContext()
{
}

void GameContext::tick()
{
	if (update_data())
		update_overlay();
}

bool GameContext::update_data()
{
	bool result;
	uintptr_t local_player_struct_ptr;

	if (result = process.read(base_address + Offsets::gameinfo + Offsets::local_player, local_player_struct_ptr)) {
		result &= process.read(local_player_struct_ptr + Offsets::local_player_index, local_player_index);
	}

	for (auto& player : players) {
		result &= player->update(*this);
	}

	return result;
}

bool GameContext::update_overlay()
{
	return false;
}
}
