#pragma once

#include "app/Config.h"
#include "renderer/Renderer.h"
#include "renderer/VirtualScreen.h"
#include "capture/CaptureTexture.h"
#include "capture/WindowEnumerator.h"
#include "tracking/HeadTracker.h"
#include "layout/LayoutManager.h"
#include "util/Timer.h"

#include <GLFW/glfw3.h>
#include <vector>
#include <memory>

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

private:
    void processInput(float dt);
    void createTestScreens();
    void refreshWindowList();
    void assignWindowToScreen(int windowIndex);
    void updateCaptureTextures();

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    GLFWwindow* m_window = nullptr;
    Config m_config;
    Renderer m_renderer;
    Timer m_timer;

    std::vector<std::unique_ptr<VirtualScreen>> m_screens;
    int m_selectedScreen = 0;

    // Capture state
    std::vector<std::unique_ptr<CaptureTexture>> m_captureTextures;
    std::vector<WindowInfo> m_windowList;

    // Head tracking
    std::unique_ptr<HeadTracker> m_headTracker;
    bool m_headTrackingEnabled = false;

    // Layout
    LayoutManager m_layoutManager;

    bool m_running = false;
};

} // namespace xr
