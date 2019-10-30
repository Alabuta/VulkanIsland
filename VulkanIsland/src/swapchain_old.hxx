#pragma once

#include "vulkan/instance.hxx"
#include "vulkan/device.hxx"
#include "platform/window.hxx"
#include "graphics/graphics.hxx"
#include "graphics/render_pass.hxx"
#include "resources/buffer.hxx"
#include "resources/image.hxx"
#include "resources/resource_manager.hxx"
#include "resources/framebuffer.hxx"


struct VulkanSwapchain final {
    VkSwapchainKHR handle;

    graphics::FORMAT format{graphics::FORMAT::UNDEFINED};
    graphics::FORMAT depth_format{graphics::FORMAT::UNDEFINED};

    VkExtent2D extent;

    std::shared_ptr<resource::texture> colorTexture, depthTexture;

    std::vector<VkImage> images;
    std::vector<std::shared_ptr<resource::image_view>> views;

    std::vector<std::shared_ptr<resource::framebuffer>> framebuffers;
};


struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;

    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

[[nodiscard]] SwapChainSupportDetails QuerySwapChainSupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);


[[nodiscard]] std::optional<VulkanSwapchain>
CreateSwapchain(vulkan::device &device, ResourceManager &resource_manager, VkSurfaceKHR surface, std::uint32_t width, std::uint32_t height,
                VkCommandPool transferCommandPool);

void CleanupSwapchain(vulkan::device const &device, VulkanSwapchain &swapchain) noexcept;


void CreateFramebuffers(resource::resource_manager &resource_manager, std::shared_ptr<graphics::render_pass> render_pass, VulkanSwapchain &swapchain);
