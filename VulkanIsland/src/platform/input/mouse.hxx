#pragma once

#include <bitset>

#include <boost/signals2.hpp>

#include "input_data.hxx"


namespace platform
{
    class mouse final {
    public:

        class handler_interface {
        public:

            virtual ~handler_interface() = default;

            using buttons_t = std::bitset<8>;

            virtual void on_move(float x, float y) = 0;
            virtual void on_wheel(float xoffset, float yoffset) = 0;
            virtual void on_down(buttons_t buttons) = 0;
            virtual void on_up(buttons_t buttons) = 0;
        };

        void connect_handler(std::shared_ptr<handler_interface> slot);

        void update(platform::mouse_data::raw &data);

    private:

        handler_interface::buttons_t buttons_{ 0 };

        boost::signals2::signal<void(float, float)> on_move_;
        boost::signals2::signal<void(float, float)> on_wheel_;
        boost::signals2::signal<void(handler_interface::buttons_t)> on_down_;
        boost::signals2::signal<void(handler_interface::buttons_t)> on_up_;
    };
}
