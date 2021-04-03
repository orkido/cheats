#include "inject/data_interface.h"

namespace Starcraft2 {

class Overlay {
public:
    Overlay(void* shared_ptr_overlay1, void* shared_ptr_overlay2);

    void update_overlay(struct SC2Data* sc2_data);

private:
    void* shared_ptr_overlay1;
    void* shared_ptr_overlay2;
};

}
