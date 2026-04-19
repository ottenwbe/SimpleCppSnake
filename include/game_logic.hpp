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

struct GameState {
    std::deque<glm::vec2> snake;
    Direction snakeDirection = Direction::RIGHT;
    glm::vec2 food;
    std::vector<glm::vec2> obstacles;
    Banana banana;
    GoldenApple goldenApple;
    float nextBananaSpawnTime = 0.0f;
    bool gameOver = false;
    bool shouldReset = false;
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
    bool isNewTopScore = false;

    void updateHighScores();
};

// Path Utility
std::string getExecutableDir();

// Grid Logic
glm::vec2 getRandomEmptyPosition(const GameState& state, const std::vector<glm::vec2>& extraExclusions);
void generateNewFood(GameState& state);
void generateBanana(GameState& state);
void generateGoldenApple(GameState& state);
void generateObstacle(GameState& state);
void resetGame(GameState& state);

// AI Logic
Direction getAIDirection(const GameState& state);
void updateGame(GameState& state, float currentTime);