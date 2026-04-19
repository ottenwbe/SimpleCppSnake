#include "input_manager.hpp"

void InputManager::update(GLFWwindow* window, const std::vector<int>& keysToTrack) {
    for (int key : keysToTrack) {
        bool isPressed = glfwGetKey(window, key) == GLFW_PRESS;
        auto& state = m_keyStates[key];
        state.justPressed = isPressed && !state.down;
        state.down = isPressed;
    }
}

bool InputManager::isJustPressed(int key) {
    return m_keyStates[key].justPressed;
}

bool InputManager::isDown(int key) {
    return m_keyStates[key].down;
}