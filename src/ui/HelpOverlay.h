#pragma once

#include <imgui.h>

namespace xr {

class HelpOverlay {
public:
    void toggle() { m_visible = !m_visible; }
    bool isVisible() const { return m_visible; }
    void show() { m_visible = true; }
    void hide() { m_visible = false; }

    void draw() {
        if (!m_visible) return;

        ImGui::SetNextWindowSize(ImVec2(420, 520), ImGuiCond_FirstUseEver);
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
    void helpEntry(const char* key, const char* desc) {
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "%-16s", key);
        ImGui::SameLine();
        ImGui::TextUnformatted(desc);
    }

    bool m_visible = false;
};

} // namespace xr
