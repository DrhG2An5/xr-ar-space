#include "app/App.h"
#include "capture/WindowCapturer.h"
#include "interaction/InputInjector.h"
#include "util/Log.h"
#include "util/MathUtils.h"
#include "util/ConfigFile.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace xr {

App::~App() {
    shutdown();
}

bool App::init(const Config& config) {
    m_config = config;

    // Initialize GLFW
    if (!glfwInit()) {
        Log::error("Failed to initialize GLFW");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    m_window = glfwCreateWindow(
        config.windowWidth, config.windowHeight,
        config.windowTitle.c_str(), nullptr, nullptr);

    if (!m_window) {
        Log::error("Failed to create GLFW window");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // VSync

    // Store this pointer for callbacks
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetCursorPosCallback(m_window, cursorPosCallback);
    glfwSetScrollCallback(m_window, scrollCallback);

    // Initialize GLAD (GLAD2 API)
    int version = gladLoadGL(glfwGetProcAddress);
    if (!version) {
        Log::error("Failed to initialize GLAD");
        return false;
    }
    Log::info("OpenGL {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

    // Initialize renderer
    if (!m_renderer.init(config)) {
        Log::error("Failed to initialize renderer");
        return false;
    }

    // Create test content
    createTestScreens();

    // Enumerate and display available windows
    refreshWindowList();

    // Initialize head tracking (non-fatal if no device found)
    m_headTracker = std::make_unique<HeadTracker>();
    if (m_headTracker->start()) {
        m_headTrackingEnabled = true;
        Log::info("Head tracking enabled (press H to toggle)");
    } else {
        Log::info("No XREAL device — use arrow keys for camera (press H to retry)");
    }

    // Detect displays and look for XREAL glasses
    m_displays = DisplayDetector::enumerate();
    DisplayDetector::printList(m_displays);
    m_xrealDisplay = DisplayDetector::findXreal();
    if (m_xrealDisplay) {
        Log::info("XREAL display found: {} ({}x{}) — press F to go fullscreen",
                  m_xrealDisplay->monitorName.empty() ? m_xrealDisplay->deviceName : m_xrealDisplay->monitorName,
                  m_xrealDisplay->width, m_xrealDisplay->height);
    } else {
        Log::info("No XREAL display detected — press D to refresh displays, F for fullscreen on primary");
    }

    m_running = true;
    Log::info("App initialized successfully");
    return true;
}

void App::run() {
    m_timer.tick(); // Reset delta

    while (m_running && !glfwWindowShouldClose(m_window)) {
        float dt = m_timer.tick();

        glfwPollEvents();
        processInput(dt);

        // Update head tracking
        if (m_headTrackingEnabled && m_headTracker) {
            m_headTracker->update();

            if (m_headTracker->isActive()) {
                auto ori = m_headTracker->orientation();
                m_renderer.camera().setYaw(ori.yaw);
                m_renderer.camera().setPitch(ori.pitch);
                m_headTrackingRetryTimer = 0.0f;
            } else {
                // Device disconnected mid-session
                m_headTrackingEnabled = false;
                Log::warn("Head tracking lost — will retry in {}s (press H to retry now)",
                          static_cast<int>(kHeadTrackingRetryInterval));
            }
        } else if (!m_headTrackingEnabled && m_headTracker) {
            // Periodic auto-retry for head tracking reconnection
            m_headTrackingRetryTimer += dt;
            if (m_headTrackingRetryTimer >= kHeadTrackingRetryInterval) {
                m_headTrackingRetryTimer = 0.0f;
                m_headTracker->stop();
                if (m_headTracker->start()) {
                    m_headTrackingEnabled = true;
                    Log::info("Head tracking reconnected");
                }
            }
        }

        // Animate layout transitions
        m_layoutManager.update(m_screens, dt);

        // Update capture textures; clean up dead captures
        updateCaptureTextures();

        // Check for dead capture sources (window was closed)
        for (size_t i = 0; i < m_captureTextures.size(); ++i) {
            auto* src = m_captureTextures[i]->source();
            if (src && !src->isRunning()) {
                Log::warn("Capture source for screen {} stopped (window closed?)", i + 1);
                m_captureTextures[i]->setSource(nullptr);
            }
        }

        m_renderer.beginFrame();
        m_renderer.render(m_screens);
        m_renderer.endFrame();

        glfwSwapBuffers(m_window);
    }
}

void App::shutdown() {
    // Stop head tracking first
    if (m_headTracker) {
        m_headTracker->stop();
        m_headTracker.reset();
    }

    // Stop all capture threads before destroying GL resources
    m_captureTextures.clear();

    m_screens.clear();

    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();

    Log::info("App shut down");
}

void App::processInput(float dt) {
    float rotSpeed = MathUtils::degToRad(m_config.rotationSpeed) * dt;

    // Camera rotation with arrow keys
    if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS)
        m_renderer.camera().rotate(-rotSpeed, 0.0f);
    if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        m_renderer.camera().rotate(rotSpeed, 0.0f);
    if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS)
        m_renderer.camera().rotate(0.0f, rotSpeed);
    if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS)
        m_renderer.camera().rotate(0.0f, -rotSpeed);
}

