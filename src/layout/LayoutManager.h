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

class LayoutManager {
public:
    void setLayout(LayoutType type) { m_type = type; }
    LayoutType layoutType() const { return m_type; }

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

    void apply(std::vector<std::unique_ptr<VirtualScreen>>& screens,
               const Config& config, int selectedScreen = 0) {
        switch (m_type) {
            case LayoutType::Arc:    applyArc(screens, config); break;
            case LayoutType::Grid:   applyGrid(screens, config); break;
            case LayoutType::Stack:  applyStack(screens, config, selectedScreen); break;
            case LayoutType::Single: applySingle(screens, config, selectedScreen); break;
        }
    }

private:
    void applyArc(std::vector<std::unique_ptr<VirtualScreen>>& screens,
                  const Config& config) {
        int count = static_cast<int>(screens.size());
        if (count == 0) return;

        float radius = config.screenDistance;
        float spanRad = MathUtils::degToRad(config.arcSpanDeg);

        for (int i = 0; i < count; ++i) {
            // Angle: centered around 0, spread across arcSpanDeg
            float angle = 0.0f;
            if (count > 1) {
                float t = static_cast<float>(i) / (count - 1) - 0.5f; // [-0.5, 0.5]
                angle = t * spanRad;
            }

            float x = radius * std::sin(angle);
            float z = -radius * std::cos(angle);

            screens[i]->setPosition({x, 0.0f, z});
            screens[i]->setRotation({0.0f, -angle, 0.0f}); // Face toward origin
            screens[i]->setOpacity(1.0f);
        }
    }

    void applyGrid(std::vector<std::unique_ptr<VirtualScreen>>& screens,
                   const Config& config) {
        int count = static_cast<int>(screens.size());
        if (count == 0) return;

        int cols = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(count))));
        int rows = static_cast<int>(std::ceil(static_cast<float>(count) / cols));

        float cellW = config.screenWidth + config.gridGap;
        float cellH = config.screenHeight + config.gridGap;

        // Center the grid
        float totalW = cols * cellW - config.gridGap;
        float totalH = rows * cellH - config.gridGap;
        float startX = -totalW * 0.5f + config.screenWidth * 0.5f;
        float startY = totalH * 0.5f - config.screenHeight * 0.5f;

        for (int i = 0; i < count; ++i) {
            int col = i % cols;
            int row = i / cols;

            float x = startX + col * cellW;
            float y = startY - row * cellH;

            screens[i]->setPosition({x, y, -config.screenDistance});
            screens[i]->setRotation({0.0f, 0.0f, 0.0f});
            screens[i]->setOpacity(1.0f);
        }
    }

    void applyStack(std::vector<std::unique_ptr<VirtualScreen>>& screens,
                    const Config& config, int selectedScreen) {
        int count = static_cast<int>(screens.size());
        if (count == 0) return;

        for (int i = 0; i < count; ++i) {
            float zOffset = 0.0f;
            if (i == selectedScreen) {
                zOffset = 0.0f; // Selected screen at front
            } else {
                // Non-selected screens behind, ordered by distance from selected
                int dist = std::abs(i - selectedScreen);
                zOffset = dist * config.stackZOffset;
            }

            screens[i]->setPosition({0.0f, 0.0f, -config.screenDistance + zOffset});
            screens[i]->setRotation({0.0f, 0.0f, 0.0f});
            screens[i]->setOpacity(i == selectedScreen ? 1.0f : 0.3f);
        }
    }

    void applySingle(std::vector<std::unique_ptr<VirtualScreen>>& screens,
                     const Config& config, int selectedScreen) {
        int count = static_cast<int>(screens.size());
        if (count == 0) return;

        for (int i = 0; i < count; ++i) {
            screens[i]->setPosition({0.0f, 0.0f, -config.screenDistance});
            screens[i]->setRotation({0.0f, 0.0f, 0.0f});
            screens[i]->setOpacity(i == selectedScreen ? 1.0f : 0.0f);
        }
    }

    LayoutType m_type = LayoutType::Arc;
};

} // namespace xr
