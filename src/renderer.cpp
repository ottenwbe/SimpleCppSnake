#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>
#include <vector>
#include <algorithm>

#include "renderer.hpp"
#include "shaders.hpp"

Renderer::Renderer() : m_screenWidth(0), m_screenHeight(0) {}

Renderer::~Renderer() {
    glDeleteVertexArrays(1, &m_textVAO); glDeleteBuffers(1, &m_textVBO);
    glDeleteVertexArrays(1, &m_VAO); glDeleteBuffers(1, &m_VBO); glDeleteBuffers(1, &m_instanceVBO);
    glDeleteVertexArrays(1, &m_borderVAO); glDeleteBuffers(1, &m_borderVBO);
    glDeleteVertexArrays(1, &m_pauseVAO); glDeleteBuffers(1, &m_pauseVBO);
    glDeleteProgram(m_shaderProgram); glDeleteProgram(m_borderShaderProgram); glDeleteProgram(m_textShaderProgram);
    for (auto const& [key, val] : m_characters) glDeleteTextures(1, &val.TextureID);
}

bool Renderer::init(unsigned int width, unsigned int height) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_shaderProgram = createShaderProgram(Shaders::vertexShaderSource, Shaders::fragmentShaderSource);
    m_borderShaderProgram = createShaderProgram(Shaders::borderVertexShaderSource, Shaders::borderFragmentShaderSource);
    m_textShaderProgram = createShaderProgram(Shaders::textVertexShaderSource, Shaders::textFragmentShaderSource);

    m_gridCountLoc = glGetUniformLocation(m_shaderProgram, "gridCount");
    m_colorLoc = glGetUniformLocation(m_shaderProgram, "objectColor");
    m_isAppleLoc = glGetUniformLocation(m_shaderProgram, "isApple");
    m_isBananaLoc = glGetUniformLocation(m_shaderProgram, "isBanana");
    m_isGoldenAppleLoc = glGetUniformLocation(m_shaderProgram, "isGoldenApple");
    m_isSnakeLoc = glGetUniformLocation(m_shaderProgram, "isSnake");
    m_snakeDirectionLoc = glGetUniformLocation(m_shaderProgram, "snakeDirection");
    m_timeLeftLoc = glGetUniformLocation(m_shaderProgram, "timeLeft");
    m_borderColorLoc = glGetUniformLocation(m_borderShaderProgram, "borderColor");
    m_borderTransformLoc = glGetUniformLocation(m_borderShaderProgram, "transform");
    m_textColorLoc = glGetUniformLocation(m_textShaderProgram, "textColor");
    m_textProjLoc = glGetUniformLocation(m_textShaderProgram, "projection");

    setupBuffers();
    setupFreetype();
    updateProjection(width, height);
    return true;
}

void Renderer::updateProjection(int width, int height) {
    m_screenWidth = width;
    m_screenHeight = height;
    m_projection = glm::ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height));
}

void Renderer::render(GameState& state, int winW, int winH) {
    float targetRatio = (float)GRID_WIDTH / (float)GRID_HEIGHT;
    float windowRatio = (float)winW / (float)winH;
    int vpX, vpY, vpW, vpH;

    if (windowRatio > targetRatio) {
        vpH = winH; vpW = static_cast<int>(winH * targetRatio);
        vpX = (winW - vpW) / 2; vpY = 0;
    } else {
        vpW = winW; vpH = static_cast<int>(vpW / targetRatio);
        vpX = 0; vpY = (winH - vpH) / 2;
    }

    glViewport(0, 0, winW, winH);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (state.isInMenu()) {
        renderMenu(state);
    } else {
        renderGameWorld(state, vpX, vpY, vpW, vpH);
        renderUI(state, winW, winH);
    }
}

