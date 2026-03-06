#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace xr {

class Camera {
public:
    Camera() = default;

    void setPerspective(float fovDeg, float aspect, float near, float far);

    void setYaw(float yaw) { m_yaw = yaw; updateVectors(); }
    void setPitch(float pitch) { m_pitch = pitch; updateVectors(); }
    void setRoll(float roll) { m_roll = roll; }

    void rotate(float dyaw, float dpitch);

    float yaw() const { return m_yaw; }
    float pitch() const { return m_pitch; }

    void setPosition(const glm::vec3& pos) { m_position = pos; }
    const glm::vec3& position() const { return m_position; }

    glm::mat4 viewMatrix() const;
    const glm::mat4& projectionMatrix() const { return m_projection; }

private:
    void updateVectors();

    glm::vec3 m_position{0.0f, 0.0f, 0.0f};
    glm::vec3 m_front{0.0f, 0.0f, -1.0f};
    glm::vec3 m_up{0.0f, 1.0f, 0.0f};
    glm::vec3 m_right{1.0f, 0.0f, 0.0f};

    float m_yaw = 0.0f;    // radians
    float m_pitch = 0.0f;  // radians
    float m_roll = 0.0f;   // radians

    glm::mat4 m_projection{1.0f};
};

} // namespace xr
