#pragma once

#include <vector>
#include <memory>

#include "graphics.hxx"
#include "render_pass.hxx"
#include "resources/image.hxx"


namespace resource
{
    class framebuffer final {
    public:

        framebuffer(VkFramebuffer handle) noexcept : handle_{handle} { }

        VkFramebuffer handle() const noexcept { return handle_; }

    private:

        VkFramebuffer handle_;
    };
}

namespace resource
{
    struct framebuffer_invariant final {
        renderer::extent extent;

        std::shared_ptr<graphics::render_pass> render_pass;

        std::vector<std::shared_ptr<resource::image_view>> attachments;

        template<class T> requires std::same_as<std::remove_cvref_t<T>, resource::framebuffer>
        auto constexpr operator== (T &&rhs) const
        {
            return extent == rhs.extent && render_pass == rhs.render_pass && attachments == rhs.attachments;
        }
    };
}

namespace resource
{
    class framebuffer_manager final {
    public:

        [[nodiscard]] std::shared_ptr<resource::framebuffer>
        create_framebuffer(renderer::extent extent, std::shared_ptr<graphics::render_pass> render_pass,
                           std::vector<std::shared_ptr<resource::image_view>> const &attachments);

    private:

        ;
    };
}

namespace graphics
{
    template<>
    struct hash<resource::framebuffer_invariant> {
        std::size_t operator() (resource::framebuffer_invariant const &framebuffer_invariant) const;
    };
}
