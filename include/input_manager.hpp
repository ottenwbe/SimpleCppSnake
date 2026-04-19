#pragma once

#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <unordered_map>
#include <vector>

class InputManager {
public:
    void update(GLFWwindow* window, const std::vector<int>& keysToTrack);
    bool isJustPressed(int key);
    bool isDown(int key);

private:
    struct KeyState {
        bool down = false;
        bool justPressed = false;
    };
    std::unordered_map<int, KeyState> m_keyStates;
};