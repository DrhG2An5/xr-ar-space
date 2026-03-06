#include "renderer/VirtualScreen.h"

namespace xr {

VirtualScreen::VirtualScreen() = default;

VirtualScreen::~VirtualScreen() {
    cleanup();
}

VirtualScreen::VirtualScreen(VirtualScreen&& other) noexcept
    : m_vao(other.m_vao), m_vbo(other.m_vbo), m_ebo(other.m_ebo),
      m_texture(other.m_texture), m_width(other.m_width), m_height(other.m_height),
      m_position(other.m_position), m_rotation(other.m_rotation), m_scale(other.m_scale),
      m_selected(other.m_selected), m_opacity(other.m_opacity) {
    other.m_vao = other.m_vbo = other.m_ebo = 0;
}

VirtualScreen& VirtualScreen::operator=(VirtualScreen&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_vao = other.m_vao; m_vbo = other.m_vbo; m_ebo = other.m_ebo;
        m_texture = other.m_texture;
        m_width = other.m_width; m_height = other.m_height;
        m_position = other.m_position; m_rotation = other.m_rotation; m_scale = other.m_scale;
        m_selected = other.m_selected; m_opacity = other.m_opacity;
        other.m_vao = other.m_vbo = other.m_ebo = 0;
    }
    return *this;
}

void VirtualScreen::init(float width, float height) {
    m_width = width;
    m_height = height;

    float hw = width / 2.0f;
    float hh = height / 2.0f;

    // Position (3) + TexCoord (2)
    float vertices[] = {
        -hw, -hh, 0.0f,   0.0f, 1.0f,  // bottom-left  (flip V for screen coords)
         hw, -hh, 0.0f,   1.0f, 1.0f,  // bottom-right
         hw,  hh, 0.0f,   1.0f, 0.0f,  // top-right
        -hw,  hh, 0.0f,   0.0f, 0.0f,  // top-left
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // TexCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void VirtualScreen::draw() const {
    if (!m_vao) return;

    if (m_texture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture);
    }

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

glm::mat4 VirtualScreen::modelMatrix() const {
    glm::mat4 model{1.0f};
    model = glm::translate(model, m_position);
    model = glm::rotate(model, m_rotation.y, glm::vec3(0, 1, 0));
    model = glm::rotate(model, m_rotation.x, glm::vec3(1, 0, 0));
    model = glm::rotate(model, m_rotation.z, glm::vec3(0, 0, 1));
    model = glm::scale(model, m_scale);
    return model;
}

void VirtualScreen::cleanup() {
    if (m_ebo) { glDeleteBuffers(1, &m_ebo); m_ebo = 0; }
    if (m_vbo) { glDeleteBuffers(1, &m_vbo); m_vbo = 0; }
    if (m_vao) { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
}

} // namespace xr
