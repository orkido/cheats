#include <BlackBone/DriverControl/DriverControl.h>
#include <optional>

using namespace blackbone;
void showMessageBox(const char* title, const char* text);

#include "hack_starcraft2.hpp"
#include "Starcraft2/Starcraft2.hpp"
#include <thread>
#include <mutex>
#include <cassert>

std::optional<DriverControl> starcraft2_driverctl;
std::thread starcraft2_thread;



bool hack_starcraft2() {
    if (!starcraft2_driverctl)
        starcraft2_driverctl.emplace();

    if (!NT_SUCCESS(starcraft2_driverctl->EnsureLoaded(L""))) {
        showMessageBox("Error", "Failed to load driver!");
        return false;
    }

    Starcraft2::starcraft2_running = true;
    if (!starcraft2_thread.joinable())
        starcraft2_thread = std::move(std::thread(Starcraft2::main_starcraft2, &*starcraft2_driverctl));
    else
        showMessageBox("Info", "Already running");

    return true;
}

bool deinit_hack_starcraft2(bool unload_driver) {
    Starcraft2::starcraft2_running = false;
    if (starcraft2_thread.joinable())
        starcraft2_thread.join();

    if (starcraft2_driverctl && unload_driver) {
        if (!NT_SUCCESS(starcraft2_driverctl->Unload())) {
            showMessageBox("Error", "Failed to unload driver!");
        } else {
            showMessageBox("Ok", "Driver unloaded!");
        }
        starcraft2_driverctl.reset();
    }
    return true;
}
