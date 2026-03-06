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

private:
    void readLoop();

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};

    // Thread-safe sample queue
    std::mutex m_queueMutex;
    std::deque<ImuSample> m_queue;

    // HID device handle (opaque, managed internally)
    void* m_device = nullptr;
    uint16_t m_productId = 0;
};

} // namespace xr