void App::createTestScreens() {
    const int screenCount = 3;

    for (int i = 0; i < screenCount; ++i) {
        auto screen = std::make_unique<VirtualScreen>();
        screen->init(m_config.screenWidth, m_config.screenHeight);

        // Create a checker texture for each (different sizes for visual variety)
        int texW = 512 + i * 256;
        int texH = static_cast<int>(texW * 9.0f / 16.0f);
        GLuint tex = m_renderer.createCheckerTexture(texW, texH);
        screen->setTexture(tex);

        if (i == 1) screen->setSelected(true); // Middle screen selected by default

        m_screens.push_back(std::move(screen));

        // Create a corresponding CaptureTexture slot (initially no source)
        m_captureTextures.push_back(std::make_unique<CaptureTexture>());
    }

    m_selectedScreen = 1;

    // Apply layout to position screens
    m_layoutManager.apply(m_screens, m_config, m_selectedScreen);

    Log::info("Created {} screens (layout: {})", m_screens.size(), m_layoutManager.layoutName());
}

void App::refreshWindowList() {
    // Exclude our own window to prevent self-capture loop
    HWND selfHwnd = glfwGetWin32Window(m_window);
    m_windowList = WindowEnumerator::enumerate(selfHwnd);
    WindowEnumerator::printList(m_windowList);
}

void App::assignWindowToScreen(int windowIndex) {
    // Auto-refresh window list to avoid stale entries
    HWND selfHwnd = glfwGetWin32Window(m_window);
    m_windowList = WindowEnumerator::enumerate(selfHwnd);

    if (windowIndex < 0 || windowIndex >= static_cast<int>(m_windowList.size())) {
        Log::warn("Window index {} out of range (1-{})", windowIndex + 1, m_windowList.size());
        return;
    }
    if (m_selectedScreen < 0 || m_selectedScreen >= static_cast<int>(m_screens.size())) {
        return;
    }

    const auto& winInfo = m_windowList[windowIndex];
    Log::info("Assigning '{}' to screen {}", winInfo.title, m_selectedScreen + 1);

    // Create a new WindowCapturer for this HWND
    auto capturer = std::make_unique<WindowCapturer>(winInfo.hwnd, m_config.captureFrameRate);
    if (!capturer->start()) {
        Log::error("Failed to start capture for '{}'", winInfo.title);
        return;
    }

    // Set the source on the CaptureTexture for this screen
    m_captureTextures[m_selectedScreen]->setSource(std::move(capturer));
}

void App::updateCaptureTextures() {
    for (size_t i = 0; i < m_captureTextures.size() && i < m_screens.size(); ++i) {
        GLuint tex = m_captureTextures[i]->update();
        if (tex != 0) {
            m_screens[i]->setTexture(tex);
        }
    }
}

HitResult App::raycastAtMouse(float mx, float my) {
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    if (width <= 0 || height <= 0) return {};

    const auto& cam = m_renderer.camera();
    glm::mat4 invView = glm::inverse(cam.viewMatrix());
    glm::mat4 invProj = glm::inverse(cam.projectionMatrix());

    Ray ray = Raycaster::screenToWorldRay(mx, my, width, height, invView, invProj);
    return Raycaster::pickScreen(ray, m_screens);
}

