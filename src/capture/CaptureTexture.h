#pragma once

#include "capture/CaptureSource.h"

#include <glad/gl.h>
#include <memory>

namespace xr {

// Owns a GL texture and uploads BGRA frames from a CaptureSource.
// All methods must be called on the main (GL) thread.
class CaptureTexture {
public:
    CaptureTexture() = default;
    ~CaptureTexture();

    CaptureTexture(const CaptureTexture&) = delete;
    CaptureTexture& operator=(const CaptureTexture&) = delete;
    CaptureTexture(CaptureTexture&& other) noexcept;
    CaptureTexture& operator=(CaptureTexture&& other) noexcept;

    void setSource(std::unique_ptr<CaptureSource> source);
    CaptureSource* source() const { return m_source.get(); }

    // Fetch latest frame from the source and upload to GL texture.
    // Returns the GL texture ID (0 if no frame available yet).
    GLuint update();

    GLuint texture() const { return m_texture; }

private:
    void ensureTexture(int width, int height);
    void cleanup();

    std::unique_ptr<CaptureSource> m_source;
    GLuint m_texture = 0;
    int m_texWidth = 0;
    int m_texHeight = 0;
};

} // namespace xr
