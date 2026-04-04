#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <deque>
#include <queue>
#include <random>
#include <algorithm>
#include <string>
#include <map>

// Game Settings
const int GRID_WIDTH = 30;
const int GRID_HEIGHT = 15;
unsigned int SCR_WIDTH = 1920; // Will be updated to monitor's resolution
unsigned int SCR_HEIGHT = 1080; // Will be updated to monitor's resolution

// Game State
enum class Direction { UP, DOWN, LEFT, RIGHT };

/// Holds all state information relevant to a character as loaded by FreeType
struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};

std::map<char, Character> Characters;
unsigned int textVAO, textVBO;
unsigned int textShaderProgram;

// Function Prototypes
void RenderText(unsigned int shader, std::string text, float x, float y, float scale, glm::vec3 color);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window, Direction& dir, bool& shouldReset, bool gameOver, bool& paused, bool& inMenu, bool& aiEnabled);
void generateNewFood(glm::vec2& food, const std::deque<glm::vec2>& snake, const std::vector<glm::vec2>& obstacles);
void generateObstacle(std::vector<glm::vec2>& obstacles, const std::deque<glm::vec2>& snake, const glm::vec2& food);
void resetGame(std::deque<glm::vec2>& snake, Direction& dir, glm::vec2& food, bool& gameOver, int& score, std::vector<glm::vec2>& obstacles, float& timeBetweenUpdates);
unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource);
Direction getAIDirection(const std::deque<glm::vec2>& snake, glm::vec2 food, const std::vector<glm::vec2>& obstacles, Direction currentDir);


const char *vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 gridPos;
    uniform vec2 gridCount;
    out vec2 TexCoord;
    flat out int InstanceID;
    void main()
    {
       TexCoord = aPos;
       InstanceID = gl_InstanceID;
       vec2 scale = vec2(2.0 / gridCount.x, 2.0 / gridCount.y);
       vec2 offset = (gridPos / gridCount) * 2.0 - 1.0;
       vec2 finalPos = aPos * scale + offset;
       // Scale down the playfield to fit inside the border, creating a visual frame.
       gl_Position = vec4(finalPos * 0.99, 0.0, 1.0);
    }
)glsl";

const char *fragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;
    in vec2 TexCoord;
    flat in int InstanceID;
    uniform vec3 objectColor;
    uniform bool isApple;
    uniform bool isSnake;
    uniform int snakeDirection;
    void main()
    {
       // Map texture coordinates from [0,1] to [-1,1] for easier calculations
       vec2 p = TexCoord * 2.0 - 1.0;

       if (isApple) {
           // Procedural apple shape with a dip at the top
           float appleDist = length(vec2(p.x, p.y + 0.1 - 0.2 * abs(p.x)));
           
           if (appleDist < 0.7) {
               vec3 appleColor = vec3(1.0, 0.0, 0.0);
               
               // Add a specular highlight to make it look shiny
               vec2 lightPos = vec2(-0.3, 0.3);
               float highlight = 1.0 - distance(p, lightPos);
               highlight = pow(max(highlight, 0.0), 8.0); // Sharp highlight
               
               vec3 finalColor = appleColor + vec3(1.0) * highlight * 0.8;
               FragColor = vec4(finalColor, 1.0);

           } else if (p.y > 0.4 && p.y < 0.8 && abs(p.x) < 0.05) {
               // A simple stem
               FragColor = vec4(0.0, 0.8, 0.0, 1.0);
           } else {
               discard;
           }
       } else if (isSnake) {
           // For snake, render as lit spheres
           float dist = length(p);

           if (dist > 1.0) {
               discard; // Make it a circle
           } else {
               // Fake a highlight from top-left
               vec2 lightPos = vec2(-0.5, 0.5);
               float highlight = 1.0 - distance(p, lightPos);
               highlight = pow(max(highlight, 0.0), 2.0); // Softer highlight than the apple
               
               // Add some ambient light and a diffuse falloff towards the edges
               float ambient = 0.4;
               float diffuse = 1.0 - dist;
               
               vec3 finalColor = objectColor * (ambient + diffuse * 0.6) + vec3(1.0) * highlight * 0.3;
               
               // Draw face on snake head (Instance 0)
               if (InstanceID == 0) {
                   vec2 rotP = p;
                   // Rotate texture coordinates based on direction so eyes are always "up" relative to head
                   // 0=UP, 1=DOWN, 2=LEFT, 3=RIGHT
                   if (snakeDirection == 1) rotP = -p; 
                   else if (snakeDirection == 2) rotP = vec2(p.y, -p.x);
                   else if (snakeDirection == 3) rotP = vec2(-p.y, p.x);
                   
                   vec2 eyeL = vec2(-0.35, 0.4);
                   vec2 eyeR = vec2(0.35, 0.4);
                   float eyeSize = 0.25;
                   float pupilSize = 0.1;
                   
                   if (length(rotP - eyeL) < eyeSize || length(rotP - eyeR) < eyeSize) {
                       finalColor = vec3(1.0); // Eye White
                       if (length(rotP - (eyeL + vec2(0.0, 0.1))) < pupilSize || length(rotP - (eyeR + vec2(0.0, 0.1))) < pupilSize) {
                           finalColor = vec3(0.0); // Pupil
                       }
                   }
               }

               FragColor = vec4(finalColor, 1.0);
           }
       } else {
           // For obstacles, render as quadratic blocks (squares)
           // Simple bevel effect based on distance to edge
           float distToEdge = min(1.0 - abs(p.x), 1.0 - abs(p.y));
           float bevel = smoothstep(0.0, 0.15, distToEdge);
           
           vec3 finalColor = objectColor * (0.5 + 0.5 * bevel);
           FragColor = vec4(finalColor, 1.0);
       }
    }
)glsl";

