#ifndef hack_starcraft2_hpp
#define hack_starcraft2_hpp

// Include configuration
#include "Starcraft2/Starcraft2.hpp"

bool hack_starcraft2(void* shared_ptr_overlay1, void* shared_ptr_overlay2);
bool deinit_hack_starcraft2(bool unload_driver);

#endif // hack_starcraft2_hpp
