#include "renderer/Camera.h"
#include "util/MathUtils.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace xr {

void Camera::setPerspective(float fovDeg, float aspect, float near, float far) {
    m_projection = glm::perspective(glm::radians(fovDeg), aspect, near, far);
}

void Camera::rotate(float dyaw, float dpitch) {
    m_yaw += dyaw;
    m_pitch += dpitch;

    // Clamp pitch to avoid gimbal lock
    m_pitch = std::clamp(m_pitch, -MathUtils::kHalfPi + 0.01f, MathUtils::kHalfPi - 0.01f);

    updateVectors();
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

void Camera::updateVectors() {
    m_front.x = std::cos(m_pitch) * std::sin(m_yaw);
    m_front.y = std::sin(m_pitch);
    m_front.z = -std::cos(m_pitch) * std::cos(m_yaw);
    m_front = glm::normalize(m_front);

    // Recalculate right and up vectors
    glm::vec3 worldUp{0.0f, 1.0f, 0.0f};
    m_right = glm::normalize(glm::cross(m_front, worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}

} // namespace xr
