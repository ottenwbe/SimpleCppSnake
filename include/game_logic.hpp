#pragma once
#include <vector>
#include <deque>
#include <queue>
#include <random>
#include <algorithm>
#include <string>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <unistd.h>
#include <limits.h>

// Constants
const int GRID_WIDTH = 30;
const int GRID_HEIGHT = 15;

enum class Direction { UP, DOWN, LEFT, RIGHT };

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

struct HighScoreEntry {
    int score;
    bool aiUsed;
    bool obstaclesEnabled;
};

class HighScoreManager {
public:
    HighScoreManager() = default;
    explicit HighScoreManager(const std::string& path) : m_path(path) {}

    void setPath(const std::string& p) { m_path = p; }
    void load();
    void save() const;
    bool update(int score, bool aiUsed, bool obstaclesEnabled);
    const std::vector<HighScoreEntry>& getEntries() const { return m_entries; }

private:
    std::vector<HighScoreEntry> m_entries;
    std::string m_path;
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
    bool isInMenu() const { return m_inMenu; }
    bool isShowingRules() const { return m_showingRules; }
    bool isShowingHighScores() const { return m_showingHighScores; }
    bool isShowingControls() const { return m_showingControls; }
    bool isAIEnabled() const { return m_aiEnabled; }
    int getScore() const { return m_score; }
    int getMenuSelection() const { return m_menuSelection; }
    bool areObstaclesEnabled() const { return m_obstaclesEnabled; }
    bool isNewTopScore() const { return m_isNewTopScore; }
    HighScoreManager& getHighScoreManager() { return m_highScores; }

    // Mutators for Input Handling
    void setDirection(Direction dir) { if (!m_paused && !m_gameOver) m_snakeDirection = dir; }
    void setShouldReset(bool val) { m_shouldReset = val; }
    void togglePause() { if (!m_gameOver && !m_inMenu) m_paused = !m_paused; }
    void setInMenu(bool val) { m_inMenu = val; }
    void setShowingRules(bool val) { m_showingRules = val; }
    void setShowingHighScores(bool val) { m_showingHighScores = val; }
    void setShowingControls(bool val) { m_showingControls = val; }
    void toggleAI() { m_aiEnabled = !m_aiEnabled; }
    void setMenuSelection(int sel) { m_menuSelection = sel; }
    void toggleObstacles() { m_obstaclesEnabled = !m_obstaclesEnabled; }

    Direction getAIDirection();

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
    bool m_inMenu = true;
    bool m_showingRules = false;
    bool m_showingHighScores = false;
    bool m_showingControls = false;
    bool m_aiEnabled = false;
    int m_score = 0;
    float m_tickAccumulator = 0.0f;
    float m_timeBetweenUpdates = 0.1f;
    HighScoreManager m_highScores;
    bool m_obstaclesEnabled = true;
    int m_menuSelection = 0;
    bool m_aiUsedThisRun = false;
    bool m_highScoreChecked = false;
    bool m_isNewTopScore = false;
};

// Path Utility
std::string getExecutableDir();
