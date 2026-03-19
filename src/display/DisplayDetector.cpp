#include "display/DisplayDetector.h"
#include "util/Log.h"

#include <SetupAPI.h>
#include <initguid.h>
#include <devpkey.h>
#include <algorithm>
#include <cctype>

#pragma comment(lib, "setupapi.lib")

namespace xr {

// Known XREAL display identifiers found in EDID manufacturer/model strings
static const char* kXrealIdentifiers[] = {
    "xreal",
    "nreal",         // Older branding (pre-rename)
    "air 2",
    "air2",
    "air ultra",
    "XREAL",
    "NR-7100",       // Air model number in some EDID dumps
    "NR-7200",       // Air 2
    "NR-7300",       // Air 2 Pro
    "NR-7400",       // Ultra
};

static std::string toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

// Get the monitor friendly name from GDI device path using EnumDisplayDevices
static std::string getMonitorName(const char* deviceName) {
    DISPLAY_DEVICEA mon{};
    mon.cb = sizeof(mon);

    // EnumDisplayDevices with the adapter name gives us the monitor
    if (EnumDisplayDevicesA(deviceName, 0, &mon, 0)) {
        // mon.DeviceString contains the friendly name (e.g., "XREAL Air 2 Pro")
        return mon.DeviceString;
    }
    return "";
}

bool DisplayDetector::isXrealDisplay(const std::string& monitorName, const std::string& devicePath) {
    std::string nameLower = toLower(monitorName);
    std::string pathLower = toLower(devicePath);

    for (const auto* id : kXrealIdentifiers) {
        std::string idLower = toLower(id);
        if (nameLower.find(idLower) != std::string::npos ||
            pathLower.find(idLower) != std::string::npos) {
            return true;
        }
    }

    // Also check for XREAL's known USB display VID in the device path
    // XREAL displays often show up with VID_3318 in their hardware path
    if (pathLower.find("vid_3318") != std::string::npos ||
        pathLower.find("vid&0003318") != std::string::npos) {
        return true;
    }

    return false;
}

std::vector<DisplayInfo> DisplayDetector::enumerate() {
    std::vector<DisplayInfo> displays;

    DISPLAY_DEVICEA adapter{};
    adapter.cb = sizeof(adapter);

    for (DWORD i = 0; EnumDisplayDevicesA(nullptr, i, &adapter, 0); ++i) {
        // Skip inactive adapters
        if (!(adapter.StateFlags & DISPLAY_DEVICE_ACTIVE)) continue;

        // Get current display settings for this adapter
        DEVMODEA mode{};
        mode.dmSize = sizeof(mode);
        if (!EnumDisplaySettingsA(adapter.DeviceName, ENUM_CURRENT_SETTINGS, &mode)) continue;

        // Get the monitor device attached to this adapter
        std::string monitorName = getMonitorName(adapter.DeviceName);

        DisplayInfo info;
        info.deviceName = adapter.DeviceName;
        info.adapterName = adapter.DeviceString;
        info.monitorName = monitorName;
        info.x = mode.dmPosition.x;
        info.y = mode.dmPosition.y;
        info.width = static_cast<int>(mode.dmPelsWidth);
        info.height = static_cast<int>(mode.dmPelsHeight);
        info.isPrimary = (adapter.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) != 0;
        info.isXreal = isXrealDisplay(monitorName, adapter.DeviceID);

        displays.push_back(std::move(info));
    }

    return displays;
}

std::optional<DisplayInfo> DisplayDetector::findXreal() {
    auto displays = enumerate();
    for (const auto& d : displays) {
        if (d.isXreal) return d;
    }
    return std::nullopt;
}

void DisplayDetector::printList(const std::vector<DisplayInfo>& displays) {
    Log::info("=== Detected Displays ===");
    for (size_t i = 0; i < displays.size(); ++i) {
        const auto& d = displays[i];
        Log::info("  [{}] {} {}x{} at ({},{}) - {} {}{}",
                  i + 1,
                  d.deviceName,
                  d.width, d.height,
                  d.x, d.y,
                  d.monitorName.empty() ? d.adapterName : d.monitorName,
                  d.isPrimary ? "[PRIMARY]" : "",
                  d.isXreal ? " [XREAL]" : "");
    }
    if (displays.empty()) {
        Log::warn("  No displays detected");
    }
}

} // namespace xr
