#include "hid/ImuReader.h"
#include "util/Log.h"

#include <hidapi.h>
#include <cstring>

namespace xr {

// Static members for HIDAPI reference counting
std::mutex ImuReader::s_hidMutex;
int ImuReader::s_hidRefCount = 0;

bool ImuReader::hidInit() {
    std::lock_guard lock(s_hidMutex);
    if (s_hidRefCount == 0) {
        if (hid_init() != 0) {
            Log::error("Failed to initialize HIDAPI");
            return false;
        }
    }
    ++s_hidRefCount;
    return true;
}

void ImuReader::hidExit() {
    std::lock_guard lock(s_hidMutex);
    if (s_hidRefCount > 0) {
        --s_hidRefCount;
        if (s_hidRefCount == 0) {
            hid_exit();
        }
    }
}

ImuReader::~ImuReader() {
    stop();
}

bool ImuReader::start() {
    if (m_running.load()) return true;

    // Initialize HIDAPI (reference-counted)
    if (!hidInit()) {
        return false;
    }

    // Enumerate devices to find XREAL glasses
    hid_device_info* devs = hid_enumerate(ImuProtocol::kVendorId, 0);
    hid_device_info* match = nullptr;

    for (hid_device_info* cur = devs; cur; cur = cur->next) {
        if (!ImuProtocol::isSupportedPid(cur->product_id)) continue;

        int targetInterface = ImuProtocol::imuInterface(cur->product_id);
        if (cur->interface_number == targetInterface) {
            match = cur;
            break;
        }
    }

    if (!match) {
        Log::warn("No XREAL device found — head tracking unavailable");
        hid_free_enumeration(devs);
        hidExit();
        return false;
    }

    m_productId = match->product_id;
    Log::info("XREAL device found (PID: 0x{:04X}, interface: {})",
              match->product_id, match->interface_number);

    // Open the device by path
    hid_device* dev = hid_open_path(match->path);
    hid_free_enumeration(devs);

    if (!dev) {
        Log::error("Failed to open XREAL HID device");
        hidExit();
        return false;
    }

    {
        std::lock_guard lock(m_deviceMutex);
        m_device = dev;
    }

    // Set non-blocking mode (we'll use a timeout in hid_read_timeout instead)
    hid_set_nonblocking(dev, 0);

    // Send IMU enable command
    auto enableCmd = ImuProtocol::buildEnableCommand();
    int written = hid_write(dev, enableCmd.data(), enableCmd.size());
    if (written < 0) {
        Log::error("Failed to send IMU enable command");
        std::lock_guard lock(m_deviceMutex);
        hid_close(dev);
        m_device = nullptr;
        hidExit();
        return false;
    }
    Log::info("IMU streaming enabled");

    // Start reading thread
    m_stopRequested.store(false);
    m_running.store(true);
    m_thread = std::thread(&ImuReader::readLoop, this);

    return true;
}

void ImuReader::stop() {
    if (!m_running.load() && !m_thread.joinable()) {
        // Clean up device if thread already exited (disconnect scenario)
        std::lock_guard lock(m_deviceMutex);
        if (m_device) {
            hid_close(static_cast<hid_device*>(m_device));
            m_device = nullptr;
            hidExit();
        }
        return;
    }

    m_stopRequested.store(true);

    if (m_thread.joinable()) {
        m_thread.join();
    }

    std::lock_guard lock(m_deviceMutex);
    auto* dev = static_cast<hid_device*>(m_device);
    if (dev) {
        // Send IMU disable command
        auto disableCmd = ImuProtocol::buildDisableCommand();
        hid_write(dev, disableCmd.data(), disableCmd.size());

        hid_close(dev);
        m_device = nullptr;
        Log::info("IMU streaming disabled, device closed");
    }

    m_running.store(false);
    hidExit();
}

void ImuReader::drainSamples(std::deque<ImuSample>& out) {
    std::lock_guard lock(m_queueMutex);
    out.swap(m_queue);
    m_queue.clear();
}

void ImuReader::drainButtonEvents(std::deque<GlassesButtonEvent>& out) {
    std::lock_guard lock(m_buttonMutex);
    out.swap(m_buttonQueue);
    m_buttonQueue.clear();
}

void ImuReader::readLoop() {
    // Take a local copy of the device pointer under lock
    hid_device* dev = nullptr;
    {
        std::lock_guard lock(m_deviceMutex);
        dev = static_cast<hid_device*>(m_device);
    }

    if (!dev) {
        m_running.store(false);
        return;
    }

    uint8_t buf[ImuProtocol::kPacketSize];

    while (!m_stopRequested.load()) {
        // Read with 100ms timeout so we can check stop flag periodically
        int bytesRead = hid_read_timeout(dev, buf, sizeof(buf), 100);

        if (bytesRead < 0) {
            Log::warn("HID read error — device may have been disconnected");
            break;
        }

        if (bytesRead == 0) {
            // Timeout, no data — loop and check stop flag
            continue;
        }

        // Try parsing as IMU data first, then as button event
        auto sample = ImuProtocol::parseImuReport(buf, static_cast<size_t>(bytesRead));
        if (sample) {
            std::lock_guard lock(m_queueMutex);
            m_queue.push_back(*sample);

            // Prevent unbounded queue growth if main thread stalls
            while (m_queue.size() > 500) {
                m_queue.pop_front();
            }
        } else {
            auto btnEvt = ImuProtocol::parseButtonEvent(buf, static_cast<size_t>(bytesRead));
            if (btnEvt && btnEvt->event != GlassesEvent::None) {
                std::lock_guard lock(m_buttonMutex);
                m_buttonQueue.push_back(*btnEvt);

                while (m_buttonQueue.size() > 32) {
                    m_buttonQueue.pop_front();
                }
            }
        }
    }

    m_running.store(false);
}

} // namespace xr