void Renderer::renderMenu(GameState& state) {
    float boxW = 1000.0f, boxH = 800.0f;
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(m_screenWidth / 2.0f, m_screenHeight / 2.0f, 0.0f));
    model = glm::scale(model, glm::vec3(boxW / 2.0f, boxH / 2.0f, 1.0f));
    
    glUseProgram(m_borderShaderProgram);
    glUniformMatrix4fv(m_borderTransformLoc, 1, GL_FALSE, glm::value_ptr(m_projection * model));
    glUniform3f(m_borderColorLoc, 0.0f, 0.8f, 0.2f);
    glBindVertexArray(m_borderVAO);
    glLineWidth(3.0f);
    glDrawArrays(GL_LINE_LOOP, 0, 4);

    if (state.isShowingRules()) {
        renderTextCentered("GAME RULES", m_screenHeight / 2.0f + 200.0f, 1.0f, glm::vec3(0.0, 0.8, 0.2));
        renderTextCentered("1. Eat apples to grow and score points.", m_screenHeight / 2.0f + 120.0f, 0.5f, glm::vec3(1.0));
        renderTextCentered("2. Avoid hitting walls, yourself, or obstacles.", m_screenHeight / 2.0f + 90.0f, 0.5f, glm::vec3(1.0));
        renderTextCentered("3. Bananas (+2) and Golden Apples (+5) are timed!", m_screenHeight / 2.0f + 60.0f, 0.5f, glm::vec3(1.0));
        renderTextCentered("4. The faster you eat, the faster the game gets.", m_screenHeight / 2.0f + 30.0f, 0.5f, glm::vec3(1.0));
        renderTextCentered("5. AI Auto-Pilot can help, but it's not perfect.", m_screenHeight / 2.0f + 0.0f, 0.5f, glm::vec3(1.0));
        renderTextCentered("Press ESC to return to Menu", m_screenHeight / 2.0f - 100.0f, 0.6f, glm::vec3(0.7));
    } else if (state.isShowingHighScores()) {
        renderTextCentered("TOP 5 HIGH SCORES", m_screenHeight / 2.0f + 200.0f, 1.0f, glm::vec3(1.0, 0.9, 0.3));
        const auto& entries = state.getHighScoreManager().getEntries();
        if (entries.empty()) {
            renderTextCentered("No scores yet. Go play!", m_screenHeight / 2.0f + 50.0f, 0.6f, glm::vec3(0.7));
        } else {
            for (size_t i = 0; i < entries.size(); ++i) {
                std::string hs = std::to_string(i + 1) + ". " + std::to_string(entries[i].score) + 
                                 (entries[i].aiUsed ? " (AI)" : " (No AI)") + (entries[i].obstaclesEnabled ? " (Obs)" : " (No Obs)");
                renderTextCentered(hs, m_screenHeight / 2.0f + 110.0f - i * 40.0f, 0.6f, glm::vec3(1.0, 0.9, 0.3));
            }
        }
        renderTextCentered("Press ESC to return to Menu", m_screenHeight / 2.0f - 150.0f, 0.6f, glm::vec3(0.7));
    } else if (state.isShowingControls()) {
        renderTextCentered("GAME CONTROLS", m_screenHeight / 2.0f + 200.0f, 1.0f, glm::vec3(0.0, 0.8, 0.2));
        renderTextCentered("Arrow Keys: Move Snake / Navigate Menu", m_screenHeight / 2.0f + 120.0f, 0.5f, glm::vec3(1.0));
        renderTextCentered("Space / Enter: Select / Pause Game", m_screenHeight / 2.0f + 80.0f, 0.5f, glm::vec3(1.0));
        renderTextCentered("Esc: Go Back / Quit", m_screenHeight / 2.0f + 40.0f, 0.5f, glm::vec3(1.0));
        renderTextCentered("F11: Toggle Fullscreen", m_screenHeight / 2.0f + 0.0f, 0.5f, glm::vec3(1.0));
        renderTextCentered("I: Toggle AI Auto-Pilot", m_screenHeight / 2.0f - 40.0f, 0.5f, glm::vec3(1.0));
        renderTextCentered("Press ESC to return to Menu", m_screenHeight / 2.0f - 150.0f, 0.6f, glm::vec3(0.7));
    } else {
        renderTextCentered("CPP SIMPLE SNAKE", m_screenHeight / 2.0f + 200.0f, 1.2f, glm::vec3(0.0, 0.8, 0.2));
        glm::vec3 startColor = (state.getMenuSelection() == 0) ? glm::vec3(1.0, 1.0, 0.0) : glm::vec3(1.0, 1.0, 1.0);
        renderTextCentered("Start Game", m_screenHeight / 2.0f - 40.0f, 1.2f, startColor);
        glm::vec3 obsColor = (state.getMenuSelection() == 1) ? glm::vec3(1.0, 1.0, 0.0) : glm::vec3(1.0, 1.0, 1.0);
        renderTextCentered("Obstacle Spawning: " + std::string(state.areObstaclesEnabled() ? "ON" : "OFF"), m_screenHeight / 2.0f - 90.0f, 0.6f, obsColor);
        glm::vec3 aiColor = (state.getMenuSelection() == 2) ? glm::vec3(1.0, 1.0, 0.0) : glm::vec3(1.0, 1.0, 1.0);
        renderTextCentered("AI Auto-Pilot: " + std::string(state.isAIEnabled() ? "ON" : "OFF"), m_screenHeight / 2.0f - 130.0f, 0.6f, aiColor);
        glm::vec3 rulesColor = (state.getMenuSelection() == 3) ? glm::vec3(1.0, 1.0, 0.0) : glm::vec3(1.0, 1.0, 1.0);
        renderTextCentered("Rules", m_screenHeight / 2.0f - 175.0f, 0.8f, rulesColor);
        glm::vec3 hsBtnColor = (state.getMenuSelection() == 4) ? glm::vec3(1.0, 1.0, 0.0) : glm::vec3(1.0, 1.0, 1.0);
        renderTextCentered("High Scores", m_screenHeight / 2.0f - 220.0f, 0.8f, hsBtnColor);
        glm::vec3 ctrlBtnColor = (state.getMenuSelection() == 5) ? glm::vec3(1.0, 1.0, 0.0) : glm::vec3(1.0, 1.0, 1.0);
        renderTextCentered("Controls", m_screenHeight / 2.0f - 265.0f, 0.8f, ctrlBtnColor);
        glm::vec3 quitBtnColor = (state.getMenuSelection() == 6) ? glm::vec3(1.0, 1.0, 0.0) : glm::vec3(1.0, 1.0, 1.0);
        renderTextCentered("Quit Game", m_screenHeight / 2.0f - 310.0f, 0.8f, quitBtnColor);
    }
}

