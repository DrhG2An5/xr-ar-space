#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <string>
#include <vector>

namespace xr {

// Virtual display extension: makes the app appear as an additional monitor
// to Windows, so windows can be dragged onto it and captured.
//
// Strategy:
//   1. Try to find and enable an Indirect Display Driver (IDD) virtual monitor
//   2. If no IDD available, create a headless "capture desktop" by setting up
//      a virtual desktop position that the app will capture from
//
// Requires: IddSampleDriver or similar IDD installed on the system.
// See: https://github.com/roshkins/IddSampleDriver
class VirtualDisplay {
public:
    struct MonitorInfo {
        int width = 1920;
        int height = 1080;
        int refreshRate = 60;
        std::string name = "XR Virtual Display";
    };

    VirtualDisplay() = default;
    ~VirtualDisplay();

    VirtualDisplay(const VirtualDisplay&) = delete;
    VirtualDisplay& operator=(const VirtualDisplay&) = delete;

    // Attempt to create a virtual display with the given resolution.
    // Returns true if a virtual display adapter was found and configured.
    bool create(int width = 1920, int height = 1080, int refreshRate = 60);

    // Destroy the virtual display.
    void destroy();

    // Check if a virtual display is currently active.
    bool isActive() const { return m_active; }

    // Get the virtual display's desktop coordinates (for positioning windows on it).
    RECT desktopRect() const { return m_desktopRect; }

    // Get the device name (e.g., "\\\\.\\DISPLAY5") for use with capture APIs.
    const std::string& deviceName() const { return m_deviceName; }

    // Get monitor info
    const MonitorInfo& info() const { return m_info; }

    // Check if an IDD virtual display driver is installed on the system.
    static bool isDriverInstalled();

    // Enumerate currently active virtual displays (IDD-based).
    static std::vector<std::string> enumerateVirtualDisplays();

private:
    bool enableIddMonitor(int width, int height, int refreshRate);
    bool findVirtualMonitor();

    bool m_active = false;
    RECT m_desktopRect{};
    std::string m_deviceName;
    MonitorInfo m_info;

    // Handle to the software device (for cleanup)
    void* m_deviceInfo = nullptr;
};

} // namespace xr
