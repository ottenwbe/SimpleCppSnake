#pragma once

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader {
public:
    unsigned int ID;
    Shader(const char* vertexSource, const char* fragmentSource);
    ~Shader();

    void use() const;
    
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;

private:
    int getUniformLocation(const std::string& name) const;
    mutable std::unordered_map<std::string, int> m_uniformCache;
    unsigned int compileShader(unsigned int type, const char* source, const std::string& typeName) const;
};