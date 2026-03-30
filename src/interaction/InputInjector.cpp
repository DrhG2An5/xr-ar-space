#include "interaction/InputInjector.h"
#include "util/Log.h"

namespace xr {

// ---------------------------------------------------------------------------
// UV → pixel coordinate helper
// ---------------------------------------------------------------------------

LPARAM InputInjector::uvToLParam(HWND hwnd, float u, float v) {
    RECT rect{};
    GetClientRect(hwnd, &rect);
    int clientW = rect.right - rect.left;
    int clientH = rect.bottom - rect.top;

    // UV is bottom-left origin (OpenGL). Window pixels are top-left origin.
    int px = static_cast<int>(u * clientW);
    int py = static_cast<int>((1.0f - v) * clientH); // Flip Y

    // Clamp to client area
    if (px < 0) px = 0;
    if (py < 0) py = 0;
    if (px >= clientW) px = clientW - 1;
    if (py >= clientH) py = clientH - 1;

    return MAKELPARAM(px, py);
}

// ---------------------------------------------------------------------------
// Mouse injection (PostMessage — works on background windows)
// ---------------------------------------------------------------------------

void InputInjector::sendLeftClick(HWND hwnd, float u, float v) {
    if (!IsWindow(hwnd)) return;
    LPARAM lp = uvToLParam(hwnd, u, v);
    PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lp);
    PostMessage(hwnd, WM_LBUTTONUP, 0, lp);
}

void InputInjector::sendLeftDown(HWND hwnd, float u, float v) {
    if (!IsWindow(hwnd)) return;
    PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, uvToLParam(hwnd, u, v));
}

void InputInjector::sendLeftUp(HWND hwnd, float u, float v) {
    if (!IsWindow(hwnd)) return;
    PostMessage(hwnd, WM_LBUTTONUP, 0, uvToLParam(hwnd, u, v));
}

void InputInjector::sendRightClick(HWND hwnd, float u, float v) {
    if (!IsWindow(hwnd)) return;
    LPARAM lp = uvToLParam(hwnd, u, v);
    PostMessage(hwnd, WM_RBUTTONDOWN, MK_RBUTTON, lp);
    PostMessage(hwnd, WM_RBUTTONUP, 0, lp);
}

void InputInjector::sendMouseMove(HWND hwnd, float u, float v) {
    if (!IsWindow(hwnd)) return;
    PostMessage(hwnd, WM_MOUSEMOVE, 0, uvToLParam(hwnd, u, v));
}

void InputInjector::sendScroll(HWND hwnd, float u, float v, float delta) {
    if (!IsWindow(hwnd)) return;
    // WM_MOUSEWHEEL uses screen coordinates for the cursor position
    POINT pt;
    RECT rect{};
    GetClientRect(hwnd, &rect);
    pt.x = static_cast<int>(u * (rect.right - rect.left));
    pt.y = static_cast<int>((1.0f - v) * (rect.bottom - rect.top));
    ClientToScreen(hwnd, &pt);

    int wheelDelta = static_cast<int>(delta * WHEEL_DELTA);
    WPARAM wp = MAKEWPARAM(0, wheelDelta);
    LPARAM lp = MAKELPARAM(pt.x, pt.y);
    PostMessage(hwnd, WM_MOUSEWHEEL, wp, lp);
}

// ---------------------------------------------------------------------------
// Simulated focus (for keyboard forwarding without stealing foreground)
// ---------------------------------------------------------------------------

void InputInjector::simulateFocus(HWND hwnd) {
    if (!IsWindow(hwnd)) return;
    // Tell the target window it is being activated and has keyboard focus.
    // This doesn't actually change the foreground window — our app stays in front.
    PostMessage(hwnd, WM_ACTIVATE, MAKEWPARAM(WA_ACTIVE, 0), 0);
    PostMessage(hwnd, WM_SETFOCUS, 0, 0);
}

