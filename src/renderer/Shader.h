#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>

namespace xr {

class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    bool loadFromFiles(const std::string& vertPath, const std::string& fragPath);

    void use() const;
    GLuint id() const { return m_program; }

    void setInt(const char* name, int value) const;
    void setFloat(const char* name, float value) const;
    void setBool(const char* name, bool value) const;
    void setVec2(const char* name, const glm::vec2& v) const;
    void setVec3(const char* name, const glm::vec3& v) const;
    void setVec4(const char* name, const glm::vec4& v) const;
    void setMat4(const char* name, const glm::mat4& m) const;

private:
    GLuint m_program = 0;

    GLuint compileShader(GLenum type, const char* source);
    GLint getLocation(const char* name) const;
};

} // namespace xr
