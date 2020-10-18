#include <Windows.h>
#include <cstdint>
#include <csignal>
#include "inject_civilization.h"

struct PlayerChangeState
{
    __int64 vftable;
    __int64 ChangesList;
    __int64 list1;
    __int64 list2;
    char field_20[120];
    __int64 vftable2;
    __int64 field_A0;
    DWORD value;
    BYTE gapAC[44];
    DWORD addValue;
    int field_DC;
};


// ASM and DLL API interface
extern "C" {
    __declspec(dllexport) struct DllConfig dll_config = { 0 };
    uint32_t new_gold_value_buffer;

    void dll_callback_impl(struct PlayerChangeState* player_change_state, uint32_t current_gold, uint32_t add_gold) {
        if (player_change_state->value != current_gold) {
            goto invalid;
        }

        int index = -1;
        // value contained in the buffer
        for (int i = 0; i < sizeof(dll_config.pValue) / sizeof(dll_config.pValue[0]); ++i) {
            if (dll_config.pValue[i] == (uint64_t)&player_change_state->value) {
                index = i;
                break;
            }
        }

        // find next free buffer
        if (index == -1) {
            for (int i = 0; i < sizeof(dll_config.pValue) / sizeof(dll_config.pValue[0]); ++i) {
                if (dll_config.pValue[i] == NULL) {
                    index = i;
                    dll_config.pValue[i] = (uint64_t)&player_change_state->value;
                    break;
                }
            }
        }

        // Buffer full and does not contain our current object, return without any changes
        if (index == -1) {
            new_gold_value_buffer = current_gold + add_gold;
            return;
        }

        // Current object contained in buffer, but not the object we want to overwrite
        if (dll_config.overwrite_pValue != dll_config.pValue[index]) {
            new_gold_value_buffer = current_gold + add_gold;
            dll_config.value[index] = new_gold_value_buffer;
            return;
        }

        // Do not modify values
        if (dll_config.overwrite_mode == OverwriteMode::None || dll_config.overwrite_mode == OverwriteMode::Init) {
            new_gold_value_buffer = current_gold + add_gold;
        } else if (dll_config.overwrite_mode == OverwriteMode::SetValue) {
            new_gold_value_buffer = dll_config.overwrite_value;
        } else if (dll_config.overwrite_mode == OverwriteMode::AddValue) {
            new_gold_value_buffer = current_gold + dll_config.overwrite_value;
        } else {
            // We got some invalid stuff
            goto invalid;
        }

        dll_config.last_mode = dll_config.overwrite_mode;
        dll_config.last_index = index;
        if (!dll_config.keep_overwrite_mode)
            dll_config.overwrite_mode = OverwriteMode::None;
        
        dll_config.value[index] = new_gold_value_buffer;
        return;

    invalid:
#if _DEBUG
        std::raise(SIGINT);
#endif
        new_gold_value_buffer = current_gold + add_gold;
    }
}

BOOL WINAPI DllMain(HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserve)
{
    return true;
}