void App::startDrag(float mx, float my) {
    if (m_hoveredScreen < 0) return;

    auto& screen = m_screens[m_hoveredScreen];

    // Don't drag pinned screens
    if (screen->pinned()) {
        Log::info("Screen {} is pinned (press P to unpin)", m_hoveredScreen + 1);
        return;
    }

    // Select the screen
    if (m_selectedScreen >= 0 && m_selectedScreen < static_cast<int>(m_screens.size())) {
        m_screens[m_selectedScreen]->setSelected(false);
    }
    m_selectedScreen = m_hoveredScreen;
    m_screens[m_selectedScreen]->setSelected(true);

    // Begin drag
    const auto& screenPos = screen->position();
    glm::vec3 camPos = m_renderer.camera().position();
    m_dragDepth = glm::length(screenPos - camPos);
    m_dragging = true;
    m_lastMousePos = glm::vec2(mx, my);
}

void App::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (app && width > 0 && height > 0) {
        app->m_renderer.onResize(width, height);
    }
}

void App::keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (!app) return;

    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;
            case GLFW_KEY_TAB:
                // Cycle selected screen
                if (!app->m_screens.empty()) {
                    app->m_screens[app->m_selectedScreen]->setSelected(false);
                    app->m_selectedScreen = (app->m_selectedScreen + 1) % static_cast<int>(app->m_screens.size());
                    app->m_screens[app->m_selectedScreen]->setSelected(true);
                    // Re-apply layout (Stack/Single need to update for new selection)
                    app->m_layoutManager.apply(app->m_screens, app->m_config, app->m_selectedScreen);
                }
                break;
            case GLFW_KEY_L:
                // Cycle layout
                app->m_layoutManager.cycleLayout();
                app->m_layoutManager.apply(app->m_screens, app->m_config, app->m_selectedScreen);
                Log::info("Layout: {}", app->m_layoutManager.layoutName());
                break;
            case GLFW_KEY_R:
                // Reset camera
                app->m_renderer.camera().setYaw(0.0f);
                app->m_renderer.camera().setPitch(0.0f);
                break;
            case GLFW_KEY_W:
                // Refresh window list
                app->refreshWindowList();
                break;
            case GLFW_KEY_H:
                // Toggle head tracking
                if (app->m_headTrackingEnabled) {
                    app->m_headTrackingEnabled = false;
                    Log::info("Head tracking disabled — using keyboard control");
                } else {
                    // Try to (re)start if not active
                    if (!app->m_headTracker) {
                        app->m_headTracker = std::make_unique<HeadTracker>();
                    }
                    if (!app->m_headTracker->isActive()) {
                        app->m_headTracker->stop();
                        if (!app->m_headTracker->start()) {
                            Log::warn("No XREAL device found — cannot enable head tracking");
                            break;
                        }
                    }
                    app->m_headTrackingEnabled = true;
                    Log::info("Head tracking enabled");
                }
                break;
            case GLFW_KEY_F:
                // Toggle fullscreen on XREAL display (or primary if no XREAL)
                if (app->m_xrealDisplay) {
                    WindowPositioner::toggle(window, *app->m_xrealDisplay);
                } else if (!app->m_displays.empty()) {
                    // Fall back to primary display
                    for (const auto& d : app->m_displays) {
                        if (d.isPrimary) {
                            WindowPositioner::toggle(window, d);
                            break;
                        }
                    }
                }
                break;
            case GLFW_KEY_S:
                // Save current config
                ConfigFile::save(app->m_config);
                break;
            case GLFW_KEY_D:
                // Refresh display list
                app->m_displays = DisplayDetector::enumerate();
                DisplayDetector::printList(app->m_displays);
                app->m_xrealDisplay = DisplayDetector::findXreal();
                if (app->m_xrealDisplay) {
                    Log::info("XREAL display found: {}", app->m_xrealDisplay->monitorName);
                } else {
                    Log::info("No XREAL display detected");
                }
                break;
            case GLFW_KEY_P:
                // Toggle pin on selected screen
                if (app->m_selectedScreen >= 0 &&
                    app->m_selectedScreen < static_cast<int>(app->m_screens.size())) {
                    auto& scr = app->m_screens[app->m_selectedScreen];
                    scr->togglePinned();
                    Log::info("Screen {} {}", app->m_selectedScreen + 1,
                              scr->pinned() ? "pinned" : "unpinned");
                }
                break;
            // Number keys 1-9: assign window to selected screen
            case GLFW_KEY_1: case GLFW_KEY_2: case GLFW_KEY_3:
            case GLFW_KEY_4: case GLFW_KEY_5: case GLFW_KEY_6:
            case GLFW_KEY_7: case GLFW_KEY_8: case GLFW_KEY_9:
                app->assignWindowToScreen(key - GLFW_KEY_1);
                break;
        }
    }
}

