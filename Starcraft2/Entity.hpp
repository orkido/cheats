namespace Starcraft2 { class PlayerEntity; }

#ifndef Entity_hpp
#define Entity_hpp

#include "sdk.hpp"
#include <cstdint>
#include "GameContext.hpp"

namespace Starcraft2
{

enum class team_t : uint32_t
{
	TEAM_FREE = 0x00,
	TEAM_NUM_TEAMS = 0x9B,
	TEAM_SPECTATOR = 0x99,
};

class PlayerEntity
{
public:
	PlayerEntity(uintptr_t address, int entityNum);
	~PlayerEntity();

	bool isValid();

	int update(GameContext& context);

private:
	uintptr_t address;
	int entityNum_check;

	int entityNum;
	int infoValid;
	team_t team;

	Vec3 origin;
	Vec3 camera_angle;
};
} // namespace Starcraft2

#endif // Entity_hpp
