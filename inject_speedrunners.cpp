#include <Windows.h>
#include "inject_speedrunners.h"

BOOL WINAPI DllMain(HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserve)
{
    /*if (dwReason == DLL_PROCESS_ATTACH)
    {
        ::MessageBoxA(NULL, (LPCSTR)"My fucking code.", (LPCSTR)"Tired of this shit", NULL);
    }*/
    return true;
}

struct userdata {
    char __unknown[0x70];
    wchar_t* username; // wchar starts at offset +8 bytes
    char __unknown2[0x23C - 0x70 - sizeof(username)];
    float boost;
};

__declspec(dllexport) int dll_callback_impl(struct DllConfig* dllconfig, struct userdata* userdata, float* _new_value) {
    if (lstrcmpW(userdata->username + 4, dllconfig->username)) {
        return 0;
    }

    float new_value = *_new_value;
    float old_value = userdata->boost;
    float factor = dllconfig->boost_factor;
    if (new_value < 0.001f) {
        new_value = 0.0f;
    } else if (new_value < old_value) {
        new_value = old_value - ((old_value - new_value) * factor);
    }
    *_new_value = new_value;
    return 0;
}

extern "C" __declspec(dllexport) int __stdcall dll_callback(struct DllConfig* dllconfig, struct userdata* userdata, float* _new_value) {
    return dll_callback_impl(dllconfig, userdata, _new_value);
}