void Renderer::renderGameWorld(GameState& state, int vpX, int vpY, int vpW, int vpH) {
    glViewport(vpX, vpY, vpW, vpH);
    glUseProgram(m_borderShaderProgram);
    glUniformMatrix4fv(m_borderTransformLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    glBindVertexArray(m_borderVAO);
    glUniform3f(m_borderColorLoc, 0.5f, 0.5f, 0.5f);
    glLineWidth(5.0f);
    glDrawArrays(GL_LINE_LOOP, 0, 4);

    glUseProgram(m_shaderProgram);
    glBindVertexArray(m_VAO);
    glUniform2f(m_gridCountLoc, (float)GRID_WIDTH, (float)GRID_HEIGHT);
    glUniform1f(m_timeLeftLoc, 1.0f);

    // Snake
    glm::vec3 sColor = state.isGameOver() ? glm::vec3(0.5f, 0.0f, 0.0f) : glm::vec3(0.0f, 0.8f, 0.2f);
    glUniform3fv(m_colorLoc, 1, glm::value_ptr(sColor));
    glUniform1i(m_isAppleLoc, 0); glUniform1i(m_isBananaLoc, 0); glUniform1i(m_isGoldenAppleLoc, 0); glUniform1i(m_isSnakeLoc, 1);
    glUniform1i(m_snakeDirectionLoc, static_cast<int>(state.getDirection()));
    const auto& snake = state.getSnake();
    std::vector<glm::vec2> snakeData(snake.begin(), snake.end());
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, snakeData.size() * sizeof(glm::vec2), snakeData.data());
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<unsigned int>(snakeData.size()));
    glUniform1i(m_isSnakeLoc, 0);

    // Food
    glUniform3f(m_colorLoc, 1.0f, 0.2f, 0.2f); glUniform1i(m_isAppleLoc, 1);
    glm::vec2 fPos = state.getFood();
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2), glm::value_ptr(fPos));
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1); glUniform1i(m_isAppleLoc, 0);

    // Obstacles
    const auto& obstacles = state.getObstacles();
    if (!obstacles.empty()) {
        glUniform3f(m_colorLoc, 0.5f, 0.5f, 0.5f);
        glBufferSubData(GL_ARRAY_BUFFER, 0, obstacles.size() * sizeof(glm::vec2), obstacles.data());
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<unsigned int>(obstacles.size()));
    }

    // Collectibles
    const auto& banana = state.getBanana();
    if (banana.active) {
        glUniform3f(m_colorLoc, 1.0f, 0.9f, 0.1f); glUniform1i(m_isBananaLoc, 1);
        glUniform1f(m_timeLeftLoc, std::max(0.0f, banana.timeLeft / 5.0f));
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2), glm::value_ptr(banana.pos));
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1); glUniform1i(m_isBananaLoc, 0);
    }
    const auto& gApple = state.getGoldenApple();
    if (gApple.active) {
        glUniform3f(m_colorLoc, 1.0f, 0.85f, 0.0f); glUniform1i(m_isGoldenAppleLoc, 1);
        glUniform1f(m_timeLeftLoc, std::max(0.0f, gApple.timeLeft / 7.0f));
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2), glm::value_ptr(gApple.pos));
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1); glUniform1i(m_isGoldenAppleLoc, 0);
    }
}

