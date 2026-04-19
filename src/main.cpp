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
#include <memory>
#include <fstream>
#include <filesystem>

#include <unistd.h>
#include <limits.h>
#include "game_logic.hpp"

// Game Settings
unsigned int SCR_WIDTH = 1920; // Will be updated to monitor's resolution
unsigned int SCR_HEIGHT = 1080; // Will be updated to monitor's resolution

struct Character { 
    unsigned int TextureID; 
    glm::ivec2 Size; 
    glm::ivec2 Bearing; 
    unsigned int Advance; 
};

struct Renderer {
    std::map<char, Character> Characters;
    unsigned int textVAO, textVBO;
    unsigned int shaderProgram, borderShaderProgram, textShaderProgram;
    unsigned int VAO, VBO, instanceVBO, borderVAO, borderVBO, pauseVAO, pauseVBO;
    unsigned int gridCountLoc, colorLoc, isAppleLoc, isBananaLoc, isGoldenAppleLoc, isSnakeLoc, snakeDirectionLoc, borderColorLoc;
};

// Function Prototypes
void RenderText(const Renderer& renderer, std::string text, float x, float y, float scale, glm::vec3 color);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void handleInput(GLFWwindow* window, GameState& state);
unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource);

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
    uniform bool isBanana;
    uniform bool isGoldenApple;
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
       } else if (isBanana) {
            // Procedural banana shape (curved crescent)
            vec2 q = p;
            q.x += 0.45 * (q.y * q.y - 0.7); // Increased curvature
            float bananaDist = length(vec2(q.x * 2.8, q.y * 0.9)) - 0.75;
            
            if (bananaDist < 0.0) {
                // Better shading for a cylindrical look
                float rim = 1.0 - max(dot(normalize(vec3(p, 0.5)), vec3(0,0,1)), 0.0);
                vec3 bananaColor = mix(vec3(1.0, 0.95, 0.2), vec3(0.9, 0.8, 0.1), rim);
                vec2 lightPos = vec2(-0.3, 0.4);
                float highlight = pow(max(1.0 - distance(p, lightPos), 0.0), 10.0);
                vec3 finalColor = bananaColor + vec3(0.5) * highlight;
                FragColor = vec4(finalColor, 1.0);
            } else if (abs(p.y) > 0.6 && abs(q.x * 2.8) < 0.25) {
                // Dark ends of the banana
                FragColor = vec4(0.2, 0.12, 0.05, 1.0);
            } else {
                discard;
            }
       } else if (isGoldenApple) {
            // Procedural golden apple shape
            float appleDist = length(vec2(p.x, p.y + 0.1 - 0.2 * abs(p.x)));
            if (appleDist < 0.7) {
                float rim = 1.0 - max(dot(normalize(vec3(p, 0.5)), vec3(0,0,1)), 0.0);
                vec3 goldColor = mix(vec3(1.0, 0.85, 0.0), vec3(0.6, 0.4, 0.0), rim);
                vec2 lightPos = vec2(-0.3, 0.3);
                float highlight = pow(max(1.0 - distance(p, lightPos), 0.0), 12.0);
                vec3 finalColor = goldColor + vec3(1.0, 1.0, 0.8) * highlight;
                FragColor = vec4(finalColor, 1.0);
            } else if (p.y > 0.4 && p.y < 0.8 && abs(p.x) < 0.05) {
                // A simple brown stem for the golden apple
                FragColor = vec4(0.4, 0.2, 0.0, 1.0);
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

    Renderer renderer;

    // Main Game Shader
    renderer.shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    // Border Shader Program
    renderer.borderShaderProgram = createShaderProgram(borderVertexShaderSource, borderFragmentShaderSource);

    // Text Shader Program
    renderer.textShaderProgram = createShaderProgram(textVertexShaderSource, textFragmentShaderSource);

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
        renderer.Characters.insert(std::pair<char, Character>(c, character));
    }
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    renderer.gridCountLoc = glGetUniformLocation(renderer.shaderProgram, "gridCount");
    renderer.colorLoc = glGetUniformLocation(renderer.shaderProgram, "objectColor");
    renderer.isAppleLoc = glGetUniformLocation(renderer.shaderProgram, "isApple");
    renderer.isBananaLoc = glGetUniformLocation(renderer.shaderProgram, "isBanana");
    renderer.isGoldenAppleLoc = glGetUniformLocation(renderer.shaderProgram, "isGoldenApple");
    renderer.isSnakeLoc = glGetUniformLocation(renderer.shaderProgram, "isSnake");
    renderer.snakeDirectionLoc = glGetUniformLocation(renderer.shaderProgram, "snakeDirection");
    renderer.borderColorLoc = glGetUniformLocation(renderer.borderShaderProgram, "borderColor");

    // A single square that we will reuse
    float vertices[] = {
        0.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f,

        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f
    };

    glGenVertexArrays(1, &renderer.VAO);
    glGenBuffers(1, &renderer.VBO);
    glBindVertexArray(renderer.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, renderer.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Instanced VBO Setup
    glGenBuffers(1, &renderer.instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, renderer.instanceVBO);
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
    glGenVertexArrays(1, &renderer.borderVAO);
    glGenBuffers(1, &renderer.borderVBO);
    glBindVertexArray(renderer.borderVAO);
    glBindBuffer(GL_ARRAY_BUFFER, renderer.borderVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(borderVertices), borderVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Text VAO/VBO
    glGenVertexArrays(1, &renderer.textVAO);
    glGenBuffers(1, &renderer.textVBO);
    glBindVertexArray(renderer.textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, renderer.textVBO);
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
    glGenVertexArrays(1, &renderer.pauseVAO);
    glGenBuffers(1, &renderer.pauseVBO);
    glBindVertexArray(renderer.pauseVAO);
    glBindBuffer(GL_ARRAY_BUFFER, renderer.pauseVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pauseVertices), pauseVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Centralized Game State
    auto state = std::make_unique<GameState>();
    state->highScorePath = (std::filesystem::path(getExecutableDir()) / "highscore.txt").string();
    loadHighScore(*state);
    resetGame(*state);
    
    // Setup text projection once
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));
    glUseProgram(renderer.textShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(renderer.textShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    auto updateUIProjection = [&](int width, int height) {
        SCR_WIDTH = width;
        SCR_HEIGHT = height;
        projection = glm::ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height));
        glUseProgram(renderer.textShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(renderer.textShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    };

    while (!glfwWindowShouldClose(window))
    {
        float currentTime = glfwGetTime();
        handleInput(window, *state);

        if (state->shouldReset) {
            updateHighScores(*state);
            resetGame(*state);
        }

        updateGame(*state, currentTime);

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

        if (state->inMenu) {
            RenderText(renderer, "SNAKE GAME", SCR_WIDTH / 2.0f - 150.0f, SCR_HEIGHT / 2.0f + 200.0f, 1.2f, glm::vec3(0.0, 0.8, 0.2));

            RenderText(renderer, "Top 5 High Scores:", SCR_WIDTH / 2.0f - 150.0f, SCR_HEIGHT / 2.0f + 140.0f, 0.6f, glm::vec3(1.0, 0.9, 0.3));
            for (size_t i = 0; i < state->highScores.size(); ++i) {
                std::string hs = std::to_string(i + 1) + ". " + std::to_string(state->highScores[i].score) + 
                                 (state->highScores[i].aiUsed ? " (AI)" : " (No AI)") +
                                 (state->highScores[i].obstaclesEnabled ? " (Obs)" : " (No Obs)");
                RenderText(renderer, hs, SCR_WIDTH / 2.0f - 100.0f, SCR_HEIGHT / 2.0f + 110.0f - i * 25.0f, 0.5f, glm::vec3(1.0, 0.9, 0.3));
            }

            // Interactive Menu Selection
            glm::vec3 startColor = (state->menuSelection == 0) ? glm::vec3(1.0, 1.0, 0.0) : glm::vec3(1.0, 1.0, 1.0);
            RenderText(renderer, "Start Game", SCR_WIDTH / 2.0f - 100.0f, SCR_HEIGHT / 2.0f - 40.0f, 0.8f, startColor);

            glm::vec3 obsColor = (state->menuSelection == 1) ? glm::vec3(1.0, 1.0, 0.0) : glm::vec3(1.0, 1.0, 1.0);
            std::string obsText = "Obstacle Spawning: " + std::string(state->obstaclesEnabled ? "ON" : "OFF");
            RenderText(renderer, obsText, SCR_WIDTH / 2.0f - 180.0f, SCR_HEIGHT / 2.0f - 80.0f, 0.6f, obsColor);

            RenderText(renderer, "Controls:", SCR_WIDTH / 2.0f - 50.0f, SCR_HEIGHT / 2.0f - 130.0f, 0.6f, glm::vec3(1.0, 0.8, 0.2));
            RenderText(renderer, "[Arrows] Move / Change Selection", SCR_WIDTH / 2.0f - 230.0f, SCR_HEIGHT / 2.0f - 160.0f, 0.5f, glm::vec3(0.7));
            RenderText(renderer, "[Space/Enter] Select / Pause", SCR_WIDTH / 2.0f - 190.0f, SCR_HEIGHT / 2.0f - 185.0f, 0.5f, glm::vec3(0.7));
            RenderText(renderer, "[Esc] Back / Quit", SCR_WIDTH / 2.0f - 120.0f, SCR_HEIGHT / 2.0f - 210.0f, 0.5f, glm::vec3(0.7));
            RenderText(renderer, "[F11] Fullscreen", SCR_WIDTH / 2.0f - 110.0f, SCR_HEIGHT / 2.0f - 235.0f, 0.5f, glm::vec3(0.7));
            RenderText(renderer, "[I] AI Auto-Pilot", SCR_WIDTH / 2.0f - 105.0f, SCR_HEIGHT / 2.0f - 260.0f, 0.5f, glm::vec3(0.7));
        } else {
            // 2. Set viewport to the Game Grid area
            glViewport(vpX, vpY, vpW, vpH);

            // Draw Border
            glUseProgram(renderer.borderShaderProgram);
            glBindVertexArray(renderer.borderVAO);
            glUniform3f(renderer.borderColorLoc, 0.5f, 0.5f, 0.5f);
            glLineWidth(5.0f);
            glDrawArrays(GL_LINE_LOOP, 0, 4);

            glUseProgram(renderer.shaderProgram);
            glBindVertexArray(renderer.VAO);

            glUniform2f(renderer.gridCountLoc, (float)GRID_WIDTH, (float)GRID_HEIGHT);

            // Draw Snake
            glm::vec3 color = state->gameOver ? glm::vec3(0.5f, 0.0f, 0.0f) : glm::vec3(0.0f, 0.8f, 0.2f);
            glUniform3fv(renderer.colorLoc, 1, glm::value_ptr(color));
            glUniform1i(renderer.isAppleLoc, 0);
            glUniform1i(renderer.isBananaLoc, 0);
            glUniform1i(renderer.isGoldenAppleLoc, 0);
            glUniform1i(renderer.isSnakeLoc, 1);
            glUniform1i(renderer.snakeDirectionLoc, static_cast<int>(state->snakeDirection));
            
            std::vector<glm::vec2> snakeData(state->snake.begin(), state->snake.end());
            glBindBuffer(GL_ARRAY_BUFFER, renderer.instanceVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, snakeData.size() * sizeof(glm::vec2), snakeData.data());
            glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<unsigned int>(snakeData.size()));

            // Draw Food
            color = glm::vec3(1.0f, 0.2f, 0.2f);
            glUniform3fv(renderer.colorLoc, 1, glm::value_ptr(color));
            glUniform1i(renderer.isAppleLoc, 1);
            glUniform1i(renderer.isBananaLoc, 0);
            glUniform1i(renderer.isGoldenAppleLoc, 0);
            glUniform1i(renderer.isSnakeLoc, 0);

            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2), &state->food);
            glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1);

            // Draw Obstacles
            if (!state->obstacles.empty()) {
                color = glm::vec3(0.5f, 0.5f, 0.5f); // Grey obstacles
                glUniform3fv(renderer.colorLoc, 1, glm::value_ptr(color));
                glUniform1i(renderer.isAppleLoc, 0);
                glUniform1i(renderer.isBananaLoc, 0);
                glUniform1i(renderer.isGoldenAppleLoc, 0);
                glUniform1i(renderer.isSnakeLoc, 0);
                glBufferSubData(GL_ARRAY_BUFFER, 0, state->obstacles.size() * sizeof(glm::vec2), state->obstacles.data());
                glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<unsigned int>(state->obstacles.size()));
            }

            // Draw Banana
            if (state->banana.active) {
                color = glm::vec3(1.0f, 0.9f, 0.1f); // Bright yellow for visibility
                glUniform3fv(renderer.colorLoc, 1, glm::value_ptr(color));
                glUniform1i(renderer.isAppleLoc, 0);
                glUniform1i(renderer.isBananaLoc, 1);
                glUniform1i(renderer.isGoldenAppleLoc, 0);
                glUniform1i(renderer.isSnakeLoc, 0);

                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2), &state->banana.pos);
                glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1);
            }

            // Draw Golden Apple
            if (state->goldenApple.active) {
                color = glm::vec3(1.0f, 0.85f, 0.0f); // Gold
                glUniform3fv(renderer.colorLoc, 1, glm::value_ptr(color));
                glUniform1i(renderer.isAppleLoc, 0);
                glUniform1i(renderer.isBananaLoc, 0);
                glUniform1i(renderer.isGoldenAppleLoc, 1);
                glUniform1i(renderer.isSnakeLoc, 0);

                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2), &state->goldenApple.pos);
                glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1);
            }

            // 3. Restore viewport to full window for UI
            glViewport(0, 0, winW, winH);

            // Draw Score
            std::string scoreStr = "Points: " + std::to_string(state->score);
            RenderText(renderer, scoreStr, 20.0f, SCR_HEIGHT - 40.0f, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f));

            // Draw Banana Timer
            if (state->banana.active && !state->gameOver && !state->paused) {
                float timeLeft = 5.0f - (currentTime - state->banana.spawnTime);
                if (timeLeft > 0) {
                    std::string timeStr = std::to_string((int)std::ceil(timeLeft)) + "s";
                    float cellW = (float)vpW / GRID_WIDTH;
                    float cellH = (float)vpH / GRID_HEIGHT;
                    // Position text above the banana
                    float tx = vpX + state->banana.pos.x * cellW + cellW * 0.2f;
                    float ty = vpY + (state->banana.pos.y + 0.8f) * cellH;
                    RenderText(renderer, timeStr, tx, ty, 0.4f, glm::vec3(1.0f, 1.0f, 0.0f));
                }
            }

            // Draw Golden Apple Timer
            if (state->goldenApple.active && !state->gameOver && !state->paused) {
                float timeLeft = 7.0f - (currentTime - state->goldenApple.spawnTime);
                if (timeLeft > 0) {
                    std::string timeStr = std::to_string((int)std::ceil(timeLeft)) + "s";
                    float cellW = (float)vpW / GRID_WIDTH;
                    float cellH = (float)vpH / GRID_HEIGHT;
                    float tx = vpX + state->goldenApple.pos.x * cellW + cellW * 0.2f;
                    float ty = vpY + (state->goldenApple.pos.y + 0.8f) * cellH;
                    RenderText(renderer, timeStr, tx, ty, 0.4f, glm::vec3(1.0f, 0.8f, 0.0f));
                }
            }

            if (state->aiEnabled && !state->gameOver) {
                RenderText(renderer, "AI AUTO-PILOT ENABLED [I]", SCR_WIDTH - 300.0f, SCR_HEIGHT - 40.0f, 0.4f, glm::vec3(0.0f, 1.0f, 1.0f));
            }

            if (state->paused) {
                glUseProgram(renderer.borderShaderProgram);
                glBindVertexArray(renderer.pauseVAO);
                glUniform3f(renderer.borderColorLoc, 1.0f, 1.0f, 1.0f);
                glDrawArrays(GL_TRIANGLES, 0, 12);
                
                RenderText(renderer, "PAUSED", SCR_WIDTH / 2.0f - 80.0f, SCR_HEIGHT / 2.0f + 120.0f, 1.0f, glm::vec3(1.0, 1.0, 1.0));
                RenderText(renderer, "Press SPACE to Resume", SCR_WIDTH / 2.0f - 200.0f, SCR_HEIGHT / 2.0f - 150.0f, 0.6f, glm::vec3(0.8, 0.8, 0.8));
            }
        }

        if (state->gameOver) {
            RenderText(renderer, "GAME OVER", SCR_WIDTH / 2.0f - 150.0f, SCR_HEIGHT / 2.0f + 100.0f, 1.0f, glm::vec3(1.0, 0.2, 0.2));
            
            if (state->isNewTopScore) {
                RenderText(renderer, "NEW TOP 5 SCORE!", SCR_WIDTH / 2.0f - 180.0f, SCR_HEIGHT / 2.0f + 50.0f, 0.7f, glm::vec3(1.0, 0.8, 0.0));
            }

            std::string scoreText = "SCORE: " + std::to_string(state->score);
            RenderText(renderer, scoreText, SCR_WIDTH / 2.0f - 110.0f, SCR_HEIGHT / 2.0f, 0.8f, glm::vec3(1.0, 1.0, 1.0));

            RenderText(renderer, "Press Space to Restart", SCR_WIDTH / 2.0f - 200.0f, SCR_HEIGHT / 2.0f - 60.0f, 0.7f, glm::vec3(0.8, 0.8, 0.8));
            RenderText(renderer, "Press ESC to Menu", SCR_WIDTH / 2.0f - 160.0f, SCR_HEIGHT / 2.0f - 100.0f, 0.6f, glm::vec3(0.6, 0.6, 0.6));
        }


        glfwSwapBuffers(window);
    }
    
    spdlog::info("Shutting down...");

    // Cleanup Text Rendering resources
    glDeleteVertexArrays(1, &renderer.textVAO);
    glDeleteBuffers(1, &renderer.textVBO);
    glDeleteProgram(renderer.textShaderProgram);

    glDeleteVertexArrays(1, &renderer.VAO);
    glDeleteBuffers(1, &renderer.VBO);
    glDeleteBuffers(1, &renderer.instanceVBO);
    glDeleteProgram(renderer.shaderProgram);

    glDeleteVertexArrays(1, &renderer.borderVAO);
    glDeleteBuffers(1, &renderer.borderVBO);
    glDeleteProgram(renderer.borderShaderProgram);

    glDeleteVertexArrays(1, &renderer.pauseVAO);
    glDeleteBuffers(1, &renderer.pauseVBO);

    // Cleanup textures
    for (auto const& [key, val] : renderer.Characters) {
        glDeleteTextures(1, &val.TextureID);
    }

    glfwTerminate();
    return 0;
}

