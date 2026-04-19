#include "game_logic.hpp"

std::string getExecutableDir() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count <= 0) return ".";
    return std::filesystem::path(std::string(result, count)).parent_path().string();
}

void HighScoreManager::save() const {
    if (path.empty()) return;
    std::ofstream file(path);
    if (file.is_open()) {
        for (const auto& entry : entries) {
            file << "Score: " << entry.score << "\n";
            file << "AI_Used: " << (entry.aiUsed ? "Yes" : "No") << "\n";
            file << "Obstacles: " << (entry.obstaclesEnabled ? "Yes" : "No") << "\n";
        }
        file.close();
    }
}

void HighScoreManager::load() {
    entries.clear();
    if (path.empty()) return;
    std::ifstream file(path);
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
                entries.push_back(entry);
            }
        }
        file.close();
    }
}

bool HighScoreManager::update(int score, bool aiUsed, bool obstaclesEnabled) {
    if (score <= 0) return false;
    bool qualifies = entries.size() < 5 || score > entries.back().score;
    if (qualifies) {
        entries.push_back({score, aiUsed, obstaclesEnabled});
        std::sort(entries.begin(), entries.end(), [](const HighScoreEntry& a, const HighScoreEntry& b) {
            return a.score > b.score;
        });
        if (entries.size() > 5) entries.resize(5);
        save();
        return true;
    }
    return false;
}

GameState::GameState() {
    reset();
}

void GameState::updateHighScores() {
    if (highScoreChecked || score <= 0) return;
    m_isNewTopScore = highScores.update(score, aiUsedThisRun, obstaclesEnabled);
    highScoreChecked = true;
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
        for (const auto& s : snake) if (pos == s) { collision = true; break; }
        if (collision) continue;
        for (const auto& o : obstacles) if (pos == o) { collision = true; break; }
        if (collision) continue;
        for (const auto& item : extraExclusions) if (pos == item) { collision = true; break; }
    } while (collision);
    return pos;
}

void GameState::generateNewFood() {
    std::vector<glm::vec2> others;
    if (banana.active) others.push_back(banana.pos);
    if (goldenApple.active) others.push_back(goldenApple.pos);
    food = getRandomEmptyPosition(others);
}

void GameState::generateBanana() {
    std::vector<glm::vec2> others = {food};
    if (goldenApple.active) others.push_back(goldenApple.pos);
    banana.pos = getRandomEmptyPosition(others);
    banana.active = true;
}

void GameState::generateGoldenApple() {
    std::vector<glm::vec2> others = {food};
    if (banana.active) others.push_back(banana.pos);
    goldenApple.pos = getRandomEmptyPosition(others);
    goldenApple.active = true;
}

void GameState::generateObstacle() {
    std::vector<glm::vec2> others = {food};
    if (banana.active) others.push_back(banana.pos);
    if (goldenApple.active) others.push_back(goldenApple.pos);
    obstacles.push_back(getRandomEmptyPosition(others));
}

void GameState::reset() {
    snake.clear();
    snake.push_front({GRID_WIDTH / 2, GRID_HEIGHT / 2});
    snake.push_back({GRID_WIDTH / 2 - 1, GRID_HEIGHT / 2});
    snake.push_back({GRID_WIDTH / 2 - 2, GRID_HEIGHT / 2});
    snakeDirection = Direction::RIGHT;
    gameOver = false;
    m_shouldReset = false;
    paused = false;
    score = 0;
    obstacles.clear();
    timeBetweenUpdates = 0.1f;
    lastUpdateTime = 0.0f;
    aiUsedThisRun = false;
    highScoreChecked = false;
    m_isNewTopScore = false;
    banana.active = false;
    goldenApple.active = false;
    generateNewFood();
    generateBanana();
    generateGoldenApple();
    nextBananaSpawnTime = 0.0f;
}

