#pragma once

#include <map>
#include <vector>

#include <GLFW/glfw3.h>


#include "input/MouseInput.hxx"


class InputManager final {
public:

    InputManager(GLFWwindow *hTargetWnd);

    ~InputManager();

    void Process();

    MouseInput &mouse() noexcept { return mouse_; }

private:
    GLFWwindow *hTargetWnd_;

    raw_mouse_t rawMouse_;
    MouseInput mouse_;

    bool needsUpdateMouse{false};
};
