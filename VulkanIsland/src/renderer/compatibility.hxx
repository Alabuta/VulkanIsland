#pragma once


#include "graphics.hxx"
#include "render_pass.hxx"
#include "attachments.hxx"


namespace graphics
{
    template<>
    struct compatibility<graphics::render_pass_attachment> {
        auto operator() (graphics::render_pass_attachment const &lhs, graphics::render_pass_attachment const &rhs) const
        {
            // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#renderpass-compatibility

            return false;
        }
    };

    template<>
    struct compatibility<graphics::render_pass> {
        auto operator() (graphics::render_pass const &lhs, graphics::render_pass const &rhs) const
        {
            /* Two render passes are compatible if their corresponding color, input, resolve, and depth/stencil
            attachment references are compatible and if they are otherwise identical except for:
                Initial and final image layout in attachment descriptions
                Load and store operations in attachment descriptions
                Image layout in attachment references

            As an additional special case, if two render passes have a single subpass, they are compatible even
            if they have different resolve attachment references or depth/stencil resolve modes but satisfy the other compatibility conditions. */

            return false;
        }
    };
}
