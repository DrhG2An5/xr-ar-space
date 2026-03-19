#include "capture/WinRTCapturer.h"
#include "util/Log.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.graphics.capture.interop.h>
#include <Windows.Graphics.DirectX.Direct3D11.interop.h>

#include <chrono>

using namespace winrt;
using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;

namespace xr {

WinRTCapturer::WinRTCapturer(HWND hwnd, int targetFps)
    : m_hwnd(hwnd), m_targetFps(targetFps) {}

WinRTCapturer::~WinRTCapturer() {
    stop();
}

bool WinRTCapturer::isSupported() {
    return GraphicsCaptureSession::IsSupported();
}

bool WinRTCapturer::initD3D11() {
    D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0};
    D3D_FEATURE_LEVEL featureLevel;

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    HRESULT hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
        featureLevels, 1, D3D11_SDK_VERSION,
        m_d3dDevice.GetAddressOf(), &featureLevel,
        m_d3dContext.GetAddressOf());

    if (FAILED(hr)) {
        Log::error("WinRTCapturer: Failed to create D3D11 device (hr=0x{:08X})", static_cast<unsigned>(hr));
        return false;
    }
    return true;
}

bool WinRTCapturer::start() {
    if (m_running.load()) return true;
    if (!IsWindow(m_hwnd)) {
        Log::warn("WinRTCapturer: HWND is not valid");
        return false;
    }
    if (!isSupported()) {
        Log::warn("WinRTCapturer: WinRT capture not supported on this system");
        return false;
    }
    if (!initD3D11()) {
        return false;
    }

    m_stopRequested.store(false);
    m_running.store(true);
    m_thread = std::thread(&WinRTCapturer::captureLoop, this);
    return true;
}

void WinRTCapturer::stop() {
    m_stopRequested.store(true);
    if (m_thread.joinable()) {
        m_thread.join();
    }
    m_running.store(false);
}

const uint8_t* WinRTCapturer::lockFrame(int& width, int& height) {
    return m_buffer.lockRead(width, height);
}

void WinRTCapturer::unlockFrame() {
    m_buffer.unlockRead();
}