void InputInjector::simulateUnfocus(HWND hwnd) {
    if (!IsWindow(hwnd)) return;
    PostMessage(hwnd, WM_KILLFOCUS, 0, 0);
    PostMessage(hwnd, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, 0), 0);
}

// ---------------------------------------------------------------------------
// Keyboard injection via PostMessage with proper GLFW→VK mapping
// ---------------------------------------------------------------------------

void InputInjector::sendKeyDownGlfw(HWND hwnd, int glfwKey) {
    if (!IsWindow(hwnd)) return;
    UINT vk = glfwKeyToVk(glfwKey);
    if (vk == 0) return;
    sendKeyDown(hwnd, vk);
}

void InputInjector::sendKeyUpGlfw(HWND hwnd, int glfwKey) {
    if (!IsWindow(hwnd)) return;
    UINT vk = glfwKeyToVk(glfwKey);
    if (vk == 0) return;
    sendKeyUp(hwnd, vk);
}

void InputInjector::sendCharForward(HWND hwnd, wchar_t ch) {
    if (!IsWindow(hwnd)) return;
    PostMessageW(hwnd, WM_CHAR, ch, 0);
}

// ---------------------------------------------------------------------------
// Legacy PostMessage-based key injection (fallback)
// ---------------------------------------------------------------------------

void InputInjector::sendKeyDown(HWND hwnd, UINT vkCode) {
    if (!IsWindow(hwnd)) return;
    UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
    LPARAM lParam = 1 | (scanCode << 16);
    PostMessage(hwnd, WM_KEYDOWN, vkCode, lParam);
}

void InputInjector::sendKeyUp(HWND hwnd, UINT vkCode) {
    if (!IsWindow(hwnd)) return;
    UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
    LPARAM lParam = 1 | (scanCode << 16) | (1 << 30) | (1 << 31);
    PostMessage(hwnd, WM_KEYUP, vkCode, lParam);
}

void InputInjector::sendChar(HWND hwnd, wchar_t ch) {
    if (!IsWindow(hwnd)) return;
    PostMessageW(hwnd, WM_CHAR, ch, 0);
}

// ---------------------------------------------------------------------------
// GLFW key → Windows VK code mapping
// ---------------------------------------------------------------------------

