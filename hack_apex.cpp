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


#include <thread>
#include <mutex>
#include <atomic>
#include <cassert>
int main_apexbot(DriverControl* driverctl);

std::optional<DriverControl> driverctl;
std::atomic<bool> apexbot_running;
std::thread apexbot_thread;


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
    apexbot_thread.join();

	if (driverctl) {
        if (!NT_SUCCESS(driverctl->Unload())) {
            showMessageBox("Error", "Failed to unload driver!");
        } else {
            showMessageBox("Ok", "Driver unloaded!");
        }
        driverctl.reset();
	}
    return true;
}
