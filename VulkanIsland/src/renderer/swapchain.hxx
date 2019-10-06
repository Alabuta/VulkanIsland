#pragma once

#include "renderer/device.hxx"
#include "graphics/graphics.hxx"
#include "resources/buffer.hxx"
#include "resources/image.hxx"


struct VulkanSwapchain final {
    VkSwapchainKHR handle;

    graphics::FORMAT format{graphics::FORMAT::UNDEFINED};
    graphics::FORMAT depth_format{graphics::FORMAT::UNDEFINED};

    VkExtent2D extent;

    VulkanTexture colorTexture, depthTexture;

    std::vector<VkImage> images;
    std::vector<VkImageView> views;

    std::vector<VkFramebuffer> framebuffers;
};


struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;

    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

[[nodiscard]] SwapChainSupportDetails QuerySwapChainSupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);


[[nodiscard]] std::optional<VulkanSwapchain>
CreateSwapchain(VulkanDevice &device, VkSurfaceKHR surface, std::uint32_t width, std::uint32_t height,
                VulkanQueue<PresentationQueue> const &presentationQueue, VulkanQueue<GraphicsQueue> const &graphicsQueue,
                TransferQueue transferQueue, VkCommandPool transferCommandPool);

void CleanupSwapchain(VulkanDevice const &device, VulkanSwapchain &swapchain) noexcept;


void CreateFramebuffers(VulkanDevice const &device, VkRenderPass renderPass, VulkanSwapchain &swapchain);
