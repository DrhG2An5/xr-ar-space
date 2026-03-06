#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <numbers>

namespace xr {

namespace MathUtils {
    inline constexpr float kPi = std::numbers::pi_v<float>;
    inline constexpr float kTwoPi = 2.0f * kPi;
    inline constexpr float kHalfPi = kPi / 2.0f;

    inline float degToRad(float deg) { return deg * (kPi / 180.0f); }
    inline float radToDeg(float rad) { return rad * (180.0f / kPi); }

    inline float clamp(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    inline float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }
} // namespace MathUtils

} // namespace xr
