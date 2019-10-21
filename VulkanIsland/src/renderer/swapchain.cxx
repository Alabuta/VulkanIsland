#include <cmath>
#include <vector>

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
    ;
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
    }
}
