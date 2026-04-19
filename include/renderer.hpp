#pragma once

#include <map>
#include <string>
#include <glm/glm.hpp>
#include "game_logic.hpp"
#include "shader.hpp"

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
    void render(GameState& state, int winW, int winH);
    void updateProjection(int width, int height);

private:
    enum class RenderType { Snake, Apple, Banana, GoldenApple, Obstacle };

    void renderMenu(GameState& state);
    void renderGameWorld(GameState& state, int vpX, int vpY, int vpW, int vpH);
    void renderUI(GameState& state, int winW, int winH);
    void drawObject(RenderType type, const glm::vec2* positions, size_t count, const glm::vec3& color, int snakeDir = 0, float timeLeft = 1.0f);

    float getTextWidth(const std::string& text, float scale);
    void renderText(const std::string& text, float x, float y, float scale, glm::vec3 color);
    void renderTextCentered(const std::string& text, float y, float scale, glm::vec3 color);
    void setupBuffers();
    void setupFreetype();

    std::map<char, Character> m_characters;
    unsigned int m_textVAO, m_textVBO;
    std::unique_ptr<Shader> m_gameShader, m_borderShader, m_textShader;
    unsigned int m_VAO, m_VBO, m_instanceVBO, m_borderVAO, m_borderVBO, m_pauseVAO, m_pauseVBO;

    unsigned int m_screenWidth;
    unsigned int m_screenHeight;
    int m_vpX = 0, m_vpY = 0, m_vpW = 0, m_vpH = 0;
    glm::mat4 m_projection;
};
