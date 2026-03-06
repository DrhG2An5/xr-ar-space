#include "capture/CaptureTexture.h"
#include "util/Log.h"

namespace xr {

CaptureTexture::~CaptureTexture() {
    cleanup();
}

CaptureTexture::CaptureTexture(CaptureTexture&& other) noexcept
    : m_source(std::move(other.m_source)),
      m_texture(other.m_texture),
      m_texWidth(other.m_texWidth),
      m_texHeight(other.m_texHeight) {
    other.m_texture = 0;
    other.m_texWidth = 0;
    other.m_texHeight = 0;
}

CaptureTexture& CaptureTexture::operator=(CaptureTexture&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_source = std::move(other.m_source);
        m_texture = other.m_texture;
        m_texWidth = other.m_texWidth;
        m_texHeight = other.m_texHeight;
        other.m_texture = 0;
        other.m_texWidth = 0;
        other.m_texHeight = 0;
    }
    return *this;
}

void CaptureTexture::setSource(std::unique_ptr<CaptureSource> source) {
    // Stop old source
    if (m_source) {
        m_source->stop();
    }
    m_source = std::move(source);
}

GLuint CaptureTexture::update() {
    if (!m_source) return m_texture;

    int w = 0, h = 0;
    const uint8_t* pixels = m_source->lockFrame(w, h);
    if (!pixels) return m_texture;

    ensureTexture(w, h);

    glBindTexture(GL_TEXTURE_2D, m_texture);

    if (w != m_texWidth || h != m_texHeight) {
        // Dimensions changed — full reallocation
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                     GL_BGRA, GL_UNSIGNED_BYTE, pixels);
        m_texWidth = w;
        m_texHeight = h;
    } else {
        // Same dimensions — fast sub-image upload
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h,
                        GL_BGRA, GL_UNSIGNED_BYTE, pixels);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    m_source->unlockFrame();
    return m_texture;
}

void CaptureTexture::ensureTexture(int width, int height) {
    if (m_texture && m_texWidth == width && m_texHeight == height) return;

    if (!m_texture) {
        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void CaptureTexture::cleanup() {
    if (m_source) {
        m_source->stop();
        m_source.reset();
    }
    if (m_texture) {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }
    m_texWidth = 0;
    m_texHeight = 0;
}

} // namespace xr
