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
#include "renderer.hpp"

// Game Settings
unsigned int SCR_WIDTH = 1920; // Will be updated to monitor's resolution
unsigned int SCR_HEIGHT = 1080; // Will be updated to monitor's resolution

// Function Prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void handleInput(GLFWwindow* window, GameState& state);

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
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    Renderer renderer;
    if (!renderer.init(SCR_WIDTH, SCR_HEIGHT)) return -1;

    // Centralized Game State
    auto state = std::make_unique<GameState>();
    state->getHighScoreManager().setPath((std::filesystem::path(getExecutableDir()) / "highscore.txt").string());
    state->getHighScoreManager().load();
    state->reset();

    while (!glfwWindowShouldClose(window))
    {
        float currentTime = glfwGetTime();
        handleInput(window, *state);

        if (state->shouldReset()) {
            state->updateHighScores();
            state->reset();
        }

        state->update(currentTime);

        int winW, winH;
        glfwGetFramebufferSize(window, &winW, &winH);
        
        // Update projection if window size changed (e.g., after F11)
        if (static_cast<unsigned int>(winW) != SCR_WIDTH || static_cast<unsigned int>(winH) != SCR_HEIGHT) {
            SCR_WIDTH = winW; SCR_HEIGHT = winH;
            renderer.updateProjection(winW, winH);
        }

        renderer.render(*state, currentTime, winW, winH);
        glfwSwapBuffers(window);
    }
    
    spdlog::info("Shutting down...");
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
        if (state.isInMenu()) {
            glfwSetWindowShouldClose(window, true);
        } else {
            state.setInMenu(true);
            state.setShouldReset(true);
        }
    }
    escPressedLastFrame = escPressed;

    static bool iPressedLastFrame = false;
    bool iPressed = glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS;
    if (iPressed && !iPressedLastFrame) {
        state.toggleAI();
    }
    iPressedLastFrame = iPressed;

    // This part handles Enter/Space key
    if (actionPressed && !actionPressedLastFrame) {
        if (state.isInMenu()) {
            if (state.getMenuSelection() == 0) {
                state.setInMenu(false);
                state.setShouldReset(true); // Reset with new settings
            } else if (state.getMenuSelection() == 1) {
                state.toggleObstacles();
            } else if (state.getMenuSelection() == 2) {
                state.toggleAI();
            }
        } else if (state.isGameOver()) {
            state.setShouldReset(true);
        } else {
            state.togglePause();
        }
    }
    actionPressedLastFrame = actionPressed;

    // Menu Navigation
    if (state.isInMenu()) {
        static bool upPressedLastFrame = false;
        bool upPressed = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
        if (upPressed && !upPressedLastFrame) {
            state.setMenuSelection((state.getMenuSelection() - 1 + 3) % 3);
        }
        upPressedLastFrame = upPressed;

        static bool downPressedLastFrame = false;
        bool downPressed = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
        if (downPressed && !downPressedLastFrame) {
            state.setMenuSelection((state.getMenuSelection() + 1) % 3);
        }
        downPressedLastFrame = downPressed;

        return; // Don't process movement keys while in menu
    }

    if (state.isAIEnabled()) return; // Don't allow manual control while AI is running

    if (state.isGameOver() || state.isPaused()) return;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && state.getDirection() != Direction::DOWN)
        state.setDirection(Direction::UP);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && state.getDirection() != Direction::UP)
        state.setDirection(Direction::DOWN);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && state.getDirection() != Direction::RIGHT)
        state.setDirection(Direction::LEFT);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS && state.getDirection() != Direction::LEFT)
        state.setDirection(Direction::RIGHT);
}

void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height)
{
    glViewport(0, 0, width, height);
}
