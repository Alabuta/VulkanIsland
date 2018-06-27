#include "swapchain.h"


#if USE_WIN32
VKAPI_ATTR VkResult VKAPI_CALL vkCreateWin32SurfaceKHR(
    VkInstance vulkanInstance, VkWin32SurfaceCreateInfoKHR const *pCreateInfo, VkAllocationCallbacks const *pAllocator, VkSurfaceKHR *pSurface)
{
    auto traverse = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(vkGetInstanceProcAddr(vulkanInstance, "vkCreateWin32SurfaceKHR"));

    if (traverse)
        return traverse(vulkanInstance, pCreateInfo, pAllocator, pSurface);

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}
#endif

namespace {

[[nodiscard]] VkImageView CreateImageView(VkDevice device, VkImage &image, VkFormat format, VkImageAspectFlags aspectFlags, std::uint32_t mipLevels)
{
    VkImageViewCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr, 0,
        image,
        VK_IMAGE_VIEW_TYPE_2D,
        format,
        { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
        { aspectFlags, 0, mipLevels, 0, 1 }
    };

    VkImageView imageView;

    if (auto result = vkCreateImageView(device, &createInfo, nullptr, &imageView); result != VK_SUCCESS)
        throw std::runtime_error("failed to create image view: "s + std::to_string(result));

    return imageView;
}

[[nodiscard]] VkExtent2D ChooseSwapExtent(VkSurfaceCapabilitiesKHR &surfaceCapabilities, std::uint32_t width, std::uint32_t height)
{
    if (surfaceCapabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
        return surfaceCapabilities.currentExtent;

    return {
        std::clamp(width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
        std::clamp(height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)
    };
}

template<class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
[[nodiscard]] VkSurfaceFormatKHR ChooseSwapSurfaceFormat(T &&surfaceFormats)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkSurfaceFormatKHR>, "iterable object does not contain VkSurfaceFormatKHR elements");

    if (surfaceFormats.size() == 1 && surfaceFormats.at(0).format == VK_FORMAT_UNDEFINED)
        return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

    auto supported = std::any_of(surfaceFormats.cbegin(), surfaceFormats.cend(), [] (auto &&surfaceFormat)
    {
        return surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    });

    if (supported)
        return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

    return surfaceFormats.at(0);
}

template<class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
[[nodiscard]] VkPresentModeKHR ChooseSwapPresentMode(T &&presentModes)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkPresentModeKHR>, "iterable object does not contain VkPresentModeKHR elements");

#ifndef _DEBUG
    auto mailbox = std::any_of(presentModes.cbegin(), presentModes.cend(), [] (auto &&mode)
    {
        return mode == VK_PRESENT_MODE_MAILBOX_KHR;
    });

    if (mailbox)
        return VK_PRESENT_MODE_MAILBOX_KHR;
#endif

    auto relaxed = std::any_of(presentModes.cbegin(), presentModes.cend(), [] (auto &&mode)
    {
        return mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    });

    if (relaxed)
        return VK_PRESENT_MODE_FIFO_RELAXED_KHR;

    auto immediate = std::any_of(presentModes.cbegin(), presentModes.cend(), [] (auto &&mode)
    {
        return mode == VK_PRESENT_MODE_IMMEDIATE_KHR;
    });

    if (immediate)
        return VK_PRESENT_MODE_IMMEDIATE_KHR;

    return VK_PRESENT_MODE_FIFO_KHR;
}

}


