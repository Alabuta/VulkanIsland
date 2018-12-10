#pragma once

#include <variant>

#include <GLFW/glfw3.h>


#include "mouse_input.hxx"


class InputManager final {
public:

    struct MouseInputData {
        std::bitset<16> buttons{0};
        std::float_t x{0.f}, y{0.f};
    };

    using InputData = std::variant<MouseInputData>;

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
