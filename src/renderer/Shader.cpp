#include "renderer/Shader.h"
#include "util/Log.h"

#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>

namespace xr {

Shader::~Shader() {
    if (m_program) glDeleteProgram(m_program);
}

Shader::Shader(Shader&& other) noexcept : m_program(other.m_program) {
    other.m_program = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_program) glDeleteProgram(m_program);
        m_program = other.m_program;
        other.m_program = 0;
    }
    return *this;
}

bool Shader::loadFromFiles(const std::string& vertPath, const std::string& fragPath) {
    auto readFile = [](const std::string& path) -> std::string {
        std::ifstream file(path);
        if (!file.is_open()) {
            Log::error("Failed to open shader file: {}", path);
            return {};
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    };

    std::string vertSrc = readFile(vertPath);
    std::string fragSrc = readFile(fragPath);
    if (vertSrc.empty() || fragSrc.empty()) return false;

    GLuint vert = compileShader(GL_VERTEX_SHADER, vertSrc.c_str());
    if (!vert) return false;

    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc.c_str());
    if (!frag) {
        glDeleteShader(vert);
        return false;
    }

    m_program = glCreateProgram();
    glAttachShader(m_program, vert);
    glAttachShader(m_program, frag);
    glLinkProgram(m_program);

    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint success;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(m_program, sizeof(log), nullptr, log);
        Log::error("Shader link failed: {}", log);
        glDeleteProgram(m_program);
        m_program = 0;
        return false;
    }

    Log::info("Shader loaded: {} + {}", vertPath, fragPath);
    return true;
}

void Shader::use() const {
    glUseProgram(m_program);
}

GLuint Shader::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        Log::error("Shader compile failed ({}): {}",
            type == GL_VERTEX_SHADER ? "vertex" : "fragment", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLint Shader::getLocation(const char* name) const {
    return glGetUniformLocation(m_program, name);
}

void Shader::setInt(const char* name, int value) const {
    glUniform1i(getLocation(name), value);
}

void Shader::setFloat(const char* name, float value) const {
    glUniform1f(getLocation(name), value);
}

void Shader::setBool(const char* name, bool value) const {
    glUniform1i(getLocation(name), static_cast<int>(value));
}

void Shader::setVec2(const char* name, const glm::vec2& v) const {
    glUniform2fv(getLocation(name), 1, glm::value_ptr(v));
}

void Shader::setVec3(const char* name, const glm::vec3& v) const {
    glUniform3fv(getLocation(name), 1, glm::value_ptr(v));
}

void Shader::setVec4(const char* name, const glm::vec4& v) const {
    glUniform4fv(getLocation(name), 1, glm::value_ptr(v));
}

void Shader::setMat4(const char* name, const glm::mat4& m) const {
    glUniformMatrix4fv(getLocation(name), 1, GL_FALSE, glm::value_ptr(m));
}

} // namespace xr
