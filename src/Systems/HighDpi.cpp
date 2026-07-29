#include "Systems/HighDpi.hpp"

// macOS gets its own implementation in HighDpi.mm (Objective-C++ is needed to talk
// to Cocoa). Everywhere else the window already renders at the screen's real
// resolution, so there is nothing to switch on.
#ifndef __APPLE__

namespace Systems {
    float enableHighDpi(sf::WindowHandle) {
        return 1.f;
    }
}

#endif
