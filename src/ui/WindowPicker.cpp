#include "ui/WindowPicker.h"

#include <imgui.h>
#include <algorithm>
#include <cctype>

namespace xr {

static bool containsInsensitive(const std::string& haystack, const char* needle) {
    if (!needle || needle[0] == '\0') return true;
    std::string lower = haystack;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    std::string needleLower(needle);
    std::transform(needleLower.begin(), needleLower.end(), needleLower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lower.find(needleLower) != std::string::npos;
}

void WindowPicker::draw() {
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(460, 500), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Window Picker", &m_visible)) {
        ImGui::End();
        return;
    }

    // Header: show which screen will receive the assignment
    ImGui::Text("Assign to: Screen %d", m_selectedScreen + 1);
    ImGui::SameLine();
    ImGui::TextDisabled("(%d screens)", m_screenCount);
    ImGui::Separator();

    // Filter
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##filter", "Filter windows...", m_filterBuf, sizeof(m_filterBuf));

    ImGui::Separator();

    // Scrollable list
    if (ImGui::BeginChild("WindowList", ImVec2(0, 0), ImGuiChildFlags_None)) {
        for (int i = 0; i < static_cast<int>(m_windowList.size()); ++i) {
            const auto& win = m_windowList[i];

            // Apply filter
            if (!containsInsensitive(win.title, m_filterBuf) &&
                !containsInsensitive(win.className, m_filterBuf)) {
                continue;
            }

            ImGui::PushID(i);

            // Window entry: title + class name + assign button
            bool clicked = ImGui::Selectable("##entry", false,
                                              ImGuiSelectableFlags_AllowDoubleClick,
                                              ImVec2(0, 40));

            // Overlay text on the selectable
            ImVec2 pos = ImGui::GetItemRectMin();
            ImDrawList* dl = ImGui::GetWindowDrawList();

            // Title line
            std::string label = win.title;
            if (label.size() > 60) label = label.substr(0, 57) + "...";
            dl->AddText(ImVec2(pos.x + 4, pos.y + 2),
                        IM_COL32(255, 255, 255, 255),
                        label.c_str(), label.c_str() + label.size());

            // Class name + dimensions
            std::string sub = win.className + "  " +
                              std::to_string(win.width) + "x" + std::to_string(win.height);
            dl->AddText(ImVec2(pos.x + 4, pos.y + 20),
                        IM_COL32(160, 160, 160, 255),
                        sub.c_str(), sub.c_str() + sub.size());

            // Assign button on the right side
            float buttonWidth = 70.0f;
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - buttonWidth + 8);
            if (ImGui::SmallButton("Assign") || (clicked && ImGui::IsMouseDoubleClicked(0))) {
                if (m_onAssign) {
                    m_onAssign(i);
                }
            }

            ImGui::PopID();
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

} // namespace xr
