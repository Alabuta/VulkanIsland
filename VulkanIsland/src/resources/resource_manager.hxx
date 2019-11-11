#pragma once

#include <memory>

#include "main.hxx"
#include "utility/mpl.hxx"

#include "vulkan/device.hxx"

#include "renderer/config.hxx"

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
        create_framebuffer(std::shared_ptr<graphics::render_pass> render_pass, renderer::extent extent,
                           std::vector<std::shared_ptr<resource::image_view>> const &attachments);

    private:

        vulkan::device const &device_;
        renderer::config const &config_;

        //std::unique_ptr<resource::resource_map> resource_map_;
        std::shared_ptr<struct resource_map> resource_map_;
    };
}
