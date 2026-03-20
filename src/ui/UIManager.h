#pragma once

#include <GLFW/glfw3.h>

namespace xr {

class UIManager {
public:
    bool init(GLFWwindow* window);
    void shutdown();

    void beginFrame();
    void endFrame();

    // Returns true if ImGui wants to consume mouse/keyboard input
    bool wantCaptureMouse() const;
    bool wantCaptureKeyboard() const;

private:
    bool m_initialized = false;
};

} // namespace xr
