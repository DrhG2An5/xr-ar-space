#pragma once

#include "app/Config.h"
#include "renderer/Renderer.h"
#include "renderer/VirtualScreen.h"
#include "capture/CaptureTexture.h"
#include "capture/WindowEnumerator.h"
#include "tracking/HeadTracker.h"
#include "layout/LayoutManager.h"
#include "interaction/Raycaster.h"
#include "display/DisplayDetector.h"
#include "display/WindowPositioner.h"
#include "util/Timer.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <optional>

namespace xr {

class App {
public:
    App() = default;
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    bool init(const Config& config = {});
    void run();
    void shutdown();

    const Config& config() const { return m_config; }

private:
    void processInput(float dt);
    void createTestScreens();
    void refreshWindowList();
    void assignWindowToScreen(int windowIndex);
    void updateCaptureTextures();
    void updateHover(float mouseX, float mouseY);

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    GLFWwindow* m_window = nullptr;
    Config m_config;
    Renderer m_renderer;
    Timer m_timer;

    std::vector<std::unique_ptr<VirtualScreen>> m_screens;
    int m_selectedScreen = 0;

    // Mouse interaction
    int m_hoveredScreen = -1;
    bool m_dragging = false;
    glm::vec2 m_lastMousePos{0.0f};
    float m_dragDepth = 0.0f;

    // Capture state
    std::vector<std::unique_ptr<CaptureTexture>> m_captureTextures;
    std::vector<WindowInfo> m_windowList;

    // Head tracking
    std::unique_ptr<HeadTracker> m_headTracker;
    bool m_headTrackingEnabled = false;

    // Layout
    LayoutManager m_layoutManager;

    // Display detection
    std::vector<DisplayInfo> m_displays;
    std::optional<DisplayInfo> m_xrealDisplay;

    bool m_running = false;
};

} // namespace xr
