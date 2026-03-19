#include "display/WindowPositioner.h"
#include "util/Log.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace xr {

bool WindowPositioner::goFullscreen(GLFWwindow* window, const DisplayInfo& display) {
    if (!window) return false;

    // Save current windowed position and size for later restoration
    glfwGetWindowPos(window, &s_savedX, &s_savedY);
    glfwGetWindowSize(window, &s_savedWidth, &s_savedHeight);

    // Get the native Win32 handle to remove decorations
    HWND hwnd = glfwGetWin32Window(window);
    if (!hwnd) return false;

    // Remove window decorations (title bar, borders) for borderless fullscreen
    LONG style = GetWindowLongA(hwnd, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    SetWindowLongA(hwnd, GWL_STYLE, style);

    // Position the window to cover the target display exactly
    SetWindowPos(hwnd, HWND_TOP,
                 display.x, display.y,
                 display.width, display.height,
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    s_isFullscreen = true;
    Log::info("Fullscreen on {} ({}x{} at {},{}) - {}",
              display.deviceName, display.width, display.height,
              display.x, display.y,
              display.monitorName.empty() ? display.adapterName : display.monitorName);

    return true;
}

bool WindowPositioner::restoreWindowed(GLFWwindow* window) {
    if (!window) return false;

    HWND hwnd = glfwGetWin32Window(window);
    if (!hwnd) return false;

    // Restore window decorations
    LONG style = GetWindowLongA(hwnd, GWL_STYLE);
    style |= WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
    SetWindowLongA(hwnd, GWL_STYLE, style);

    // Restore previous position and size
    SetWindowPos(hwnd, HWND_NOTOPMOST,
                 s_savedX, s_savedY,
                 s_savedWidth, s_savedHeight,
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    s_isFullscreen = false;
    Log::info("Restored windowed mode ({}x{} at {},{})",
              s_savedWidth, s_savedHeight, s_savedX, s_savedY);

    return true;
}

bool WindowPositioner::toggle(GLFWwindow* window, const DisplayInfo& display) {
    if (s_isFullscreen) {
        return restoreWindowed(window);
    } else {
        return goFullscreen(window, display);
    }
}

} // namespace xr
