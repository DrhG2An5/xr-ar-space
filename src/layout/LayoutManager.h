#pragma once

#include "app/Config.h"
#include "renderer/VirtualScreen.h"
#include "util/MathUtils.h"

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include <cmath>

namespace xr {

enum class LayoutType { Arc, Grid, Stack, Single };

struct ScreenTarget {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    float opacity = 1.0f;
};

class LayoutManager {
public:
    void setLayout(LayoutType type) { m_type = type; }
    LayoutType layoutType() const { return m_type; }

    float transitionDuration() const { return m_transitionDuration; }
    void setTransitionDuration(float seconds) { m_transitionDuration = seconds; }

    bool isTransitioning() const { return m_transitioning; }

    const char* layoutName() const {
        switch (m_type) {
            case LayoutType::Arc:    return "Arc";
            case LayoutType::Grid:   return "Grid";
            case LayoutType::Stack:  return "Stack";
            case LayoutType::Single: return "Single";
        }
        return "Unknown";
    }

    void cycleLayout() {
        m_type = static_cast<LayoutType>((static_cast<int>(m_type) + 1) % 4);
    }

    // Compute target positions for the current layout and start a transition.
    void apply(std::vector<std::unique_ptr<VirtualScreen>>& screens,
               const Config& config, int selectedScreen = 0) {
        computeTargets(screens, config, selectedScreen);

        // If screens don't have targets yet (first call), snap immediately
        if (m_targets.size() != screens.size() || !m_initialized) {
            snapToTargets(screens);
            m_initialized = true;
            return;
        }

        // Start animated transition
        m_transitionProgress = 0.0f;
        m_transitioning = true;
    }

    // Call each frame to animate screens toward their target positions.
    // Returns true if still animating.
    bool update(std::vector<std::unique_ptr<VirtualScreen>>& screens, float dt) {
        if (!m_transitioning || m_targets.size() != screens.size()) return false;

        m_transitionProgress += dt / m_transitionDuration;
        if (m_transitionProgress >= 1.0f) {
            m_transitionProgress = 1.0f;
            m_transitioning = false;
        }

        // Smooth ease-in-out curve
        float t = smoothstep(m_transitionProgress);

        for (size_t i = 0; i < screens.size(); ++i) {
            auto& scr = screens[i];
            const auto& target = m_targets[i];

            glm::vec3 curPos = glm::mix(m_startPositions[i], target.position, t);
            glm::vec3 curRot = glm::mix(m_startRotations[i], target.rotation, t);
            float curOpacity = MathUtils::lerp(m_startOpacities[i], target.opacity, t);

            scr->setPosition(curPos);
            scr->setRotation(curRot);
            scr->setOpacity(curOpacity);
        }

        return m_transitioning;
    }

private:
    static float smoothstep(float t) {
        // Hermite interpolation: 3t^2 - 2t^3
        return t * t * (3.0f - 2.0f * t);
    }

    void computeTargets(const std::vector<std::unique_ptr<VirtualScreen>>& screens,
                        const Config& config, int selectedScreen) {
        size_t count = screens.size();
        m_targets.resize(count);

        // Capture current positions as start points for animation
        m_startPositions.resize(count);
        m_startRotations.resize(count);
        m_startOpacities.resize(count);
        for (size_t i = 0; i < count; ++i) {
            m_startPositions[i] = screens[i]->position();
            m_startRotations[i] = screens[i]->rotation();
            m_startOpacities[i] = screens[i]->opacity();
        }

        switch (m_type) {
            case LayoutType::Arc:    computeArc(config, count); break;
            case LayoutType::Grid:   computeGrid(config, count); break;
            case LayoutType::Stack:  computeStack(config, count, selectedScreen); break;
            case LayoutType::Single: computeSingle(config, count, selectedScreen); break;
        }
    }

    void snapToTargets(std::vector<std::unique_ptr<VirtualScreen>>& screens) {
        for (size_t i = 0; i < screens.size() && i < m_targets.size(); ++i) {
            screens[i]->setPosition(m_targets[i].position);
            screens[i]->setRotation(m_targets[i].rotation);
            screens[i]->setOpacity(m_targets[i].opacity);
        }
        m_transitioning = false;
    }

    void computeArc(const Config& config, size_t count) {
        if (count == 0) return;
        float radius = config.screenDistance;
        float spanRad = MathUtils::degToRad(config.arcSpanDeg);

        for (size_t i = 0; i < count; ++i) {
            float angle = 0.0f;
            if (count > 1) {
                float t = static_cast<float>(i) / (count - 1) - 0.5f;
                angle = t * spanRad;
            }
            float x = radius * std::sin(angle);
            float z = -radius * std::cos(angle);

            m_targets[i].position = {x, 0.0f, z};
            m_targets[i].rotation = {0.0f, -angle, 0.0f};
            m_targets[i].opacity = 1.0f;
        }
    }

    void computeGrid(const Config& config, size_t count) {
        if (count == 0) return;
        int cols = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(count))));

        float cellW = config.screenWidth + config.gridGap;
        float cellH = config.screenHeight + config.gridGap;
        float totalW = cols * cellW - config.gridGap;
        int rows = static_cast<int>(std::ceil(static_cast<float>(count) / cols));
        float totalH = rows * cellH - config.gridGap;
        float startX = -totalW * 0.5f + config.screenWidth * 0.5f;
        float startY = totalH * 0.5f - config.screenHeight * 0.5f;

        for (size_t i = 0; i < count; ++i) {
            int col = static_cast<int>(i) % cols;
            int row = static_cast<int>(i) / cols;
            float x = startX + col * cellW;
            float y = startY - row * cellH;

            m_targets[i].position = {x, y, -config.screenDistance};
            m_targets[i].rotation = {0.0f, 0.0f, 0.0f};
            m_targets[i].opacity = 1.0f;
        }
    }

    void computeStack(const Config& config, size_t count, int selectedScreen) {
        if (count == 0) return;
        for (size_t i = 0; i < count; ++i) {
            int dist = std::abs(static_cast<int>(i) - selectedScreen);
            float zOffset = (static_cast<int>(i) == selectedScreen) ? 0.0f : dist * config.stackZOffset;

            m_targets[i].position = {0.0f, 0.0f, -config.screenDistance + zOffset};
            m_targets[i].rotation = {0.0f, 0.0f, 0.0f};
            m_targets[i].opacity = (static_cast<int>(i) == selectedScreen) ? 1.0f : 0.3f;
        }
    }

    void computeSingle(const Config& config, size_t count, int selectedScreen) {
        if (count == 0) return;
        for (size_t i = 0; i < count; ++i) {
            m_targets[i].position = {0.0f, 0.0f, -config.screenDistance};
            m_targets[i].rotation = {0.0f, 0.0f, 0.0f};
            m_targets[i].opacity = (static_cast<int>(i) == selectedScreen) ? 1.0f : 0.0f;
        }
    }

    LayoutType m_type = LayoutType::Arc;
    float m_transitionDuration = 0.3f; // seconds
    float m_transitionProgress = 0.0f;
    bool m_transitioning = false;
    bool m_initialized = false;

    std::vector<ScreenTarget> m_targets;
    std::vector<glm::vec3> m_startPositions;
    std::vector<glm::vec3> m_startRotations;
    std::vector<float> m_startOpacities;
};

} // namespace xr
