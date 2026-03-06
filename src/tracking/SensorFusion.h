#pragma once

#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>

namespace xr {

// Complementary filter for 3DoF orientation from gyroscope + accelerometer.
// Pitch and roll use a blend of gyro integration and accelerometer reference.
// Yaw uses pure gyro integration (no magnetometer correction).
class SensorFusion {
public:
    struct Orientation {
        float yaw   = 0.0f; // radians
        float pitch = 0.0f; // radians
        float roll  = 0.0f; // radians
    };

    explicit SensorFusion(float alpha = 0.98f) : m_alpha(alpha) {}

    // Update with new IMU sample. dt in seconds.
    void update(const glm::vec3& gyro, const glm::vec3& accel, float dt) {
        if (dt <= 0.0f || dt > 1.0f) return; // Skip bogus deltas

        // Gyro integration
        float gyroPitch = m_orientation.pitch + gyro.x * dt;
        float gyroRoll  = m_orientation.roll  + gyro.z * dt;
        float gyroYaw   = m_orientation.yaw   + gyro.y * dt;

        // Accelerometer-derived angles (gravity reference for pitch and roll)
        float accelMag = glm::length(accel);
        bool accelValid = (accelMag > 5.0f && accelMag < 15.0f); // ~0.5g to ~1.5g

        if (accelValid) {
            float accelPitch = std::atan2(-accel.x, std::sqrt(accel.y * accel.y + accel.z * accel.z));
            float accelRoll  = std::atan2(accel.z, accel.y);

            // Complementary filter: blend gyro (fast) with accel (drift correction)
            m_orientation.pitch = m_alpha * gyroPitch + (1.0f - m_alpha) * accelPitch;
            m_orientation.roll  = m_alpha * gyroRoll  + (1.0f - m_alpha) * accelRoll;
        } else {
            // During high acceleration/freefall, trust only gyro
            m_orientation.pitch = gyroPitch;
            m_orientation.roll  = gyroRoll;
        }

        // Yaw: pure gyro integration (no absolute reference without magnetometer)
        m_orientation.yaw = gyroYaw;

        // Clamp pitch to avoid gimbal lock
        constexpr float kHalfPi = 1.5707963f;
        m_orientation.pitch = std::clamp(m_orientation.pitch, -kHalfPi + 0.01f, kHalfPi - 0.01f);

        m_initialized = true;
    }

    const Orientation& orientation() const { return m_orientation; }

    void reset() {
        m_orientation = {};
        m_initialized = false;
    }

    bool isInitialized() const { return m_initialized; }

private:
    float m_alpha;
    Orientation m_orientation;
    bool m_initialized = false;
};

} // namespace xr
