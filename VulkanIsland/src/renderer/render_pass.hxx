#pragma once

#include <vector>

#include "device/device.hxx"
#include "graphics.hxx"
#include "attachments.hxx"


namespace graphics
{
    struct render_subpass final {
        ;
    };
}

namespace graphics
{
    class render_pass final {
    public:

        render_pass(VkRenderPass handle) noexcept : handle_{handle} { }

        VkRenderPass handle() const noexcept { return handle_; }

    private:

        VkRenderPass handle_;

        std::vector<graphics::render_pass_attachment> attachments_;
    };
}
