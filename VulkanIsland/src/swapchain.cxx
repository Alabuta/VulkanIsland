#include <vector>

#include <fmt/format.h>

#include "utility/mpl.hxx"
#include "swapchain.hxx"
#include "commandBuffer.hxx"
#include "renderer/graphics_api.hxx"


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


namespace presentation
{
    struct surface_format final {
        graphics::FORMAT format;
        graphics::COLOR_SPACE color_space;
    };
}


namespace {

[[nodiscard]] VkImageView CreateImageView(VkDevice device, VkImage &image, graphics::FORMAT format, VkImageAspectFlags aspectFlags, std::uint32_t mipLevels)
{
    VkImageViewCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr, 0,
        image,
        VK_IMAGE_VIEW_TYPE_2D,
        convert_to::vulkan(format),
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

[[nodiscard]] presentation::surface_format
ChooseSwapSurfaceFormat(std::vector<VkSurfaceFormatKHR> const &supported, std::vector<presentation::surface_format> const &required)
{
    if (std::size(supported) == 1 && supported.at(0).format == VK_FORMAT_UNDEFINED)
        return { graphics::FORMAT::BGRA8_UNORM, graphics::COLOR_SPACE::SRGB_NONLINEAR };

    for (auto [format, color_space] : required) {
        auto exist = std::any_of(std::cbegin(supported), std::cend(supported), [format, color_space] (auto &&surfaceFormat)
        {
            return surfaceFormat.format == convert_to::vulkan(format) && surfaceFormat.colorSpace == convert_to::vulkan(color_space);
        });

        if (exist)
            return { format, color_space };
    }

    throw std::runtime_error("required surface formats is not supported"s);
}

template<class T> requires mpl::iterable<std::remove_cvref_t<T>>
[[nodiscard]] VkPresentModeKHR ChooseSwapPresentMode(T &&presentModes)
{
    static_assert(std::is_same_v<typename std::remove_cvref_t<T>::value_type, VkPresentModeKHR>, "iterable object does not contain VkPresentModeKHR elements");

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

[[nodiscard]] std::optional<VulkanTexture>
CreateColorAttachement(VulkanDevice &device, TransferQueue transferQueue, VkCommandPool transferCommandPool, graphics::FORMAT format, std::uint16_t width, std::uint16_t height)
{
    std::optional<VulkanTexture> texture;

    auto constexpr mipLevels = 1u;

    auto constexpr usageFlags = graphics::IMAGE_USAGE::TRANSIENT_ATTACHMENT | graphics::IMAGE_USAGE::COLOR_ATTACHMENT | graphics::IMAGE_USAGE::TRANSFER_DESTINATION;
    auto constexpr propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    auto constexpr tiling = graphics::IMAGE_TILING::OPTIMAL;

    texture = CreateTexture(device, format, graphics::IMAGE_VIEW_TYPE::TYPE_2D, width, height, mipLevels, device.samplesCount(),
                            tiling, VK_IMAGE_ASPECT_COLOR_BIT, usageFlags, propertyFlags);

    if (texture)
        TransitionImageLayout(device, transferQueue, *texture->image, graphics::IMAGE_LAYOUT::UNDEFINED,
                              graphics::IMAGE_LAYOUT::COLOR_ATTACHMENT, transferCommandPool);

    return texture;
}

[[nodiscard]]std::pair<std::optional<VulkanTexture>, std::optional<graphics::FORMAT>>
CreateDepthAttachement(VulkanDevice &device, TransferQueue transferQueue, VkCommandPool transferCommandPool, std::uint16_t width, std::uint16_t height)
{
    std::optional<VulkanTexture> texture;

    if (auto const format = FindDepthImageFormat(device); format) {
        auto constexpr mipLevels = 1u;

        auto constexpr usageFlags = graphics::IMAGE_USAGE::DEPTH_STENCIL_ATTACHMENT;
        auto constexpr propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        auto constexpr tiling = graphics::IMAGE_TILING::OPTIMAL;

        texture = CreateTexture(device, *format, graphics::IMAGE_VIEW_TYPE::TYPE_2D, width, height, mipLevels, device.samplesCount(),
                                tiling, VK_IMAGE_ASPECT_DEPTH_BIT, usageFlags, propertyFlags);

        if (texture)
            TransitionImageLayout(device, transferQueue, *texture->image, graphics::IMAGE_LAYOUT::UNDEFINED,
                                  graphics::IMAGE_LAYOUT::DEPTH_STENCIL_ATTACHMENT, transferCommandPool);

        return std::make_pair(texture, format);
    }

    else std::cerr << "failed to find format for depth attachement\n"s;

    return { };
}

}


[[nodiscard]] SwapChainSupportDetails QuerySwapChainSupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;

    if (auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities); result != VK_SUCCESS)
        throw std::runtime_error(fmt::format("failed to retrieve device surface capabilities: {0:#x}\n"s, result));

