#pragma once

#include "renderer/VirtualScreen.h"

#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace xr {

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

struct HitResult {
    bool hit = false;
    float distance = 0.0f;
    glm::vec2 uv{0.0f};
    int screenIndex = -1;
};

class Raycaster {
public:
    /// Convert mouse pixel coordinates to a world-space ray
    static Ray screenToWorldRay(float mouseX, float mouseY,
                                int viewportW, int viewportH,
                                const glm::mat4& invView,
                                const glm::mat4& invProj) {
        // Mouse -> NDC [-1, 1]
        float ndcX = (2.0f * mouseX) / viewportW - 1.0f;
        float ndcY = 1.0f - (2.0f * mouseY) / viewportH; // Flip Y

        // NDC -> clip space (near and far planes)
        glm::vec4 clipNear{ndcX, ndcY, -1.0f, 1.0f};
        glm::vec4 clipFar{ndcX, ndcY, 1.0f, 1.0f};

        // Clip -> eye space
        glm::vec4 eyeNear = invProj * clipNear;
        glm::vec4 eyeFar = invProj * clipFar;
        eyeNear /= eyeNear.w;
        eyeFar /= eyeFar.w;

        // Eye -> world space
        glm::vec4 worldNear = invView * eyeNear;
        glm::vec4 worldFar = invView * eyeFar;

        glm::vec3 origin = glm::vec3(worldNear);
        glm::vec3 direction = glm::normalize(glm::vec3(worldFar) - origin);

        return {origin, direction};
    }

    /// Test ray against a screen's quad (in screen's local space, Z=0 plane)
    static HitResult intersectQuad(const Ray& ray, const VirtualScreen& screen) {
        HitResult result;

        glm::mat4 model = screen.modelMatrix();
        glm::mat4 invModel = glm::inverse(model);

        // Transform ray into screen's local space
        glm::vec3 localOrigin = glm::vec3(invModel * glm::vec4(ray.origin, 1.0f));
        glm::vec3 localDir = glm::normalize(glm::vec3(invModel * glm::vec4(ray.direction, 0.0f)));

        // Intersect with Z=0 plane in local space
        if (std::abs(localDir.z) < 1e-6f) {
            return result; // Ray parallel to plane
        }

        float t = -localOrigin.z / localDir.z;
        if (t < 0.0f) {
            return result; // Behind camera
        }

        glm::vec3 hitLocal = localOrigin + t * localDir;

        // Check bounds: quad goes from [-w/2, w/2] x [-h/2, h/2]
        float hw = screen.width() * 0.5f;
        float hh = screen.height() * 0.5f;

        if (hitLocal.x >= -hw && hitLocal.x <= hw &&
            hitLocal.y >= -hh && hitLocal.y <= hh) {
            result.hit = true;
            // Compute world-space distance
            glm::vec3 worldHit = glm::vec3(model * glm::vec4(hitLocal, 1.0f));
            result.distance = glm::length(worldHit - ray.origin);
            // UV: [0,1] from bottom-left
            result.uv = glm::vec2(
                (hitLocal.x + hw) / screen.width(),
                (hitLocal.y + hh) / screen.height()
            );
        }

        return result;
    }

    /// Find the closest screen hit by a ray
    static HitResult pickScreen(const Ray& ray,
                                const std::vector<std::unique_ptr<VirtualScreen>>& screens) {
        HitResult closest;
        closest.distance = std::numeric_limits<float>::max();

        for (int i = 0; i < static_cast<int>(screens.size()); ++i) {
            HitResult hit = intersectQuad(ray, *screens[i]);
            if (hit.hit && hit.distance < closest.distance) {
                closest = hit;
                closest.screenIndex = i;
            }
        }

        if (closest.screenIndex == -1) {
            closest.hit = false;
        }
        return closest;
    }
};

} // namespace xr
