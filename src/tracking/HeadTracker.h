#pragma once

#include "hid/ImuReader.h"
#include "tracking/SensorFusion.h"

#include <memory>

namespace xr {

class HeadTracker {
public:
    HeadTracker() = default;
    ~HeadTracker();

    HeadTracker(const HeadTracker&) = delete;
    HeadTracker& operator=(const HeadTracker&) = delete;

    // Attempts to connect to XREAL glasses and start IMU streaming.
    // Returns true if device found, false if not (app should continue without tracking).
    bool start();

    // Stops IMU reader and releases device.
    void stop();

    // Called from main thread each frame: drains IMU queue and updates sensor fusion.
    void update();

    // Returns current orientation (yaw, pitch, roll in radians).
    SensorFusion::Orientation orientation() const;

    bool isActive() const;

    void reset();

    // Drain any button events from the glasses (shade toggle, brightness, etc.)
    void drainButtonEvents(std::deque<GlassesButtonEvent>& out);

private:
    std::unique_ptr<ImuReader> m_reader;
    SensorFusion m_fusion;
    uint64_t m_lastTimestamp_us = 0;
    bool m_started = false;
};

} // namespace xr