const char *borderVertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    void main()
    {
       gl_Position = vec4(aPos, 0.0, 1.0);
    }
)glsl";

const char *borderFragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;
    uniform vec3 borderColor;
    void main()
    {
       FragColor = vec4(borderColor, 1.0);
    }
)glsl";

const char *textVertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
    out vec2 TexCoords;

    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
        TexCoords = vertex.zw;
    }
)glsl";

const char *textFragmentShaderSource = R"glsl(
    #version 330 core
    in vec2 TexCoords;
    out vec4 color;

    uniform sampler2D text;
    uniform vec3 textColor;

    void main()
    {    
        vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
        color = vec4(textColor, 1.0) * sampled;
    }
)glsl";

// Utility function to reduce boilerplate in main()
unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        spdlog::error("ERROR::SHADER::VERTEX::COMPILATION_FAILED: {}", infoLog);
    }
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        spdlog::error("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED: {}", infoLog);
    }
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        spdlog::error("ERROR::SHADER::PROGRAM::LINKING_FAILED: {}", infoLog);
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

int main()
{
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    spdlog::info("Starting Snake Game...");

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // Get primary monitor and its video mode for fullscreen
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    // Update SCR_WIDTH and SCR_HEIGHT to the monitor's resolution
    SCR_WIDTH = mode->width;
    SCR_HEIGHT = mode->height;

    // Create a fullscreen window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "SimpleCppSnake", monitor, NULL);
    if (window == NULL)
    {
        spdlog::critical("Failed to create GLFW window");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Main Game Shader
    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    // Border Shader Program
    unsigned int borderShaderProgram = createShaderProgram(borderVertexShaderSource, borderFragmentShaderSource);

    // Text Shader Program
    textShaderProgram = createShaderProgram(textVertexShaderSource, textFragmentShaderSource);

    // Freetype initialization
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        spdlog::error("ERROR::FREETYPE: Could not init FreeType Library");
        return -1;
    }

    FT_Face face;
    if (FT_New_Face(ft, FONT_PATH, 0, &face))
    {
        spdlog::error("ERROR::FREETYPE: Failed to load font at {}", FONT_PATH);
        return -1;
    }
    FT_Set_Pixel_Sizes(face, 0, 48); // Set font size

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment restriction

    // Load first 128 characters of ASCII set
    for (unsigned char c = 0; c < 128; c++)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            spdlog::warn("ERROR::FREETYTPE: Failed to load Glyph for char {}", c);
            continue;
        }
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    unsigned int gridCountLoc = glGetUniformLocation(shaderProgram, "gridCount");
    unsigned int colorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    unsigned int isAppleLoc = glGetUniformLocation(shaderProgram, "isApple");
    unsigned int isSnakeLoc = glGetUniformLocation(shaderProgram, "isSnake");
    unsigned int snakeDirectionLoc = glGetUniformLocation(shaderProgram, "snakeDirection");
    unsigned int borderColorLoc = glGetUniformLocation(borderShaderProgram, "borderColor");

    // A single square that we will reuse
    float vertices[] = {
        0.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f,

        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f
    };

    unsigned int VBO, VAO, instanceVBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Instanced VBO Setup
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    // Use a max size for the buffer since grid size is dynamic
    const int MAX_GRID_CELLS = GRID_WIDTH * GRID_HEIGHT;
    glBufferData(GL_ARRAY_BUFFER, MAX_GRID_CELLS * sizeof(glm::vec2), NULL, GL_STREAM_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glVertexAttribDivisor(1, 1); // Tell OpenGL this attribute changes once per instance

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Border VAO
    float borderVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };
    unsigned int borderVBO, borderVAO;
    glGenVertexArrays(1, &borderVAO);
    glGenBuffers(1, &borderVBO);
    glBindVertexArray(borderVAO);
    glBindBuffer(GL_ARRAY_BUFFER, borderVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(borderVertices), borderVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Text VAO/VBO
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Pause Info VAO
    float pauseVertices[] = {
        -0.15f,  0.3f, -0.15f, -0.3f, -0.05f, -0.3f,
        -0.15f,  0.3f, -0.05f, -0.3f, -0.05f,  0.3f,
         0.05f,  0.3f,  0.05f, -0.3f,  0.15f, -0.3f,
         0.05f,  0.3f,  0.15f, -0.3f,  0.15f,  0.3f
    };
    unsigned int pauseVAO, pauseVBO;
    glGenVertexArrays(1, &pauseVAO);
    glGenBuffers(1, &pauseVBO);
    glBindVertexArray(pauseVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pauseVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pauseVertices), pauseVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Game State Variables
    std::deque<glm::vec2> snake;
    Direction snakeDirection;
    glm::vec2 food;
    std::vector<glm::vec2> obstacles;
    bool gameOver = false;
    bool shouldReset = false;
    bool paused = false;
    bool inMenu = true;
    bool aiEnabled = false;
    int score = 0;

    float lastUpdateTime = 0.0f;
    float timeBetweenUpdates = 0.1f; // 10 moves per second
    resetGame(snake, snakeDirection, food, gameOver, score, obstacles, timeBetweenUpdates);
    
    // Setup text projection once
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));
    glUseProgram(textShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    auto updateUIProjection = [&](int width, int height) {
        SCR_WIDTH = width;
        SCR_HEIGHT = height;
        projection = glm::ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height));
        glUseProgram(textShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    };

    while (!glfwWindowShouldClose(window))
    {
        float currentTime = glfwGetTime();
        processInput(window, snakeDirection, shouldReset, gameOver, paused, inMenu, aiEnabled);

        if (shouldReset) {
            resetGame(snake, snakeDirection, food, gameOver, score, obstacles, timeBetweenUpdates);
            shouldReset = false;
            // Keep AI state across resets if desired, 
            // but usually cleaner to let user toggle it.
            // aiEnabled = false; 
            paused = false;
        }

        // Fixed-timestep update
        if (currentTime - lastUpdateTime >= timeBetweenUpdates && !gameOver && !paused && !inMenu)
        {
            lastUpdateTime = currentTime;

            if (aiEnabled) {
                snakeDirection = getAIDirection(snake, food, obstacles, snakeDirection);
            }

            glm::vec2 newHead = snake.front();
            if (snakeDirection == Direction::UP)    newHead.y += 1;
            if (snakeDirection == Direction::DOWN)  newHead.y -= 1;
            if (snakeDirection == Direction::LEFT)  newHead.x -= 1;
            if (snakeDirection == Direction::RIGHT) newHead.x += 1;

            // Wall collision
            if (newHead.x < 0 || newHead.x >= GRID_WIDTH || newHead.y < 0 || newHead.y >= GRID_HEIGHT) {
                gameOver = true;
            }

            // Obstacle collision
            for (const auto& obs : obstacles) {
                if (newHead == obs) {
                    gameOver = true;
                    break;
                }
            }

            // Self collision
            for (size_t i = 1; i < snake.size(); ++i) {
                if (newHead == snake[i]) {
                    gameOver = true;
                    break;
                }
            }

            if (!gameOver) {
                snake.push_front(newHead);

                if (newHead == food) {
                    score++;
                    // Increase speed and add obstacle
                    timeBetweenUpdates = std::max(0.03f, timeBetweenUpdates * 0.98f);
                    generateObstacle(obstacles, snake, food);
                    generateNewFood(food, snake, obstacles);
                } else {
                    snake.pop_back();
                }
            }
        }

        // Render        
        int winW, winH;
        glfwGetFramebufferSize(window, &winW, &winH);
        
        // Calculate aspect-correct viewport to keep cells square
        float targetRatio = (float)GRID_WIDTH / (float)GRID_HEIGHT;
        float windowRatio = (float)winW / (float)winH;
        int vpX, vpY, vpW, vpH;

        if (windowRatio > targetRatio) {
            vpH = winH;
            vpW = static_cast<int>(winH * targetRatio);
            vpX = (winW - vpW) / 2;
            vpY = 0;
        } else {
            vpW = winW;
            vpH = static_cast<int>(vpW / targetRatio);
            vpX = 0;
            vpY = (winH - vpH) / 2;
        }

        // 1. Clear the full window
        glViewport(0, 0, winW, winH);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Update projection if window size changed (e.g., after F11)
        if (static_cast<unsigned int>(winW) != SCR_WIDTH || static_cast<unsigned int>(winH) != SCR_HEIGHT)
            updateUIProjection(winW, winH);

        if (inMenu) {
            RenderText(textShaderProgram, "SNAKE GAME", SCR_WIDTH / 2.0f - 150.0f, SCR_HEIGHT / 2.0f + 100.0f, 1.2f, glm::vec3(0.0, 0.8, 0.2));
           
            RenderText(textShaderProgram, "Press ENTER to Start", SCR_WIDTH / 2.0f - 180.0f, SCR_HEIGHT / 2.0f - 40.0f, 0.8f, glm::vec3(1.0, 1.0, 1.0));

            RenderText(textShaderProgram, "Controls:", SCR_WIDTH / 2.0f - 50.0f, SCR_HEIGHT / 2.0f - 100.0f, 0.6f, glm::vec3(1.0, 0.8, 0.2));
            RenderText(textShaderProgram, "[Arrows] Move / Change Selection", SCR_WIDTH / 2.0f - 230.0f, SCR_HEIGHT / 2.0f - 130.0f, 0.5f, glm::vec3(0.7));
            RenderText(textShaderProgram, "[Space/Enter] Select / Pause", SCR_WIDTH / 2.0f - 190.0f, SCR_HEIGHT / 2.0f - 155.0f, 0.5f, glm::vec3(0.7));
            RenderText(textShaderProgram, "[Esc] Back / Quit", SCR_WIDTH / 2.0f - 120.0f, SCR_HEIGHT / 2.0f - 180.0f, 0.5f, glm::vec3(0.7));
            RenderText(textShaderProgram, "[F11] Fullscreen", SCR_WIDTH / 2.0f - 110.0f, SCR_HEIGHT / 2.0f - 205.0f, 0.5f, glm::vec3(0.7));
            RenderText(textShaderProgram, "[I] AI Auto-Pilot", SCR_WIDTH / 2.0f - 105.0f, SCR_HEIGHT / 2.0f - 230.0f, 0.5f, glm::vec3(0.7));
        } else {
            // 2. Set viewport to the Game Grid area
            glViewport(vpX, vpY, vpW, vpH);

            // Draw Border
            glUseProgram(borderShaderProgram);
            glBindVertexArray(borderVAO);
            glUniform3f(borderColorLoc, 0.5f, 0.5f, 0.5f);
            glLineWidth(5.0f);
            glDrawArrays(GL_LINE_LOOP, 0, 4);

            glUseProgram(shaderProgram);
            glBindVertexArray(VAO);

            glUniform2f(gridCountLoc, (float)GRID_WIDTH, (float)GRID_HEIGHT);

            // Draw Snake
            glm::vec3 color = gameOver ? glm::vec3(0.5f, 0.0f, 0.0f) : glm::vec3(0.0f, 0.8f, 0.2f);
            glUniform3fv(colorLoc, 1, glm::value_ptr(color));
            glUniform1i(isAppleLoc, 0);
            glUniform1i(isSnakeLoc, 1);
            glUniform1i(snakeDirectionLoc, static_cast<int>(snakeDirection));
            
            std::vector<glm::vec2> snakeData(snake.begin(), snake.end());
            glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, snakeData.size() * sizeof(glm::vec2), snakeData.data());
            glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<unsigned int>(snakeData.size()));

            // Draw Food
            color = glm::vec3(1.0f, 0.2f, 0.2f);
            glUniform3fv(colorLoc, 1, glm::value_ptr(color));
            glUniform1i(isAppleLoc, 1);
            glUniform1i(isSnakeLoc, 0);

            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2), &food);
            glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1);

            // Draw Obstacles
            if (!obstacles.empty()) {
                color = glm::vec3(0.5f, 0.5f, 0.5f); // Grey obstacles
                glUniform3fv(colorLoc, 1, glm::value_ptr(color));
                glUniform1i(isAppleLoc, 0);
                glUniform1i(isSnakeLoc, 0);
                glBufferSubData(GL_ARRAY_BUFFER, 0, obstacles.size() * sizeof(glm::vec2), obstacles.data());
                glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<unsigned int>(obstacles.size()));
            }

            // 3. Restore viewport to full window for UI
            glViewport(0, 0, winW, winH);

            // Draw Score
            std::string scoreStr = "Points: " + std::to_string(score);
            RenderText(textShaderProgram, scoreStr, 20.0f, SCR_HEIGHT - 40.0f, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f));

            if (aiEnabled && !gameOver) {
                RenderText(textShaderProgram, "AI AUTO-PILOT ENABLED [I]", SCR_WIDTH - 300.0f, SCR_HEIGHT - 40.0f, 0.4f, glm::vec3(0.0f, 1.0f, 1.0f));
            }

            if (paused) {
                glUseProgram(borderShaderProgram);
                glBindVertexArray(pauseVAO);
                glUniform3f(borderColorLoc, 1.0f, 1.0f, 1.0f);
                glDrawArrays(GL_TRIANGLES, 0, 12);
                
                RenderText(textShaderProgram, "PAUSED", SCR_WIDTH / 2.0f - 80.0f, SCR_HEIGHT / 2.0f + 120.0f, 1.0f, glm::vec3(1.0, 1.0, 1.0));
                RenderText(textShaderProgram, "Press SPACE to Resume", SCR_WIDTH / 2.0f - 200.0f, SCR_HEIGHT / 2.0f - 150.0f, 0.6f, glm::vec3(0.8, 0.8, 0.8));
            }
        }

        if (gameOver) {
            RenderText(textShaderProgram, "GAME OVER", SCR_WIDTH / 2.0f - 150.0f, SCR_HEIGHT / 2.0f + 50.0f, 1.0f, glm::vec3(1.0, 0.2, 0.2));
            
            std::string scoreText = "SCORE: " + std::to_string(score);
            RenderText(textShaderProgram, scoreText, SCR_WIDTH / 2.0f - 110.0f, SCR_HEIGHT / 2.0f, 0.8f, glm::vec3(1.0, 1.0, 1.0));

            RenderText(textShaderProgram, "Press Space to Restart", SCR_WIDTH / 2.0f - 200.0f, SCR_HEIGHT / 2.0f - 60.0f, 0.7f, glm::vec3(0.8, 0.8, 0.8));
            RenderText(textShaderProgram, "Press ESC to Menu", SCR_WIDTH / 2.0f - 160.0f, SCR_HEIGHT / 2.0f - 100.0f, 0.6f, glm::vec3(0.6, 0.6, 0.6));
        }


        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    spdlog::info("Shutting down...");

    // Cleanup Text Rendering resources
    glDeleteVertexArrays(1, &textVAO);
    glDeleteBuffers(1, &textVBO);
    glDeleteProgram(textShaderProgram);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &instanceVBO);
    glDeleteProgram(shaderProgram);

    glDeleteVertexArrays(1, &borderVAO);
    glDeleteBuffers(1, &borderVBO);
    glDeleteProgram(borderShaderProgram);

    glDeleteVertexArrays(1, &pauseVAO);
    glDeleteBuffers(1, &pauseVBO);

    // Cleanup textures
    for (auto const& [key, val] : Characters) {
        glDeleteTextures(1, &val.TextureID);
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window, Direction& dir, bool& shouldReset, bool gameOver, bool& paused, bool& inMenu, bool& aiEnabled)
{
    static bool actionPressedLastFrame = false;
    bool actionPressed = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS);

    static bool isFullscreen = true; // Start in fullscreen
    static int windowedX = 100, windowedY = 100, windowedW = 1280, windowedH = 720; // Default windowed size
    static bool f11PressedLastFrame = false;
    bool f11Pressed = glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS;

    if (f11Pressed && !f11PressedLastFrame) {
        if (isFullscreen) {
            // Restore windowed mode
            glfwSetWindowMonitor(window, NULL, windowedX, windowedY, windowedW, windowedH, 0);
            isFullscreen = false;
        } else {
            // Go fullscreen
            glfwGetWindowPos(window, &windowedX, &windowedY);
            glfwGetWindowSize(window, &windowedW, &windowedH);

            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            isFullscreen = true;
        }
    }
    f11PressedLastFrame = f11Pressed;

    static bool escPressedLastFrame = false;
    bool escPressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;

    if (escPressed && !escPressedLastFrame) {
        if (inMenu) {
            glfwSetWindowShouldClose(window, true);
        } else {
            inMenu = true;
            shouldReset = true;
        }
    }
    escPressedLastFrame = escPressed;

    static bool iPressedLastFrame = false;
    bool iPressed = glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS;
    if (iPressed && !iPressedLastFrame) {
        aiEnabled = !aiEnabled;
    }
    iPressedLastFrame = iPressed;

    // This part handles Enter/Space key
    if (actionPressed && !actionPressedLastFrame) {
        if (inMenu) {
            inMenu = false;
            shouldReset = true; // Reset with new settings
        } else if (gameOver) {
            shouldReset = true;
        } else {
            paused = !paused;
        }
    }
    actionPressedLastFrame = actionPressed;

    // Menu Navigation for Resolution
    if (inMenu) {
        // Arrow keys for menu navigation can go here if more options are added.
        // The resolution switching has been removed as the game now starts in native fullscreen.
        return; // Don't process movement keys while in menu
    }

    if (aiEnabled) return; // Don't allow manual control while AI is running

    if (gameOver || paused) return;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && dir != Direction::DOWN)
        dir = Direction::UP;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && dir != Direction::UP)
        dir = Direction::DOWN;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && dir != Direction::RIGHT)
        dir = Direction::LEFT;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS && dir != Direction::LEFT)
        dir = Direction::RIGHT;
}

