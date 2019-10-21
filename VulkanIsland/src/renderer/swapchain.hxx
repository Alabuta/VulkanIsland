#pragma once

#include "vulkan/instance.hxx"
#include "vulkan/device.hxx"
#include "platform/window.hxx"
#include "graphics/graphics.hxx"
#include "resources/buffer.hxx"
#include "resources/image.hxx"


struct VulkanSwapchain final {
    VkSwapchainKHR handle;
namespace renderer
{
    struct surface_format final {
        graphics::FORMAT format;
        graphics::COLOR_SPACE color_space;
    };

    graphics::FORMAT format{graphics::FORMAT::UNDEFINED};
    graphics::FORMAT depth_format{graphics::FORMAT::UNDEFINED};

    VkExtent2D extent;
    struct extent final {
        std::uint32_t widht, height;
    };

    VulkanTexture colorTexture, depthTexture;
    class platform_surface final {
    public:

    std::vector<VkImage> images;
    std::vector<VkImageView> views;
        platform_surface(vulkan::instance const &instance, platform::window &window);

    std::vector<VkFramebuffer> framebuffers;
};
        VkSurfaceKHR handle() const noexcept { return handle_; }

    private:

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
        VkSurfaceKHR handle_;
    };
}

    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

[[nodiscard]] SwapChainSupportDetails QuerySwapChainSupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);


[[nodiscard]] std::optional<VulkanSwapchain>
CreateSwapchain(vulkan::device &device, ResourceManager &resource_manager, VkSurfaceKHR surface, std::uint32_t width, std::uint32_t height,
                VkCommandPool transferCommandPool);

void CleanupSwapchain(vulkan::device const &device, VulkanSwapchain &swapchain) noexcept;


void CreateFramebuffers(vulkan::device const &device, VkRenderPass renderPass, VulkanSwapchain &swapchain);
