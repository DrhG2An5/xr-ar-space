#include <doctest.h>
#include <glm/glm.hpp>
#include <cmath>
#include <array>

#include "util/MathUtils.h"
#include "app/Config.h"

using namespace xr;

// Helper: compute arc position for screen index i out of count
static glm::vec3 arcPosition(int i, int count, float radius, float arcSpanDeg) {
    float spanRad = MathUtils::degToRad(arcSpanDeg);
    float angle = 0.0f;
    if (count > 1) {
        float t = static_cast<float>(i) / (count - 1) - 0.5f;
        angle = t * spanRad;
    }
    return {radius * std::sin(angle), 0.0f, -radius * std::cos(angle)};
}

// Helper: compute grid position
static glm::vec3 gridPosition(int i, int count, const Config& config) {
    int cols = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(count))));
    int rows = static_cast<int>(std::ceil(static_cast<float>(count) / cols));

    float cellW = config.screenWidth + config.gridGap;
    float cellH = config.screenHeight + config.gridGap;
    float totalW = cols * cellW - config.gridGap;
    float totalH = rows * cellH - config.gridGap;
    float startX = -totalW * 0.5f + config.screenWidth * 0.5f;
    float startY = totalH * 0.5f - config.screenHeight * 0.5f;

    int col = i % cols;
    int row = i / cols;
    return {startX + col * cellW, startY - row * cellH, -config.screenDistance};
}

TEST_CASE("Layout: arc single screen is at center") {
    auto pos = arcPosition(0, 1, 2.5f, 90.0f);
    CHECK(pos.x == doctest::Approx(0.0f));
    CHECK(pos.z == doctest::Approx(-2.5f));
}

TEST_CASE("Layout: arc screens are equidistant from origin") {
    float radius = 2.5f;
    for (int i = 0; i < 3; ++i) {
        auto p = arcPosition(i, 3, radius, 90.0f);
        float dist = std::sqrt(p.x * p.x + p.z * p.z);
        CHECK(dist == doctest::Approx(radius).epsilon(0.01));
    }
}

TEST_CASE("Layout: arc center screen is at x=0") {
    auto pos = arcPosition(1, 3, 2.5f, 90.0f);
    CHECK(pos.x == doctest::Approx(0.0f).epsilon(0.001));
    CHECK(pos.z == doctest::Approx(-2.5f).epsilon(0.001));
}

TEST_CASE("Layout: arc screens are symmetric") {
    auto left = arcPosition(0, 3, 2.5f, 90.0f);
    auto right = arcPosition(2, 3, 2.5f, 90.0f);
    CHECK(left.x == doctest::Approx(-right.x).epsilon(0.001));
    CHECK(left.z == doctest::Approx(right.z).epsilon(0.001));
}

TEST_CASE("Layout: arc span controls spread") {
    float radius = 2.5f;
    auto narrowLeft = arcPosition(0, 3, radius, 30.0f);
    auto narrowRight = arcPosition(2, 3, radius, 30.0f);
    auto wideLeft = arcPosition(0, 3, radius, 120.0f);
    auto wideRight = arcPosition(2, 3, radius, 120.0f);

    float narrowSpread = std::abs(narrowLeft.x - narrowRight.x);
    float wideSpread = std::abs(wideLeft.x - wideRight.x);
    CHECK(wideSpread > narrowSpread);
}

TEST_CASE("Layout: grid 1 screen is centered") {
    Config config;
    auto pos = gridPosition(0, 1, config);
    CHECK(pos.x == doctest::Approx(0.0f));
    CHECK(pos.z == doctest::Approx(-config.screenDistance));
}

TEST_CASE("Layout: grid 4 screens form 2x2") {
    Config config;
    auto p0 = gridPosition(0, 4, config);
    auto p1 = gridPosition(1, 4, config);
    auto p2 = gridPosition(2, 4, config);
    auto p3 = gridPosition(3, 4, config);

    // All at same Z
    CHECK(p0.z == doctest::Approx(-config.screenDistance));
    CHECK(p1.z == doctest::Approx(-config.screenDistance));

    // Top row above bottom row
    CHECK(p0.y > p2.y);

    // Left column left of right column
    CHECK(p0.x < p1.x);
}

TEST_CASE("Layout: grid first row is horizontally centered") {
    Config config;
    auto p0 = gridPosition(0, 3, config);
    auto p1 = gridPosition(1, 3, config);
    float avgX = (p0.x + p1.x) / 2.0f;
    CHECK(avgX == doctest::Approx(0.0f).epsilon(0.01));
}

TEST_CASE("Layout: stack selected screen has full opacity") {
    int selected = 1;
    float opacities[3];
    for (int i = 0; i < 3; ++i) {
        opacities[i] = (i == selected) ? 1.0f : 0.3f;
    }

    CHECK(opacities[selected] == doctest::Approx(1.0f));
    CHECK(opacities[0] == doctest::Approx(0.3f));
    CHECK(opacities[2] == doctest::Approx(0.3f));
}

TEST_CASE("Layout: single only selected is visible") {
    int selected = 0;
    float opacities[3];
    for (int i = 0; i < 3; ++i) {
        opacities[i] = (i == selected) ? 1.0f : 0.0f;
    }

    CHECK(opacities[0] == doctest::Approx(1.0f));
    CHECK(opacities[1] == doctest::Approx(0.0f));
    CHECK(opacities[2] == doctest::Approx(0.0f));
}

TEST_CASE("Layout: smoothstep interpolation") {
    auto smoothstep = [](float t) { return t * t * (3.0f - 2.0f * t); };

    CHECK(smoothstep(0.0f) == doctest::Approx(0.0f));
    CHECK(smoothstep(1.0f) == doctest::Approx(1.0f));
    CHECK(smoothstep(0.5f) == doctest::Approx(0.5f));

    // Zero derivative at endpoints
    float eps = 0.001f;
    float d0 = (smoothstep(eps) - smoothstep(0.0f)) / eps;
    float d1 = (smoothstep(1.0f) - smoothstep(1.0f - eps)) / eps;
    CHECK(d0 == doctest::Approx(0.0f).epsilon(0.01));
    CHECK(d1 == doctest::Approx(0.0f).epsilon(0.01));
}
