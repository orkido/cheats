#ifndef INJECT_CIVILIZATION
#define INJECT_CIVILIZATION

#include <cstdint>

enum OverwriteMode : uint32_t {
    None,
    SetValue,
    AddValue,
    Init,
};

struct DllConfig {
    uint64_t pValue[128];
    uint32_t value[128];

    OverwriteMode overwrite_mode;
    uint64_t overwrite_pValue;
    uint32_t overwrite_value;

    bool keep_overwrite_mode;

    OverwriteMode last_mode;
    uint32_t last_index;
};
#endif