void Renderer::renderUI(GameState& state, int winW, int winH) {
    glViewport(0, 0, winW, winH);
    renderText("Points: " + std::to_string(state.getScore()), 20.0f, m_screenHeight - 40.0f, 0.5f, glm::vec3(1.0f));
    
    if (state.isAIEnabled() && !state.isGameOver())
        renderText("AI AUTO-PILOT ENABLED [I]", m_screenWidth - 300.0f, m_screenHeight - 40.0f, 0.4f, glm::vec3(0.0f, 1.0f, 1.0f));

    if (state.isPaused()) {
        glUseProgram(m_borderShaderProgram); glBindVertexArray(m_pauseVAO);
        glUniformMatrix4fv(m_borderTransformLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glUniform3f(m_borderColorLoc, 1.0f, 1.0f, 1.0f); glDrawArrays(GL_TRIANGLES, 0, 12);
        renderTextCentered("PAUSED", m_screenHeight / 2.0f + 120.0f, 1.0f, glm::vec3(1.0));
        renderTextCentered("Press SPACE to Resume", m_screenHeight / 2.0f - 150.0f, 0.6f, glm::vec3(0.8));
    }

    if (state.isGameOver()) {
        renderTextCentered("GAME OVER", m_screenHeight / 2.0f + 100.0f, 1.0f, glm::vec3(1.0, 0.2, 0.2));
        if (state.isNewTopScore()) renderTextCentered("NEW TOP 5 SCORE!", m_screenHeight / 2.0f + 50.0f, 0.7f, glm::vec3(1.0, 0.8, 0.0));
        renderTextCentered("SCORE: " + std::to_string(state.getScore()), m_screenHeight / 2.0f, 0.8f, glm::vec3(1.0));
        renderTextCentered("Press Space to Restart", m_screenHeight / 2.0f - 60.0f, 0.7f, glm::vec3(0.8));
        renderTextCentered("Press ESC to Menu", m_screenHeight / 2.0f - 100.0f, 0.6f, glm::vec3(0.6));
    }
}

float Renderer::getTextWidth(const std::string& text, float scale) {
    float width = 0.0f;
    for (char c : text) {
        auto it = m_characters.find(c);
        if (it != m_characters.end()) {
            width += (it->second.Advance >> 6) * scale;
        }
    }
    return width;
}

void Renderer::renderTextCentered(const std::string& text, float y, float scale, glm::vec3 color) {
    float x = (m_screenWidth - getTextWidth(text, scale)) / 2.0f;
    renderText(text, x, y, scale, color);
}

void Renderer::renderText(const std::string& text, float x, float y, float scale, glm::vec3 color) {
    glUseProgram(m_textShaderProgram);
    glUniform3f(m_textColorLoc, color.x, color.y, color.z);
    glUniformMatrix4fv(m_textProjLoc, 1, GL_FALSE, glm::value_ptr(m_projection));
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(m_textVAO);

    for (auto c = text.begin(); c != text.end(); c++) {
        auto it = m_characters.find(*c);
        if (it == m_characters.end()) continue;
        Character ch = it->second;
        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
        float w = ch.Size.x * scale, h = ch.Size.y * scale;
        float vertices[6][4] = {
            { xpos, ypos + h, 0.0f, 0.0f }, { xpos, ypos, 0.0f, 1.0f }, { xpos + w, ypos, 1.0f, 1.0f },
            { xpos, ypos + h, 0.0f, 0.0f }, { xpos + w, ypos, 1.0f, 1.0f }, { xpos + w, ypos + h, 1.0f, 0.0f }
        };
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, m_textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (ch.Advance >> 6) * scale;
    }
    glBindVertexArray(0); glBindTexture(GL_TEXTURE_2D, 0);
}

unsigned int Renderer::createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    auto compile = [](unsigned int type, const char* source) {
        unsigned int shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, NULL);
        glCompileShader(shader);
        int success; glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char info[512]; glGetShaderInfoLog(shader, 512, NULL, info);
            spdlog::error("Shader Compilation Failed ({}): {}", (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment"), info);
        }
        return shader;
    };

    unsigned int vs = compile(GL_VERTEX_SHADER, vertexSource);
    unsigned int fs = compile(GL_FRAGMENT_SHADER, fragmentSource);
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    int success; glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512]; glGetProgramInfoLog(prog, 512, NULL, info);
        spdlog::error("Shader Program Linking Failed: {}", info);
    }

    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

