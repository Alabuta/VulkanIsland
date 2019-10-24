#include <cmath>
#include <vector>
#include <optional>
#include <algorithm>

#include <fmt/format.h>

#include "utility/mpl.hxx"
#include "graphics/graphics_api.hxx"
#include "command_buffer.hxx"
#include "swapchain.hxx"


#if USE_WIN32
VKAPI_ATTR VkResult VKAPI_CALL vkCreateWin32SurfaceKHR(
VkInstance vulkan_instance, VkWin32SurfaceCreateInfoKHR const *pCreateInfo, VkAllocationCallbacks const *pAllocator, VkSurfaceKHR *pSurface)
{
    auto traverse = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(vkGetInstanceProcAddr(vulkan_instance, "vkCreateWin32SurfaceKHR"));

    if (traverse)
        return traverse(vulkan_instance, pCreateInfo, pAllocator, pSurface);

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}
#endif


namespace
{

    renderer::surface_format
    choose_supported_surface_format(renderer::swapchain_support_details const &support_details,
                                    std::vector<renderer::surface_format> const &required_surface_formats)
    {
        auto &&supported_surface_formats = support_details.surface_formats;

        if (std::size(supported_surface_formats) == 1 && supported_surface_formats.at(0).format == VK_FORMAT_UNDEFINED)
            return { graphics::FORMAT::BGRA8_UNORM, graphics::COLOR_SPACE::SRGB_NONLINEAR };

        for (auto [format, color_space] : required_surface_formats) {
            auto supported = std::any_of(std::cbegin(supported_surface_formats), std::cend(supported_surface_formats),
                                         [format, color_space] (auto &&surface_format)
            {
                return surface_format.format == convert_to::vulkan(format) && surface_format.colorSpace == convert_to::vulkan(color_space);
            });

            if (supported)
                return { format, color_space };
        }

        throw std::runtime_error("none of required surface formats are supported"s);
    }

    graphics::PRESENTATION_MODE
    choose_supported_presentation_mode(renderer::swapchain_support_details const &support_details,
                                       std::vector<graphics::PRESENTATION_MODE> &&required_modes)
    {
        auto &&supported_present_modes = support_details.presentation_modes;

        for (auto required_mode : required_modes) {
            auto supported = std::any_of(std::cbegin(supported_present_modes), std::cend(supported_present_modes), [required_mode] (auto mode)
            {
                return mode == convert_to::vulkan(required_mode);
            });

            if (supported)
                return required_mode;
        }

        return graphics::PRESENTATION_MODE::FIFO;
    }

    renderer::extent adjust_swapchain_extent(renderer::swapchain_support_details const &support_details, renderer::extent extent)
    {
        auto &&surface_capabilities = support_details.surface_capabilities;
        
        if (surface_capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
            auto [width, height] = surface_capabilities.currentExtent;

            return { width, height };
        }

        return {
            std::clamp(extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width),
            std::clamp(extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height)
        };
    }

    std::vector<VkImage> get_swapchain_images(vulkan::device const &device, renderer::swapchain const &swapchain)
    {
        std::uint32_t image_count = 0;

        if (auto result = vkGetSwapchainImagesKHR(device.handle(), swapchain.handle(), &image_count, nullptr); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to retrieve swap chain images count: {0:#x}\n"s, result));

        std::vector<VkImage> images(image_count);

        if (auto result = vkGetSwapchainImagesKHR(device.handle(), swapchain.handle(), &image_count, std::data(images)); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to retrieve swap chain images: {0:#x}\n"s, result));

        return images;
    }

    std::vector<VkImageView>
    get_swapchain_image_views(vulkan::device const &device, renderer::swapchain const &swapchain)
    {
        auto &&surface_format = swapchain.surface_format();

        std::vector<VkImageView> image_views;

        std::transform(std::cbegin(swapchain.images()), std::cend(swapchain.images()),
                       std::back_inserter(image_views), [&] (auto &&swapchain_image)
        {
            VkImageViewCreateInfo const create_info{
                VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                nullptr, 0,
                swapchain_image,
                convert_to::vulkan(graphics::IMAGE_VIEW_TYPE::TYPE_2D),
                convert_to::vulkan(surface_format.format),
                { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
                { convert_to::vulkan(graphics::IMAGE_ASPECT::COLOR_BIT), 0, 1, 0, 1 }
            };

            VkImageView handle;

            if (auto result = vkCreateImageView(device.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
                throw std::runtime_error(fmt::format("failed to create image view: {0:#x}\n"s, result));

            return handle;
        });

        return image_views;
    }
}

namespace renderer
{
    platform_surface::platform_surface(vulkan::instance const &instance, platform::window &window)
    {
        if (auto result = glfwCreateWindowSurface(instance.handle(), window.handle(), nullptr, &handle_); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to create window surface: {0:#x}\n"s, result));
    }
}

namespace renderer
{
    swapchain::swapchain(vulkan::device const &device, renderer::platform_surface const &platform_surface,
                         renderer::surface_format surface_format, renderer::extent extent) : device_{device}
    {
        auto &&presentation_queue = device.presentation_queue;
        auto &&graphics_queue = device.graphics_queue;
        auto &&transfer_queue = device.transfer_queue;

        auto swapchain_support_details = device.query_swapchain_support_details(&platform_surface);

        surface_format_ = choose_supported_surface_format(swapchain_support_details, std::vector{surface_format});

        presentation_mode_ = choose_supported_presentation_mode(swapchain_support_details, std::vector{
        #ifndef _DEBUG
            graphics::PRESENTATION_MODE::MAILBOX,
        #endif
            graphics::PRESENTATION_MODE::FIFO_RELAXED,
            graphics::PRESENTATION_MODE::IMMEDIATE
        });

        extent_ = adjust_swapchain_extent(swapchain_support_details, extent);

        auto image_count = swapchain_support_details.surface_capabilities.minImageCount + 1;

        if (swapchain_support_details.surface_capabilities.maxImageCount > 0)
            image_count = std::min(image_count, swapchain_support_details.surface_capabilities.maxImageCount);

        VkSwapchainCreateInfoKHR create_info{
            VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            nullptr, 0,
            platform_surface.handle(),
            image_count,
            convert_to::vulkan(surface_format_.format), convert_to::vulkan(surface_format_.color_space),
            VkExtent2D{extent_.width, extent_.height},
            1,
            convert_to::vulkan(image_usage_),
            VK_SHARING_MODE_EXCLUSIVE,
            0, nullptr,
            swapchain_support_details.surface_capabilities.currentTransform,
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            convert_to::vulkan(presentation_mode_),
            VK_FALSE,
            nullptr
        };

        if (graphics_queue.family() != presentation_queue.family()) {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;

            auto queue_family_indices = std::array{
                graphics_queue.family(), presentation_queue.family()
            };

            create_info.queueFamilyIndexCount = static_cast<std::uint32_t>(std::size(queue_family_indices));
            create_info.pQueueFamilyIndices = std::data(queue_family_indices);
        }

        if (auto result = vkCreateSwapchainKHR(device.handle(), &create_info, nullptr, &handle_); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to create required swap chain: {0:#x}\n"s, result));

        images_ = get_swapchain_images(device, *this);

        image_views_ = get_swapchain_image_views(device, *this);
    }

    swapchain::~swapchain()
    {
        for (auto &&image_view : image_views_)
            vkDestroyImageView(device_.handle(), image_view, nullptr);

        vkDestroySwapchainKHR(device_.handle(), handle_, nullptr);
    }
}
