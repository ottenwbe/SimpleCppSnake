#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <deque>
#include <queue>
#include "game_logic.hpp"

class AIPathfinder {
public:
    static Direction getNextMove(
        glm::vec2 head, 
        glm::vec2 target, 
        const std::deque<glm::vec2>& snake, 
        const std::vector<glm::vec2>& obstacles,
        Direction currentDir
    );
};