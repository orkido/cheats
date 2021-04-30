#include "overlay_data_interface.hpp"
//#include <Qt3DExtras>

namespace QtOverlay {

struct OverlayConfig overlay_config;

enum OverlayItemType {
    CubeMiddle,

    BoxLeft,
    BoxRight,
    BoxTop,
    BoxBottom,

    BarHealth,
    BarShield,
};
}