UINT InputInjector::glfwKeyToVk(int glfwKey) {
    // GLFW printable keys (A-Z, 0-9) use ASCII values which match VK codes
    if (glfwKey >= GLFW_KEY_A && glfwKey <= GLFW_KEY_Z) {
        return static_cast<UINT>(glfwKey); // GLFW_KEY_A == 65 == 'A' == VK_A
    }
    if (glfwKey >= GLFW_KEY_0 && glfwKey <= GLFW_KEY_9) {
        return static_cast<UINT>(glfwKey); // GLFW_KEY_0 == 48 == '0' == VK_0
    }

    switch (glfwKey) {
        // Function keys
        case GLFW_KEY_F1:            return VK_F1;
        case GLFW_KEY_F2:            return VK_F2;
        case GLFW_KEY_F3:            return VK_F3;
        case GLFW_KEY_F4:            return VK_F4;
        case GLFW_KEY_F5:            return VK_F5;
        case GLFW_KEY_F6:            return VK_F6;
        case GLFW_KEY_F7:            return VK_F7;
        case GLFW_KEY_F8:            return VK_F8;
        case GLFW_KEY_F9:            return VK_F9;
        case GLFW_KEY_F10:           return VK_F10;
        case GLFW_KEY_F11:           return VK_F11;
        case GLFW_KEY_F12:           return VK_F12;

        // Arrow keys
        case GLFW_KEY_UP:            return VK_UP;
        case GLFW_KEY_DOWN:          return VK_DOWN;
        case GLFW_KEY_LEFT:          return VK_LEFT;
        case GLFW_KEY_RIGHT:         return VK_RIGHT;

        // Navigation
        case GLFW_KEY_HOME:          return VK_HOME;
        case GLFW_KEY_END:           return VK_END;
        case GLFW_KEY_PAGE_UP:       return VK_PRIOR;
        case GLFW_KEY_PAGE_DOWN:     return VK_NEXT;
        case GLFW_KEY_INSERT:        return VK_INSERT;
        case GLFW_KEY_DELETE:        return VK_DELETE;

        // Editing
        case GLFW_KEY_BACKSPACE:     return VK_BACK;
        case GLFW_KEY_TAB:           return VK_TAB;
        case GLFW_KEY_ENTER:         return VK_RETURN;
        case GLFW_KEY_SPACE:         return VK_SPACE;
        case GLFW_KEY_ESCAPE:        return VK_ESCAPE;

        // Modifiers
        case GLFW_KEY_LEFT_SHIFT:    return VK_LSHIFT;
        case GLFW_KEY_RIGHT_SHIFT:   return VK_RSHIFT;
        case GLFW_KEY_LEFT_CONTROL:  return VK_LCONTROL;
        case GLFW_KEY_RIGHT_CONTROL: return VK_RCONTROL;
        case GLFW_KEY_LEFT_ALT:      return VK_LMENU;
        case GLFW_KEY_RIGHT_ALT:     return VK_RMENU;
        case GLFW_KEY_LEFT_SUPER:    return VK_LWIN;
        case GLFW_KEY_RIGHT_SUPER:   return VK_RWIN;

        // Punctuation / symbols
        case GLFW_KEY_MINUS:         return VK_OEM_MINUS;
        case GLFW_KEY_EQUAL:         return VK_OEM_PLUS;
        case GLFW_KEY_LEFT_BRACKET:  return VK_OEM_4;
        case GLFW_KEY_RIGHT_BRACKET: return VK_OEM_6;
        case GLFW_KEY_BACKSLASH:     return VK_OEM_5;
        case GLFW_KEY_SEMICOLON:     return VK_OEM_1;
        case GLFW_KEY_APOSTROPHE:    return VK_OEM_7;
        case GLFW_KEY_GRAVE_ACCENT:  return VK_OEM_3;
        case GLFW_KEY_COMMA:         return VK_OEM_COMMA;
        case GLFW_KEY_PERIOD:        return VK_OEM_PERIOD;
        case GLFW_KEY_SLASH:         return VK_OEM_2;

        // Locks
        case GLFW_KEY_CAPS_LOCK:     return VK_CAPITAL;
        case GLFW_KEY_SCROLL_LOCK:   return VK_SCROLL;
        case GLFW_KEY_NUM_LOCK:      return VK_NUMLOCK;
        case GLFW_KEY_PRINT_SCREEN:  return VK_SNAPSHOT;
        case GLFW_KEY_PAUSE:         return VK_PAUSE;

        // Numpad
        case GLFW_KEY_KP_0:          return VK_NUMPAD0;
        case GLFW_KEY_KP_1:          return VK_NUMPAD1;
        case GLFW_KEY_KP_2:          return VK_NUMPAD2;
        case GLFW_KEY_KP_3:          return VK_NUMPAD3;
        case GLFW_KEY_KP_4:          return VK_NUMPAD4;
        case GLFW_KEY_KP_5:          return VK_NUMPAD5;
        case GLFW_KEY_KP_6:          return VK_NUMPAD6;
        case GLFW_KEY_KP_7:          return VK_NUMPAD7;
        case GLFW_KEY_KP_8:          return VK_NUMPAD8;
        case GLFW_KEY_KP_9:          return VK_NUMPAD9;
        case GLFW_KEY_KP_DECIMAL:    return VK_DECIMAL;
        case GLFW_KEY_KP_DIVIDE:     return VK_DIVIDE;
        case GLFW_KEY_KP_MULTIPLY:   return VK_MULTIPLY;
        case GLFW_KEY_KP_SUBTRACT:   return VK_SUBTRACT;
        case GLFW_KEY_KP_ADD:        return VK_ADD;
        case GLFW_KEY_KP_ENTER:      return VK_RETURN;

        // Menu
        case GLFW_KEY_MENU:          return VK_APPS;

        default: return 0;
    }
}

} // namespace xr
