#include "interaction/InputInjector.h"
#include "util/Log.h"

namespace xr {

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

} // namespace xr
