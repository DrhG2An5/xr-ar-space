#include <doctest.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

// Replicate Raycaster::screenToWorldRay without including the full header
// (which pulls in VirtualScreen.h -> glad/gl.h)
struct TestRay {
    glm::vec3 origin;
    glm::vec3 direction;
};

static TestRay screenToWorldRay(float mouseX, float mouseY,
                                int viewportW, int viewportH,
                                const glm::mat4& invView,
                                const glm::mat4& invProj) {
    float ndcX = (2.0f * mouseX) / viewportW - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY) / viewportH;

    glm::vec4 clipNear{ndcX, ndcY, -1.0f, 1.0f};
    glm::vec4 clipFar{ndcX, ndcY, 1.0f, 1.0f};

    glm::vec4 eyeNear = invProj * clipNear;
    glm::vec4 eyeFar = invProj * clipFar;
    eyeNear /= eyeNear.w;
    eyeFar /= eyeFar.w;

    glm::vec4 worldNear = invView * eyeNear;
    glm::vec4 worldFar = invView * eyeFar;

    glm::vec3 origin = glm::vec3(worldNear);
    glm::vec3 direction = glm::normalize(glm::vec3(worldFar) - origin);

    return {origin, direction};
}

TEST_CASE("Raycaster: screenToWorldRay center of screen") {
    int w = 800, h = 600;
    float fov = glm::radians(60.0f);
    float aspect = static_cast<float>(w) / h;
    glm::mat4 proj = glm::perspective(fov, aspect, 0.1f, 100.0f);
    glm::mat4 view = glm::mat4(1.0f);

    glm::mat4 invView = glm::inverse(view);
    glm::mat4 invProj = glm::inverse(proj);

    auto ray = screenToWorldRay(400.0f, 300.0f, w, h, invView, invProj);

    CHECK(ray.origin.x == doctest::Approx(0.0f).epsilon(0.01));
    CHECK(ray.origin.y == doctest::Approx(0.0f).epsilon(0.01));
    CHECK(ray.direction.z < -0.9f);
    CHECK(std::abs(ray.direction.x) < 0.01f);
    CHECK(std::abs(ray.direction.y) < 0.01f);
}

TEST_CASE("Raycaster: screenToWorldRay left side of screen") {
    int w = 800, h = 600;
    glm::mat4 proj = glm::perspective(glm::radians(60.0f),
                                       static_cast<float>(w) / h, 0.1f, 100.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 invView = glm::inverse(view);
    glm::mat4 invProj = glm::inverse(proj);

    auto ray = screenToWorldRay(0.0f, 300.0f, w, h, invView, invProj);

    CHECK(ray.direction.x < 0.0f);
    CHECK(ray.direction.z < 0.0f);
}

TEST_CASE("Raycaster: screenToWorldRay with camera rotation") {
    int w = 800, h = 600;
    glm::mat4 proj = glm::perspective(glm::radians(60.0f),
                                       static_cast<float>(w) / h, 0.1f, 100.0f);
    // View matrix rotated 90 degrees around Y: camera looks along +X
    glm::mat4 view = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 1, 0));
    glm::mat4 invView = glm::inverse(view);
    glm::mat4 invProj = glm::inverse(proj);

    auto ray = screenToWorldRay(400.0f, 300.0f, w, h, invView, invProj);

    // Center ray should point along +X (view rotation rotates world, so camera looks +X)
    CHECK(std::abs(ray.direction.x) > 0.9f);
    CHECK(std::abs(ray.direction.z) < 0.1f);
}

TEST_CASE("Ray-plane intersection: basic Z-plane hit") {
    glm::vec3 origin{0.0f, 0.0f, 0.0f};
    glm::vec3 dir{0.0f, 0.0f, -1.0f};
    glm::vec3 planeNormal{0.0f, 0.0f, 1.0f};
    float planeD = -2.0f;

    float denom = glm::dot(planeNormal, dir);
    REQUIRE(std::abs(denom) > 1e-6f);

    float t = (planeD - glm::dot(planeNormal, origin)) / denom;
    CHECK(t == doctest::Approx(2.0f));

    glm::vec3 hit = origin + t * dir;
    CHECK(hit.z == doctest::Approx(-2.0f));
}

TEST_CASE("Ray-plane intersection: angled ray") {
    glm::vec3 origin{0.0f, 0.0f, 0.0f};
    glm::vec3 dir = glm::normalize(glm::vec3{1.0f, 0.0f, -1.0f});

    float t = 5.0f / std::abs(dir.z);
    glm::vec3 hit = origin + t * dir;

    CHECK(hit.z == doctest::Approx(-5.0f).epsilon(0.01));
    CHECK(hit.x > 0.0f);
}

TEST_CASE("Ray-quad intersection: centered quad hit") {
    // Simulate a quad at Z=-2 with width=1.6, height=0.9
    float hw = 0.8f, hh = 0.45f;
    glm::vec3 origin{0.0f, 0.0f, 0.0f};
    glm::vec3 dir{0.0f, 0.0f, -1.0f};

    float t = 2.0f; // hits Z=-2
    glm::vec3 hit = origin + t * dir;

    bool inBounds = (hit.x >= -hw && hit.x <= hw && hit.y >= -hh && hit.y <= hh);
    CHECK(inBounds);

    // UV computation
    float u = (hit.x + hw) / (2.0f * hw);
    float v = (hit.y + hh) / (2.0f * hh);
    CHECK(u == doctest::Approx(0.5f));
    CHECK(v == doctest::Approx(0.5f));
}

TEST_CASE("Ray-quad intersection: miss outside bounds") {
    float hw = 0.8f, hh = 0.45f;
    glm::vec3 origin{5.0f, 0.0f, 0.0f}; // far right
    glm::vec3 dir{0.0f, 0.0f, -1.0f};

    float t = 2.0f;
    glm::vec3 hit = origin + t * dir;

    bool inBounds = (hit.x >= -hw && hit.x <= hw && hit.y >= -hh && hit.y <= hh);
    CHECK_FALSE(inBounds);
}
