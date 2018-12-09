#include <iostream>

#include <string>
#include <string_view>
#include <unordered_set>

#include <boost/functional/hash_fwd.hpp>

#include "input/InputManager.hxx"

using namespace std::string_literals;
using namespace std::string_view_literals;

InputManager::InputManager(GLFWwindow *hTargetWnd) : hTargetWnd_{hTargetWnd}
{
    glfwSetMouseButtonCallback(hTargetWnd_, [&] (auto window, auto button, auto action, [[maybe_unused]] auto mods)
    {
        rawMouse_.buttons.reset();

        auto offset = action == GLFW_PRESS ? 0 : 1;

        rawMouse_[button * 2 + offset] = 1;

        if (action == GLFW_PRESS) {
            std::cout << "pressed"s << button << '\n';
        }

        if (action == GLFW_RELEASE) {
            std::cout << "unpressed"s << button << '\n';
        }

        needsUpdateMouse = true;
    });

    glfwSetCursorPosCallback(hTargetWnd_, [] (auto window, auto x, auto y)
    {
        ;
    });
}

InputManager::~InputManager()
{
    ;
}

void InputManager::Process()
{
    if (needsUpdateMouse)
        mouse_.update(rawMouse_);

    needsUpdateMouse = false;
}
