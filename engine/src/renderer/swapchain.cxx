#include <cmath>
#include <vector>
#include <ranges>
#include <optional>
#include <iostream>
#include <algorithm>

#include <fmt/format.h>

#include "utility/mpl.hxx"
#include "utility/exceptions.hxx"
#include "graphics/graphics_api.hxx"
#include "command_buffer.hxx"
#include "swapchain.hxx"


#if USE_WIN32
VKAPI_ATTR VkResult VKAPI_CALL vkCreateWin32SurfaceKHR(
VkInstance instance, VkWin32SurfaceCreateInfoKHR const *pCreateInfo, VkAllocationCallbacks const *pAllocator, VkSurfaceKHR *pSurface)
{
    auto traverse = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR"));

    if (traverse)
        return traverse(instance, pCreateInfo, pAllocator, pSurface);

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}
#endif


namespace
{
    renderer::surface_format
    choose_supported_surface_format(renderer::swapchain_support_details const &support_details,
                                    std::vector<renderer::surface_format> const &required_surface_formats)
    {
        auto &&surface = support_details.surface_formats;

        if (std::size(surface) == 1 && surface.at(0).format == graphics::FORMAT::UNDEFINED)
            return { graphics::FORMAT::BGRA8_UNORM, graphics::COLOR_SPACE::SRGB_NONLINEAR };

        for (auto [format, color_space] : required_surface_formats) {
            auto supported = std::ranges::any_of(surface, [f = format, cs = color_space] (auto &&surface_format)
            {
                return surface_format.format == f && surface_format.color_space == cs;
            });

            if (supported)
                return { format, color_space };
        }

        throw vulkan::swapchain_exception("none of required surface formats are supported");
    }

    graphics::PRESENTATION_MODE
    choose_supported_presentation_mode(renderer::swapchain_support_details const &support_details,
                                       std::vector<graphics::PRESENTATION_MODE> &&required_modes)
    {
        auto &&supported_modes = support_details.presentation_modes;

        auto it = std::ranges::find_first_of(required_modes, supported_modes);

        return it != std::cend(required_modes) ? *it : graphics::PRESENTATION_MODE::FIFO;
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

    std::vector<VkImage> get_swapchain_image_handles(vulkan::device const &device, renderer::swapchain const &swapchain)
    {
        std::uint32_t image_count = 0;

        if (auto result = vkGetSwapchainImagesKHR(device.handle(), swapchain.handle(), &image_count, nullptr); result != VK_SUCCESS)
            throw vulkan::swapchain_exception(fmt::format("failed to retrieve swap chain images count: {0:#x}", result));

        std::vector<VkImage> handles(image_count);

        if (auto result = vkGetSwapchainImagesKHR(device.handle(), swapchain.handle(), &image_count, std::data(handles)); result != VK_SUCCESS)
            throw vulkan::swapchain_exception(fmt::format("failed to retrieve swap chain images: {0:#x}", result));

        return handles;
    }
}

namespace renderer
{
    swapchain::swapchain(vulkan::device const &device, renderer::platform_surface const &platform_surface,
                         renderer::surface_format surface_format, renderer::extent extent) : device_{device}
    {
        auto &&presentation_queue = device.presentation_queue;
        auto &&graphics_queue = device.graphics_queue;

        auto swapchain_support_details = device.query_swapchain_support_details(platform_surface);

        surface_format_ = choose_supported_surface_format(swapchain_support_details, std::vector{surface_format});

        presentation_mode_ = choose_supported_presentation_mode(swapchain_support_details, std::vector{
        #ifndef _DEBUG
            graphics::PRESENTATION_MODE::MAILBOX,
        #endif
            graphics::PRESENTATION_MODE::FIFO_RELAXED,
            graphics::PRESENTATION_MODE::IMMEDIATE
        });

        extent_ = adjust_swapchain_extent(swapchain_support_details, extent);

        {
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
                std::cout << "graphics and presentation queues are not from one family\n";

                create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;

                auto queue_family_indices = std::array{
                    graphics_queue.family(), presentation_queue.family()
                };

                create_info.queueFamilyIndexCount = static_cast<std::uint32_t>(std::size(queue_family_indices));
                create_info.pQueueFamilyIndices = std::data(queue_family_indices);
            }

            if (auto result = vkCreateSwapchainKHR(device.handle(), &create_info, nullptr, &handle_); result != VK_SUCCESS)
                throw vulkan::swapchain_exception(fmt::format("failed to create required swap chain: {0:#x}", result));
        }

        for (auto image_handle : get_swapchain_image_handles(device, *this)) {
            auto image = std::shared_ptr<resource::image>(
                new resource::image{nullptr, image_handle, surface_format_.format, graphics::IMAGE_TILING::OPTIMAL, 1, extent_}
            );

            VkImageViewCreateInfo const create_info{
                VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                nullptr, 0,
                image->handle(),
                convert_to::vulkan(graphics::IMAGE_VIEW_TYPE::TYPE_2D),
                convert_to::vulkan(surface_format_.format),
                { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
                { convert_to::vulkan(graphics::IMAGE_ASPECT::COLOR_BIT), 0, 1, 0, 1 }
            };

            VkImageView image_view_handle;

            if (auto result = vkCreateImageView(device.handle(), &create_info, nullptr, &image_view_handle); result != VK_SUCCESS)
                throw vulkan::swapchain_exception(fmt::format("failed to create image view: {0:#x}", result));

            images_.push_back(image);

            image_views_.push_back(std::shared_ptr<resource::image_view>(
                new resource::image_view{image_view_handle, image, graphics::IMAGE_VIEW_TYPE::TYPE_2D},
                [device = device_.handle()] (resource::image_view *ptr_image_view)
                {
                    vkDestroyImageView(device, ptr_image_view->handle(), nullptr);

                    delete ptr_image_view;
                }
            ));
        }
    }

    swapchain::~swapchain()
    {
        image_views_.clear();

        vkDestroySwapchainKHR(device_.handle(), handle_, nullptr);

        images_.clear();
    }
}


std::unique_ptr<renderer::swapchain>
create_swapchain(vulkan::device const &device, renderer::platform_surface const &platform_surface, renderer::extent extent)
{
    auto surface_formats = std::vector<renderer::surface_format>{
        { graphics::FORMAT::BGRA8_SRGB, graphics::COLOR_SPACE::SRGB_NONLINEAR },
        { graphics::FORMAT::RGBA8_SRGB, graphics::COLOR_SPACE::SRGB_NONLINEAR }
    };

    std::unique_ptr<renderer::swapchain> swapchain;

    for (auto surface_format : surface_formats) {
        try {
            swapchain = std::make_unique<renderer::swapchain>(device, platform_surface, surface_format, extent);

        } catch (vulkan::swapchain_exception const &ex) {
            std::cout << ex.what() << std::endl;
        }

        if (swapchain)
            break;
    }

    return swapchain;
}