[[nodiscard]] SwapChainSupportDetails QuerySwapChainSupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;

    if (auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve device surface capabilities: "s + std::to_string(result));

    std::uint32_t formatsCount = 0;
    if (auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, nullptr); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve device surface formats count: "s + std::to_string(result));

    details.formats.resize(formatsCount);
    if (auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, std::data(details.formats)); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve device surface formats: "s + std::to_string(result));

    if (details.formats.empty())
        return { };

    std::uint32_t presentModeCount = 0;
    if (auto result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve device surface presentation modes count: "s + std::to_string(result));

    details.presentModes.resize(presentModeCount);
    if (auto result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, std::data(details.presentModes)); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve device surface presentation modes: "s + std::to_string(result));

    if (details.presentModes.empty())
        return { };

    return details;
}



[[nodiscard]] std::optional<VulkanSwapchain>
CreateSwapchain(VulkanDevice &device, VkSurfaceKHR surface, std::uint32_t width, std::uint32_t height,
                VulkanQueue<PresentationQueue> const &presentationQueue, VulkanQueue<GraphicsQueue> const &graphicsQueue,
                TransferQueue transferQueue, VkCommandPool transferCommandPool)
{
    VulkanSwapchain swapchain;
    
    auto swapChainSupportDetails = QuerySwapChainSupportDetails(device.physical_handle(), surface);

    auto surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupportDetails.formats);
    auto presentMode = ChooseSwapPresentMode(swapChainSupportDetails.presentModes);

    swapchain.format = surfaceFormat.format;
    swapchain.extent = ChooseSwapExtent(swapChainSupportDetails.capabilities, width, height);

    auto imageCount = swapChainSupportDetails.capabilities.minImageCount + 1;

    if (swapChainSupportDetails.capabilities.maxImageCount > 0)
        imageCount = std::min(imageCount, swapChainSupportDetails.capabilities.maxImageCount);

    VkSwapchainCreateInfoKHR swapchainCreateInfo{
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        nullptr, 0,
        surface,
        imageCount,
        surfaceFormat.format, surfaceFormat.colorSpace,
        swapchain.extent,
        1,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0, nullptr,
        swapChainSupportDetails.capabilities.currentTransform,
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        presentMode,
        VK_FALSE,
        nullptr
    };

    auto const queueFamilyIndices = make_array(
        graphicsQueue.family(), presentationQueue.family()
    );

    if (graphicsQueue.family() != presentationQueue.family()) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = static_cast<std::uint32_t>(std::size(queueFamilyIndices));
        swapchainCreateInfo.pQueueFamilyIndices = std::data(queueFamilyIndices);
    }

    if (auto result = vkCreateSwapchainKHR(device.handle(), &swapchainCreateInfo, nullptr, &swapchain.handle); result != VK_SUCCESS) {
        std::cerr << "failed to create required swap chain: "s << result << '\n';
        return { };
    }
    
    std::uint32_t imagesCount = 0;

    if (auto result = vkGetSwapchainImagesKHR(device.handle(), swapchain.handle, &imagesCount, nullptr); result != VK_SUCCESS) {
        std::cerr << "failed to retrieve swap chain images count: "s << result << '\n';
        return { };
    }

    swapchain.images.resize(imagesCount);

    if (auto result = vkGetSwapchainImagesKHR(device.handle(), swapchain.handle, &imagesCount, std::data(swapchain.images)); result != VK_SUCCESS) {
        std::cerr << "failed to retrieve swap chain images: "s << result << '\n';
        return { };
    }

    swapchain.views.clear();

    for (auto &&swapChainImage : swapchain.images) {
        auto imageView = CreateImageView(device.handle(), swapChainImage, swapchain.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        swapchain.views.emplace_back(std::move(imageView));
    }

    if (auto result = CreateDepthAttachement(device, transferQueue, transferCommandPool, swapchain.extent.width, swapchain.extent.height); !result) {
        std::cerr << "failed to create depth texture\n"s;
        return { };
    }

    else swapchain.depthTexture = std::move(result.value());

    return swapchain;
}

void CleanupSwapchain(VulkanDevice const &device, VulkanSwapchain &swapchain) noexcept
{
    for (auto &&framebuffer : swapchain.framebuffers)
        vkDestroyFramebuffer(device.handle(), framebuffer, nullptr);

    swapchain.framebuffers.clear();

    for (auto &&view : swapchain.views)
        vkDestroyImageView(device.handle(), view, nullptr);

    vkDestroySwapchainKHR(device.handle(), swapchain.handle, nullptr);
    swapchain.views.clear();

    vkDestroyImageView(device.handle(), swapchain.depthTexture.view.handle(), nullptr);
    swapchain.depthTexture.image.reset();

    swapchain.images.clear();
}


[[nodiscard]] std::optional<VulkanTexture>
CreateDepthAttachement(VulkanDevice &device, TransferQueue transferQueue, VkCommandPool transferCommandPool, std::uint32_t width, std::uint32_t height)
{
    std::optional<VulkanTexture> texture;

    if (auto const format = FindDepthImageFormat(device.physical_handle()); format) {
        auto constexpr mipLevels = 1u;

        auto constexpr usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        auto constexpr propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        auto constexpr tiling = VK_IMAGE_TILING_OPTIMAL;

        texture = CreateTexture(device, *format, VK_IMAGE_VIEW_TYPE_2D, width, height, mipLevels, tiling, VK_IMAGE_ASPECT_DEPTH_BIT, usageFlags, propertyFlags);

        if (texture)
            TransitionImageLayout(device, transferQueue, *texture->image, VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, transferCommandPool);
    }

    else std::cerr << "failed to find format for depth attachement\n"s;

    return texture;
}
