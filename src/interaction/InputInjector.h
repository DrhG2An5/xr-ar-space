#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace xr {

class InputInjector {
public:
    // Send a left-click at the given UV coordinate (0-1 range) to the target window.
    // UV origin is bottom-left (OpenGL convention); this handles the Y-flip internally.
    static void sendLeftClick(HWND hwnd, float u, float v);

    // Send left button down (for click-and-hold scenarios).
    static void sendLeftDown(HWND hwnd, float u, float v);

    // Send left button up.
    static void sendLeftUp(HWND hwnd, float u, float v);

    // Send right-click at UV coordinate.
    static void sendRightClick(HWND hwnd, float u, float v);

    // Send mouse move at UV coordinate (for hover forwarding).
    static void sendMouseMove(HWND hwnd, float u, float v);

    // Send scroll wheel event at UV coordinate.
    static void sendScroll(HWND hwnd, float u, float v, float delta);

    // Send a key down event.
    static void sendKeyDown(HWND hwnd, UINT vkCode);

    // Send a key up event.
    static void sendKeyUp(HWND hwnd, UINT vkCode);

    // Send a character (WM_CHAR).
    static void sendChar(HWND hwnd, wchar_t ch);

private:
    // Convert UV (bottom-left origin) to window pixel LPARAM (top-left origin).
    static LPARAM uvToLParam(HWND hwnd, float u, float v);
};

} // namespace xr
