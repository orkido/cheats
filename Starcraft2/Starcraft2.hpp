#ifndef CoD_Warzone_hpp
#define CoD_Warzone_hpp

#include <atomic>

namespace Starcraft2 {
int main_starcraft2(void* driverctl);

extern uint32_t starcraft2_config_refresh_rate;
extern std::atomic<bool> starcraft2_running;
}

#endif // CoD_Warzone_hpp
