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

    // Get window class name
    char classBuf[256] = {};
    GetClassNameA(hwnd, classBuf, sizeof(classBuf));
    std::string className(classBuf);

    // Get client rect dimensions
    RECT rect{};
    GetClientRect(hwnd, &rect);
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;

    // Skip zero-size windows
    if (w <= 0 || h <= 0) return TRUE;

    list->push_back({hwnd, std::move(title), std::move(className), w, h});
    return TRUE;
}

std::vector<WindowInfo> WindowEnumerator::enumerate(HWND excludeHwnd) {
    std::vector<WindowInfo> windows;
    EnumContext ctx{&windows, excludeHwnd};
    EnumWindows(enumCallback, reinterpret_cast<LPARAM>(&ctx));
    return windows;
}

bool WindowEnumerator::captureThumbnail(WindowInfo& info, int maxWidth) {
    if (!IsWindow(info.hwnd)) return false;

    RECT rect{};
    GetClientRect(info.hwnd, &rect);
    int srcW = rect.right - rect.left;
    int srcH = rect.bottom - rect.top;
    if (srcW <= 0 || srcH <= 0) return false;

    // Scale to fit maxWidth while preserving aspect ratio
    int thumbW = maxWidth;
    int thumbH = static_cast<int>(static_cast<float>(srcH) / srcW * thumbW);
    if (thumbH <= 0) thumbH = 1;

    // Capture full window into a memory DC, then StretchBlt to thumbnail size
    HDC hdcWin = GetDC(info.hwnd);
    if (!hdcWin) return false;

    HDC hdcMem = CreateCompatibleDC(hdcWin);
    HBITMAP hBmp = CreateCompatibleBitmap(hdcWin, thumbW, thumbH);
    HBITMAP hOld = static_cast<HBITMAP>(SelectObject(hdcMem, hBmp));

    SetStretchBltMode(hdcMem, HALFTONE);
    StretchBlt(hdcMem, 0, 0, thumbW, thumbH, hdcWin, 0, 0, srcW, srcH, SRCCOPY);

    // Read pixels
    BITMAPINFOHEADER bi{};
    bi.biSize = sizeof(bi);
    bi.biWidth = thumbW;
    bi.biHeight = -thumbH; // top-down
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    info.thumbnail.resize(thumbW * thumbH * 4);
    GetDIBits(hdcMem, hBmp, 0, thumbH, info.thumbnail.data(),
              reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);
    info.thumbnailW = thumbW;
    info.thumbnailH = thumbH;

    SelectObject(hdcMem, hOld);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(info.hwnd, hdcWin);

    return true;
}

void WindowEnumerator::printList(const std::vector<WindowInfo>& windows) {
    Log::info("=== Available Windows ===");
    for (size_t i = 0; i < windows.size(); ++i) {
        Log::info("  [{}] {}x{} - {}", i + 1, windows[i].width, windows[i].height, windows[i].title);
    }
    Log::info("Press W to open window picker, or 1-9 for quick assign.");
}

} // namespace xr
