#include "game_logic.hpp"
#include "ai_pathfinder.hpp"

std::string getExecutableDir() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count <= 0) return ".";
    return std::filesystem::path(std::string(result, count)).parent_path().string();
}

void HighScoreManager::save() const {
    if (m_path.empty()) return;
    std::ofstream file(m_path);
    if (file.is_open()) {
        for (const auto& entry : m_entries) {
            file << "Score: " << entry.score << "\n";
            file << "AI_Used: " << (entry.aiUsed ? "Yes" : "No") << "\n";
            file << "Obstacles: " << (entry.obstaclesEnabled ? "Yes" : "No") << "\n";
        }
        file.close();
    }
}

void HighScoreManager::load() {
    m_entries.clear();
    if (m_path.empty()) return;
    std::ifstream file(m_path);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            HighScoreEntry entry;
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                try { entry.score = std::stoi(line.substr(pos + 1)); } catch (...) { continue; }
            } else continue;
            if (std::getline(file, line)) entry.aiUsed = (line.find("Yes") != std::string::npos);
            if (std::getline(file, line)) {
                entry.obstaclesEnabled = (line.find("Yes") != std::string::npos);
                m_entries.push_back(entry);
            }
        }
        file.close();
    }
}

bool HighScoreManager::update(int score, bool aiUsed, bool obstaclesEnabled) {
    if (score <= 0) return false;
    bool qualifies = m_entries.size() < 5 || score > m_entries.back().score;
    if (qualifies) {
        m_entries.push_back({score, aiUsed, obstaclesEnabled});
        std::sort(m_entries.begin(), m_entries.end(), [](const HighScoreEntry& a, const HighScoreEntry& b) {
            return a.score > b.score;
        });
        if (m_entries.size() > 5) m_entries.resize(5);
        save();
        return true;
    }
    return false;
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
    emptyPositions.reserve(GRID_WIDTH * GRID_HEIGHT);

    for (int x = 0; x < GRID_WIDTH; ++x) {
        for (int y = 0; y < GRID_HEIGHT; ++y) {
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
    m_snake.push_front({GRID_WIDTH / 2, GRID_HEIGHT / 2});
    m_snake.push_back({GRID_WIDTH / 2 - 1, GRID_HEIGHT / 2});
    m_snake.push_back({GRID_WIDTH / 2 - 2, GRID_HEIGHT / 2});
    m_snakeDirection = Direction::RIGHT;
    m_gameOver = false;
    m_shouldReset = false;
    m_paused = false;
    m_showingRules = false;
    m_showingHighScores = false;
    m_showingControls = false;
    m_score = 0;
    m_obstacles.clear();
    m_timeBetweenUpdates = 0.1f;
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
    m_banana.timeLeft = 5.0f;
    generateGoldenApple();
    m_goldenApple.timeLeft = 7.0f;
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
            m_banana.timeLeft = 5.0f;
        }
    }

    if (m_goldenApple.active) {
        m_goldenApple.timeLeft -= deltaTime;
        if (m_goldenApple.timeLeft <= 0.0f) m_goldenApple.active = false;
    } else {
        if (std::uniform_real_distribution<float>(0, 1)(m_rng) < 0.002f) {
            generateGoldenApple();
            m_goldenApple.timeLeft = 7.0f;
        }
    }
}

void GameState::performTick() {
    if (m_aiEnabled) {
        m_snakeDirection = AIPathfinder::getNextMove(m_snake.front(), m_food, m_snake, m_obstacles, m_snakeDirection);
        m_aiUsedThisRun = true;
    }

    glm::vec2 newHead = m_snake.front();
    if (m_snakeDirection == Direction::UP)    newHead.y += 1;
    if (m_snakeDirection == Direction::DOWN)  newHead.y -= 1;
    if (m_snakeDirection == Direction::LEFT)  newHead.x -= 1;
    if (m_snakeDirection == Direction::RIGHT) newHead.x += 1;

    // Wall collision
    if (newHead.x < 0 || newHead.x >= GRID_WIDTH || newHead.y < 0 || newHead.y >= GRID_HEIGHT) {
        m_gameOver = true;
    }

    // Obstacle/Self collision
    for (const auto& obs : m_obstacles) if (newHead == obs) { m_gameOver = true; break; }
    for (size_t i = 1; i < m_snake.size(); ++i) if (newHead == m_snake[i]) { m_gameOver = true; break; }

    if (!m_gameOver) {
        m_snake.push_front(newHead);

        if (newHead == m_food) {
            m_score++;
            m_timeBetweenUpdates = std::max(0.03f, m_timeBetweenUpdates * 0.98f);
            if (m_obstaclesEnabled && std::uniform_real_distribution<float>(0,1)(m_rng) < 0.5f) generateObstacle();
            generateNewFood();
        } else if (m_banana.active && newHead == m_banana.pos) {
            m_score += 2;
            m_banana.active = false;
            m_bananaSpawnTimer = 10.0f;
            if (m_obstaclesEnabled) generateObstacle();
        } else if (m_goldenApple.active && newHead == m_goldenApple.pos) {
            m_score += 5;
            m_goldenApple.active = false;
            if (m_obstaclesEnabled) { generateObstacle(); generateObstacle(); }
        } else {
            m_snake.pop_back();
        }
    }
}

void GameState::update(float /*currentTime*/, float deltaTime) {
    if (m_gameOver || m_paused || m_inMenu) return;

    updateTimers(deltaTime);

    m_tickAccumulator += deltaTime;
    while (m_tickAccumulator >= m_timeBetweenUpdates)
    {
        m_tickAccumulator -= m_timeBetweenUpdates;
        performTick();
    }

    if (m_gameOver && !m_highScoreChecked) updateHighScores();
}