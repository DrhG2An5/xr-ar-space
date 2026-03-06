#include "capture/WindowCapturer.h"
#include "util/Log.h"

#include <chrono>

namespace xr {

WindowCapturer::WindowCapturer(HWND hwnd, int targetFps)
    : m_hwnd(hwnd), m_targetFps(targetFps) {}

WindowCapturer::~WindowCapturer() {
    stop();
}

bool WindowCapturer::start() {
    if (m_running.load()) return true;
    if (!IsWindow(m_hwnd)) {
        Log::warn("WindowCapturer: HWND is not valid");
        return false;
    }

    m_stopRequested.store(false);
    m_running.store(true);
    m_thread = std::thread(&WindowCapturer::captureLoop, this);
    return true;
}

void WindowCapturer::stop() {
    m_stopRequested.store(true);
    if (m_thread.joinable()) {
        m_thread.join();
    }
    m_running.store(false);
}

const uint8_t* WindowCapturer::lockFrame(int& width, int& height) {
    return m_buffer.lockRead(width, height);
}

void WindowCapturer::unlockFrame() {
    m_buffer.unlockRead();
}

void WindowCapturer::captureLoop() {
    using clock = std::chrono::steady_clock;
    auto frameDuration = std::chrono::microseconds(1'000'000 / m_targetFps);

    // Create a memory DC and DIB section for capturing
    HDC screenDC = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(screenDC);
    ReleaseDC(nullptr, screenDC);

    HBITMAP hBitmap = nullptr;
    uint8_t* dibPixels = nullptr;
    int lastW = 0, lastH = 0;

    while (!m_stopRequested.load()) {
        auto frameStart = clock::now();

        // Check if window still exists
        if (!IsWindow(m_hwnd)) {
            Log::warn("WindowCapturer: Window was destroyed, stopping capture");
            break;
        }

        // Get current client rect
        RECT clientRect{};
        GetClientRect(m_hwnd, &clientRect);
        int w = clientRect.right - clientRect.left;
        int h = clientRect.bottom - clientRect.top;

        // Skip if minimized or zero-size
        if (w <= 0 || h <= 0) {
            std::this_thread::sleep_for(frameDuration);
            continue;
        }

        // Recreate DIB if dimensions changed
        if (w != lastW || h != lastH) {
            if (hBitmap) {
                DeleteObject(hBitmap);
            }

            BITMAPINFO bmi{};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = w;
            bmi.bmiHeader.biHeight = -h; // Top-down DIB (negative = top-down)
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            hBitmap = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS,
                                       reinterpret_cast<void**>(&dibPixels), nullptr, 0);
            if (!hBitmap) {
                Log::error("WindowCapturer: CreateDIBSection failed");
                break;
            }

            lastW = w;
            lastH = h;
            Log::debug("WindowCapturer: Resized capture to {}x{}", w, h);
        }

        // Select bitmap into memory DC and capture
        HGDIOBJ oldBitmap = SelectObject(memDC, hBitmap);

        BOOL ok = PrintWindow(m_hwnd, memDC, PW_RENDERFULLCONTENT);

        SelectObject(memDC, oldBitmap);

        if (ok) {
            // dibPixels contains BGRA data (GDI DIB with 32bpp is BGRA)
            m_buffer.write(dibPixels, w, h);
        }

        // Sleep for remainder of frame budget
        auto elapsed = clock::now() - frameStart;
        if (elapsed < frameDuration) {
            std::this_thread::sleep_for(frameDuration - elapsed);
        }
    }

    // Cleanup GDI resources
    if (hBitmap) DeleteObject(hBitmap);
    if (memDC) DeleteDC(memDC);

    m_running.store(false);
}

} // namespace xr