void App::updateHover(float mouseX, float mouseY) {
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    if (width <= 0 || height <= 0) return;

    const auto& cam = m_renderer.camera();
    glm::mat4 invView = glm::inverse(cam.viewMatrix());
    glm::mat4 invProj = glm::inverse(cam.projectionMatrix());

    Ray ray = Raycaster::screenToWorldRay(mouseX, mouseY, width, height, invView, invProj);
    HitResult hit = Raycaster::pickScreen(ray, m_screens);

    // Clear previous hover
    if (m_hoveredScreen >= 0 && m_hoveredScreen < static_cast<int>(m_screens.size())) {
        m_screens[m_hoveredScreen]->setHovered(false);
    }

    m_hoveredScreen = hit.hit ? hit.screenIndex : -1;

    if (m_hoveredScreen >= 0) {
        m_screens[m_hoveredScreen]->setHovered(true);
    }
}

void App::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (!app) return;

    double dMx, dMy;
    glfwGetCursorPos(window, &dMx, &dMy);
    float mx = static_cast<float>(dMx);
    float my = static_cast<float>(dMy);

    // --- Middle mouse button: always drag ---
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            app->startDrag(mx, my);
        } else if (action == GLFW_RELEASE) {
            app->m_dragging = false;
        }
        return;
    }

    // --- Left mouse button ---
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        bool ctrlHeld = (mods & GLFW_MOD_CONTROL) != 0;

        if (action == GLFW_PRESS) {
            if (ctrlHeld) {
                // Ctrl + Left-click: drag the frame
                app->startDrag(mx, my);
            } else if (app->m_hoveredScreen >= 0) {
                // Plain left-click on a screen: select + inject click into captured window
                if (app->m_selectedScreen >= 0 &&
                    app->m_selectedScreen < static_cast<int>(app->m_screens.size())) {
                    app->m_screens[app->m_selectedScreen]->setSelected(false);
                }
                app->m_selectedScreen = app->m_hoveredScreen;
                app->m_screens[app->m_selectedScreen]->setSelected(true);

                // Click injection: forward to captured window
                int idx = app->m_hoveredScreen;
                if (idx >= 0 && idx < static_cast<int>(app->m_captureTextures.size())) {
                    auto* src = app->m_captureTextures[idx]->source();
                    if (src) {
                        auto* wcap = dynamic_cast<WindowCapturer*>(src);
                        if (wcap && IsWindow(wcap->hwnd())) {
                            HitResult hit = app->raycastAtMouse(mx, my);
                            if (hit.hit) {
                                InputInjector::sendLeftDown(wcap->hwnd(), hit.uv.x, hit.uv.y);
                                app->m_clickingIntoWindow = true;
                                app->m_clickTargetScreen = idx;
                            }
                        }
                    }
                }
            }
        } else if (action == GLFW_RELEASE) {
            // Release drag
            app->m_dragging = false;

            // Release click injection
            if (app->m_clickingIntoWindow && app->m_clickTargetScreen >= 0 &&
                app->m_clickTargetScreen < static_cast<int>(app->m_captureTextures.size())) {
                auto* src = app->m_captureTextures[app->m_clickTargetScreen]->source();
                if (src) {
                    auto* wcap = dynamic_cast<WindowCapturer*>(src);
                    if (wcap && IsWindow(wcap->hwnd())) {
                        HitResult hit = app->raycastAtMouse(mx, my);
                        if (hit.hit) {
                            InputInjector::sendLeftUp(wcap->hwnd(), hit.uv.x, hit.uv.y);
                        }
                    }
                }
            }
            app->m_clickingIntoWindow = false;
            app->m_clickTargetScreen = -1;
        }
        return;
    }

    // --- Right mouse button: right-click injection ---
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS && app->m_hoveredScreen >= 0) {
            int idx = app->m_hoveredScreen;
            if (idx >= 0 && idx < static_cast<int>(app->m_captureTextures.size())) {
                auto* src = app->m_captureTextures[idx]->source();
                if (src) {
                    auto* wcap = dynamic_cast<WindowCapturer*>(src);
                    if (wcap && IsWindow(wcap->hwnd())) {
                        HitResult hit = app->raycastAtMouse(mx, my);
                        if (hit.hit) {
                            InputInjector::sendRightClick(wcap->hwnd(), hit.uv.x, hit.uv.y);
                        }
                    }
                }
            }
        }
    }
}

