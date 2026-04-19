#include "game_logic.hpp"
#include "ai_pathfinder.hpp"
#include <filesystem>
#include <unistd.h>
#include <limits.h>

std::string getExecutableDir() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count <= 0) return ".";
    return std::filesystem::path(std::string(result, count)).parent_path().string();
}

GameState::GameState() : m_rng(std::random_device{}()) {
    reset();
}

void GameState::updateHighScores() {
    if (m_highScoreChecked || m_score <= 0) return;
    m_isNewTopScore = m_highScores.update(m_score, m_aiUsedThisRun, m_obstaclesEnabled);
    m_highScoreChecked = true;
}

bool GameState::isPositionOccupied(glm::vec2 pos, const std::vector<glm::vec2>& extraExclusions) const {
    for (const auto& s : m_snake) if (pos == s) return true;
    for (const auto& o : m_obstacles) if (pos == o) return true;
    for (const auto& item : extraExclusions) if (pos == item) return true;
    return false;
}

glm::vec2 GameState::getRandomEmptyPosition(const std::vector<glm::vec2>& extraExclusions) {
    std::vector<glm::vec2> emptyPositions;
    emptyPositions.reserve(Config::GRID_WIDTH * Config::GRID_HEIGHT);

    for (int x = 0; x < Config::GRID_WIDTH; ++x) {
        for (int y = 0; y < Config::GRID_HEIGHT; ++y) {
            glm::vec2 p(x, y);
            if (!isPositionOccupied(p, extraExclusions)) {
                emptyPositions.push_back(p);
            }
        }
    }

    if (emptyPositions.empty()) return {-1, -1};
    std::uniform_int_distribution<size_t> dist(0, emptyPositions.size() - 1);
    return emptyPositions[dist(m_rng)];
}

void GameState::generateNewFood() {
    std::vector<glm::vec2> others;
    if (m_banana.active) others.push_back(m_banana.pos);
    if (m_goldenApple.active) others.push_back(m_goldenApple.pos);
    m_food = getRandomEmptyPosition(others);
}

void GameState::generateBanana() {
    std::vector<glm::vec2> others = {m_food};
    if (m_goldenApple.active) others.push_back(m_goldenApple.pos);
    m_banana.pos = getRandomEmptyPosition(others);
    m_banana.active = true;
}

void GameState::generateGoldenApple() {
    std::vector<glm::vec2> others = {m_food};
    if (m_banana.active) others.push_back(m_banana.pos);
    m_goldenApple.pos = getRandomEmptyPosition(others);
    m_goldenApple.active = true;
}

void GameState::generateObstacle() {
    std::vector<glm::vec2> others = {m_food};
    if (m_banana.active) others.push_back(m_banana.pos);
    if (m_goldenApple.active) others.push_back(m_goldenApple.pos);
    m_obstacles.push_back(getRandomEmptyPosition(others));
}

void GameState::reset(float /*currentTime*/) {
    m_snake.clear();
    m_snake.push_front({Config::GRID_WIDTH / 2, Config::GRID_HEIGHT / 2});
    m_snake.push_back({Config::GRID_WIDTH / 2 - 1, Config::GRID_HEIGHT / 2});
    m_snake.push_back({Config::GRID_WIDTH / 2 - 2, Config::GRID_HEIGHT / 2});
    m_snakeDirection = Direction::RIGHT;
    m_gameOver = false;
    m_shouldReset = false;
    m_paused = false;
    m_score = 0;
    m_obstacles.clear();
    if (m_initialSpeedSetting == 0) { // Easy
        m_timeBetweenUpdates = Config::INITIAL_STEP_TIME * 1.5f;
    } else if (m_initialSpeedSetting == 1) { // Medium
        m_timeBetweenUpdates = Config::INITIAL_STEP_TIME;
    } else { // Hard
        m_timeBetweenUpdates = Config::INITIAL_STEP_TIME * 0.7f;
    }
    m_tickAccumulator = 0.0f;
    m_aiUsedThisRun = false;
    m_highScoreChecked = false;
    m_isNewTopScore = false;
    m_banana.active = false;
    m_banana.timeLeft = 0.0f;
    m_goldenApple.active = false;
    m_goldenApple.timeLeft = 0.0f;
    generateNewFood();
    generateBanana();
    m_banana.timeLeft = Config::BANANA_TIME;
    generateGoldenApple();
    m_goldenApple.timeLeft = Config::GOLDEN_APPLE_TIME;
    m_bananaSpawnTimer = 0.0f;
}

void GameState::updateTimers(float deltaTime) {
    if (m_banana.active) {
        m_banana.timeLeft -= deltaTime;
        if (m_banana.timeLeft <= 0.0f) {
            m_banana.active = false;
            m_bananaSpawnTimer = 10.0f; // Seconds until next banana
        }
    } else {
        m_bananaSpawnTimer -= deltaTime;
        if (m_bananaSpawnTimer <= 0.0f) {
            generateBanana();
            m_banana.timeLeft = Config::BANANA_TIME;
        }
    }

    if (m_goldenApple.active) {
        m_goldenApple.timeLeft -= deltaTime;
        if (m_goldenApple.timeLeft <= 0.0f) m_goldenApple.active = false;
    } else {
        if (std::uniform_real_distribution<float>(0, 1)(m_rng) < 0.002f) {
            generateGoldenApple();
            m_goldenApple.timeLeft = Config::GOLDEN_APPLE_TIME;
        }
    }
}

void GameState::performTick() {
    if (m_aiEnabled) {
        m_snakeDirection = AIPathfinder::getNextMove(m_snake.front(), m_food, m_snake, m_obstacles, m_snakeDirection);
        m_aiUsedThisRun = true;
    }

    glm::vec2 newHead = m_snake.front() + getDirectionOffset(m_snakeDirection);

    if (!isInsideGrid(newHead)) {
        m_gameOver = true;
    }

    // Obstacle/Self collision
    for (const auto& obs : m_obstacles) if (newHead == obs) { m_gameOver = true; break; }
    for (size_t i = 1; i < m_snake.size(); ++i) if (newHead == m_snake[i]) { m_gameOver = true; break; }

    if (!m_gameOver) {
        m_snake.push_front(newHead);

        if (newHead == m_food) {
            m_score += Config::SCORE_APPLE;
            m_timeBetweenUpdates = std::max(Config::MIN_STEP_TIME, m_timeBetweenUpdates * Config::SPEED_INCREMENT);
            if (m_obstaclesEnabled && std::uniform_real_distribution<float>(0,1)(m_rng) < 0.5f) generateObstacle();
            generateNewFood();
        } else if (m_banana.active && newHead == m_banana.pos) {
            m_score += Config::SCORE_BANANA;
            m_banana.active = false;
            m_bananaSpawnTimer = 10.0f;
            if (m_obstaclesEnabled) generateObstacle();
        } else if (m_goldenApple.active && newHead == m_goldenApple.pos) {
            m_score += Config::SCORE_GOLDEN_APPLE;
            m_goldenApple.active = false;
            if (m_obstaclesEnabled) { generateObstacle(); generateObstacle(); }
        } else {
            m_snake.pop_back();
        }
    }
}

void GameState::update(float /*currentTime*/, float deltaTime) {
    if (m_gameOver || m_paused || m_currentScene != GameScene::Gameplay) return;

    updateTimers(deltaTime);

    m_tickAccumulator += deltaTime;
    while (m_tickAccumulator >= m_timeBetweenUpdates)
    {
        m_tickAccumulator -= m_timeBetweenUpdates;
        performTick();
    }

    if (m_gameOver && !m_highScoreChecked) updateHighScores();
}