void WinRTCapturer::captureLoop() {
    using clock = std::chrono::steady_clock;
    auto frameDuration = std::chrono::microseconds(1'000'000 / m_targetFps);

    // Initialize WinRT on this thread
    winrt::init_apartment(winrt::apartment_type::multi_threaded);

    try {
        // Create IDXGIDevice from D3D11 device
        Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
        HRESULT hr = m_d3dDevice.As(&dxgiDevice);
        if (FAILED(hr)) {
            Log::error("WinRTCapturer: Failed to get IDXGIDevice");
            m_running.store(false);
            return;
        }

        // Wrap D3D11 device as WinRT IDirect3DDevice
        com_ptr<::IInspectable> inspectable;
        hr = CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.Get(), inspectable.put());
        if (FAILED(hr)) {
            Log::error("WinRTCapturer: Failed to create WinRT D3D device");
            m_running.store(false);
            return;
        }
        IDirect3DDevice winrtDevice = inspectable.as<IDirect3DDevice>();

        // Create GraphicsCaptureItem from HWND
        auto interopFactory = get_activation_factory<GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
        GraphicsCaptureItem item{nullptr};
        hr = interopFactory->CreateForWindow(
            m_hwnd,
            guid_of<GraphicsCaptureItem>(),
            put_abi(item));

        if (FAILED(hr) || !item) {
            Log::error("WinRTCapturer: Failed to create capture item for window");
            m_running.store(false);
            return;
        }

        auto size = item.Size();
        if (size.Width <= 0 || size.Height <= 0) {
            Log::error("WinRTCapturer: Window has zero size");
            m_running.store(false);
            return;
        }

        // Create frame pool
        auto framePool = Direct3D11CaptureFramePool::CreateFreeThreaded(
            winrtDevice,
            DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2, // buffer count
            size);

        // Create capture session
        auto session = framePool.CreateCaptureSession(item);

        // Disable cursor capture and border (Windows 11 / 20H1+)
        try {
            session.IsCursorCaptureEnabled(false);
        } catch (...) {}
        try {
            session.IsBorderRequired(false);
        } catch (...) {}

        session.StartCapture();
        Log::info("WinRTCapturer: Started capture ({}x{})", size.Width, size.Height);

        // Create a staging texture for CPU readback
        Microsoft::WRL::ComPtr<ID3D11Texture2D> stagingTex;
        int stagingW = 0, stagingH = 0;

        while (!m_stopRequested.load()) {
            auto frameStart = clock::now();

            if (!IsWindow(m_hwnd)) {
                Log::warn("WinRTCapturer: Window destroyed, stopping");
                break;
            }

            // Try to get a frame
            auto frame = framePool.TryGetNextFrame();
            if (frame) {
                auto surface = frame.Surface();
                auto access = surface.as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();

                Microsoft::WRL::ComPtr<ID3D11Texture2D> frameTex;
                hr = access->GetInterface(IID_PPV_ARGS(frameTex.GetAddressOf()));

                if (SUCCEEDED(hr) && frameTex) {
                    D3D11_TEXTURE2D_DESC desc{};
                    frameTex->GetDesc(&desc);
                    int w = static_cast<int>(desc.Width);
                    int h = static_cast<int>(desc.Height);

                    // Recreate staging texture if size changed
                    if (w != stagingW || h != stagingH) {
                        D3D11_TEXTURE2D_DESC stageDesc = desc;
                        stageDesc.Usage = D3D11_USAGE_STAGING;
                        stageDesc.BindFlags = 0;
                        stageDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                        stageDesc.MiscFlags = 0;

                        stagingTex.Reset();
                        hr = m_d3dDevice->CreateTexture2D(&stageDesc, nullptr, stagingTex.GetAddressOf());
                        if (FAILED(hr)) {
                            frame.Close();
                            continue;
                        }
                        stagingW = w;
                        stagingH = h;
                    }

                    // Copy frame to staging texture
                    m_d3dContext->CopyResource(stagingTex.Get(), frameTex.Get());

                    // Map staging texture for CPU read
                    D3D11_MAPPED_SUBRESOURCE mapped{};
                    hr = m_d3dContext->Map(stagingTex.Get(), 0, D3D11_MAP_READ, 0, &mapped);
                    if (SUCCEEDED(hr)) {
                        // Copy rows (mapped pitch may differ from w*4)
                        int rowBytes = w * 4;
                        if (static_cast<int>(mapped.RowPitch) == rowBytes) {
                            m_buffer.write(static_cast<const uint8_t*>(mapped.pData), w, h);
                        } else {
                            // Need to copy row by row due to pitch difference
                            std::vector<uint8_t> pixels(static_cast<size_t>(rowBytes) * h);
                            const uint8_t* src = static_cast<const uint8_t*>(mapped.pData);
                            for (int row = 0; row < h; ++row) {
                                std::memcpy(pixels.data() + row * rowBytes,
                                           src + row * mapped.RowPitch, rowBytes);
                            }
                            m_buffer.write(pixels.data(), w, h);
                        }
                        m_d3dContext->Unmap(stagingTex.Get(), 0);
                    }
                }

                frame.Close();

                // Recreate frame pool if size changed
                auto newSize = item.Size();
                if (newSize.Width != size.Width || newSize.Height != size.Height) {
                    size = newSize;
                    framePool.Recreate(winrtDevice,
                                       DirectXPixelFormat::B8G8R8A8UIntNormalized,
                                       2, size);
                }
            }

            auto elapsed = clock::now() - frameStart;
            if (elapsed < frameDuration) {
                std::this_thread::sleep_for(frameDuration - elapsed);
            }
        }

        session.Close();
        framePool.Close();

    } catch (const winrt::hresult_error& e) {
        Log::error("WinRTCapturer: WinRT error: {} (0x{:08X})",
                   winrt::to_string(e.message()), static_cast<unsigned>(e.code()));
    } catch (const std::exception& e) {
        Log::error("WinRTCapturer: {}", e.what());
    }

    winrt::uninit_apartment();
    m_running.store(false);
}

} // namespace xr
