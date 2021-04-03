#ifndef Starcraft2_hpp
#define Starcraft2_hpp

#include <atomic>

namespace Starcraft2 {
int main_starcraft2(void* driverctl, void* shared_ptr_overlay1, void* shared_ptr_overlay2);

extern uint32_t starcraft2_config_refresh_rate;
extern std::atomic<bool> starcraft2_running;
}

#endif // Starcraft2_hpp
