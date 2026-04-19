#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <memory>
#include <filesystem>
#include "game_logic.hpp"
#include "renderer.hpp"
#include <glm/glm.hpp>

class SnakeApplication {
public:
    SnakeApplication() {
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        spdlog::info("Initializing Snake Game...");
        
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        m_screenWidth = mode->width;
        m_screenHeight = mode->height;

        m_window = glfwCreateWindow(m_screenWidth, m_screenHeight, "SimpleCppSnake", monitor, nullptr);
        if (!m_window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(m_window);
        
        if (!m_renderer.init(m_screenWidth, m_screenHeight)) {
            throw std::runtime_error("Failed to initialize Renderer");
        }

        m_gameState = std::make_unique<GameState>();
        m_gameState->getHighScoreManager().setPath((std::filesystem::path(getExecutableDir()) / "highscore.txt").string());
        m_gameState->getHighScoreManager().load();
        float now = static_cast<float>(glfwGetTime());
        m_lastFrameTime = now;
        m_gameState->reset(now);
    }

    ~SnakeApplication() {
        spdlog::info("Shutting down...");
        glfwTerminate();
    }

    void run() {
        while (!glfwWindowShouldClose(m_window)) {
            float currentTime = static_cast<float>(glfwGetTime());
            float deltaTime = currentTime - m_lastFrameTime;
            m_lastFrameTime = currentTime;

            // 1. Process User Input
            processInput();

            // 2. State Management (Reset logic)
            if (m_gameState->shouldReset()) {
                m_gameState->updateHighScores();
                m_gameState->reset(currentTime);
            }

            // 3. Game Logic Update
            m_gameState->update(currentTime, deltaTime);

            // 4. Rendering
            updateViewportAndProjection();
            renderFrame(currentTime);

            glfwSwapBuffers(m_window);
        }
    }

private:
    void updateViewportAndProjection() {
        int winW, winH;
        glfwGetFramebufferSize(m_window, &winW, &winH);
        if (static_cast<unsigned int>(winW) != m_screenWidth || static_cast<unsigned int>(winH) != m_screenHeight) {
            m_screenWidth = winW;
            m_screenHeight = winH;
            glViewport(0, 0, winW, winH);
            m_renderer.updateProjection(winW, winH);
        }
    }

    void renderFrame(float currentTime) {
        int winW, winH;
        glfwGetFramebufferSize(m_window, &winW, &winH);
        m_renderer.render(*m_gameState, currentTime, winW, winH);
    }

    bool isKeyJustPressed(int key, bool& wasPressed) {
        bool isPressed = glfwGetKey(m_window, key) == GLFW_PRESS;
        bool result = isPressed && !wasPressed;
        wasPressed = isPressed;
        return result;
    }

    void processInput() {
        glfwPollEvents();

        // Global Controls
        if (isKeyJustPressed(GLFW_KEY_F11, m_inputStates.f11)) toggleFullscreen();
        
        if (isKeyJustPressed(GLFW_KEY_ESCAPE, m_inputStates.esc)) {
            handleEscapeKey();
        }

        if (isKeyJustPressed(GLFW_KEY_I, m_inputStates.i)) m_gameState->toggleAI();

        // Action Key (Space/Enter)
        bool action = (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) || (glfwGetKey(m_window, GLFW_KEY_ENTER) == GLFW_PRESS);
        if (action && !m_inputStates.action) handleActionKey();
        m_inputStates.action = action;

        // Contextual Controls
        if (m_gameState->isInMenu()) {
            handleMenuNavigation();
        } else if (!m_gameState->isAIEnabled() && !m_gameState->isGameOver() && !m_gameState->isPaused()) {
            handleGameMovement();
        }
    }

    void handleEscapeKey() {
        if (m_gameState->isInMenu()) glfwSetWindowShouldClose(m_window, true);
        else { m_gameState->setInMenu(true); m_gameState->setShouldReset(true); }
    }

    void handleActionKey() {
        if (m_gameState->isInMenu()) {
            int sel = m_gameState->getMenuSelection();
            if (sel == 0) { m_gameState->setInMenu(false); m_gameState->setShouldReset(true); }
            else if (sel == 1) m_gameState->toggleObstacles();
            else if (sel == 2) m_gameState->toggleAI();
        } else if (m_gameState->isGameOver()) {
            m_gameState->setShouldReset(true);
        } else {
            m_gameState->togglePause();
        }
    }

    void handleMenuNavigation() {
        if (isKeyJustPressed(GLFW_KEY_UP, m_inputStates.up)) 
            m_gameState->setMenuSelection((m_gameState->getMenuSelection() - 1 + 3) % 3);
        if (isKeyJustPressed(GLFW_KEY_DOWN, m_inputStates.down)) 
            m_gameState->setMenuSelection((m_gameState->getMenuSelection() + 1) % 3);
    }

    void handleGameMovement() {
        if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS && m_gameState->getDirection() != Direction::DOWN)
            m_gameState->setDirection(Direction::UP);
        if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS && m_gameState->getDirection() != Direction::UP)
            m_gameState->setDirection(Direction::DOWN);
        if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS && m_gameState->getDirection() != Direction::RIGHT)
            m_gameState->setDirection(Direction::LEFT);
        if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS && m_gameState->getDirection() != Direction::LEFT)
            m_gameState->setDirection(Direction::RIGHT);
    }

    void toggleFullscreen() {
        if (m_isFullscreen) {
            glfwSetWindowMonitor(m_window, nullptr, m_windowedPos.x, m_windowedPos.y, m_windowedSize.x, m_windowedSize.y, 0);
            m_isFullscreen = false;
        } else {
            glfwGetWindowPos(m_window, &m_windowedPos.x, &m_windowedPos.y);
            glfwGetWindowSize(m_window, &m_windowedSize.x, &m_windowedSize.y);
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            m_isFullscreen = true;
        }
    }

    GLFWwindow* m_window = nullptr;
    Renderer m_renderer;
    std::unique_ptr<GameState> m_gameState;
    unsigned int m_screenWidth, m_screenHeight;
    float m_lastFrameTime = 0.0f;

    bool m_isFullscreen = true;
    glm::ivec2 m_windowedPos{100, 100}, m_windowedSize{1280, 720};

    struct {
        bool f11 = false, esc = false, i = false, action = false, up = false, down = false;
    } m_inputStates;
};

int main() {
    try {
        SnakeApplication app;
        app.run();
    } catch (const std::exception& e) {
        spdlog::critical("Application failure: {}", e.what());
        return -1;
    }
    return 0;
}
