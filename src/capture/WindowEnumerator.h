#pragma once

#include <string>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace xr {

struct WindowInfo {
    HWND hwnd = nullptr;
    std::string title;
    std::string className;
    int width = 0;
    int height = 0;

    // Thumbnail (BGRA pixels, thumbnailW x thumbnailH)
    std::vector<uint8_t> thumbnail;
    int thumbnailW = 0;
    int thumbnailH = 0;
};

class WindowEnumerator {
public:
    // Enumerate all visible, non-minimized, titled top-level windows.
    // Optionally excludes a specific HWND (e.g., the app's own window).
    static std::vector<WindowInfo> enumerate(HWND excludeHwnd = nullptr);

    // Capture a small thumbnail (BGRA) of a window. Returns false on failure.
    static bool captureThumbnail(WindowInfo& info, int maxWidth = 128);

    // Print a numbered list to the console.
    static void printList(const std::vector<WindowInfo>& windows);
};

} // namespace xr
