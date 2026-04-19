#include <catch2/catch_test_macros.hpp>
#include "game_logic.hpp"

bool isWithinBounds(glm::vec2 pos, int width, int height) {
    return pos.x >= 0 && pos.x < width && pos.y >= 0 && pos.y < height;
}

TEST_CASE("Basic Game Logic", "[logic]") {
    GameState state;
    state.snake = {{5, 5}, {4, 5}, {3, 5}};
    
    SECTION("Boundary Checks") {
        CHECK(isWithinBounds({0,0}, 30, 15));
        CHECK_FALSE(isWithinBounds({-1,0}, 30, 15));
        CHECK_FALSE(isWithinBounds({30,15}, 30, 15));
    }

    SECTION("Snake Initialization") {
        resetGame(state);
        CHECK(state.snake.size() == 3);
        CHECK(state.snake.front() == glm::vec2(15, 7));
        CHECK(state.score == 0);
    }

    SECTION("Snake Movement and Growth") {
        resetGame(state);
        state.inMenu = false;
        state.paused = false;
        state.snakeDirection = Direction::RIGHT;
        state.lastUpdateTime = 0.0f;
        state.banana.active = false;      // Ensure no interference
        state.goldenApple.active = false;
        state.nextBananaSpawnTime = 100.0f; // Prevent respawn
        float tick = state.timeBetweenUpdates + 0.01f;

        // Normal Movement
        size_t initialSize = state.snake.size();
        state.food = {0, 0}; // Move food away
        updateGame(state, tick);
        CHECK(state.snake.front() == glm::vec2(16, 7));
        CHECK(state.snake.size() == initialSize);

        // Eating Food (Growth)
        state.food = {17, 7};
        updateGame(state, tick * 2.0f);
        CHECK(state.snake.front() == glm::vec2(17, 7));
        CHECK(state.snake.size() == initialSize + 1);
        CHECK(state.score == 1);
    }

    SECTION("Bonus Item Mechanics") {
        resetGame(state);
        state.inMenu = false;
        state.paused = false;
        state.snakeDirection = Direction::RIGHT;
        state.lastUpdateTime = 0.0f;
        float tick = state.timeBetweenUpdates + 0.01f;
        state.food = {0, 0}; // Move food away
        size_t initialSize = state.snake.size();

        // Banana Growth and Points (+2)
        state.banana.active = true;
        state.banana.pos = {16, 7};
        updateGame(state, tick);
        CHECK(state.score == 2);
        CHECK(state.snake.size() == initialSize + 1);

        // Golden Apple Growth and Points (+5)
        state.goldenApple.active = true;
        state.goldenApple.pos = {17, 7};
        updateGame(state, tick * 2.0f);
        CHECK(state.score == 7); // 2 + 5
        CHECK(state.snake.size() == initialSize + 2);
    }

    SECTION("Collision and Game Over") {
        resetGame(state);
        state.inMenu = false;
        state.paused = false;
        state.lastUpdateTime = 0.0f;
        float tick = state.timeBetweenUpdates + 0.01f;

        // Wall Collision
        state.snake = {{29, 7}, {28, 7}, {27, 7}};
        state.snakeDirection = Direction::RIGHT;
        updateGame(state, tick);
        CHECK(state.gameOver == true);

        // Obstacle Collision
        resetGame(state);
        state.inMenu = false;
        state.obstacles = {{16, 7}};
        state.snakeDirection = Direction::RIGHT;
        updateGame(state, tick);
        CHECK(state.gameOver == true);
    }

    SECTION("High Score Ranking") {
        state.highScores = HighScoreManager(""); // Reset and prevent file I/O during tests
        state.highScores.update(10, false, true);

        state.score = 20;
        state.gameOver = true;
        state.highScoreChecked = false;
        state.obstaclesEnabled = true;
        state.aiUsedThisRun = false;

        updateHighScores(state);
        REQUIRE(state.highScores.getEntries().size() >= 1);
        CHECK(state.highScores.getEntries()[0].score == 20);
        CHECK(state.isNewTopScore == true);
    }
}

TEST_CASE("AI Pathfinding", "[ai]") {
    GameState state;
    state.snake = {{5, 5}, {4, 5}, {3, 5}};
    state.obstacles.clear();

    SECTION("AI moves toward food - Up") {
        state.food = {5, 6};
        CHECK(getAIDirection(state) == Direction::UP);
    }

    SECTION("AI moves toward food - Right") {
        state.food = {6, 5};
        CHECK(getAIDirection(state) == Direction::RIGHT);
    }

    SECTION("AI avoids obstacles") {
        state.food = {5, 6};
        state.obstacles.push_back({5, 6}); // Food blocked
        Direction d = getAIDirection(state);
        CHECK(d != Direction::LEFT); // Don't turn into self
        // Even if food is blocked, it should pick a safe move
    }
}