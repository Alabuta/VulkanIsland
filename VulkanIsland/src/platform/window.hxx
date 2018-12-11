#pragma once 

#include <cstdint>
#include <string>
#include <string_view>

#include <boost/signals2.hpp>

#include <GLFW/glfw3.h>

#include "../input/input_data_types.hxx"
#include "../input/input_manager.hxx"



class Window final {
public:

    Window(std::string_view name, std::int32_t width, std::int32_t height);

    ~Window();

    struct IEventHandler {
        virtual ~IEventHandler() = default;

        virtual void onResize(std::int32_t width, std::int32_t height) { };
        virtual void onInputUpdate(InputManager::InputData &data) { };
    };

    void connectEventHandler(std::shared_ptr<IEventHandler> handler);

    struct IInputHandler {
        virtual ~IInputHandler() = default;

        virtual void onMouseUpdate(InputManager::InputData &data) = 0;
    };

    void connectInputHandler(std::shared_ptr<IInputHandler> handler);

    GLFWwindow *handle() const noexcept { return handle_; }

private:
    GLFWwindow *handle_;

    std::int32_t width_{0}, height_{0};

    std::string name_;

    boost::signals2::signal<void(std::int32_t, std::int32_t)> resizeCallback_;
    boost::signals2::signal<void(InputManager::InputData &)> inputUpdateCallback_;
};