void generateNewFood(glm::vec2& food, const std::deque<glm::vec2>& snake, const std::vector<glm::vec2>& obstacles) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribX(0, GRID_WIDTH - 1);
    std::uniform_int_distribution<> distribY(0, GRID_HEIGHT - 1);

    bool onSnake;
    do {
        onSnake = false;
        food.x = distribX(gen);
        food.y = distribY(gen);
        for (const auto& segment : snake) {
            if (food == segment) {
                onSnake = true;
                break;
            }
        }
        if (!onSnake) {
            for (const auto& obs : obstacles) {
                if (food == obs) {
                    onSnake = true; // reusing onSnake flag for collision check
                    break;
                }
            }
        }
    } while (onSnake);
}

void generateObstacle(std::vector<glm::vec2>& obstacles, const std::deque<glm::vec2>& snake, const glm::vec2& food) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribX(0, GRID_WIDTH - 1);
    std::uniform_int_distribution<> distribY(0, GRID_HEIGHT - 1);

    glm::vec2 obs;
    bool collision;
    do {
        collision = false;
        obs.x = distribX(gen);
        obs.y = distribY(gen);
        
        if (obs == food) collision = true;
        for (const auto& s : snake) {
            if (obs == s) collision = true;
        }
        for (const auto& o : obstacles) {
            if (obs == o) collision = true;
        }
    } while (collision);
    obstacles.push_back(obs);
}

