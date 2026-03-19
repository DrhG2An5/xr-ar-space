#include "capture/WindowEnumerator.h"
#include "util/Log.h"

#include <dwmapi.h>

namespace xr {

struct EnumContext {
    std::vector<WindowInfo>* list;
    HWND excludeHwnd;
};

static BOOL CALLBACK enumCallback(HWND hwnd, LPARAM lParam) {
    auto* ctx = reinterpret_cast<EnumContext*>(lParam);
    auto* list = ctx->list;

    // Skip excluded window (the app itself)
    if (ctx->excludeHwnd && hwnd == ctx->excludeHwnd) return TRUE;

    // Skip invisible windows
    if (!IsWindowVisible(hwnd)) return TRUE;

    // Skip minimized windows
    if (IsIconic(hwnd)) return TRUE;

    // Skip windows with no title
    int len = GetWindowTextLengthA(hwnd);
    if (len == 0) return TRUE;

    // Skip cloaked windows (UWP background windows, etc.)
    BOOL cloaked = FALSE;
    DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
    if (cloaked) return TRUE;

    // Skip tool windows (tooltips, floating palettes)
    LONG exStyle = GetWindowLongA(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) return TRUE;

    // Get title
    std::string title(len + 1, '\0');
    GetWindowTextA(hwnd, title.data(), len + 1);
    title.resize(len);

    // Get client rect dimensions
    RECT rect{};
    GetClientRect(hwnd, &rect);
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;

    // Skip zero-size windows
    if (w <= 0 || h <= 0) return TRUE;

    list->push_back({hwnd, std::move(title), w, h});
    return TRUE;
}

std::vector<WindowInfo> WindowEnumerator::enumerate(HWND excludeHwnd) {
    std::vector<WindowInfo> windows;
    EnumContext ctx{&windows, excludeHwnd};
    EnumWindows(enumCallback, reinterpret_cast<LPARAM>(&ctx));
    return windows;
}

void WindowEnumerator::printList(const std::vector<WindowInfo>& windows) {
    Log::info("=== Available Windows ===");
    for (size_t i = 0; i < windows.size(); ++i) {
        Log::info("  [{}] {}x{} - {}", i + 1, windows[i].width, windows[i].height, windows[i].title);
    }
    Log::info("Press 1-9 to assign a window to the selected screen. W to refresh.");
}

} // namespace xr
