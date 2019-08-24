#pragma once

#include <unordered_set>
#include <set>
#include <vector>
#include <memory>

#include "device/device.hxx"
#include "graphics.hxx"
#include "attachments.hxx"


namespace graphics
{
    struct render_subpass final {
        ;
    };

    struct subpass_description final {
        std::set<graphics::attachment_reference> input_attachments_;
        std::set<graphics::attachment_reference> color_attachments_;
        std::set<graphics::attachment_reference> depth_stencil_attachments_;
        std::set<graphics::attachment_reference> resolve_attachments_;
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

        std::vector<graphics::render_subpass> subpasses_;
    };
}
