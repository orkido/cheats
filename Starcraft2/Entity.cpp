#include "Entity.hpp"
#include "Offsets.hpp"

namespace Starcraft2
{
PlayerEntity::PlayerEntity(uintptr_t _address, int _entityNum) :
	address(_address), entityNum_check(_entityNum), entityNum(0), infoValid(0), team(team_t::TEAM_FREE), origin(), camera_angle()
{
}

PlayerEntity::~PlayerEntity()
{
}

bool PlayerEntity::isValid()
{
	return infoValid == 1 && entityNum == entityNum_check;
}

int PlayerEntity::update(GameContext& context)
{
	context.process.read(address + Offsets::entity_num, entityNum);
    context.process.read(address + Offsets::info_valid, infoValid);
    context.process.read(address + Offsets::team, team);

    context.process.read(address + Offsets::origin, origin);
    context.process.read(address + Offsets::camera_angle, camera_angle);

	return isValid();
}
}
