#pragma once

#include <string>
#include <vector>
#include <optional>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace xr {

struct DisplayInfo {
    std::string deviceName;    // e.g., "\\\\.\\DISPLAY1"
    std::string adapterName;   // GPU adapter description
    std::string monitorName;   // Monitor friendly name from EDID
    int x = 0, y = 0;         // Position in virtual desktop
    int width = 0, height = 0; // Resolution
    bool isPrimary = false;
    bool isXreal = false;
};

class DisplayDetector {
public:
    // Enumerate all active displays and identify XREAL devices.
    static std::vector<DisplayInfo> enumerate();

    // Find the first XREAL display, if any.
    static std::optional<DisplayInfo> findXreal();

    // Print all detected displays to the log.
    static void printList(const std::vector<DisplayInfo>& displays);

private:
    // Check if a monitor name matches known XREAL display identifiers.
    static bool isXrealDisplay(const std::string& monitorName, const std::string& devicePath);
};

} // namespace xr