void App::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (!app) return;

    float mx = static_cast<float>(xpos);
    float my = static_cast<float>(ypos);

    if (app->m_dragging && app->m_selectedScreen >= 0 &&
        app->m_selectedScreen < static_cast<int>(app->m_screens.size())) {
        // Compute world-space positions for old and new mouse positions
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        if (width <= 0 || height <= 0) return;

        const auto& cam = app->m_renderer.camera();
        glm::mat4 invView = glm::inverse(cam.viewMatrix());
        glm::mat4 invProj = glm::inverse(cam.projectionMatrix());

        Ray rayOld = Raycaster::screenToWorldRay(
            app->m_lastMousePos.x, app->m_lastMousePos.y, width, height, invView, invProj);
        Ray rayNew = Raycaster::screenToWorldRay(mx, my, width, height, invView, invProj);

        // Intersect both rays with the drag plane at m_dragDepth from camera
        glm::vec3 camFwd = glm::normalize(glm::vec3(invView[2]));
        camFwd = -camFwd;
        glm::vec3 planePoint = cam.position() + camFwd * app->m_dragDepth;
        float planeD = glm::dot(camFwd, planePoint);

        auto intersectPlane = [&](const Ray& r) -> glm::vec3 {
            float denom = glm::dot(camFwd, r.direction);
            if (std::abs(denom) < 1e-6f) return planePoint;
            float t = (planeD - glm::dot(camFwd, r.origin)) / denom;
            return r.origin + t * r.direction;
        };

        glm::vec3 worldOld = intersectPlane(rayOld);
        glm::vec3 worldNew = intersectPlane(rayNew);
        glm::vec3 delta = worldNew - worldOld;

        auto& screen = app->m_screens[app->m_selectedScreen];
        screen->setPosition(screen->position() + delta);

        app->m_lastMousePos = glm::vec2(mx, my);
    } else if (app->m_clickingIntoWindow && app->m_clickTargetScreen >= 0 &&
               app->m_clickTargetScreen < static_cast<int>(app->m_captureTextures.size())) {
        // Forward mouse move to captured window during click-hold
        auto* src = app->m_captureTextures[app->m_clickTargetScreen]->source();
        if (src) {
            auto* wcap = dynamic_cast<WindowCapturer*>(src);
            if (wcap && IsWindow(wcap->hwnd())) {
                HitResult hit = app->raycastAtMouse(mx, my);
                if (hit.hit) {
                    InputInjector::sendMouseMove(wcap->hwnd(), hit.uv.x, hit.uv.y);
                }
            }
        }
        app->updateHover(mx, my);
    } else {
        app->updateHover(mx, my);
    }
}

void App::scrollCallback(GLFWwindow* window, double /*xoffset*/, double yoffset) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (!app) return;

    bool ctrlHeld = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                    glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

    // Ctrl + Scroll: zoom (change screen distance)
    // Plain Scroll over a captured window: forward scroll to the window
    if (ctrlHeld) {
        app->m_config.screenDistance -= static_cast<float>(yoffset) * app->m_config.scrollSpeed;
        app->m_config.screenDistance = glm::clamp(
            app->m_config.screenDistance, app->m_config.minDistance, app->m_config.maxDistance);
        app->m_layoutManager.apply(app->m_screens, app->m_config, app->m_selectedScreen);
    } else if (app->m_hoveredScreen >= 0 &&
               app->m_hoveredScreen < static_cast<int>(app->m_captureTextures.size())) {
        // Forward scroll to captured window
        auto* src = app->m_captureTextures[app->m_hoveredScreen]->source();
        if (src) {
            auto* wcap = dynamic_cast<WindowCapturer*>(src);
            if (wcap && IsWindow(wcap->hwnd())) {
                double mx, my;
                glfwGetCursorPos(window, &mx, &my);
                HitResult hit = app->raycastAtMouse(static_cast<float>(mx), static_cast<float>(my));
                if (hit.hit) {
                    InputInjector::sendScroll(wcap->hwnd(), hit.uv.x, hit.uv.y,
                                             static_cast<float>(yoffset));
                }
            }
        }
    } else {
        // No captured window hovered: zoom as before
        app->m_config.screenDistance -= static_cast<float>(yoffset) * app->m_config.scrollSpeed;
        app->m_config.screenDistance = glm::clamp(
            app->m_config.screenDistance, app->m_config.minDistance, app->m_config.maxDistance);
        app->m_layoutManager.apply(app->m_screens, app->m_config, app->m_selectedScreen);
    }
}

} // namespace xr
