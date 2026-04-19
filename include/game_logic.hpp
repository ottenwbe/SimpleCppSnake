#pragma once
#include <vector>
#include <deque>
#include <queue>
#include <random>
#include <algorithm>
#include <string>
#include <glm/glm.hpp>
#include "highscore_manager.hpp"

namespace Config {
    constexpr int GRID_WIDTH = 30;
    constexpr int GRID_HEIGHT = 15;
    constexpr float INITIAL_STEP_TIME = 0.1f;
    constexpr float MIN_STEP_TIME = 0.03f;
    constexpr float SPEED_INCREMENT = 0.98f;
    
    constexpr float BANANA_TIME = 5.0f;
    constexpr float GOLDEN_APPLE_TIME = 7.0f;
    constexpr int SCORE_APPLE = 1;
    constexpr int SCORE_BANANA = 2;
    constexpr int SCORE_GOLDEN_APPLE = 5;
}

enum class Direction { UP, DOWN, LEFT, RIGHT };
enum class GameScene { Menu, Rules, HighScores, Controls, Settings, Gameplay };

inline glm::vec2 getDirectionOffset(Direction dir) {
    switch (dir) {
        case Direction::UP:    return {0, 1};
        case Direction::DOWN:  return {0, -1};
        case Direction::LEFT:  return {-1, 0};
        case Direction::RIGHT: return {1, 0};
    }
    return {0, 0};
}

inline bool isInsideGrid(glm::vec2 pos) {
    return pos.x >= 0 && pos.x < Config::GRID_WIDTH && pos.y >= 0 && pos.y < Config::GRID_HEIGHT;
}

struct Banana {
    glm::vec2 pos;
    float timeLeft = 0.0f;
    bool active = false;
};

struct GoldenApple {
    glm::vec2 pos;
    float timeLeft = 0.0f;
    bool active = false;
};

class GameState {
public:
    GameState();

    void reset(float currentTime = 0.0f);
    void update(float currentTime, float deltaTime);
    void updateHighScores();

    // Getters for Rendering and UI
    float getTimeBetweenUpdates() const { return m_timeBetweenUpdates; }
    const std::deque<glm::vec2>& getSnake() const { return m_snake; }
    Direction getDirection() const { return m_snakeDirection; }
    glm::vec2 getFood() const { return m_food; }
    const std::vector<glm::vec2>& getObstacles() const { return m_obstacles; }
    const Banana& getBanana() const { return m_banana; }
    const GoldenApple& getGoldenApple() const { return m_goldenApple; }
    bool isGameOver() const { return m_gameOver; }
    bool shouldReset() const { return m_shouldReset; }
    bool isPaused() const { return m_paused; }
    GameScene getScene() const { return m_currentScene; }
    bool isAIEnabled() const { return m_aiEnabled; }
    int getScore() const { return m_score; }
    int getMenuSelection() const { return m_menuSelection; }
    bool areObstaclesEnabled() const { return m_obstaclesEnabled; }
    bool isNewTopScore() const { return m_isNewTopScore; }
    HighScoreManager& getHighScoreManager() { return m_highScores; }

    // Mutators for Input Handling
    void setDirection(Direction dir) { if (!m_paused && !m_gameOver) m_snakeDirection = dir; }
    void setShouldReset(bool val) { m_shouldReset = val; }
    void togglePause() { if (!m_gameOver && m_currentScene == GameScene::Gameplay) m_paused = !m_paused; }
    void setScene(GameScene scene) { m_currentScene = scene; }
    void toggleAI() { m_aiEnabled = !m_aiEnabled; }
    void setMenuSelection(int sel) { m_menuSelection = sel; }
    void toggleObstacles() { m_obstaclesEnabled = !m_obstaclesEnabled; }

    int getInitialSpeedSetting() const { return m_initialSpeedSetting; }
    void setInitialSpeedSetting(int speed) { m_initialSpeedSetting = speed; }
    // Test Helpers
    void debugSetSnake(const std::deque<glm::vec2>& s) { m_snake = s; }
    void debugSetFood(glm::vec2 f) { m_food = f; }
    void debugSetObstacles(const std::vector<glm::vec2>& o) { m_obstacles = o; }
    void debugSetTickAccumulator(float t) { m_tickAccumulator = t; }
    void debugSetBananaSpawnTimer(float t) { m_bananaSpawnTimer = t; }
    void debugSetBanana(const Banana& b) { m_banana = b; }
    void debugSetGoldenApple(const GoldenApple& g) { m_goldenApple = g; }
    void debugSetScore(int s) { m_score = s; }
    void debugSetGameOver(bool go) { m_gameOver = go; }
    void debugSetHighScoreChecked(bool checked) { m_highScoreChecked = checked; }
    void debugSetAiUsedThisRun(bool used) { m_aiUsedThisRun = used; }
    void debugSetObstaclesEnabled(bool e) { m_obstaclesEnabled = e; }

private:
    void updateTimers(float deltaTime);
    void performTick();
    bool isPositionOccupied(glm::vec2 pos, const std::vector<glm::vec2>& extraExclusions = {}) const;
    glm::vec2 getRandomEmptyPosition(const std::vector<glm::vec2>& extraExclusions);
    void generateNewFood();
    void generateBanana();
    void generateGoldenApple();
    void generateObstacle();

    std::deque<glm::vec2> m_snake;
    Direction m_snakeDirection = Direction::RIGHT;
    glm::vec2 m_food;
    std::vector<glm::vec2> m_obstacles;
    Banana m_banana;
    GoldenApple m_goldenApple;
    float m_bananaSpawnTimer = 0.0f;
    bool m_gameOver = false;
    bool m_shouldReset = false;
    bool m_paused = false;
    GameScene m_currentScene = GameScene::Menu;
    bool m_aiEnabled = false;
    int m_score = 0;
    float m_tickAccumulator = 0.0f;
    float m_timeBetweenUpdates = 0.1f;
    HighScoreManager m_highScores;
    bool m_obstaclesEnabled = true;
    int m_menuSelection = 0;
    bool m_aiUsedThisRun = false;
    int m_initialSpeedSetting = 1; // 0: Easy, 1: Medium, 2: Hard
    bool m_highScoreChecked = false;
    bool m_isNewTopScore = false;
    std::mt19937 m_rng;
};

// Path Utility
std::string getExecutableDir();
