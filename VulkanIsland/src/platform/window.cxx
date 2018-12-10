#include <stdexcept>

#include "window.hxx"

using namespace std::string_literals;
using namespace std::string_view_literals;

Window::Window(std::string_view name, std::int32_t width, std::int32_t height)
    : handle_{nullptr}, width_{width}, height_{height}, name_{name}
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    handle_ = glfwCreateWindow(width_, height_, name_.c_str(), nullptr, nullptr);

    if (handle_ == nullptr)
        throw std::runtime_error("failed to create '"s + name_ + "' window"s);

    glfwSetWindowUserPointer(handle_, this);

    glfwSetWindowSizeCallback(handle_, [] (auto handle, auto width, auto height)
    {
        auto instance = reinterpret_cast<Window *>(glfwGetWindowUserPointer(handle));

        if (instance) {
            instance->width_ = width;
            instance->height_ = height;

            instance->resizeCallback_(width, height);
        }
    });

    glfwSetCursorPosCallback(handle_, [] (auto handle, auto x, auto y)
    {
        auto instance = reinterpret_cast<Window *>(glfwGetWindowUserPointer(handle));

        if (instance) {
            MouseInput::InputData mouse;

            mouse = MouseInput::relative_coords_t{
                static_cast<decltype(MouseInput::relative_coords_t::x)>(x),
                static_cast<decltype(MouseInput::relative_coords_t::y)>(y)
            };

            InputManager::InputData data = std::move(mouse);

            instance->inputUpdateCallback_(data);
        }
    });

    // glfwSetMouseButtonCallback(handle_, [] (auto handle, auto button, auto action, auto)
    // {
    //     auto instance = reinterpret_cast<Window *>(glfwGetWindowUserPointer(handle));

    //     if (instance) {
    //         InputManager::MouseInputData data;

    //         data.x = static_cast<decltype(data.x)>(x);
    //         data.y = static_cast<decltype(data.y)>(y);

    //         instance->inputProcessCallback(std::move(data));
    //     }
    // });
}

Window::~Window()
{
    if (handle_)
        glfwDestroyWindow(handle_);
}

void Window::connectEventHandler(std::shared_ptr<IEventHandler> handler)
{
    resizeCallback_.connect(decltype(resizeCallback_)::slot_type(
        &IEventHandler::onResize, handler.get(), _1, _2
    ).track_foreign(handler));

    inputUpdateCallback_.connect(decltype(inputUpdateCallback_)::slot_type(
        &IEventHandler::onInputUpdate, handler.get(), _1
    ).track_foreign(handler));
}
