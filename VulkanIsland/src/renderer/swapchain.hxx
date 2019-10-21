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

        VkSurfaceKHR handle_;
    };
}

namespace renderer
{
    class swapchain final {
    public:

        swapchain(vulkan::device const &device, renderer::platform_surface const &platform_surface,
                  renderer::surface_format surface_format, renderer::extent extent);

    private:

        VkSwapchainKHR handle_;

        std::vector<VkImage> images_;
        std::vector<VkImageView> views_;

        std::vector<VkFramebuffer> framebuffers_;

        swapchain() = delete;
        swapchain(swapchain const &) = delete;
        swapchain(swapchain &&) = delete;
    };
}
