#include <atomic>

// apexbot main loop state
extern std::atomic<bool> apexbot_running;

// config
extern int32_t config_fov;
// update every N ms
extern uint32_t config_refresh_rate;
// radius of sphere in overlay
extern float config_overlay_propsurvival_radius;
// unload driver on exit
extern bool config_unload_driver;
// enable apexbot functionality
extern bool config_highlight;
extern bool config_highlight_teammates;
extern bool config_aimbot;
extern bool config_aimbot_teammates;
// enable Qt overlay
extern bool config_display_overlay;
