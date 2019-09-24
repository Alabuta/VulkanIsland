#include <stdexcept>
using namespace std::string_literals;

#include <fmt/format.h>

#include "window.hxx"


namespace platform
{
    window::window(std::string_view name, std::int32_t width, std::int32_t height)
        : handle_{nullptr}, width_{width}, height_{height}, name_{name}
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        handle_ = glfwCreateWindow(width_, height_, name_.c_str(), nullptr, nullptr);

        if (handle_ == nullptr)
            throw std::runtime_error(fmt::format("failed to create '{}' window"s, name_));

        glfwSetWindowUserPointer(handle_, this);

        set_callbacks();
    }

    window::~window()
    {
        if (handle_)
            glfwDestroyWindow(handle_);
    }

    void window::connect_event_handler(std::shared_ptr<event_handler_interface> handler)
    {
        resize_callback_.connect(decltype(resize_callback_)::slot_type(
            &event_handler_interface::on_resize, handler.get(), _1, _2
        ).track_foreign(handler));
    }

    void window::connect_input_handler(std::shared_ptr<input_handler_interface> handler)
    {
        input_update_callback_.connect(decltype(input_update_callback_)::slot_type(
            &input_handler_interface::update, handler.get(), _1
        ).track_foreign(handler));
    }

    void window::update(std::function<void()> &&callback)
    {
        while (glfwWindowShouldClose(handle_) == GLFW_FALSE && glfwGetKey(handle_, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
            callback();
        }
    }

    void window::set_callbacks()
    {
        glfwSetWindowSizeCallback(handle_, [] (auto handle, auto width, auto height)
        {
            auto instance = reinterpret_cast<window *>(glfwGetWindowUserPointer(handle));

            if (instance) {
                instance->width_ = width;
                instance->height_ = height;

                instance->resize_callback_(width, height);
            }
        });

        glfwSetCursorPosCallback(handle_, [] (auto handle, auto x, auto y)
        {
            auto instance = reinterpret_cast<window *>(glfwGetWindowUserPointer(handle));

            if (instance) {
                auto coords = platform::mouse_data::relative_coords{
                    static_cast<decltype(platform::mouse_data::relative_coords::x)>(x),
                    static_cast<decltype(platform::mouse_data::relative_coords::y)>(y)
                };

                platform::raw data = platform::mouse_data::raw{std::move(coords)};

                instance->input_update_callback_(data);
            }
        });

        glfwSetMouseButtonCallback(handle_, [] (auto handle, auto button, auto action, auto)
        {
            auto instance = reinterpret_cast<window *>(glfwGetWindowUserPointer(handle));

            if (instance) {
                auto buttons = platform::mouse_data::buttons{};

                std::size_t offset = action == GLFW_PRESS ? 0 : 1;

                buttons.value[static_cast<std::size_t>(button) * 2 + offset] = 1;

                platform::raw data = platform::mouse_data::raw{std::move(buttons)};

                instance->input_update_callback_(data);
            }
        });

        // TODO: replace to 'glfwSetMouseWheelCallback'
        glfwSetScrollCallback(handle_, [] (auto handle, auto xoffset, auto yoffset)
        {
            auto instance = reinterpret_cast<window *>(glfwGetWindowUserPointer(handle));

            if (instance) {
                auto wheel = platform::mouse_data::wheel{
                    static_cast<decltype(platform::mouse_data::wheel::xoffset)>(xoffset),
                    static_cast<decltype(platform::mouse_data::wheel::yoffset)>(yoffset)
                };

                platform::raw data = platform::mouse_data::raw{std::move(wheel)};

                instance->input_update_callback_(data);
            }
        });
    }
}
