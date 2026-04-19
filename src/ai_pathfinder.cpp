#include "ai_pathfinder.hpp"

Direction AIPathfinder::getNextMove(
    glm::vec2 head, 
    glm::vec2 target, 
    const std::deque<glm::vec2>& snake, 
    const std::vector<glm::vec2>& obstacles,
    Direction currentDir) 
{
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
            if (next == target) return d;
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
            if (next == target) return curr.firstMove;
            if (next.x >= 0 && next.x < GRID_WIDTH && next.y >= 0 && next.y < GRID_HEIGHT && !blocked[(int)next.x][(int)next.y]) {
                blocked[(int)next.x][(int)next.y] = true;
                q.push({next, curr.firstMove});
            }
        }
    }
    return currentDir;
}