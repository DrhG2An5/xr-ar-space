#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace xr {

class VirtualScreen {
public:
    VirtualScreen();
    ~VirtualScreen();

    VirtualScreen(const VirtualScreen&) = delete;
    VirtualScreen& operator=(const VirtualScreen&) = delete;
    VirtualScreen(VirtualScreen&& other) noexcept;
    VirtualScreen& operator=(VirtualScreen&& other) noexcept;

    void init(float width, float height);
    void draw() const;

    void setTexture(GLuint texture) { m_texture = texture; }
    GLuint texture() const { return m_texture; }

    void setPosition(const glm::vec3& pos) { m_position = pos; }
    const glm::vec3& position() const { return m_position; }

    void setRotation(const glm::vec3& rot) { m_rotation = rot; }
    const glm::vec3& rotation() const { return m_rotation; }

    void setScale(const glm::vec3& scale) { m_scale = scale; }

    void setSelected(bool sel) { m_selected = sel; }
    bool selected() const { return m_selected; }

    void setOpacity(float opacity) { m_opacity = opacity; }
    float opacity() const { return m_opacity; }

    glm::mat4 modelMatrix() const;

    float width() const { return m_width; }
    float height() const { return m_height; }

private:
    void cleanup();

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    GLuint m_texture = 0;

    float m_width = 1.6f;
    float m_height = 0.9f;

    glm::vec3 m_position{0.0f};
    glm::vec3 m_rotation{0.0f};
    glm::vec3 m_scale{1.0f};

    bool m_selected = false;
    float m_opacity = 1.0f;
};

} // namespace xr
