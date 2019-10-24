#pragma once

#include <vector>
#include <memory>

#include "graphics.hxx"
#include "render_pass.hxx"
#include "resources/image.hxx"


namespace resource
{
    struct framebuffer final {
        std::uint32_t width, height;

        std::shared_ptr<graphics::render_pass> render_pass;

        std::vector<std::shared_ptr<VulkanImage>> attachments;

        template<class T> requires std::same_as<std::remove_cvref_t<T>, resource::framebuffer>
        auto constexpr operator== (T &&rhs) const
        {
            return width == rhs.width && height == rhs.height && render_pass == rhs.render_pass && attachments == rhs.attachments;
        }
    };
}

namespace graphics
{
    template<>
    struct hash<resource::framebuffer> {
        std::size_t operator() (resource::framebuffer const &framebuffer) const;
    };
}
