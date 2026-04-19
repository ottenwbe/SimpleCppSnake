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
    float spawnTime = 0.0f;
    bool active = false;
};

struct GoldenApple {
    glm::vec2 pos;
    float spawnTime = 0.0f;
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
    explicit HighScoreManager(const std::string& path) : path(path) {}

    void setPath(const std::string& p) { path = p; }
    void load();
    void save() const;
    bool update(int score, bool aiUsed, bool obstaclesEnabled);
    const std::vector<HighScoreEntry>& getEntries() const { return entries; }

private:
    std::vector<HighScoreEntry> entries;
    std::string path;
};

class GameState {
public:
    GameState();

    void reset();
    void update(float currentTime);
    void updateHighScores();

    // Getters for Rendering and UI
    float getTimeBetweenUpdates() const { return timeBetweenUpdates; }
    const std::deque<glm::vec2>& getSnake() const { return snake; }
    Direction getDirection() const { return snakeDirection; }
    glm::vec2 getFood() const { return food; }
    const std::vector<glm::vec2>& getObstacles() const { return obstacles; }
    const Banana& getBanana() const { return banana; }
    const GoldenApple& getGoldenApple() const { return goldenApple; }
    bool isGameOver() const { return gameOver; }
    bool shouldReset() const { return m_shouldReset; }
    bool isPaused() const { return paused; }
    bool isInMenu() const { return inMenu; }
    bool isAIEnabled() const { return aiEnabled; }
    int getScore() const { return score; }
    int getMenuSelection() const { return menuSelection; }
    bool areObstaclesEnabled() const { return obstaclesEnabled; }
    bool isNewTopScore() const { return m_isNewTopScore; }
    HighScoreManager& getHighScoreManager() { return highScores; }

    // Mutators for Input Handling
    void setDirection(Direction dir) { if (!paused && !gameOver) snakeDirection = dir; }
    void setShouldReset(bool val) { m_shouldReset = val; }
    void togglePause() { if (!gameOver && !inMenu) paused = !paused; }
    void setInMenu(bool val) { inMenu = val; }
    void toggleAI() { aiEnabled = !aiEnabled; }
    void setMenuSelection(int sel) { menuSelection = sel; }
    void toggleObstacles() { obstaclesEnabled = !obstaclesEnabled; }

    Direction getAIDirection();

    // Test Helpers
    void debugSetSnake(const std::deque<glm::vec2>& s) { snake = s; }
    void debugSetFood(glm::vec2 f) { food = f; }
    void debugSetObstacles(const std::vector<glm::vec2>& o) { obstacles = o; }
    void debugSetLastUpdateTime(float t) { lastUpdateTime = t; }
    void debugSetNextBananaSpawnTime(float t) { nextBananaSpawnTime = t; }
    void debugSetBanana(const Banana& b) { banana = b; }
    void debugSetGoldenApple(const GoldenApple& g) { goldenApple = g; }
    void debugSetScore(int s) { score = s; }
    void debugSetGameOver(bool go) { gameOver = go; }
    void debugSetHighScoreChecked(bool checked) { highScoreChecked = checked; }
    void debugSetAiUsedThisRun(bool used) { aiUsedThisRun = used; }
    void debugSetObstaclesEnabled(bool e) { obstaclesEnabled = e; }

private:
    glm::vec2 getRandomEmptyPosition(const std::vector<glm::vec2>& extraExclusions);
    void generateNewFood();
    void generateBanana();
    void generateGoldenApple();
    void generateObstacle();

    std::deque<glm::vec2> snake;
    Direction snakeDirection = Direction::RIGHT;
    glm::vec2 food;
    std::vector<glm::vec2> obstacles;
    Banana banana;
    GoldenApple goldenApple;
    float nextBananaSpawnTime = 0.0f;
    bool gameOver = false;
    bool m_shouldReset = false;
    bool paused = false;
    bool inMenu = true;
    bool aiEnabled = false;
    int score = 0;
    float lastUpdateTime = 0.0f;
    float timeBetweenUpdates = 0.1f;
    HighScoreManager highScores;
    bool obstaclesEnabled = true;
    int menuSelection = 0;
    bool aiUsedThisRun = false;
    bool highScoreChecked = false;
    bool m_isNewTopScore = false;
};

// Path Utility
std::string getExecutableDir();
