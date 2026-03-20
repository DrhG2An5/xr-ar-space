#include "renderer/VirtualScreen.h"

#include <cmath>
#include <vector>

namespace xr {

VirtualScreen::VirtualScreen() = default;

VirtualScreen::~VirtualScreen() {
    cleanup();
}

VirtualScreen::VirtualScreen(VirtualScreen&& other) noexcept
    : m_vao(other.m_vao), m_vbo(other.m_vbo), m_ebo(other.m_ebo),
      m_texture(other.m_texture), m_indexCount(other.m_indexCount),
      m_width(other.m_width), m_height(other.m_height),
      m_curvature(other.m_curvature), m_segments(other.m_segments),
      m_position(other.m_position), m_rotation(other.m_rotation), m_scale(other.m_scale),
      m_selected(other.m_selected), m_hovered(other.m_hovered),
      m_pinned(other.m_pinned), m_opacity(other.m_opacity) {
    other.m_vao = other.m_vbo = other.m_ebo = 0;
}

VirtualScreen& VirtualScreen::operator=(VirtualScreen&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_vao = other.m_vao; m_vbo = other.m_vbo; m_ebo = other.m_ebo;
        m_texture = other.m_texture; m_indexCount = other.m_indexCount;
        m_width = other.m_width; m_height = other.m_height;
        m_curvature = other.m_curvature; m_segments = other.m_segments;
        m_position = other.m_position; m_rotation = other.m_rotation; m_scale = other.m_scale;
        m_selected = other.m_selected; m_hovered = other.m_hovered;
        m_pinned = other.m_pinned; m_opacity = other.m_opacity;
        other.m_vao = other.m_vbo = other.m_ebo = 0;
    }
    return *this;
}

void VirtualScreen::init(float width, float height) {
    m_width = width;
    m_height = height;
    rebuildMesh();
}

void VirtualScreen::rebuildMesh() {
    cleanup();

    // Decide segment count based on curvature
    m_segments = (m_curvature > 0.001f) ? 32 : 1;

    float hw = m_width / 2.0f;
    float hh = m_height / 2.0f;

    int cols = m_segments + 1;
    int rows = 2; // top and bottom rows

    std::vector<float> vertices;
    vertices.reserve(cols * rows * 5);

    for (int r = 0; r < rows; ++r) {
        float y = (r == 0) ? -hh : hh;
        float v = (r == 0) ? 1.0f : 0.0f; // Flipped V for screen coords

        for (int c = 0; c < cols; ++c) {
            float t = static_cast<float>(c) / m_segments; // 0..1 across width
            float u = t;

            float x, z;
            if (m_curvature > 0.001f) {
                // Bend into a circular arc
                // curvature=1 means the screen subtends ~90 degrees
                float arcAngle = m_curvature * 1.5708f; // pi/2
                float radius = m_width / arcAngle;
                float angle = (t - 0.5f) * arcAngle; // centered

                x = radius * std::sin(angle);
                z = radius * (1.0f - std::cos(angle));
            } else {
                x = -hw + t * m_width;
                z = 0.0f;
            }

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(u);
            vertices.push_back(v);
        }
    }

    // Indices: two triangles per column segment
    std::vector<unsigned int> indices;
    indices.reserve(m_segments * 6);
    for (int c = 0; c < m_segments; ++c) {
        int bl = c;           // bottom-left
        int br = c + 1;       // bottom-right
        int tl = cols + c;    // top-left
        int tr = cols + c + 1;// top-right

        indices.push_back(bl);
        indices.push_back(br);
        indices.push_back(tr);
        indices.push_back(tr);
        indices.push_back(tl);
        indices.push_back(bl);
    }

    m_indexCount = static_cast<int>(indices.size());

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                 indices.data(), GL_STATIC_DRAW);

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
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void VirtualScreen::setSize(float w, float h) {
    if (w < 0.2f) w = 0.2f;
    if (h < 0.15f) h = 0.15f;
    if (w == m_width && h == m_height) return;
    m_width = w;
    m_height = h;
    rebuildMesh();
}

void VirtualScreen::resize(float dw, float dh) {
    setSize(m_width + dw, m_height + dh);
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
