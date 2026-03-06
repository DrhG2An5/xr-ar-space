#pragma once

#include <chrono>

namespace xr {

class Timer {
public:
    using Clock = std::chrono::high_resolution_clock;

    Timer() : m_start(Clock::now()), m_last(m_start) {}

    float tick() {
        auto now = Clock::now();
        float dt = std::chrono::duration<float>(now - m_last).count();
        m_last = now;
        return dt;
    }

    float elapsed() const {
        return std::chrono::duration<float>(Clock::now() - m_start).count();
    }

private:
    Clock::time_point m_start;
    Clock::time_point m_last;
};

} // namespace xr
