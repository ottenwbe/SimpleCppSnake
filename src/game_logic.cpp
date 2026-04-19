#include "game_logic.hpp"

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

GameState::GameState() {
    reset();
}

void GameState::updateHighScores() {
    if (m_highScoreChecked || m_score <= 0) return;
    m_isNewTopScore = m_highScores.update(m_score, m_aiUsedThisRun, m_obstaclesEnabled);
    m_highScoreChecked = true;
}

glm::vec2 GameState::getRandomEmptyPosition(const std::vector<glm::vec2>& extraExclusions) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribX(0, GRID_WIDTH - 1);
    std::uniform_int_distribution<> distribY(0, GRID_HEIGHT - 1);
    glm::vec2 pos;
    bool collision;
    do {
        collision = false;
        pos = {distribX(gen), distribY(gen)};
        for (const auto& s : m_snake) if (pos == s) { collision = true; break; }
        if (collision) continue;
        for (const auto& o : m_obstacles) if (pos == o) { collision = true; break; }
        if (collision) continue;
        for (const auto& item : extraExclusions) if (pos == item) { collision = true; break; }
    } while (collision);
    return pos;
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

void GameState::reset(float currentTime) {
    m_snake.clear();
    m_snake.push_front({GRID_WIDTH / 2, GRID_HEIGHT / 2});
    m_snake.push_back({GRID_WIDTH / 2 - 1, GRID_HEIGHT / 2});
    m_snake.push_back({GRID_WIDTH / 2 - 2, GRID_HEIGHT / 2});
    m_snakeDirection = Direction::RIGHT;
    m_gameOver = false;
    m_shouldReset = false;
    m_paused = false;
    m_score = 0;
    m_obstacles.clear();
    m_timeBetweenUpdates = 0.1f;
    m_lastUpdateTime = currentTime;
    m_aiUsedThisRun = false;
    m_highScoreChecked = false;
    m_isNewTopScore = false;
    m_banana.active = true;
    m_banana.spawnTime = currentTime;
    m_goldenApple.active = true;
    m_goldenApple.spawnTime = currentTime;
    generateNewFood();
    generateBanana();
    generateGoldenApple();
    m_nextBananaSpawnTime = currentTime;
}

Direction GameState::getAIDirection() {
    glm::vec2 head = m_snake.front();
    struct Node { glm::vec2 pos; Direction firstMove; };
    std::queue<Node> q;
    bool blocked[GRID_WIDTH][GRID_HEIGHT] = {};
    for (const auto& s : m_snake) blocked[(int)s.x][(int)s.y] = true;
    for (const auto& o : m_obstacles) blocked[(int)o.x][(int)o.y] = true;
    blocked[(int)head.x][(int)head.y] = true;
    std::vector<Direction> dirs = {Direction::UP, Direction::DOWN, Direction::LEFT, Direction::RIGHT};
    for (Direction d : dirs) {
        glm::vec2 next = head;
        if (d == Direction::UP) next.y += 1;
        else if (d == Direction::DOWN) next.y -= 1;
        else if (d == Direction::LEFT) next.x -= 1;
        else if (d == Direction::RIGHT) next.x += 1;
        if (next.x >= 0 && next.x < GRID_WIDTH && next.y >= 0 && next.y < GRID_HEIGHT && !blocked[(int)next.x][(int)next.y]) {
            if (next == m_food) return d;
            q.push({next, d});
            blocked[(int)next.x][(int)next.y] = true;
        }
    }
    while (!q.empty()) {
        Node curr = q.front(); q.pop();
        for (Direction d : dirs) {
            glm::vec2 next = curr.pos;
            if (d == Direction::UP) next.y += 1;
            else if (d == Direction::DOWN) next.y -= 1;
            else if (d == Direction::LEFT) next.x -= 1;
            else if (d == Direction::RIGHT) next.x += 1;
            if (next == m_food) return curr.firstMove;
            if (next.x >= 0 && next.x < GRID_WIDTH && next.y >= 0 && next.y < GRID_HEIGHT && !blocked[(int)next.x][(int)next.y]) {
                blocked[(int)next.x][(int)next.y] = true;
                q.push({next, curr.firstMove});
            }
        }
    }
    return m_snakeDirection;
}

void GameState::update(float currentTime) {
    // Handle Banana Expiry and Spawning
    if (m_banana.active && (currentTime - m_banana.spawnTime) > 5.0f) {
        m_banana.active = false;
        m_nextBananaSpawnTime = currentTime + 10.0f;
    }

    // Handle Golden Apple Expiry
    if (m_goldenApple.active && (currentTime - m_goldenApple.spawnTime) > 7.0f) {
        m_goldenApple.active = false;
    }

    // Fixed-timestep update
    if (currentTime - m_lastUpdateTime >= m_timeBetweenUpdates && !m_gameOver && !m_paused && !m_inMenu)
    {
        m_lastUpdateTime = currentTime;

        if (m_aiEnabled) {
            m_snakeDirection = getAIDirection();
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
                static std::default_random_engine rd(std::random_device{}());
                if (m_obstaclesEnabled && std::uniform_real_distribution<float>(0,1)(rd) < 0.5f) generateObstacle();
                generateNewFood();
            } else if (m_banana.active && newHead == m_banana.pos) {
                m_score += 2;
                m_banana.active = false;
                m_nextBananaSpawnTime = currentTime + 10.0f;
                if (m_obstaclesEnabled) generateObstacle();
            } else if (m_goldenApple.active && newHead == m_goldenApple.pos) {
                m_score += 5;
                m_goldenApple.active = false;
                if (m_obstaclesEnabled) { generateObstacle(); generateObstacle(); }
            } else {
                m_snake.pop_back();
            }

            // Spawning Logic
            if (!m_banana.active && currentTime >= m_nextBananaSpawnTime && !m_paused) {
                generateBanana();
                m_banana.spawnTime = currentTime;
            }
            if (!m_goldenApple.active && !m_paused) {
                static std::default_random_engine rd(std::random_device{}());
                if (std::uniform_real_distribution<float>(0,1)(rd) < 0.002f) {
                    generateGoldenApple();
                    m_goldenApple.spawnTime = currentTime;
                }
            }
        }
    }

    if (m_gameOver && !m_highScoreChecked) updateHighScores();
}