#include <catch2/catch_test_macros.hpp>
#include "game_logic.hpp"

bool isWithinBounds(glm::vec2 pos, int width, int height) {
    return pos.x >= 0 && pos.x < width && pos.y >= 0 && pos.y < height;
}

TEST_CASE("Basic Game Logic", "[logic]") {
    GameState state;
    state.debugSetSnake({{5, 5}, {4, 5}, {3, 5}});
    
    SECTION("Boundary Checks") {
        CHECK(isWithinBounds({0,0}, 30, 15));
        CHECK_FALSE(isWithinBounds({-1,0}, 30, 15));
        CHECK_FALSE(isWithinBounds({30,15}, 30, 15));
    }

    SECTION("Snake Initialization") {
        state.reset();
        CHECK(state.getSnake().size() == 3);
        CHECK(state.getSnake().front() == glm::vec2(15, 7));
        CHECK(state.getScore() == 0);
    }

    SECTION("Snake Movement and Growth") {
        state.reset();
        state.setInMenu(false);
        state.setDirection(Direction::RIGHT);
        state.debugSetLastUpdateTime(0.0f);
        state.debugSetNextBananaSpawnTime(100.0f); // Prevent respawn
        float tick = state.getTimeBetweenUpdates() + 0.01f;

        // Normal Movement
        size_t initialSize = state.getSnake().size();
        state.debugSetFood({0, 0}); // Move food away
        state.update(tick);
        CHECK(state.getSnake().front() == glm::vec2(16, 7));
        CHECK(state.getSnake().size() == initialSize);

        // Eating Food (Growth)
        state.debugSetFood({17, 7});
        state.update(tick * 2.0f);
        CHECK(state.getSnake().front() == glm::vec2(17, 7));
        CHECK(state.getSnake().size() == initialSize + 1);
        CHECK(state.getScore() == 1);
    }

    SECTION("Bonus Item Mechanics") {
        state.reset();
        state.setInMenu(false);
        state.setDirection(Direction::RIGHT);
        state.debugSetLastUpdateTime(0.0f);
        float tick = state.getTimeBetweenUpdates() + 0.01f;
        state.debugSetFood({0, 0}); // Move food away
        size_t initialSize = state.getSnake().size();

        // Banana Growth and Points (+2)
        Banana b; b.active = true; b.pos = {16, 7};
        state.debugSetBanana(b);
        state.update(tick);
        CHECK(state.getScore() == 2);
        CHECK(state.getSnake().size() == initialSize + 1);

        // Golden Apple Growth and Points (+5)
        GoldenApple ga; ga.active = true; ga.pos = {17, 7};
        state.debugSetGoldenApple(ga);
        state.update(tick * 2.0f);
        CHECK(state.getScore() == 7); // 2 + 5
        CHECK(state.getSnake().size() == initialSize + 2);
    }

    SECTION("Collision and Game Over") {
        state.reset();
        state.setInMenu(false);
        state.debugSetLastUpdateTime(0.0f);
        float tick = state.getTimeBetweenUpdates() + 0.01f;

        // Wall Collision
        state.debugSetSnake({{29, 7}, {28, 7}, {27, 7}});
        state.setDirection(Direction::RIGHT);
        state.update(tick);
        CHECK(state.isGameOver() == true);

        // Obstacle Collision
        state.reset();
        state.setInMenu(false);
        state.debugSetObstacles({{16, 7}});
        state.setDirection(Direction::RIGHT);
        state.update(tick);
        CHECK(state.isGameOver() == true);
    }

    SECTION("High Score Ranking") {
        state.getHighScoreManager() = HighScoreManager(""); // Reset and prevent file I/O during tests
        state.getHighScoreManager().update(10, false, true);

        state.debugSetScore(20);
        state.debugSetGameOver(true);
        state.debugSetHighScoreChecked(false);
        state.debugSetObstaclesEnabled(true);
        state.debugSetAiUsedThisRun(false);

        state.updateHighScores();
        REQUIRE(state.getHighScoreManager().getEntries().size() >= 1);
        CHECK(state.getHighScoreManager().getEntries()[0].score == 20);
        CHECK(state.isNewTopScore() == true);
    }
}

TEST_CASE("AI Pathfinding", "[ai]") {
    GameState state;
    state.debugSetSnake({{5, 5}, {4, 5}, {3, 5}});
    state.debugSetObstacles({});

    SECTION("AI moves toward food - Up") {
        state.debugSetFood({5, 6});
        CHECK(state.getAIDirection() == Direction::UP);
    }

    SECTION("AI moves toward food - Right") {
        state.debugSetFood({6, 5});
        CHECK(state.getAIDirection() == Direction::RIGHT);
    }

    SECTION("AI avoids obstacles") {
        state.debugSetFood({5, 6});
        state.debugSetObstacles({{5, 6}}); // Food blocked
        Direction d = state.getAIDirection();
        CHECK(d != Direction::LEFT); // Don't turn into self
        // Even if food is blocked, it should pick a safe move
    }
}