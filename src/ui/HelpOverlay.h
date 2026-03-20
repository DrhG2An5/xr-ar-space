#pragma once

#include <imgui.h>
#include <string>

namespace xr {

class HelpOverlay {
public:
    void toggle() { m_visible = !m_visible; }
    bool isVisible() const { return m_visible; }
    void show() { m_visible = true; }
    void hide() { m_visible = false; }

    // Set transient status text shown in the bottom bar (e.g., "Keyboard → Screen 2")
    void setStatus(const char* text) { m_status = text ? text : ""; }
    void clearStatus() { m_status.clear(); }

    void draw() {
        drawHint();

        if (!m_visible) return;

        ImGui::SetNextWindowSize(ImVec2(440, 560), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(
            ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f),
            ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

        if (ImGui::Begin("Help — Keyboard Shortcuts", &m_visible)) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "General");
            ImGui::Separator();
            helpEntry("F1",           "Toggle this help overlay");
            helpEntry("Escape",       "Close app / exit keyboard mode");
            helpEntry("Ctrl+S",       "Save configuration");

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Screens");
            ImGui::Separator();
            helpEntry("Ctrl+Tab",     "Cycle selected screen");
            helpEntry("Ctrl+L",       "Cycle layout (arc/grid/stack/single)");
            helpEntry("Ctrl+P",       "Pin / unpin selected screen");
            helpEntry("Ctrl+Z",       "Zoom / unzoom selected screen");
            helpEntry("Ctrl+1..9",    "Assign window to selected screen");

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "UI Panels");
            ImGui::Separator();
            helpEntry("Ctrl+W",       "Window picker");
            helpEntry("Ctrl+G",       "Settings panel");

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Camera & Display");
            ImGui::Separator();
            helpEntry("Ctrl+R",       "Reset camera orientation");
            helpEntry("Ctrl+H",       "Toggle head tracking");
            helpEntry("Ctrl+F",       "Toggle fullscreen");
            helpEntry("Ctrl+D",       "Refresh display list");
            helpEntry("Ctrl+V",       "Toggle virtual display");
            helpEntry("Arrow keys",   "Rotate camera");
            helpEntry("Ctrl+Scroll",  "Zoom (change screen distance)");

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Interaction");
            ImGui::Separator();
            helpEntry("Ctrl+K",       "Toggle keyboard forwarding");
            helpEntry("Left-click",   "Select screen / click into window");
            helpEntry("Ctrl+Drag",    "Move screen");
            helpEntry("Shift+Drag",   "Resize screen");
            helpEntry("Middle-drag",  "Move screen (alt)");
            helpEntry("Right-click",  "Right-click into window");
            helpEntry("Scroll",       "Scroll in captured window");
        }
        ImGui::End();
    }

private:
    // Always-visible hint bar at the bottom of the screen
    void drawHint() {
        ImGuiIO& io = ImGui::GetIO();
        float barHeight = 28.0f;
        float y = io.DisplaySize.y - barHeight;

        ImGui::SetNextWindowPos(ImVec2(0, y));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, barHeight));

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
                               | ImGuiWindowFlags_NoMove
                               | ImGuiWindowFlags_NoResize
                               | ImGuiWindowFlags_NoSavedSettings
                               | ImGuiWindowFlags_NoFocusOnAppearing
                               | ImGuiWindowFlags_NoNav
                               | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 4));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));

        if (ImGui::Begin("##StatusBar", nullptr, flags)) {
            // Left side: status text
            if (!m_status.empty()) {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "%s", m_status.c_str());
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "|");
                ImGui::SameLine();
            }

            // Right side: help hint (clickable)
            float hintWidth = ImGui::CalcTextSize("F1 Shortcuts").x + 16.0f;
            ImGui::SetCursorPosX(io.DisplaySize.x - hintWidth - 8.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
            if (ImGui::SmallButton("F1 Shortcuts")) {
                m_visible = !m_visible;
            }
            ImGui::PopStyleColor(2);
        }
        ImGui::End();

        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

    void helpEntry(const char* key, const char* desc) {
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "%-16s", key);
        ImGui::SameLine();
        ImGui::TextUnformatted(desc);
    }

    bool m_visible = false;
    std::string m_status;
};

} // namespace xr
