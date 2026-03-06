#pragma once

#include "capture/CaptureSource.h"
#include "capture/FrameBuffer.h"

#include <thread>
#include <atomic>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace xr {

// Captures a window using GDI PrintWindow on a background thread.
// Writes BGRA frames into a FrameBuffer at the target FPS.
class WindowCapturer : public CaptureSource {
public:
    WindowCapturer(HWND hwnd, int targetFps = 30);
    ~WindowCapturer() override;

    WindowCapturer(const WindowCapturer&) = delete;
    WindowCapturer& operator=(const WindowCapturer&) = delete;

    bool start() override;
    void stop() override;
    bool isRunning() const override { return m_running.load(); }

    const uint8_t* lockFrame(int& width, int& height) override;
    void unlockFrame() override;

    HWND hwnd() const { return m_hwnd; }

private:
    void captureLoop();

    HWND m_hwnd;
    int m_targetFps;
    FrameBuffer m_buffer;

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};
};

} // namespace xr
