#include <atomic>

// apexbot main loop state
extern std::atomic<bool> apexbot_running;

// config
extern int32_t config_fov;
// update every N ms
extern uint32_t config_refresh_rate;
// radius of sphere in overlay
extern float config_overlay_propsurvival_radius;
