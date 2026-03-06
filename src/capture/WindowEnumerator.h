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
    int width = 0;
    int height = 0;
};

class WindowEnumerator {
public:
    // Enumerate all visible, non-minimized, titled top-level windows.
    static std::vector<WindowInfo> enumerate();

    // Print a numbered list to the console.
    static void printList(const std::vector<WindowInfo>& windows);
};

} // namespace xr
