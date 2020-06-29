#ifndef INJECT_CIVILIZATION
#define INJECT_CIVILIZATION

#include <cstdint>

enum OverwriteMode {
    None,
    SetValue,
    AddValue,
};

struct DllConfig {
    uintptr_t pValue[128];
    uint32_t value[128];

    OverwriteMode overwrite_mode;
    uintptr_t overwrite_pValue;
    uint32_t overwrite_value;

    bool keep_overwrite_mode;

    OverwriteMode last_mode;
    uint32_t last_index;
};
#endif
