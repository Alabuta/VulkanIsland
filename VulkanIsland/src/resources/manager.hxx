#pragma once

#include <memory>

#include "main.hxx"
#include "utility/mpl.hxx"
#include "vulkan/device.hxx"

#include "graphics/render_pass.hxx"


namespace resource
{
    class image;
    class image_view;

    class framebuffer;

    template<class T>
    struct hash;
}

namespace resource
{
    class resource_manager final {
    public:

        resource_manager(vulkan::device const &device) noexcept : device_{device} { };
        
        [[nodiscard]] std::shared_ptr<resource::framebuffer>
        create_framebuffer(renderer::extent extent, std::shared_ptr<graphics::render_pass> render_pass,
                           std::vector<std::shared_ptr<resource::image_view>> const &attachments);

    private:

        vulkan::device const &device_;
    };
}
