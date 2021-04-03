#include <BlackBone/DriverControl/DriverControl.h>
#include <optional>

using namespace blackbone;
void showMessageBox(const char* title, const char* text);

#include "hack_apex.hpp"
#include <thread>
#include <mutex>
#include <cassert>
int main_apexbot(DriverControl* driverctl, void* shared_ptr_overlay);

std::optional<DriverControl> driverctl;
std::atomic<bool> apexbot_running;
std::thread apexbot_thread;

uint32_t config_refresh_rate;
float config_overlay_propsurvival_radius;
bool config_unload_driver, config_aimbot, config_aimbot_teammates, config_highlight, config_highlight_teammates, config_display_overlay;

bool hack_apex(void* shared_ptr_overlay) {
    if (!driverctl)
        driverctl.emplace();

    if (!NT_SUCCESS(driverctl->EnsureLoaded(L""))) {
        showMessageBox("Error", "Failed to load driver!");
    }

    apexbot_running = true;
    if (!apexbot_thread.joinable())
        apexbot_thread = std::move(std::thread(main_apexbot, &*driverctl, shared_ptr_overlay));
    else
        showMessageBox("Info", "Already running");

	return true;
}

bool deinit_hack_apex() {
    // Stop apexbot background worker
    apexbot_running = false;
    if (apexbot_thread.joinable())
        apexbot_thread.join();

    // Unload kernel module
	if (driverctl && config_unload_driver) {
        if (!NT_SUCCESS(driverctl->Unload())) {
            showMessageBox("Error", "Failed to unload driver!");
        } else {
            showMessageBox("Ok", "Driver unloaded!");
        }
        driverctl.reset();
	}
    return true;
}
