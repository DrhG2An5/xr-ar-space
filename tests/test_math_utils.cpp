#include <doctest.h>
#include "util/MathUtils.h"

using namespace xr;

TEST_CASE("MathUtils: degToRad") {
    CHECK(MathUtils::degToRad(0.0f) == doctest::Approx(0.0f));
    CHECK(MathUtils::degToRad(180.0f) == doctest::Approx(MathUtils::kPi));
    CHECK(MathUtils::degToRad(90.0f) == doctest::Approx(MathUtils::kHalfPi));
    CHECK(MathUtils::degToRad(360.0f) == doctest::Approx(MathUtils::kTwoPi));
}

TEST_CASE("MathUtils: radToDeg") {
    CHECK(MathUtils::radToDeg(0.0f) == doctest::Approx(0.0f));
    CHECK(MathUtils::radToDeg(MathUtils::kPi) == doctest::Approx(180.0f));
    CHECK(MathUtils::radToDeg(MathUtils::kHalfPi) == doctest::Approx(90.0f));
}

TEST_CASE("MathUtils: degToRad and radToDeg are inverses") {
    float deg = 42.5f;
    CHECK(MathUtils::radToDeg(MathUtils::degToRad(deg)) == doctest::Approx(deg));
}

TEST_CASE("MathUtils: clamp") {
    CHECK(MathUtils::clamp(5.0f, 0.0f, 10.0f) == doctest::Approx(5.0f));
    CHECK(MathUtils::clamp(-1.0f, 0.0f, 10.0f) == doctest::Approx(0.0f));
    CHECK(MathUtils::clamp(15.0f, 0.0f, 10.0f) == doctest::Approx(10.0f));
    CHECK(MathUtils::clamp(0.0f, 0.0f, 0.0f) == doctest::Approx(0.0f));
}

TEST_CASE("MathUtils: lerp") {
    CHECK(MathUtils::lerp(0.0f, 10.0f, 0.0f) == doctest::Approx(0.0f));
    CHECK(MathUtils::lerp(0.0f, 10.0f, 1.0f) == doctest::Approx(10.0f));
    CHECK(MathUtils::lerp(0.0f, 10.0f, 0.5f) == doctest::Approx(5.0f));
    CHECK(MathUtils::lerp(2.0f, 8.0f, 0.25f) == doctest::Approx(3.5f));
}

TEST_CASE("MathUtils: lerp extrapolation") {
    // lerp should work beyond [0,1] range
    CHECK(MathUtils::lerp(0.0f, 10.0f, 2.0f) == doctest::Approx(20.0f));
    CHECK(MathUtils::lerp(0.0f, 10.0f, -1.0f) == doctest::Approx(-10.0f));
}
