#pragma once

#include "main.hxx"
#include "device/device.hxx"
#include "resources/buffer.hxx"
#include "resources/image.hxx"
#include "renderer/graphics.hxx"


struct VulkanSwapchain final {
    VkSwapchainKHR handle;

    graphics::FORMAT format;
    VkExtent2D extent;

    graphics::FORMAT depth_format;

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
