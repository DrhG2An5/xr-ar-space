#include "ui/SettingsPanel.h"

#include <imgui.h>

namespace xr {

void SettingsPanel::draw() {
    if (!m_visible || !m_config) return;

    ImGui::SetNextWindowSize(ImVec2(360, 480), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(500, 20), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Settings", &m_visible)) {
        ImGui::End();
        return;
    }

    bool layoutChanged = false;

    // --- Display ---
    if (ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen)) {
        layoutChanged |= ImGui::SliderFloat("FOV", &m_config->fovDegrees, 30.0f, 120.0f, "%.0f deg");
        layoutChanged |= ImGui::SliderFloat("Screen Distance", &m_config->screenDistance,
                                             m_config->minDistance, m_config->maxDistance, "%.1f");
        ImGui::SliderFloat("Min Distance", &m_config->minDistance, 0.5f, 5.0f, "%.1f");
        ImGui::SliderFloat("Max Distance", &m_config->maxDistance, 3.0f, 20.0f, "%.1f");
    }

    // --- Screen Size ---
    if (ImGui::CollapsingHeader("Screen Size", ImGuiTreeNodeFlags_DefaultOpen)) {
        layoutChanged |= ImGui::SliderFloat("Width", &m_config->screenWidth, 0.5f, 4.0f, "%.2f");
        layoutChanged |= ImGui::SliderFloat("Height", &m_config->screenHeight, 0.3f, 2.5f, "%.2f");
        if (ImGui::Button("16:9")) {
            m_config->screenHeight = m_config->screenWidth * 9.0f / 16.0f;
            layoutChanged = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("21:9")) {
            m_config->screenHeight = m_config->screenWidth * 9.0f / 21.0f;
            layoutChanged = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("4:3")) {
            m_config->screenHeight = m_config->screenWidth * 3.0f / 4.0f;
            layoutChanged = true;
        }
        layoutChanged |= ImGui::SliderFloat("Curvature", &m_config->screenCurvature, 0.0f, 1.0f, "%.2f");
    }

    // --- Layout ---
    if (ImGui::CollapsingHeader("Layout")) {
        layoutChanged |= ImGui::SliderFloat("Arc Span", &m_config->arcSpanDeg, 30.0f, 180.0f, "%.0f deg");
        layoutChanged |= ImGui::SliderFloat("Grid Gap", &m_config->gridGap, 0.0f, 0.5f, "%.2f");
        layoutChanged |= ImGui::SliderFloat("Stack Z-Offset", &m_config->stackZOffset, 0.01f, 0.2f, "%.3f");
    }

    // --- Interaction ---
    if (ImGui::CollapsingHeader("Interaction")) {
        ImGui::SliderFloat("Rotation Speed", &m_config->rotationSpeed, 30.0f, 180.0f, "%.0f deg/s");
        ImGui::SliderFloat("Scroll Speed", &m_config->scrollSpeed, 0.1f, 1.0f, "%.2f");
    }

    // --- Capture ---
    if (ImGui::CollapsingHeader("Capture")) {
        int fps = m_config->captureFrameRate;
        if (ImGui::SliderInt("Capture FPS", &fps, 10, 60)) {
            m_config->captureFrameRate = fps;
        }
    }

    if (layoutChanged && m_onLayoutChange) {
        m_onLayoutChange();
    }

    ImGui::End();
}

} // namespace xr
