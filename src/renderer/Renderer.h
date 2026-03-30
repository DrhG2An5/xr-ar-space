#pragma once

#include "renderer/Shader.h"
#include "renderer/Camera.h"
#include "renderer/VirtualScreen.h"
#include "app/Config.h"

#include <vector>
#include <memory>

namespace xr {

class Renderer {
public:
    Renderer() = default;

    bool init(const Config& config);
    void shutdown();

    void beginFrame();
    void render(const std::vector<std::unique_ptr<VirtualScreen>>& screens);
    void endFrame();

    Camera& camera() { return m_camera; }
    const Camera& camera() const { return m_camera; }

    void onResize(int width, int height);

    GLuint createCheckerTexture(int width, int height);

    // Transparent background: when true, clear color alpha = 0 so desktop shows through
    void setTransparentBackground(bool transparent) { m_transparentBg = transparent; }
    bool transparentBackground() const { return m_transparentBg; }
    void toggleTransparentBackground() { m_transparentBg = !m_transparentBg; }

private:
    Shader m_screenShader;
    Camera m_camera;
    Config m_config;
    bool m_transparentBg = false;
};

} // namespace xr
