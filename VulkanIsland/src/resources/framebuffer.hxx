#pragma once

#include <vector>
#include <memory>

#include "graphics/graphics.hxx"
#include "graphics/render_pass.hxx"
#include "resources/image.hxx"


namespace resource
{
    template<class T>
    struct hash;
}

namespace resource
{
    class framebuffer final {
    public:

        framebuffer(VkFramebuffer handle) noexcept : handle_{handle} { }

        VkFramebuffer handle() const noexcept { return handle_; }

    private:

        VkFramebuffer handle_;

        framebuffer() = delete;
        framebuffer(framebuffer const &) = delete;
        framebuffer(framebuffer &&) = delete;
    };
}

namespace resource
{
    struct framebuffer_invariant final {
        renderer::extent extent;

        std::shared_ptr<graphics::render_pass> render_pass;

        std::vector<std::shared_ptr<resource::image_view>> attachments;

        template<class T> requires mpl::same_as<std::remove_cvref_t<T>, resource::framebuffer_invariant>
        auto constexpr operator== (T &&rhs) const
        {
            return extent == rhs.extent && render_pass == rhs.render_pass && attachments == rhs.attachments;
        }
    };

    template<>
    struct hash<resource::framebuffer_invariant> {
        std::size_t operator() (resource::framebuffer_invariant const &framebuffer_invariant) const;
    };
}