void handleInput(GLFWwindow* window, GameState& state)
{
    // Poll for and process events
    glfwPollEvents();

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
        if (state.inMenu) {
            glfwSetWindowShouldClose(window, true);
        } else {
            state.inMenu = true;
            state.shouldReset = true;
        }
    }
    escPressedLastFrame = escPressed;

    static bool iPressedLastFrame = false;
    bool iPressed = glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS;
    if (iPressed && !iPressedLastFrame) {
        state.aiEnabled = !state.aiEnabled;
    }
    iPressedLastFrame = iPressed;

    // This part handles Enter/Space key
    if (actionPressed && !actionPressedLastFrame) {
        if (state.inMenu) {
            if (state.menuSelection == 0) {
                state.inMenu = false;
                state.shouldReset = true; // Reset with new settings
            } else if (state.menuSelection == 1) {
                state.obstaclesEnabled = !state.obstaclesEnabled;
            }
        } else if (state.gameOver) {
            state.shouldReset = true;
        } else {
            state.paused = !state.paused;
        }
    }
    actionPressedLastFrame = actionPressed;

    // Menu Navigation
    if (state.inMenu) {
        static bool upPressedLastFrame = false;
        bool upPressed = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
        if (upPressed && !upPressedLastFrame) {
            state.menuSelection = (state.menuSelection - 1 + 2) % 2;
        }
        upPressedLastFrame = upPressed;

        static bool downPressedLastFrame = false;
        bool downPressed = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
        if (downPressed && !downPressedLastFrame) {
            state.menuSelection = (state.menuSelection + 1) % 2;
        }
        downPressedLastFrame = downPressed;

        return; // Don't process movement keys while in menu
    }

    if (state.aiEnabled) return; // Don't allow manual control while AI is running

    if (state.gameOver || state.paused) return;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && state.snakeDirection != Direction::DOWN)
        state.snakeDirection = Direction::UP;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && state.snakeDirection != Direction::UP)
        state.snakeDirection = Direction::DOWN;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && state.snakeDirection != Direction::RIGHT)
        state.snakeDirection = Direction::LEFT;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS && state.snakeDirection != Direction::LEFT)
        state.snakeDirection = Direction::RIGHT;
}

void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height)
{
    glViewport(0, 0, width, height);
}

void RenderText(const Renderer& renderer, std::string text, float x, float y, float scale, glm::vec3 color)
{
    // Activate corresponding render state	
    glUseProgram(renderer.textShaderProgram);
    glUniform3f(glGetUniformLocation(renderer.textShaderProgram, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(renderer.textVAO);

    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        auto it = renderer.Characters.find(*c);
        if (it == renderer.Characters.end()) continue; // Skip unknown characters
        
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
        glBindBuffer(GL_ARRAY_BUFFER, renderer.textVBO);
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
