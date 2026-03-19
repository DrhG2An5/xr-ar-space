#pragma once

#include "capture/CaptureSource.h"
#include "capture/FrameBuffer.h"

#include <thread>
#include <atomic>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>

namespace xr {

// Captures a window using WinRT Windows.Graphics.Capture API.
// Falls back to GDI PrintWindow if WinRT capture is unavailable (pre-1903).
// Provides BGRA frames via the CaptureSource interface.
class WinRTCapturer : public CaptureSource {
public:
    WinRTCapturer(HWND hwnd, int targetFps = 30);
    ~WinRTCapturer() override;

    WinRTCapturer(const WinRTCapturer&) = delete;
    WinRTCapturer& operator=(const WinRTCapturer&) = delete;

    bool start() override;
    void stop() override;
    bool isRunning() const override { return m_running.load(); }

    const uint8_t* lockFrame(int& width, int& height) override;
    void unlockFrame() override;

    HWND hwnd() const { return m_hwnd; }

    // Check if WinRT capture is available on this system (Windows 10 1903+).
    static bool isSupported();

private:
    void captureLoop();
    bool initD3D11();

    HWND m_hwnd;
    int m_targetFps;
    FrameBuffer m_buffer;

    // D3D11 resources
    Microsoft::WRL::ComPtr<ID3D11Device> m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_d3dContext;

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};
};

} // namespace xr
