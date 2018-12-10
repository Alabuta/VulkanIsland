#pragma once

#include <memory>
#include <variant>
#include <tuple>

#include "mouse_input.hxx"


class InputManager final {
public:

    using InputData = std::variant<
        MouseInput::InputData
    >;

#if 0
    InputManager(std::shared_ptr<class Window> window);

    ~InputManager();
#endif

    void Process();

    MouseInput &mouse() noexcept { return mouse_; }

private:
    //std::shared_ptr<Window> window_;

    MouseInput mouse_;
};
