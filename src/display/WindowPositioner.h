#pragma once

#include "display/DisplayDetector.h"

#include <GLFW/glfw3.h>

namespace xr {

class WindowPositioner {
public:
    // Move a GLFW window to borderless fullscreen on the specified display.
    // Stores the previous windowed position/size for restoration.
    static bool goFullscreen(GLFWwindow* window, const DisplayInfo& display);

    // Restore the window to its previous windowed position and size.
    static bool restoreWindowed(GLFWwindow* window);

    // Check if the window is currently in fullscreen mode.
    static bool isFullscreen() { return s_isFullscreen; }

    // Toggle between fullscreen on the given display and windowed mode.
    static bool toggle(GLFWwindow* window, const DisplayInfo& display);

private:
    static inline bool s_isFullscreen = false;
    static inline int s_savedX = 0;
    static inline int s_savedY = 0;
    static inline int s_savedWidth = 0;
    static inline int s_savedHeight = 0;
};

} // namespace xr
