#pragma once

#include "main.h"
#include "device.h"
#include "buffer.h"
#include "image.h"


extern VkSwapchainKHR swapChain;

extern VkFormat swapChainImageFormat;
extern VkExtent2D swapChainExtent;

extern std::vector<VkImage> swapChainImages;
extern std::vector<VkImageView> swapChainImageViews;
extern std::vector<VkFramebuffer> swapChainFramebuffers;

extern VulkanTexture depthTexture;
extern VkDeviceSize depthImageOffset;

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};



[[nodiscard]] SwapChainSupportDetails QuerySwapChainSupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

[[nodiscard]] VkExtent2D ChooseSwapExtent(VkSurfaceCapabilitiesKHR &surfaceCapabilities, std::uint32_t width, std::uint32_t height);


void CreateSwapChain(VulkanDevice const &device, VkSurfaceKHR surface, VkSwapchainKHR &swapChain, std::uint32_t width, std::uint32_t height,
                     VulkanQueue<PresentationQueue> const &presentationQueue, VulkanQueue<GraphicsQueue> const &graphicsQueue);

void CreateSwapChainImageAndViews(VulkanDevice const &device, std::vector<VkImage> &swapChainImages, std::vector<VkImageView> &swapChainImageViews);


void CleanupSwapChain(VulkanDevice const &device, VkSwapchainKHR swapChain);


[[nodiscard]] std::optional<VulkanTexture>
CreateDepthAttachement(VulkanDevice &device, TransferQueue transferQueue, VkCommandPool transferCommandPool);
