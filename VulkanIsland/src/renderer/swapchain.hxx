#pragma once

#include "vulkan/instance.hxx"
#include "vulkan/device.hxx"
#include "platform/window.hxx"
#include "graphics/graphics.hxx"
#include "resources/buffer.hxx"
#include "resources/image.hxx"


namespace renderer
{
    struct surface_format final {
        graphics::FORMAT format;
        graphics::COLOR_SPACE color_space;
    };

    struct extent final {
        std::uint32_t widht, height;
    };

    class platform_surface final {
    public:

        platform_surface(vulkan::instance const &instance, platform::window &window);

        VkSurfaceKHR handle() const noexcept { return handle_; }

    private:

        VkSurfaceKHR handle_{VK_NULL_HANDLE};
    };
}

namespace renderer
{
    class swapchain final {
    public:

        swapchain(vulkan::device const &device, renderer::platform_surface const &platform_surface,
                  renderer::surface_format surface_format, renderer::extent extent);

        ~swapchain();

        renderer::surface_format const &surface_format() const noexcept { return surface_format_; }

    private:

        vulkan::device const &device_;

        VkSwapchainKHR handle_{VK_NULL_HANDLE};

        renderer::extent extent_;

        renderer::surface_format surface_format_;
        graphics::PRESENTATION_MODE presentation_mode_;

        graphics::IMAGE_USAGE image_usage_{graphics::IMAGE_USAGE::COLOR_ATTACHMENT};

        std::vector<VkImage> images_;
        std::vector<VkImageView> image_views_;

        swapchain() = delete;
        swapchain(swapchain const &) = delete;
        swapchain(swapchain &&) = delete;
    };
}
