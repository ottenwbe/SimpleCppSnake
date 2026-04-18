#include "game_logic.hpp"

std::string getExecutableDir() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count <= 0) return ".";
    return std::filesystem::path(std::string(result, count)).parent_path().string();
}

void saveHighScore(const GameState& state) {
    if (state.highScorePath.empty()) return;
    std::ofstream file(state.highScorePath);
    if (file.is_open()) {
        for (const auto& entry : state.highScores) {
            file << "Score: " << entry.score << "\n";
            file << "AI_Used: " << (entry.aiUsed ? "Yes" : "No") << "\n";
            file << "Obstacles: " << (entry.obstaclesEnabled ? "Yes" : "No") << "\n";
        }
        file.close();
    }
}

void loadHighScore(GameState& state) {
    state.highScores.clear();
    if (state.highScorePath.empty()) return;
    std::ifstream file(state.highScorePath);
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
                state.highScores.push_back(entry);
            }
        }
        file.close();
    }
}

void updateHighScores(GameState& state) {
    if (state.highScoreChecked || state.score <= 0) return;
    bool qualifies = state.highScores.size() < 5 || state.score > state.highScores.back().score;
    if (qualifies) {
        state.highScores.push_back({state.score, state.aiUsedThisRun, state.obstaclesEnabled});
        std::sort(state.highScores.begin(), state.highScores.end(), [](const HighScoreEntry& a, const HighScoreEntry& b) {
            return a.score > b.score;
        });
        if (state.highScores.size() > 5) state.highScores.resize(5);
        state.isNewTopScore = true;
        saveHighScore(state);
    }
    state.highScoreChecked = true;
}

glm::vec2 getRandomEmptyPosition(const GameState& state, const std::vector<glm::vec2>& extraExclusions) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribX(0, GRID_WIDTH - 1);
    std::uniform_int_distribution<> distribY(0, GRID_HEIGHT - 1);
    glm::vec2 pos;
    bool collision;
    do {
        collision = false;
        pos = {distribX(gen), distribY(gen)};
        for (const auto& s : state.snake) if (pos == s) { collision = true; break; }
        if (collision) continue;
        for (const auto& o : state.obstacles) if (pos == o) { collision = true; break; }
        if (collision) continue;
        for (const auto& item : extraExclusions) if (pos == item) { collision = true; break; }
    } while (collision);
    return pos;
}

void generateNewFood(GameState& state) {
    std::vector<glm::vec2> others;
    if (state.banana.active) others.push_back(state.banana.pos);
    if (state.goldenApple.active) others.push_back(state.goldenApple.pos);
    state.food = getRandomEmptyPosition(state, others);
}

void generateBanana(GameState& state) {
    std::vector<glm::vec2> others = {state.food};
    if (state.goldenApple.active) others.push_back(state.goldenApple.pos);
    state.banana.pos = getRandomEmptyPosition(state, others);
    state.banana.active = true;
}

void generateGoldenApple(GameState& state) {
    std::vector<glm::vec2> others = {state.food};
    if (state.banana.active) others.push_back(state.banana.pos);
    state.goldenApple.pos = getRandomEmptyPosition(state, others);
    state.goldenApple.active = true;
}

void generateObstacle(GameState& state) {
    std::vector<glm::vec2> others = {state.food};
    if (state.banana.active) others.push_back(state.banana.pos);
    if (state.goldenApple.active) others.push_back(state.goldenApple.pos);
    state.obstacles.push_back(getRandomEmptyPosition(state, others));
}

void resetGame(GameState& state) {
    state.snake.clear();
    state.snake.push_front({GRID_WIDTH / 2, GRID_HEIGHT / 2});
    state.snake.push_back({GRID_WIDTH / 2 - 1, GRID_HEIGHT / 2});
    state.snake.push_back({GRID_WIDTH / 2 - 2, GRID_HEIGHT / 2});
    state.snakeDirection = Direction::RIGHT;
    state.gameOver = false;
    state.shouldReset = false;
    state.paused = false;
    state.score = 0;
    state.obstacles.clear();
    state.timeBetweenUpdates = 0.1f;
    state.lastUpdateTime = 0.0f;
    state.aiUsedThisRun = false;
    state.highScoreChecked = false;
    state.isNewTopScore = false;
    state.banana.active = false;
    state.goldenApple.active = false;
    generateNewFood(state);
    generateBanana(state);
    generateGoldenApple(state);
    state.nextBananaSpawnTime = 0.0f;
}