void Renderer::setupBuffers() {
    float sqVerts[] = { 0,1, 1,0, 0,0, 0,1, 1,1, 1,0 };
    glGenVertexArrays(1, &m_VAO); glGenBuffers(1, &m_VBO); glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO); glBufferData(GL_ARRAY_BUFFER, sizeof(sqVerts), sqVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glGenBuffers(1, &m_instanceVBO); glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, GRID_WIDTH * GRID_HEIGHT * sizeof(glm::vec2), NULL, GL_STREAM_DRAW);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribDivisor(1, 1);

    float bVerts[] = { -1,-1, 1,-1, 1,1, -1,1 };
    glGenVertexArrays(1, &m_borderVAO); glGenBuffers(1, &m_borderVBO); glBindVertexArray(m_borderVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_borderVBO); glBufferData(GL_ARRAY_BUFFER, sizeof(bVerts), bVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glGenVertexArrays(1, &m_textVAO); glGenBuffers(1, &m_textVBO); glBindVertexArray(m_textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_textVBO); glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    float pVerts[] = { -0.15f,0.3f, -0.15f,-0.3f, -0.05f,-0.3f, -0.15f,0.3f, -0.05f,-0.3f, -0.05f,0.3f, 0.05f,0.3f, 0.05f,-0.3f, 0.15f,-0.3f, 0.05f,0.3f, 0.15f,-0.3f, 0.15f,0.3f };
    glGenVertexArrays(1, &m_pauseVAO); glGenBuffers(1, &m_pauseVBO); glBindVertexArray(m_pauseVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_pauseVBO); glBufferData(GL_ARRAY_BUFFER, sizeof(pVerts), pVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindVertexArray(0);
}

void Renderer::setupFreetype() {
    FT_Library ft; FT_Init_FreeType(&ft);
    FT_Face face; FT_New_Face(ft, FONT_PATH, 0, &face);
    FT_Set_Pixel_Sizes(face, 0, 48);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;
        unsigned int tex; glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        m_characters[c] = { tex, glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows), glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top), static_cast<unsigned int>(face->glyph->advance.x) };
    }
    FT_Done_Face(face); FT_Done_FreeType(ft);
}
