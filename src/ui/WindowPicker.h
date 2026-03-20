#pragma once

#include "capture/WindowEnumerator.h"

#include <functional>
#include <vector>
#include <string>

namespace xr {

class WindowPicker {
public:
    using AssignCallback = std::function<void(int windowIndex)>;

    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }
    void toggle() { m_visible = !m_visible; }

    void setWindowList(const std::vector<WindowInfo>& list) { m_windowList = list; }
    void setScreenCount(int count) { m_screenCount = count; }
    void setSelectedScreen(int idx) { m_selectedScreen = idx; }
    void setOnAssign(AssignCallback cb) { m_onAssign = std::move(cb); }

    // Call once per frame inside ImGui begin/end frame
    void draw();

private:
    bool m_visible = false;
    std::vector<WindowInfo> m_windowList;
    int m_screenCount = 0;
    int m_selectedScreen = 0;
    AssignCallback m_onAssign;
    char m_filterBuf[256] = {};
};

} // namespace xr
