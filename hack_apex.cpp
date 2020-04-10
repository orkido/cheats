#include <BlackBone/DriverControl/DriverControl.h>
#include <optional>

using namespace blackbone;
void showMessageBox(const char* title, const char* text);

struct HackStateException : public std::exception {
    std::string caption;
    std::string text;

    HackStateException(const char* caption, const char* text) {
        this->caption = caption;
        this->text = text;
    }

    HackStateException(std::string caption, std::string text) {
        this->caption = caption;
        this->text = text;
    }
    /*const char* what() const throw () {
        return "C++ Exception";
    }*/
};

#include "hack_apex.hpp"
#include <thread>
#include <mutex>
#include <cassert>
int main_apexbot(DriverControl* driverctl);

std::optional<DriverControl> driverctl;
std::atomic<bool> apexbot_running;
std::thread apexbot_thread;

int32_t config_fov;
uint32_t config_refresh_rate;
float config_overlay_propsurvival_radius;
bool config_unload_driver, config_aimbot, config_aimbot_teammates, config_highlight, config_highlight_teammates, config_display_overlay;


bool hack_apex() {
    // ***REMOVED***
    if (!driverctl)
        driverctl.emplace();

    try {
        if (!NT_SUCCESS(driverctl->EnsureLoaded(L""))) {
            throw HackStateException("Error", "Failed to load driver!");
        }
    }
    catch (const HackStateException& e) {
        showMessageBox(e.caption.c_str(), e.text.c_str());
        return false;
    }

    apexbot_running = true;
    if (!apexbot_thread.joinable())
        apexbot_thread = std::move(std::thread(main_apexbot, &*driverctl));
    else
        showMessageBox("Info", "Already running");

	return true;
}

bool deinit_hack_apex() {
    apexbot_running = false;
    if (apexbot_thread.joinable())
        apexbot_thread.join();

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