    std::uint32_t formatsCount = 0;
    if (auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, nullptr); result != VK_SUCCESS)
        throw std::runtime_error(fmt::format("failed to retrieve device surface formats count: {0:#x}\n"s, result));

    if (formatsCount == 0)
        return { };

    details.formats.resize(formatsCount);

    if (auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, std::data(details.formats)); result != VK_SUCCESS)
        throw std::runtime_error(fmt::format("failed to retrieve device surface formats: {0:#x}\n"s, result));

    std::uint32_t presentModeCount = 0;
    if (auto result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr); result != VK_SUCCESS)
        throw std::runtime_error(fmt::format("failed to retrieve device surface presentation modes count: {0:#x}\n"s, result));

    details.presentModes.resize(presentModeCount);
    if (auto result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, std::data(details.presentModes)); result != VK_SUCCESS)
        throw std::runtime_error(fmt::format("failed to retrieve device surface presentation modes: {0:#x}\n"s, result));

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

    auto surfaceFormat = ChooseSwapSurfaceFormat(
        swapChainSupportDetails.formats,
        {
            { graphics::FORMAT::RGBA8_SRGB, graphics::COLOR_SPACE::SRGB_NONLINEAR },
            { graphics::FORMAT::BGRA8_SRGB, graphics::COLOR_SPACE::SRGB_NONLINEAR }
        }
    );
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
        convert_to::vulkan(surfaceFormat.format), convert_to::vulkan(surfaceFormat.color_space),
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

    auto const queueFamilyIndices = mpl::make_array(
        graphicsQueue.family(), presentationQueue.family()
    );

    if (graphicsQueue.family() != presentationQueue.family()) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = static_cast<std::uint32_t>(std::size(queueFamilyIndices));
        swapchainCreateInfo.pQueueFamilyIndices = std::data(queueFamilyIndices);
    }

    if (auto result = vkCreateSwapchainKHR(device.handle(), &swapchainCreateInfo, nullptr, &swapchain.handle); result != VK_SUCCESS) {
        std::cerr << fmt::format("failed to create required swap chain: {0:#x}\n"s, result);
        return { };
    }
    
    std::uint32_t imagesCount = 0;

    if (auto result = vkGetSwapchainImagesKHR(device.handle(), swapchain.handle, &imagesCount, nullptr); result != VK_SUCCESS) {
        std::cerr << fmt::format("failed to retrieve swap chain images count: {0:#x}\n"s, result);
        return { };
    }

    swapchain.images.resize(imagesCount);

    if (auto result = vkGetSwapchainImagesKHR(device.handle(), swapchain.handle, &imagesCount, std::data(swapchain.images)); result != VK_SUCCESS) {
        std::cerr << fmt::format("failed to retrieve swap chain images: {0:#x}\n"s, result);
        return { };
    }

    swapchain.views.clear();

    for (auto &&swapChainImage : swapchain.images) {
        auto imageView = CreateImageView(device.handle(), swapChainImage, swapchain.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        swapchain.views.emplace_back(std::move(imageView));
    }

    auto const swapchainWidth = static_cast<std::uint16_t>(swapchain.extent.width);
    auto const swapchainHeight = static_cast<std::uint16_t>(swapchain.extent.height);

    if (auto result = CreateColorAttachement(device, transferQueue, transferCommandPool, swapchain.format, swapchainWidth, swapchainHeight); !result) {
        std::cerr << "failed to create color texture\n"s;
        return { };
    }

    else swapchain.colorTexture = std::move(result.value());

    if (auto [texture, format] = CreateDepthAttachement(device, transferQueue, transferCommandPool, swapchainWidth, swapchainHeight); !texture) {
        std::cerr << "failed to create depth texture\n"s;
        return { };
    }

    else {
        swapchain.depthTexture = std::move(*texture);
        swapchain.depth_format = *format;
    }

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

    vkDestroyImageView(device.handle(), swapchain.colorTexture.view.handle(), nullptr);
    swapchain.colorTexture.image.reset();

    vkDestroyImageView(device.handle(), swapchain.depthTexture.view.handle(), nullptr);
    swapchain.depthTexture.image.reset();

    swapchain.images.clear();
}


void CreateFramebuffers(VulkanDevice const &device, VkRenderPass renderPass, VulkanSwapchain &swapchain)
{
    auto &&framebuffers = swapchain.framebuffers;
    auto &&views = swapchain.views;

    framebuffers.clear();

    std::transform(std::cbegin(views), std::cend(views), std::back_inserter(framebuffers), [&device, renderPass, &swapchain] (auto &&view)
    {
        auto const attachements = std::array{swapchain.colorTexture.view.handle(), swapchain.depthTexture.view.handle(), view};

        VkFramebufferCreateInfo const createInfo{
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            nullptr, 0,
            renderPass,
            static_cast<std::uint32_t>(std::size(attachements)), std::data(attachements),
            swapchain.extent.width, swapchain.extent.height,
            1
        };

        VkFramebuffer framebuffer;

        if (auto result = vkCreateFramebuffer(device.handle(), &createInfo, nullptr, &framebuffer); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to create a framebuffer: {0:#x}\n"s, result));

        return framebuffer;
    });
}
