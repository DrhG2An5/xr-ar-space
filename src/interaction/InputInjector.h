#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <GLFW/glfw3.h>

namespace xr {

class InputInjector {
public:
    // --- Mouse injection (PostMessage-based, works on background windows) ---

    // Send a left-click at the given UV coordinate (0-1 range) to the target window.
    // UV origin is bottom-left (OpenGL convention); this handles the Y-flip internally.
    static void sendLeftClick(HWND hwnd, float u, float v);
    static void sendLeftDown(HWND hwnd, float u, float v);
    static void sendLeftUp(HWND hwnd, float u, float v);
    static void sendRightClick(HWND hwnd, float u, float v);
    static void sendMouseMove(HWND hwnd, float u, float v);
    static void sendScroll(HWND hwnd, float u, float v, float delta);

    // --- Keyboard injection (PostMessage with simulated focus) ---

    // Simulate focus on the target window so it accepts PostMessage keyboard input.
    // Sends WM_ACTIVATE + WM_SETFOCUS without actually changing foreground window.
    // Call once when entering keyboard forwarding mode.
    static void simulateFocus(HWND hwnd);

    // Remove simulated focus (sends WM_KILLFOCUS + WM_ACTIVATE deactivate).
    static void simulateUnfocus(HWND hwnd);

    // Send a key event via PostMessage. Accepts GLFW key codes — maps to Windows VK codes.
    static void sendKeyDownGlfw(HWND hwnd, int glfwKey);
    static void sendKeyUpGlfw(HWND hwnd, int glfwKey);

    // Send a character via PostMessage (WM_CHAR).
    static void sendCharForward(HWND hwnd, wchar_t ch);

    // Low-level PostMessage key injection by VK code.
    static void sendKeyDown(HWND hwnd, UINT vkCode);
    static void sendKeyUp(HWND hwnd, UINT vkCode);
    static void sendChar(HWND hwnd, wchar_t ch);

    // --- Key mapping ---

    // Convert a GLFW key code to a Windows virtual key code.
    // Returns 0 if the key has no mapping.
    static UINT glfwKeyToVk(int glfwKey);

private:
    // Convert UV (bottom-left origin) to window pixel LPARAM (top-left origin).
    static LPARAM uvToLParam(HWND hwnd, float u, float v);
};

} // namespace xr
