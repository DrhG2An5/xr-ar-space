#include "ui/UIManager.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace xr {

bool UIManager::init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // Don't save layout to disk

    ImGui::StyleColorsDark();

    // Slightly transparent background for overlay feel
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.Colors[ImGuiCol_WindowBg].w = 0.92f;

    ImGui_ImplGlfw_InitForOpenGL(window, true); // Install GLFW callbacks
    ImGui_ImplOpenGL3_Init("#version 460");

    m_initialized = true;
    return true;
}

void UIManager::shutdown() {
    if (!m_initialized) return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}

void UIManager::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIManager::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool UIManager::wantCaptureMouse() const {
    return m_initialized && ImGui::GetIO().WantCaptureMouse;
}

bool UIManager::wantCaptureKeyboard() const {
    return m_initialized && ImGui::GetIO().WantCaptureKeyboard;
}

} // namespace xr
