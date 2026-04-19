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

    std::map<char, Character> characters;
    unsigned int textVAO, textVBO;
    unsigned int shaderProgram, borderShaderProgram, textShaderProgram;
    unsigned int VAO, VBO, instanceVBO, borderVAO, borderVBO, pauseVAO, pauseVBO;
    
    // Uniform locations
    unsigned int gridCountLoc, colorLoc, isAppleLoc, isBananaLoc, isGoldenAppleLoc, isSnakeLoc, snakeDirectionLoc, borderColorLoc;
    unsigned int textColorLoc, textProjLoc;

    unsigned int screenWidth;
    unsigned int screenHeight;
    glm::mat4 projection;
};
