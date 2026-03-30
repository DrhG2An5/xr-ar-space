#pragma once

#include "hid/ImuProtocol.h"

#include <deque>
#include <mutex>
#include <thread>
#include <atomic>
#include <cstdint>

namespace xr {

class ImuReader {
public:
    ImuReader() = default;
    ~ImuReader();

    ImuReader(const ImuReader&) = delete;
    ImuReader& operator=(const ImuReader&) = delete;

    // Attempts to find and open an XREAL HID device.
    // Returns true if device was found and IMU streaming was enabled.
    bool start();

    // Stops reading thread and closes the device.
    void stop();

    bool isRunning() const { return m_running.load(); }

    // Drain all queued samples into the output vector.
    // Called from the main thread.
    void drainSamples(std::deque<ImuSample>& out);

    // Drain all queued button events. Called from the main thread.
    void drainButtonEvents(std::deque<GlassesButtonEvent>& out);

private:
    void readLoop();

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};

    // Thread-safe sample queue
    std::mutex m_queueMutex;
    std::deque<ImuSample> m_queue;

    // Thread-safe button event queue
    std::mutex m_buttonMutex;
    std::deque<GlassesButtonEvent> m_buttonQueue;

    // HID device handle — protected by m_deviceMutex for thread-safe access
    std::mutex m_deviceMutex;
    void* m_device = nullptr;
    uint16_t m_productId = 0;

    // Global HIDAPI init/exit reference counting
    static std::mutex s_hidMutex;
    static int s_hidRefCount;
    static bool hidInit();
    static void hidExit();
};

} // namespace xr