void resetGame(std::deque<glm::vec2>& snake, Direction& dir, glm::vec2& food, bool& gameOver, int& score, std::vector<glm::vec2>& obstacles, float& timeBetweenUpdates) {
    spdlog::info("--- New Game Started ---");
    snake.clear();
    snake.push_front({GRID_WIDTH / 2, GRID_HEIGHT / 2});
    snake.push_back({GRID_WIDTH / 2 - 1, GRID_HEIGHT / 2});
    snake.push_back({GRID_WIDTH / 2 - 2, GRID_HEIGHT / 2});

    dir = Direction::RIGHT;
    gameOver = false;
    score = 0;
    obstacles.clear();
    timeBetweenUpdates = 0.1f;
    generateNewFood(food, snake, obstacles);
}

Direction getAIDirection(const std::deque<glm::vec2>& snake, glm::vec2 food, const std::vector<glm::vec2>& obstacles, Direction currentDir) {
    glm::vec2 head = snake.front();
    
    // Data structure for BFS nodes
    struct Node {
        glm::vec2 pos;
        Direction firstMove;
    };

    std::queue<Node> q;
    // Use a grid to track both visited and occupied cells for O(1) lookup
    bool blocked[GRID_WIDTH][GRID_HEIGHT] = {false};

    // Pre-populate the occupancy grid
    for (const auto& s : snake) blocked[(int)s.x][(int)s.y] = true;
    for (const auto& o : obstacles) blocked[(int)o.x][(int)o.y] = true;

    // Mark head as visited (it's already occupied, but we need to track it)
    blocked[(int)head.x][(int)head.y] = true;

    // Explore neighbors of the head first to establish the 'firstMove' for the path
    std::vector<Direction> dirs = {Direction::UP, Direction::DOWN, Direction::LEFT, Direction::RIGHT};
    for (Direction d : dirs) {
        glm::vec2 next = head;
        if (d == Direction::UP) next.y += 1;
        else if (d == Direction::DOWN) next.y -= 1;
        else if (d == Direction::LEFT) next.x -= 1;
        else if (d == Direction::RIGHT) next.x += 1;

        if (next.x >= 0 && next.x < GRID_WIDTH && next.y >= 0 && next.y < GRID_HEIGHT && !blocked[(int)next.x][(int)next.y]) {
            if (next == food) return d;
            q.push({next, d});
            blocked[(int)next.x][(int)next.y] = true;
        }
    }

    // BFS to find the shortest path to food
    while (!q.empty()) {
        Node curr = q.front();
        q.pop();

        for (Direction d : dirs) {
            glm::vec2 next = curr.pos;
            if (d == Direction::UP) next.y += 1;
            else if (d == Direction::DOWN) next.y -= 1;
            else if (d == Direction::LEFT) next.x -= 1;
            else if (d == Direction::RIGHT) next.x += 1;

            if (next == food) return curr.firstMove;

            if (next.x >= 0 && next.x < GRID_WIDTH && next.y >= 0 && next.y < GRID_HEIGHT && 
                !blocked[(int)next.x][(int)next.y]) {
                blocked[(int)next.x][(int)next.y] = true;
                q.push({next, curr.firstMove});
            }
        }
    }

    // Fallback: If no path to food is found, try to survive by picking any valid move
    // We prioritize moves that don't lead to immediate death
    for (Direction d : dirs) {
        // Avoid 180-degree turns
        if ((d == Direction::UP && currentDir == Direction::DOWN) ||
            (d == Direction::DOWN && currentDir == Direction::UP) ||
            (d == Direction::LEFT && currentDir == Direction::RIGHT) ||
            (d == Direction::RIGHT && currentDir == Direction::LEFT)) continue;

        glm::vec2 next = head;
        if (d == Direction::UP) next.y += 1;
        else if (d == Direction::DOWN) next.y -= 1;
        else if (d == Direction::LEFT) next.x -= 1;
        else if (d == Direction::RIGHT) next.x += 1;

        // Temporary check for fallback without modifying outer blocked state
        if (next.x >= 0 && next.x < GRID_WIDTH && next.y >= 0 && next.y < GRID_HEIGHT) {
             bool isReallyOccupied = false;
             for (const auto& s : snake) if (next == s) isReallyOccupied = true;
             for (const auto& o : obstacles) if (next == o) isReallyOccupied = true;
             if (!isReallyOccupied) return d;
        }
    }

    return currentDir;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void RenderText(unsigned int shader, std::string text, float x, float y, float scale, glm::vec3 color)
{
    // Activate corresponding render state	
    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        auto it = Characters.find(*c);
        if (it == Characters.end()) continue; // Skip unknown characters
        
        Character ch = it->second;

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // Update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
