#include <catch2/catch_test_macros.hpp>
#include "game_logic.hpp"
#include "ai_pathfinder.hpp"
#include <algorithm> // For std::min/max

bool isWithinBounds(glm::vec2 pos, int width, int height) {
    return pos.x >= 0 && pos.x < width && pos.y >= 0 && pos.y < height;
}

TEST_CASE("HighScoreManager Logic", "[highscore]") {
    HighScoreManager hsm(""); // Use empty path to avoid file I/O

    SECTION("Ranking and Capacity") {
        for (int i = 1; i <= 10; ++i) {
            hsm.update(i * 10, false, false);
        }
        auto entries = hsm.getEntries();
        REQUIRE(entries.size() == 5);
        CHECK(entries[0].score == 100);
        CHECK(entries[4].score == 60);
    }

    SECTION("Qualifying Scores") {
        hsm.update(50, false, false);
        hsm.update(40, false, false);
        hsm.update(30, false, false);
        hsm.update(20, false, false);
        hsm.update(15, false, false); // Fill the list to capacity (5)
        
        CHECK_FALSE(hsm.update(10, false, false)); // Too low
        CHECK(hsm.update(60, false, false));      // New top
    }
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

    SECTION("Input Constraints") {
        state.reset();
        state.setScene(GameScene::Gameplay);
        
        state.setDirection(Direction::UP);
        CHECK(state.getDirection() == Direction::UP);
        
        state.togglePause();
        state.setDirection(Direction::LEFT);
        // Direction should not change while paused
        CHECK(state.getDirection() == Direction::UP);
    }

    SECTION("Pause and Menu behavior") {
        state.reset(0.0f);
        state.setScene(GameScene::Gameplay);
        state.togglePause();
        
        glm::vec2 head = state.getSnake().front();
        state.update(1.0f, 1.0f); // Fast forward time
        CHECK(state.getSnake().front() == head); // No movement while paused
        
        state.togglePause();
        state.setScene(GameScene::Menu);
        state.update(1.0f, 1.0f);
        CHECK(state.getSnake().front() == head); // No movement in menu
    }

    SECTION("Update Timing") {
        state.reset(0.0f);
        state.setScene(GameScene::Gameplay);
        float step = state.getTimeBetweenUpdates();
        
        glm::vec2 head = state.getSnake().front();
        state.update(step * 0.5f, step * 0.5f);
        CHECK(state.getSnake().front() == head); // Too early
        
        state.update(step, step * 0.5f);
        CHECK(state.getSnake().front() != head); // Should move
    }

    SECTION("Snake Movement and Growth") {
        state.reset();
        state.setScene(GameScene::Gameplay);
        state.setDirection(Direction::RIGHT);
        state.debugSetTickAccumulator(0.0f);
        state.debugSetBananaSpawnTimer(100.0f); // Prevent respawn
        float tick = state.getTimeBetweenUpdates() + 0.01f;

        // Normal Movement
        size_t initialSize = state.getSnake().size();
        state.debugSetFood({0, 0}); // Move food away
        state.update(tick, tick);
        CHECK(state.getSnake().front() == glm::vec2(16, 7));
        CHECK(state.getSnake().size() == initialSize);

        // Eating Food (Growth)
        state.debugSetFood({17, 7});
        state.update(tick * 2.0f, tick);
        CHECK(state.getSnake().front() == glm::vec2(17, 7));
        CHECK(state.getSnake().size() == initialSize + 1);
        CHECK(state.getScore() == 1);
    }

    SECTION("Bonus Item Mechanics") {
        state.reset();
        state.setScene(GameScene::Gameplay);
        state.setDirection(Direction::RIGHT);
        state.debugSetTickAccumulator(0.0f);
        float tick = state.getTimeBetweenUpdates() + 0.01f;
        state.debugSetFood({0, 0}); // Move food away
        size_t initialSize = state.getSnake().size();

        // Banana Growth and Points (+2)
        Banana b; b.active = true; b.pos = {16, 7}; b.timeLeft = 5.0f;
        state.debugSetBanana(b);
        state.update(tick, tick);
        CHECK(state.getScore() == 2);
        CHECK(state.getSnake().size() == initialSize + 1);

        // Golden Apple Growth and Points (+5)
        GoldenApple ga; ga.active = true; ga.pos = {17, 7}; ga.timeLeft = 7.0f;
        state.debugSetGoldenApple(ga);
        state.update(tick * 2.0f, tick);
        CHECK(state.getScore() == 7); // 2 + 5
        CHECK(state.getSnake().size() == initialSize + 2);
    }

    SECTION("Collision and Game Over") {
        state.reset();
        state.setScene(GameScene::Gameplay);
        state.debugSetTickAccumulator(0.0f);
        float tick = state.getTimeBetweenUpdates() + 0.01f;

        // Wall Collision
        state.debugSetSnake({{29, 7}, {28, 7}, {27, 7}});
        state.setDirection(Direction::RIGHT);
        state.update(tick, tick);
        CHECK(state.isGameOver() == true);

        // Obstacle Collision
        state.reset();
        state.setScene(GameScene::Gameplay);
        state.debugSetObstacles({{16, 7}});
        state.setDirection(Direction::RIGHT);
        state.update(tick, tick);
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
        auto next = AIPathfinder::getNextMove(state.getSnake().front(), state.getFood(), state.getSnake(), state.getObstacles(), state.getDirection());
        CHECK(next == Direction::UP);
    }

    SECTION("AI moves toward food - Right") {
        state.debugSetFood({6, 5});
        auto next = AIPathfinder::getNextMove(state.getSnake().front(), state.getFood(), state.getSnake(), state.getObstacles(), state.getDirection());
        CHECK(next == Direction::RIGHT);
    }

    SECTION("AI avoids obstacles") {
        state.debugSetFood({5, 6});
        state.debugSetObstacles({{5, 6}}); // Food blocked
        Direction d = AIPathfinder::getNextMove(state.getSnake().front(), state.getFood(), state.getSnake(), state.getObstacles(), state.getDirection());
        CHECK(d != Direction::LEFT); // Don't turn into self
        // Even if food is blocked, it should pick a safe move
    }

    SECTION("AI when trapped") {
        // Surround head with obstacles and body
        state.debugSetSnake({{5, 5}, {4, 5}, {3, 5}});
        state.debugSetObstacles({{5, 6}, {6, 5}, {5, 4}}); 
        state.setDirection(Direction::RIGHT);
        // If trapped, return current direction and hope for the best
        auto next = AIPathfinder::getNextMove(state.getSnake().front(), state.getFood(), state.getSnake(), state.getObstacles(), state.getDirection());
        CHECK(next == Direction::RIGHT);
    }

    SECTION("Initial Speed Setting") {
        state.reset();
        CHECK(state.getInitialSpeedSetting() == 1); // Default to Medium

        state.setInitialSpeedSetting(0); // Easy
        state.reset();
        CHECK(state.getTimeBetweenUpdates() == Config::INITIAL_STEP_TIME * 1.5f);

        state.setInitialSpeedSetting(2); // Hard
        state.reset();
        CHECK(state.getTimeBetweenUpdates() == Config::INITIAL_STEP_TIME * 0.7f);

        state.setInitialSpeedSetting(5); // Out of bounds high
        state.reset();
        CHECK(state.getInitialSpeedSetting() == 5); // Setter doesn't clamp, but reset uses it

        state.setInitialSpeedSetting(-1); // Out of bounds low
        state.reset();
        CHECK(state.getInitialSpeedSetting() == -1); // Setter doesn't clamp, but reset uses it
    }
}