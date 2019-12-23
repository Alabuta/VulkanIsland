#pragma once

#include "vulkan/instance.hxx"
#include "vulkan/device.hxx"
#include "platform/window.hxx"
#include "graphics/graphics.hxx"
#include "resources/buffer.hxx"
#include "resources/image.hxx"


namespace renderer
{
    class swapchain final {
    public:

        swapchain(vulkan::device const &device, renderer::platform_surface const &platform_surface,
                  renderer::surface_format surface_format, renderer::extent extent);

        ~swapchain();

        VkSwapchainKHR handle() const noexcept { return handle_; }

        renderer::surface_format const &surface_format() const noexcept { return surface_format_; }

        renderer::extent extent() const noexcept { return extent_; }

        std::vector<std::shared_ptr<resource::image>> const &images() const noexcept { return images_; }
        std::vector<std::shared_ptr<resource::image_view>> const &image_views() const noexcept { return image_views_; }

    private:

        vulkan::device const &device_;

        VkSwapchainKHR handle_{VK_NULL_HANDLE};

        renderer::extent extent_;

        renderer::surface_format surface_format_;
        graphics::PRESENTATION_MODE presentation_mode_;

        graphics::IMAGE_USAGE image_usage_{graphics::IMAGE_USAGE::COLOR_ATTACHMENT};

        std::vector<std::shared_ptr<resource::image>> images_;
        std::vector<std::shared_ptr<resource::image_view>> image_views_;

        swapchain() = delete;
        swapchain(swapchain const &) = delete;
        swapchain(swapchain &&) = delete;
    };
}
