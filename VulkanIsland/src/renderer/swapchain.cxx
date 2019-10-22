#include <cmath>
#include <vector>
#include <optional>

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
    struct swapchain_support_details final {
        VkSurfaceCapabilitiesKHR surface_capabilities;

        std::vector<VkSurfaceFormatKHR> surface_formats;
        std::vector<VkPresentModeKHR> present_modes;
    };

    swapchain_support_details query_swapchain_support_details(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        VkSurfaceCapabilitiesKHR surface_capabilities;

        if (auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surface_capabilities); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to retrieve device surface capabilities: {0:#x}\n"s, result));

        std::uint32_t surface_formats_count = 0;

        if (auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surface_formats_count, nullptr); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to retrieve device surface formats count: {0:#x}\n"s, result));

        if (surface_formats_count == 0)
            throw std::runtime_error("zero number of presentation format pairs"s);

        std::vector<VkSurfaceFormatKHR> surface_formats(surface_formats_count);

        if (auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surface_formats_count, std::data(surface_formats)); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to retrieve device surface formats: {0:#x}\n"s, result));

        std::uint32_t present_modes_count = 0;

        if (auto result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_modes_count, nullptr); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to retrieve device surface presentation modes count: {0:#x}\n"s, result));

        if (present_modes_count == 0)
            throw std::runtime_error("zero number of presentation modes"s);

        std::vector<VkPresentModeKHR> present_modes(present_modes_count);

        if (auto result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_modes_count, std::data(present_modes)); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to retrieve device surface presentation modes: {0:#x}\n"s, result));

        return { surface_capabilities, surface_formats, present_modes };
    }

    renderer::surface_format
    choose_supported_surface_format(swapchain_support_details const &support_details, std::vector<renderer::surface_format> const &required_surface_formats)
    {
        auto &&supported_surface_formats = support_details.surface_formats;

        if (std::size(supported_surface_formats) == 1 && supported_surface_formats.at(0).format == VK_FORMAT_UNDEFINED)
            return { graphics::FORMAT::BGRA8_UNORM, graphics::COLOR_SPACE::SRGB_NONLINEAR };

        for (auto [format, color_space] : required_surface_formats) {
            auto exist = std::any_of(std::cbegin(supported_surface_formats), std::cend(supported_surface_formats),
                                     [format, color_space] (auto &&surfaceFormat)
            {
                return surfaceFormat.format == convert_to::vulkan(format) && surfaceFormat.colorSpace == convert_to::vulkan(color_space);
            });

            if (exist)
                return { format, color_space };
        }

        throw std::runtime_error("none of required surface formats are supported"s);
    }

    VkPresentModeKHR choose_supported_present_mode(swapchain_support_details const &support_details)
    {
        auto &&supported_present_modes = support_details.present_modes;

    #ifndef _DEBUG
        auto mailbox = std::any_of(std::cbegin(supported_present_modes), std::cend(supported_present_modes), [] (auto &&mode)
        {
            return mode == VK_PRESENT_MODE_MAILBOX_KHR;
        });

        if (mailbox)
            return VK_PRESENT_MODE_MAILBOX_KHR;
    #endif

        auto relaxed = std::any_of(std::cbegin(supported_present_modes), std::cend(supported_present_modes), [] (auto &&mode)
        {
            return mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        });

        if (relaxed)
            return VK_PRESENT_MODE_FIFO_RELAXED_KHR;

        auto immediate = std::any_of(std::cbegin(supported_present_modes), std::cend(supported_present_modes), [] (auto &&mode)
        {
            return mode == VK_PRESENT_MODE_IMMEDIATE_KHR;
        });

        if (immediate)
            return VK_PRESENT_MODE_IMMEDIATE_KHR;

        return VK_PRESENT_MODE_FIFO_KHR;
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
                         renderer::surface_format surface_format, renderer::extent extent)
    {
        auto &&presentation_queue = device.presentation_queue;
        auto &&graphics_queue = device.graphics_queue;
        auto &&transfer_queue = device.transfer_queue;

        auto swapchain_support_details = query_swapchain_support_details(device.physical_handle(), platform_surface.handle());

        surface_format_ = choose_supported_surface_format(swapchain_support_details, std::vector{surface_format});

        auto present_mode = choose_supported_present_mode(swapchain_support_details);
    }
}
