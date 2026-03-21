#include "renderer/Renderer.h"
#include "util/Log.h"

#include <vector>

namespace xr {

bool Renderer::init(const Config& config) {
    m_config = config;

    if (!m_screenShader.loadFromFiles(
            config.shaderPath + "screen.vert",
            config.shaderPath + "screen.frag")) {
        Log::error("Failed to load screen shader");
        return false;
    }

    // Set up camera
    float aspect = static_cast<float>(config.windowWidth) / config.windowHeight;
    m_camera.setPerspective(config.fovDegrees, aspect, config.nearPlane, config.farPlane);

    // OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Log::info("Renderer initialized ({}x{})", config.windowWidth, config.windowHeight);
    return true;
}

void Renderer::shutdown() {
    // Shader cleans up in destructor
}

void Renderer::beginFrame() {
    if (m_transparentBg) {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    } else {
        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::render(const std::vector<std::unique_ptr<VirtualScreen>>& screens) {
    m_screenShader.use();
    m_screenShader.setMat4("uView", m_camera.viewMatrix());
    m_screenShader.setMat4("uProjection", m_camera.projectionMatrix());
    m_screenShader.setInt("uTexture", 0);

    for (const auto& screen : screens) {
        m_screenShader.setMat4("uModel", screen->modelMatrix());
        m_screenShader.setFloat("uOpacity", screen->opacity());
        m_screenShader.setBool("uSelected", screen->selected());
        m_screenShader.setVec4("uBorderColor", glm::vec4(0.2f, 0.6f, 1.0f, 1.0f));
        m_screenShader.setFloat("uBorderWidth", 0.01f);
        m_screenShader.setBool("uHovered", screen->hovered());
        m_screenShader.setVec4("uHoverColor", glm::vec4(1.0f, 1.0f, 1.0f, 0.5f));
        screen->draw();
    }
}

void Renderer::endFrame() {
    // Swap is handled by GLFW in App
}

void Renderer::onResize(int width, int height) {
    glViewport(0, 0, width, height);
    float aspect = static_cast<float>(width) / height;
    m_camera.setPerspective(m_config.fovDegrees, aspect, m_config.nearPlane, m_config.farPlane);
}

GLuint Renderer::createCheckerTexture(int width, int height) {
    std::vector<uint8_t> pixels(width * height * 4);
    int checkerSize = 32;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * 4;
            bool white = ((x / checkerSize) + (y / checkerSize)) % 2 == 0;
            uint8_t c = white ? 200 : 60;
            pixels[idx + 0] = c;
            pixels[idx + 1] = c;
            pixels[idx + 2] = c;
            pixels[idx + 3] = 255;
        }
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    return tex;
}

} // namespace xr