Direction getAIDirection(const GameState& state) {
    glm::vec2 head = state.snake.front();
    struct Node { glm::vec2 pos; Direction firstMove; };
    std::queue<Node> q;
    bool blocked[GRID_WIDTH][GRID_HEIGHT] = {};
    for (const auto& s : state.snake) blocked[(int)s.x][(int)s.y] = true;
    for (const auto& o : state.obstacles) blocked[(int)o.x][(int)o.y] = true;
    blocked[(int)head.x][(int)head.y] = true;
    std::vector<Direction> dirs = {Direction::UP, Direction::DOWN, Direction::LEFT, Direction::RIGHT};
    for (Direction d : dirs) {
        glm::vec2 next = head;
        if (d == Direction::UP) next.y += 1;
        else if (d == Direction::DOWN) next.y -= 1;
        else if (d == Direction::LEFT) next.x -= 1;
        else if (d == Direction::RIGHT) next.x += 1;
        if (next.x >= 0 && next.x < GRID_WIDTH && next.y >= 0 && next.y < GRID_HEIGHT && !blocked[(int)next.x][(int)next.y]) {
            if (next == state.food) return d;
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
            if (next == state.food) return curr.firstMove;
            if (next.x >= 0 && next.x < GRID_WIDTH && next.y >= 0 && next.y < GRID_HEIGHT && !blocked[(int)next.x][(int)next.y]) {
                blocked[(int)next.x][(int)next.y] = true;
                q.push({next, curr.firstMove});
            }
        }
    }
    return state.snakeDirection;
}

void updateGame(GameState& state, float currentTime) {
    // Handle Banana Expiry and Spawning
    if (state.banana.active && (currentTime - state.banana.spawnTime) > 5.0f) {
        state.banana.active = false;
        state.nextBananaSpawnTime = currentTime + 10.0f;
    }

    // Handle Golden Apple Expiry
    if (state.goldenApple.active && (currentTime - state.goldenApple.spawnTime) > 7.0f) {
        state.goldenApple.active = false;
    }

    // Fixed-timestep update
    if (currentTime - state.lastUpdateTime >= state.timeBetweenUpdates && !state.gameOver && !state.paused && !state.inMenu)
    {
        state.lastUpdateTime = currentTime;

        if (state.aiEnabled) {
            state.snakeDirection = getAIDirection(state);
            state.aiUsedThisRun = true;
        }

        glm::vec2 newHead = state.snake.front();
        if (state.snakeDirection == Direction::UP)    newHead.y += 1;
        if (state.snakeDirection == Direction::DOWN)  newHead.y -= 1;
        if (state.snakeDirection == Direction::LEFT)  newHead.x -= 1;
        if (state.snakeDirection == Direction::RIGHT) newHead.x += 1;

        // Wall collision
        if (newHead.x < 0 || newHead.x >= GRID_WIDTH || newHead.y < 0 || newHead.y >= GRID_HEIGHT) {
            state.gameOver = true;
        }

        // Obstacle/Self collision
        for (const auto& obs : state.obstacles) if (newHead == obs) { state.gameOver = true; break; }
        for (size_t i = 1; i < state.snake.size(); ++i) if (newHead == state.snake[i]) { state.gameOver = true; break; }

        if (!state.gameOver) {
            state.snake.push_front(newHead);

            if (newHead == state.food) {
                state.score++;
                state.timeBetweenUpdates = std::max(0.03f, state.timeBetweenUpdates * 0.98f);
                static std::default_random_engine rd(std::random_device{}());
                if (state.obstaclesEnabled && std::uniform_real_distribution<float>(0,1)(rd) < 0.5f) generateObstacle(state);
                generateNewFood(state);
            } else if (state.banana.active && newHead == state.banana.pos) {
                state.score += 2;
                state.banana.active = false;
                state.nextBananaSpawnTime = currentTime + 10.0f;
                if (state.obstaclesEnabled) generateObstacle(state);
            } else if (state.goldenApple.active && newHead == state.goldenApple.pos) {
                state.score += 5;
                state.goldenApple.active = false;
                if (state.obstaclesEnabled) { generateObstacle(state); generateObstacle(state); }
            } else {
                state.snake.pop_back();
            }

            // Spawning Logic
            if (!state.banana.active && currentTime >= state.nextBananaSpawnTime && !state.paused) {
                generateBanana(state);
                state.banana.spawnTime = currentTime;
            }
            if (!state.goldenApple.active && !state.paused) {
                static std::default_random_engine rd(std::random_device{}());
                if (std::uniform_real_distribution<float>(0,1)(rd) < 0.002f) {
                    generateGoldenApple(state);
                    state.goldenApple.spawnTime = currentTime;
                }
            }
        }
    }

    if (state.gameOver && !state.highScoreChecked) updateHighScores(state);
}