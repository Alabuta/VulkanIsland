#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <boost/signals2.hpp>

#include <GLFW/glfw3.h>

#include "input/input_data.hxx"


namespace platform
{
    class window final {
    public:

        struct event_handler_interface {
            virtual ~event_handler_interface() = default;

            virtual void on_resize(std::int32_t width, std::int32_t height) = 0;
        };

        struct input_handler_interface {
            virtual ~input_handler_interface() = default;

            virtual void update(platform::raw &data) = 0;
        };

        window(std::string_view name, std::int32_t width, std::int32_t height);
        ~window();

        void update(std::function<void()> &&callback) const;

        GLFWwindow *handle() noexcept { return handle_; }

        void connect_event_handler(std::shared_ptr<event_handler_interface> handler);
        void connect_input_handler(std::shared_ptr<input_handler_interface> handler);

    private:
        GLFWwindow *handle_;

        std::int32_t width_{0}, height_{0};

        std::string name_;

        boost::signals2::signal<void(std::int32_t, std::int32_t)> resize_callback_;

        boost::signals2::signal<void(platform::raw &)> input_update_callback_;

        void set_callbacks();

        window(window const &) = delete;
        window const &operator=(window const &) = delete;
    };
}
