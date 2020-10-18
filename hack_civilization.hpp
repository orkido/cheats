#ifndef hack_civilization_hpp
#define hack_civilization_hpp

#include "inject_civilization.h"

namespace CivilizationVI {
    bool hack(std::vector<std::pair<uint64_t, uint32_t>>& pointers, OverwriteMode mode, uint64_t selected_address);
    bool deinit_hack();
}

#endif // hack_civilization_hpp
