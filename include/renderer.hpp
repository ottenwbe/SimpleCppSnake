#pragma once

#include <map>
#include <string>
#include <glm/glm.hpp>
#include "game_logic.hpp"

struct Character {
    unsigned int TextureID;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    unsigned int Advance;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool init(unsigned int width, unsigned int height);
    void render(GameState& state, float currentTime, int winW, int winH);
    void updateProjection(int width, int height);

private:
    void renderText(std::string text, float x, float y, float scale, glm::vec3 color);
    unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource);
    void setupBuffers();
    void setupFreetype();

    std::map<char, Character> m_characters;
    unsigned int m_textVAO, m_textVBO;
    unsigned int m_shaderProgram, m_borderShaderProgram, m_textShaderProgram;
    unsigned int m_VAO, m_VBO, m_instanceVBO, m_borderVAO, m_borderVBO, m_pauseVAO, m_pauseVBO;
    
    // Uniform locations
    unsigned int m_gridCountLoc, m_colorLoc, m_isAppleLoc, m_isBananaLoc, m_isGoldenAppleLoc, m_isSnakeLoc, m_snakeDirectionLoc, m_borderColorLoc;
    unsigned int m_textColorLoc, m_textProjLoc, m_timeLeftLoc;

    unsigned int m_screenWidth;
    unsigned int m_screenHeight;
    glm::mat4 m_projection;
};
