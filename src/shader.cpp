#include "shader.hpp"
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

Shader::Shader(const char* vertexSource, const char* fragmentSource) {
    unsigned int sVertex = compileShader(GL_VERTEX_SHADER, vertexSource, "VERTEX"); // Compile vertex shader
    unsigned int sFragment = compileShader(GL_FRAGMENT_SHADER, fragmentSource, "FRAGMENT"); // Compile fragment shader

    ID = glCreateProgram(); // Create shader program
    glAttachShader(ID, sVertex); // Attach shaders
    glAttachShader(ID, sFragment);
    glLinkProgram(ID); // Link program

    // Check for linking errors
    int success;
    char infoLog[1024];
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ID, 1024, NULL, infoLog);
        spdlog::error("PROGRAM_LINKING_ERROR: {}", infoLog);
    }

    glDeleteShader(sVertex); // Delete individual shaders after linking
    glDeleteShader(sFragment);
}

unsigned int Shader::compileShader(unsigned int type, const char* source, const std::string& typeName) const {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Check for compilation errors
    int success;
    char infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        spdlog::error("SHADER_COMPILATION_ERROR of type {}: {}", typeName, infoLog);
    }
    return shader;
}

Shader::~Shader() {
    glDeleteProgram(ID);
}
void Shader::use() const {
    glUseProgram(ID);
}

int Shader::getUniformLocation(const std::string& name) const {
    if (m_uniformCache.contains(name)) {
        return m_uniformCache[name];
    }
    int location = glGetUniformLocation(ID, name.c_str());
    m_uniformCache[name] = location;
    return location;
}

void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(getUniformLocation(name), static_cast<int>(value));
}
void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(getUniformLocation(name), value);
}
void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(getUniformLocation(name), value);
}
void Shader::setVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(value));
}
void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
}
void Shader::setMat4(const std::string& name, const glm::mat4& value) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}