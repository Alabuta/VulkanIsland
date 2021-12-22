#pragma once

#include <memory>
#include <variant>
#include <tuple>

#include "platform/window.hxx"
#include "mouse.hxx"


namespace platform
{
    class input_manager final : public window::input_handler_interface {
    public:

        platform::mouse &mouse() noexcept { return mouse_; }

    private:

        platform::mouse mouse_;

        void update(platform::raw &raw_data) override;

    };
}
