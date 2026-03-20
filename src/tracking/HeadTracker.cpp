#include "tracking/HeadTracker.h"
#include "util/Log.h"

#include <deque>

namespace xr {

HeadTracker::~HeadTracker() {
    stop();
}

bool HeadTracker::start() {
    // Always allow restart — don't early-return based on m_started.
    // If the reader exists but isn't running (disconnected), clean it up first.
    if (m_reader && !m_reader->isRunning()) {
        m_reader->stop();
        m_reader.reset();
        m_started = false;
    }

    // Already running — nothing to do
    if (m_started && m_reader && m_reader->isRunning()) {
        return true;
    }

    m_reader = std::make_unique<ImuReader>();
    if (!m_reader->start()) {
        m_reader.reset();
        m_started = false;
        return false;
    }

    m_lastTimestamp_us = 0;
    m_fusion.reset();
    m_started = true;

    Log::info("Head tracking started");
    return true;
}

void HeadTracker::stop() {
    if (m_reader) {
        m_reader->stop();
        m_reader.reset();
    }

    m_started = false;
}

void HeadTracker::update() {
    if (!m_started || !m_reader) return;

    // Check if device disconnected
    if (!m_reader->isRunning()) {
        Log::warn("XREAL device disconnected — head tracking disabled");
        m_reader->stop();
        m_reader.reset();
        m_started = false;
        return;
    }

    // Drain all queued IMU samples
    std::deque<ImuSample> samples;
    m_reader->drainSamples(samples);

    for (const auto& s : samples) {
        float dt;
        if (m_lastTimestamp_us == 0) {
            // First sample — use a nominal dt
            dt = 1.0f / 60.0f;
        } else {
            int64_t diff = static_cast<int64_t>(s.timestamp_us) - static_cast<int64_t>(m_lastTimestamp_us);
            // Guard against bogus timestamps (negative or huge jumps)
            if (diff <= 0 || diff > 1'000'000) {
                dt = 1.0f / 60.0f;
            } else {
                dt = static_cast<float>(diff) / 1'000'000.0f;
            }
        }
        m_lastTimestamp_us = s.timestamp_us;

        m_fusion.update(s.gyro, s.accel, dt);
    }
}

SensorFusion::Orientation HeadTracker::orientation() const {
    return m_fusion.orientation();
}

bool HeadTracker::isActive() const {
    return m_started && m_reader && m_reader->isRunning();
}

void HeadTracker::reset() {
    m_fusion.reset();
    m_lastTimestamp_us = 0;
}

} // namespace xr
