#pragma once

#include <string>

namespace xr {

struct Config {
    // Window
    int windowWidth = 1920;
    int windowHeight = 1080;
    std::string windowTitle = "XREAL AR Multiscreen";
    bool fullscreen = false;

    // Rendering
    float fovDegrees = 52.0f;  // XREAL Air 2 Pro FOV
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    float screenDistance = 2.5f;  // Default distance from camera

    // Virtual screens
    float screenWidth = 1.6f;   // 16:9 aspect, width in world units
    float screenHeight = 0.9f;

    // Camera
    float rotationSpeed = 90.0f;  // Degrees per second for keyboard rotation

    // Layout
    float arcSpanDeg = 90.0f;      // Total arc span for Arc layout
    float gridGap = 0.15f;          // Gap between screens in Grid layout
    float stackZOffset = 0.05f;     // Z-offset between stacked screens

    // Capture
    int captureFrameRate = 30;

    // Paths
    std::string shaderPath = "shaders/";
};

} // namespace xr
