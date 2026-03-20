#include "ui/WindowPicker.h"

#include <imgui.h>
#include <algorithm>
#include <cctype>

namespace xr {

WindowPicker::~WindowPicker() {
    clearThumbnails();
}

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

void WindowPicker::setWindowList(const std::vector<WindowInfo>& list) {
    m_windowList = list;
    m_listDirty = true;
}

void WindowPicker::uploadThumbnail(int index) {
    auto& win = m_windowList[index];
    if (win.thumbnail.empty()) {
        WindowEnumerator::captureThumbnail(win);
    }
    if (win.thumbnail.empty()) return;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, win.thumbnailW, win.thumbnailH,
                 0, GL_BGRA, GL_UNSIGNED_BYTE, win.thumbnail.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_thumbnailTextures[index] = tex;
}

void WindowPicker::clearThumbnails() {
    for (auto& [idx, tex] : m_thumbnailTextures) {
        if (tex) glDeleteTextures(1, &tex);
    }
    m_thumbnailTextures.clear();
}

void WindowPicker::draw() {
    if (!m_visible) return;

    // Lazy-load thumbnails when list changes
    if (m_listDirty) {
        clearThumbnails();
        m_listDirty = false;
    }

    ImGui::SetNextWindowSize(ImVec2(500, 550), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Window Picker", &m_visible)) {
        ImGui::End();
        return;
    }

    // Header
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
        const float thumbDisplayW = 80.0f;
        const float thumbDisplayH = 50.0f;
        const float rowHeight = 56.0f;

        for (int i = 0; i < static_cast<int>(m_windowList.size()); ++i) {
            const auto& win = m_windowList[i];

            if (!containsInsensitive(win.title, m_filterBuf) &&
                !containsInsensitive(win.className, m_filterBuf)) {
                continue;
            }

            ImGui::PushID(i);

            // Lazy-load thumbnail on first visible frame
            if (m_thumbnailTextures.find(i) == m_thumbnailTextures.end()) {
                uploadThumbnail(i);
            }

            // Thumbnail
            float startY = ImGui::GetCursorPosY();
            auto it = m_thumbnailTextures.find(i);
            if (it != m_thumbnailTextures.end() && it->second != 0) {
                ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(it->second)),
                             ImVec2(thumbDisplayW, thumbDisplayH));
            } else {
                // Placeholder
                ImVec2 p = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddRectFilled(
                    p, ImVec2(p.x + thumbDisplayW, p.y + thumbDisplayH),
                    IM_COL32(40, 40, 40, 255));
                ImGui::Dummy(ImVec2(thumbDisplayW, thumbDisplayH));
            }

            // Text beside thumbnail
            ImGui::SameLine();
            ImGui::BeginGroup();

            // Truncated title
            std::string label = win.title;
            if (label.size() > 45) label = label.substr(0, 42) + "...";
            ImGui::TextUnformatted(label.c_str());

            // Class + dimensions
            ImGui::TextDisabled("%s  %dx%d", win.className.c_str(), win.width, win.height);

            // Assign button
            if (ImGui::SmallButton("Assign")) {
                if (m_onAssign) m_onAssign(i);
            }
            ImGui::EndGroup();

            // Ensure consistent row height
            float usedH = ImGui::GetCursorPosY() - startY;
            if (usedH < rowHeight) {
                ImGui::Dummy(ImVec2(0, rowHeight - usedH));
            }

            ImGui::Separator();
            ImGui::PopID();
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

} // namespace xr
