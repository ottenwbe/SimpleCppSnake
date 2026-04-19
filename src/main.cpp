#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <memory>
#include <filesystem>
#include "game_logic.hpp"
#include "renderer.hpp"
#include "input_manager.hpp"
#include <glm/glm.hpp>

class SnakeApplication {
public:
    SnakeApplication() {
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        spdlog::info("Initializing Viperpath...");

        setupWindow();

        if (!m_renderer.init(m_screenWidth, m_screenHeight)) {
            throw std::runtime_error("Failed to initialize Renderer");
        }

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
            int winW, winH;
            updateViewportAndProjection(winW, winH);
            m_renderer.render(*m_gameState, winW, winH);

            glfwSwapBuffers(m_window);
        }
    }

private:
    void setupWindow() {
        if (!glfwInit()) throw std::runtime_error("Failed to initialize GLFW");

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        m_screenWidth = mode->width;
        m_screenHeight = mode->height;

        m_window = glfwCreateWindow(m_screenWidth, m_screenHeight, "Viperpath", monitor, nullptr);
        if (!m_window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(m_window);
        // VSync on for smooth movement
        glfwSwapInterval(1);

        m_gameState = std::make_unique<GameState>();
    }

    void updateViewportAndProjection(int& winW, int& winH) {
        glfwGetFramebufferSize(m_window, &winW, &winH);
        if (static_cast<unsigned int>(winW) != m_screenWidth || static_cast<unsigned int>(winH) != m_screenHeight) {
            m_screenWidth = winW;
            m_screenHeight = winH;
            glViewport(0, 0, winW, winH);
            m_renderer.updateProjection(winW, winH);
        }
    }

    void processInput() {
        glfwPollEvents();
        m_input.update(m_window, {GLFW_KEY_F11, GLFW_KEY_ESCAPE, GLFW_KEY_I, GLFW_KEY_SPACE, 
                                  GLFW_KEY_ENTER, GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT});

        if (m_input.isJustPressed(GLFW_KEY_F11)) toggleFullscreen(); // Global control
        if (m_input.isJustPressed(GLFW_KEY_ESCAPE)) handleEscapeKey(); // Global control
        if (m_input.isJustPressed(GLFW_KEY_I)) m_gameState->toggleAI();

        if (m_input.isJustPressed(GLFW_KEY_SPACE) || m_input.isJustPressed(GLFW_KEY_ENTER)) handleActionKey();

        // Contextual Controls
        if (m_gameState->getScene() != GameScene::Gameplay) {
            handleMenuNavigation();
        } else if (!m_gameState->isAIEnabled() && !m_gameState->isGameOver() && !m_gameState->isPaused()) {
            handleGameMovement();
        }
    }

    void handleEscapeKey() {
        if (m_gameState->getScene() == GameScene::Settings ||
            m_gameState->getScene() == GameScene::Rules ||
            m_gameState->getScene() == GameScene::HighScores ||
            m_gameState->getScene() == GameScene::Controls) {
            m_gameState->setScene(GameScene::Menu);
        } else if (m_gameState->getScene() == GameScene::Menu) glfwSetWindowShouldClose(m_window, true);
        else { m_gameState->setScene(GameScene::Menu); m_gameState->setShouldReset(true); }
    }

    void handleActionKey() {
        switch (m_gameState->getScene()) {
        case GameScene::Gameplay:
            if (m_gameState->isGameOver()) {
                m_gameState->setShouldReset(true);
            } else {
                m_gameState->togglePause();
            }
            break;
        case GameScene::Menu:
        {
            int sel = m_gameState->getMenuSelection();
            switch (sel) {
                case 0: m_gameState->setScene(GameScene::Gameplay); m_gameState->setShouldReset(true); break;
                case 1: m_gameState->toggleObstacles(); break;
                case 2: m_gameState->toggleAI(); break;
                case 3: m_gameState->setScene(GameScene::Rules); break;
                case 4: m_gameState->setScene(GameScene::HighScores); break;
                case 5: m_gameState->setScene(GameScene::Controls); break;
                case 6: m_gameState->setScene(GameScene::Settings); break;
                case 7: glfwSetWindowShouldClose(m_window, true); break;
            }
            break;
        }
        default:
            break;
        }
    }

    void handleMenuNavigation() {
        if (m_gameState->getScene() == GameScene::Menu) {
            if (m_input.isJustPressed(GLFW_KEY_UP))
                m_gameState->setMenuSelection((m_gameState->getMenuSelection() - 1 + 8) % 8); // 8 total menu items now
            if (m_input.isJustPressed(GLFW_KEY_DOWN))
                m_gameState->setMenuSelection((m_gameState->getMenuSelection() + 1) % 8);
        } else if (m_gameState->getScene() == GameScene::Settings) {
            int currentSpeed = m_gameState->getInitialSpeedSetting();
            if (m_input.isJustPressed(GLFW_KEY_LEFT)) {
                m_gameState->setInitialSpeedSetting(std::max(0, currentSpeed - 1));
            }
            if (m_input.isJustPressed(GLFW_KEY_RIGHT)) {
                m_gameState->setInitialSpeedSetting(std::min(2, currentSpeed + 1));
            }
        }
    }

    void handleGameMovement() {
        if (m_input.isDown(GLFW_KEY_UP) && m_gameState->getDirection() != Direction::DOWN)
            m_gameState->setDirection(Direction::UP);
        if (m_input.isDown(GLFW_KEY_DOWN) && m_gameState->getDirection() != Direction::UP)
            m_gameState->setDirection(Direction::DOWN);
        if (m_input.isDown(GLFW_KEY_LEFT) && m_gameState->getDirection() != Direction::RIGHT)
            m_gameState->setDirection(Direction::LEFT);
        if (m_input.isDown(GLFW_KEY_RIGHT) && m_gameState->getDirection() != Direction::LEFT)
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
    InputManager m_input;

    bool m_isFullscreen = true;
    glm::ivec2 m_windowedPos{100, 100}, m_windowedSize{1280, 720};
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
