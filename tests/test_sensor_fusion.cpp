#include <doctest.h>
#include "tracking/SensorFusion.h"

using namespace xr;

TEST_CASE("SensorFusion: initial state") {
    SensorFusion sf;
    CHECK_FALSE(sf.isInitialized());
    CHECK(sf.orientation().yaw == doctest::Approx(0.0f));
    CHECK(sf.orientation().pitch == doctest::Approx(0.0f));
    CHECK(sf.orientation().roll == doctest::Approx(0.0f));
}

TEST_CASE("SensorFusion: reset clears state") {
    SensorFusion sf;
    sf.update({0.0f, 1.0f, 0.0f}, {0.0f, 9.81f, 0.0f}, 0.01f);
    CHECK(sf.isInitialized());
    sf.reset();
    CHECK_FALSE(sf.isInitialized());
    CHECK(sf.orientation().yaw == doctest::Approx(0.0f));
}

TEST_CASE("SensorFusion: gyro integration updates yaw") {
    SensorFusion sf;
    // Pure gyro rotation around Y axis (yaw)
    glm::vec3 gyro{0.0f, 1.0f, 0.0f}; // 1 rad/s yaw
    glm::vec3 accel{0.0f, 9.81f, 0.0f}; // gravity along Y (stationary)
    float dt = 0.01f;

    sf.update(gyro, accel, dt);
    CHECK(sf.orientation().yaw == doctest::Approx(0.01f).epsilon(0.001));
}

TEST_CASE("SensorFusion: accumulated yaw over multiple steps") {
    SensorFusion sf;
    glm::vec3 gyro{0.0f, 2.0f, 0.0f}; // 2 rad/s yaw
    glm::vec3 accel{0.0f, 9.81f, 0.0f};
    float dt = 0.01f;

    for (int i = 0; i < 100; ++i) {
        sf.update(gyro, accel, dt);
    }
    // Expected: 2.0 * 1.0s = 2.0 rad
    CHECK(sf.orientation().yaw == doctest::Approx(2.0f).epsilon(0.01));
}

TEST_CASE("SensorFusion: pitch is clamped near +-pi/2") {
    SensorFusion sf;
    glm::vec3 gyro{10.0f, 0.0f, 0.0f}; // Rapid pitch
    glm::vec3 accel{0.0f, 9.81f, 0.0f};
    float dt = 0.1f;

    // Push pitch past limits
    for (int i = 0; i < 50; ++i) {
        sf.update(gyro, accel, dt);
    }

    constexpr float kHalfPi = 1.5707963f;
    CHECK(sf.orientation().pitch < kHalfPi);
    CHECK(sf.orientation().pitch > -kHalfPi);
}

TEST_CASE("SensorFusion: skips bogus dt values") {
    SensorFusion sf;
    glm::vec3 gyro{0.0f, 1.0f, 0.0f};
    glm::vec3 accel{0.0f, 9.81f, 0.0f};

    // Zero dt — should be skipped
    sf.update(gyro, accel, 0.0f);
    CHECK_FALSE(sf.isInitialized());

    // Negative dt — should be skipped
    sf.update(gyro, accel, -0.01f);
    CHECK_FALSE(sf.isInitialized());

    // Very large dt (> 1s) — should be skipped
    sf.update(gyro, accel, 2.0f);
    CHECK_FALSE(sf.isInitialized());
}

TEST_CASE("SensorFusion: alpha parameter affects filter blend") {
    // alpha=1.0 means pure gyro (no accel correction)
    SensorFusion sf_gyro(1.0f);
    // alpha=0.0 means pure accel (no gyro)
    SensorFusion sf_accel(0.0f);

    glm::vec3 gyro{0.5f, 0.0f, 0.0f}; // some pitch rotation
    glm::vec3 accel{0.0f, 9.81f, 0.0f}; // stationary, gravity along Y

    float dt = 0.01f;
    sf_gyro.update(gyro, accel, dt);
    sf_accel.update(gyro, accel, dt);

    // Pure gyro should have pitch = 0.5 * 0.01 = 0.005
    CHECK(sf_gyro.orientation().pitch == doctest::Approx(0.005f).epsilon(0.001));

    // Pure accel at rest should have pitch near 0 (gravity is along Y)
    CHECK(sf_accel.orientation().pitch == doctest::Approx(0.0f).epsilon(0.01));
}

TEST_CASE("SensorFusion: invalid accel magnitude falls back to gyro only") {
    SensorFusion sf;
    glm::vec3 gyro{1.0f, 0.0f, 0.0f};
    // Very low accel (freefall) — below 5.0 threshold
    glm::vec3 accel{0.0f, 0.1f, 0.0f};
    float dt = 0.01f;

    sf.update(gyro, accel, dt);
    // Should still integrate gyro pitch
    CHECK(sf.orientation().pitch == doctest::Approx(0.01f).epsilon(0.001));
}
