namespace Starcraft2 { class GameContext; }

#ifndef GameContext_hpp
#define GameContext_hpp

#include "Entity.hpp"
#include "Process.hpp"
#include <vector>
#include <memory>

namespace Starcraft2
{

class GameContext
{
public:
	GameContext(GameProcess& process);
	~GameContext();

	void tick();

	GameProcess process;
	uintptr_t base_address;

	uint32_t local_player_index;
	std::vector<std::unique_ptr<PlayerEntity>> players;

private:
	bool update_data();
	bool update_overlay();
};
} // namespace Starcraft2

#endif // GameContext_hpp
