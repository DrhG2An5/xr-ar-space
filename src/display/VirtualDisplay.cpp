#include "display/VirtualDisplay.h"
#include "util/Log.h"

#include <SetupAPI.h>
#include <devguid.h>
#include <initguid.h>

// IDD Sample Driver device interface GUID
// This matches the GUID used by common IDD virtual display drivers
// (e.g., roshkins/IddSampleDriver, ge9/IddSampleDriver)
// {781EF630-72B2-11d2-B852-00C04FAD5101} — GUID_DEVINTERFACE_MONITOR is standard
DEFINE_GUID(GUID_DEVINTERFACE_IDD_SAMPLE,
    0x781EF630, 0x72B2, 0x11d2, 0xB8, 0x52, 0x00, 0xC0, 0x4F, 0xAD, 0x51, 0x01);

namespace xr {

VirtualDisplay::~VirtualDisplay() {
    destroy();
}

bool VirtualDisplay::isDriverInstalled() {
    // Check if any IDD-based virtual display adapter is present
    HDEVINFO devInfo = SetupDiGetClassDevs(
        &GUID_DEVCLASS_DISPLAY, nullptr, nullptr,
        DIGCF_PRESENT);

    if (devInfo == INVALID_HANDLE_VALUE) return false;

    SP_DEVINFO_DATA devInfoData{};
    devInfoData.cbSize = sizeof(devInfoData);

    bool found = false;
    for (DWORD i = 0; SetupDiEnumDeviceInfo(devInfo, i, &devInfoData); ++i) {
        char buf[512] = {};
        if (SetupDiGetDeviceRegistryPropertyA(devInfo, &devInfoData,
                                               SPDRP_HARDWAREID, nullptr,
                                               reinterpret_cast<PBYTE>(buf),
                                               sizeof(buf), nullptr)) {
            std::string hwid(buf);
            // Common IDD virtual display hardware IDs
            if (hwid.find("IddSample") != std::string::npos ||
                hwid.find("VirtualDisplay") != std::string::npos ||
                hwid.find("IndirectDisplay") != std::string::npos) {
                found = true;
                break;
            }
        }
    }

    SetupDiDestroyDeviceInfoList(devInfo);
    return found;
}

std::vector<std::string> VirtualDisplay::enumerateVirtualDisplays() {
    std::vector<std::string> result;

    DISPLAY_DEVICEA dd{};
    dd.cb = sizeof(dd);

    for (DWORD i = 0; EnumDisplayDevicesA(nullptr, i, &dd, 0); ++i) {
        if (!(dd.StateFlags & DISPLAY_DEVICE_ACTIVE)) continue;

        // Check if this is a virtual/indirect display
        std::string desc(dd.DeviceString);
        if (desc.find("IddSample") != std::string::npos ||
            desc.find("Virtual Display") != std::string::npos ||
            desc.find("Indirect Display") != std::string::npos) {
            result.emplace_back(dd.DeviceName);
        }
    }

    return result;
}

bool VirtualDisplay::create(int width, int height, int refreshRate) {
    m_info.width = width;
    m_info.height = height;
    m_info.refreshRate = refreshRate;

    // First, try to find an already-active virtual monitor
    if (findVirtualMonitor()) {
        Log::info("Found existing virtual display: {} ({}x{})",
                  m_deviceName, m_info.width, m_info.height);
        m_active = true;
        return true;
    }

    // Try to enable an IDD monitor
    if (enableIddMonitor(width, height, refreshRate)) {
        Log::info("Enabled IDD virtual display: {} ({}x{})",
                  m_deviceName, width, height);
        m_active = true;
        return true;
    }

    Log::warn("No virtual display driver found. Install an IDD driver "
              "(e.g., IddSampleDriver) to use virtual desktop extension.");
    Log::info("Tip: https://github.com/ge9/IddSampleDriver — free virtual display driver for Windows");
    return false;
}

void VirtualDisplay::destroy() {
    if (!m_active) return;

    // Note: IDD monitors persist until the driver removes them or the system restarts.
    // We don't forcibly remove them here to avoid disrupting other apps.
    Log::info("Virtual display released: {}", m_deviceName);
    m_active = false;
    m_deviceName.clear();
    m_desktopRect = {};
}

bool VirtualDisplay::findVirtualMonitor() {
    auto virtuals = enumerateVirtualDisplays();
    if (virtuals.empty()) return false;

    m_deviceName = virtuals[0];

    // Get the desktop rect for this display
    DEVMODEA dm{};
    dm.dmSize = sizeof(dm);
    if (EnumDisplaySettingsA(m_deviceName.c_str(), ENUM_CURRENT_SETTINGS, &dm)) {
        m_desktopRect.left = dm.dmPosition.x;
        m_desktopRect.top = dm.dmPosition.y;
        m_desktopRect.right = dm.dmPosition.x + static_cast<LONG>(dm.dmFields & DM_PELSWIDTH ? dm.dmPelsWidth : m_info.width);
        m_desktopRect.bottom = dm.dmPosition.y + static_cast<LONG>(dm.dmFields & DM_PELSHEIGHT ? dm.dmPelsHeight : m_info.height);
        m_info.width = m_desktopRect.right - m_desktopRect.left;
        m_info.height = m_desktopRect.bottom - m_desktopRect.top;
        return true;
    }

    return false;
}

bool VirtualDisplay::enableIddMonitor(int width, int height, int refreshRate) {
    // Try to change the display settings for an existing but disabled virtual monitor
    DISPLAY_DEVICEA dd{};
    dd.cb = sizeof(dd);

    for (DWORD i = 0; EnumDisplayDevicesA(nullptr, i, &dd, 0); ++i) {
        std::string desc(dd.DeviceString);
        if (desc.find("IddSample") == std::string::npos &&
            desc.find("Virtual Display") == std::string::npos &&
            desc.find("Indirect Display") == std::string::npos) {
            continue;
        }

        // Found a virtual display adapter — try to set its mode
        DEVMODEA dm{};
        dm.dmSize = sizeof(dm);
        dm.dmPelsWidth = width;
        dm.dmPelsHeight = height;
        dm.dmDisplayFrequency = refreshRate;
        dm.dmBitsPerPel = 32;
        dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_BITSPERPEL;

        LONG result = ChangeDisplaySettingsExA(dd.DeviceName, &dm,
                                                nullptr, CDS_UPDATEREGISTRY | CDS_NORESET, nullptr);

        if (result == DISP_CHANGE_SUCCESSFUL) {
            // Apply the change
            ChangeDisplaySettingsExA(nullptr, nullptr, nullptr, 0, nullptr);

            m_deviceName = dd.DeviceName;

            // Re-read the actual rect
            DEVMODEA actualDm{};
            actualDm.dmSize = sizeof(actualDm);
            if (EnumDisplaySettingsA(dd.DeviceName, ENUM_CURRENT_SETTINGS, &actualDm)) {
                m_desktopRect.left = actualDm.dmPosition.x;
                m_desktopRect.top = actualDm.dmPosition.y;
                m_desktopRect.right = actualDm.dmPosition.x + width;
                m_desktopRect.bottom = actualDm.dmPosition.y + height;
            }

            return true;
        }
    }

    return false;
}

} // namespace xr
