#include "swapchain.h"

VkSwapchainKHR swapChain;

VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;

std::vector<VkImage> swapChainImages;
std::vector<VkImageView> swapChainImageViews;

VkImage depthImage;
std::optional<DeviceMemoryPool::DeviceMemory> depthImageMemory;
VkImageView depthImageView;
VkDeviceSize depthImageOffset;


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

[[nodiscard]] VkExtent2D ChooseSwapExtent(VkSurfaceCapabilitiesKHR &surfaceCapabilities, std::uint32_t width, std::uint32_t height)
{
    if (surfaceCapabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
        return surfaceCapabilities.currentExtent;

    return {
        std::clamp(width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
        std::clamp(height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)
    };
}


void CreateSwapChain(VulkanDevice *device, VkSurfaceKHR surface, VkSwapchainKHR &swapChain, std::uint32_t width, std::uint32_t height,
                     VulkanQueue<PresentationQueue> const &presentationQueue, VulkanQueue<GraphicsQueue> const &graphicsQueue)
{
    auto swapChainSupportDetails = QuerySwapChainSupportDetails(device->physical_handle(), surface);

    auto surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupportDetails.formats);
    auto presentMode = ChooseSwapPresentMode(swapChainSupportDetails.presentModes);
    auto extent = ChooseSwapExtent(swapChainSupportDetails.capabilities, width, height);

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

    auto imageCount = swapChainSupportDetails.capabilities.minImageCount + 1;

    if (swapChainSupportDetails.capabilities.maxImageCount > 0)
        imageCount = std::min(imageCount, swapChainSupportDetails.capabilities.maxImageCount);

    VkSwapchainCreateInfoKHR swapchainCreateInfo{
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        nullptr, 0,
        surface,
        imageCount,
        surfaceFormat.format, surfaceFormat.colorSpace,
        extent,
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

    if (auto result = vkCreateSwapchainKHR(device->handle(), &swapchainCreateInfo, nullptr, &swapChain); result != VK_SUCCESS)
        throw std::runtime_error("failed to create required swap chain: "s + std::to_string(result));
}


void CreateSwapChainImageAndViews(VulkanDevice *device, std::vector<VkImage> &swapChainImages, std::vector<VkImageView> &swapChainImageViews)
{
    std::uint32_t imagesCount = 0;
    if (auto result = vkGetSwapchainImagesKHR(device->handle(), swapChain, &imagesCount, nullptr); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve swap chain images count: "s + std::to_string(result));

    swapChainImages.resize(imagesCount);
    if (auto result = vkGetSwapchainImagesKHR(device->handle(), swapChain, &imagesCount, std::data(swapChainImages)); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve swap chain images: "s + std::to_string(result));

    swapChainImageViews.clear();

    for (auto &&swapChainImage : swapChainImages) {
        auto imageView = CreateImageView(device->handle(), swapChainImage, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        swapChainImageViews.emplace_back(std::move(imageView));
    }
}


void CleanupSwapChain(VulkanDevice *vulkanDevice, VkSwapchainKHR swapChain)
{
    for (auto &&swapChainFramebuffer : swapChainFramebuffers)
        vkDestroyFramebuffer(vulkanDevice->handle(), swapChainFramebuffer, nullptr);

    swapChainFramebuffers.clear();

    for (auto &&swapChainImageView : swapChainImageViews)
        vkDestroyImageView(vulkanDevice->handle(), swapChainImageView, nullptr);

    if (swapChain)
        vkDestroySwapchainKHR(vulkanDevice->handle(), swapChain, nullptr);

    if (depthImageView)
        vkDestroyImageView(vulkanDevice->handle(), depthImageView, nullptr);

    /*if (depthImageMemory)
        vkFreeMemory(vulkanDevice->handle(), depthImageMemory, nullptr);*/

    if (depthImage)
        vkDestroyImage(vulkanDevice->handle(), depthImage, nullptr);

    swapChainImageViews.clear();
    swapChainImages.clear();
}