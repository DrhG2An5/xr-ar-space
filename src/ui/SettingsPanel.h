#pragma once

#include "app/Config.h"

#include <functional>

namespace xr {

class SettingsPanel {
public:
    using LayoutChangeCallback = std::function<void()>;

    void setVisible(bool v) { m_visible = v; }
    bool isVisible() const { return m_visible; }
    void toggle() { m_visible = !m_visible; }

    void setConfig(Config* config) { m_config = config; }
    void setOnLayoutChange(LayoutChangeCallback cb) { m_onLayoutChange = std::move(cb); }

    void draw();

private:
    bool m_visible = false;
    Config* m_config = nullptr;
    LayoutChangeCallback m_onLayoutChange;
};

} // namespace xr