Direction GameState::getAIDirection() {
    glm::vec2 head = snake.front();
    struct Node { glm::vec2 pos; Direction firstMove; };
    std::queue<Node> q;
    bool blocked[GRID_WIDTH][GRID_HEIGHT] = {};
    for (const auto& s : snake) blocked[(int)s.x][(int)s.y] = true;
    for (const auto& o : obstacles) blocked[(int)o.x][(int)o.y] = true;
    blocked[(int)head.x][(int)head.y] = true;
    std::vector<Direction> dirs = {Direction::UP, Direction::DOWN, Direction::LEFT, Direction::RIGHT};
    for (Direction d : dirs) {
        glm::vec2 next = head;
        if (d == Direction::UP) next.y += 1;
        else if (d == Direction::DOWN) next.y -= 1;
        else if (d == Direction::LEFT) next.x -= 1;
        else if (d == Direction::RIGHT) next.x += 1;
        if (next.x >= 0 && next.x < GRID_WIDTH && next.y >= 0 && next.y < GRID_HEIGHT && !blocked[(int)next.x][(int)next.y]) {
            if (next == food) return d;
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
            if (next == food) return curr.firstMove;
            if (next.x >= 0 && next.x < GRID_WIDTH && next.y >= 0 && next.y < GRID_HEIGHT && !blocked[(int)next.x][(int)next.y]) {
                blocked[(int)next.x][(int)next.y] = true;
                q.push({next, curr.firstMove});
            }
        }
    }
    return snakeDirection;
}

void GameState::update(float currentTime) {
    // Handle Banana Expiry and Spawning
    if (banana.active && (currentTime - banana.spawnTime) > 5.0f) {
        banana.active = false;
        nextBananaSpawnTime = currentTime + 10.0f;
    }

    // Handle Golden Apple Expiry
    if (goldenApple.active && (currentTime - goldenApple.spawnTime) > 7.0f) {
        goldenApple.active = false;
    }

    // Fixed-timestep update
    if (currentTime - lastUpdateTime >= timeBetweenUpdates && !gameOver && !paused && !inMenu)
    {
        lastUpdateTime = currentTime;

        if (aiEnabled) {
            snakeDirection = getAIDirection();
            aiUsedThisRun = true;
        }

        glm::vec2 newHead = snake.front();
        if (snakeDirection == Direction::UP)    newHead.y += 1;
        if (snakeDirection == Direction::DOWN)  newHead.y -= 1;
        if (snakeDirection == Direction::LEFT)  newHead.x -= 1;
        if (snakeDirection == Direction::RIGHT) newHead.x += 1;

        // Wall collision
        if (newHead.x < 0 || newHead.x >= GRID_WIDTH || newHead.y < 0 || newHead.y >= GRID_HEIGHT) {
            gameOver = true;
        }

        // Obstacle/Self collision
        for (const auto& obs : obstacles) if (newHead == obs) { gameOver = true; break; }
        for (size_t i = 1; i < snake.size(); ++i) if (newHead == snake[i]) { gameOver = true; break; }

        if (!gameOver) {
            snake.push_front(newHead);

            if (newHead == food) {
                score++;
                timeBetweenUpdates = std::max(0.03f, timeBetweenUpdates * 0.98f);
                static std::default_random_engine rd(std::random_device{}());
                if (obstaclesEnabled && std::uniform_real_distribution<float>(0,1)(rd) < 0.5f) generateObstacle();
                generateNewFood();
            } else if (banana.active && newHead == banana.pos) {
                score += 2;
                banana.active = false;
                nextBananaSpawnTime = currentTime + 10.0f;
                if (obstaclesEnabled) generateObstacle();
            } else if (goldenApple.active && newHead == goldenApple.pos) {
                score += 5;
                goldenApple.active = false;
                if (obstaclesEnabled) { generateObstacle(); generateObstacle(); }
            } else {
                snake.pop_back();
            }

            // Spawning Logic
            if (!banana.active && currentTime >= nextBananaSpawnTime && !paused) {
                generateBanana();
                banana.spawnTime = currentTime;
            }
            if (!goldenApple.active && !paused) {
                static std::default_random_engine rd(std::random_device{}());
                if (std::uniform_real_distribution<float>(0,1)(rd) < 0.002f) {
                    generateGoldenApple();
                    goldenApple.spawnTime = currentTime;
                }
            }
        }
    }

    if (gameOver && !highScoreChecked) updateHighScores();
}