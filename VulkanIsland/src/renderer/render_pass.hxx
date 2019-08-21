#pragma once

#include "device/device.hxx"
#include "graphics.hxx"


namespace graphics
{
    class render_pass final {
    public:

        render_pass(VkRenderPass handle) noexcept : handle_{handle} { }

        VkRenderPass handle() const noexcept { return handle_; }

    private:

        VkRenderPass handle_;
    };
}
