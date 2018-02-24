#pragma once


#include <iostream>
#include <memory>
#include <vector>
#include <array>
#include <tuple>
#include <set>
#include <optional>
#include <string>
#include <string_view>
#include <fstream>

#include <gsl\gsl>

#define GLFW_EXPOSE_NATIVE_WIN32

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib, "glfw3.lib")

auto constexpr kVULKAN_VERSION = VK_API_VERSION_1_0;

#define USE_LAYERS 1

#include "helpers.h"

using namespace std::string_literals;
using namespace std::string_view_literals;


extern VkSurfaceKHR surface;

VkApplicationInfo constexpr app_info{
    VK_STRUCTURE_TYPE_APPLICATION_INFO,
    nullptr,
    "VulkanIsland", VK_MAKE_VERSION(1, 0, 0),
    "VulkanIsland", VK_MAKE_VERSION(1, 0, 0),
    kVULKAN_VERSION
};

auto constexpr extensions = make_array(
    VK_KHR_SURFACE_EXTENSION_NAME,
#if USE_WIN32
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#else
    "VK_KHR_win32_surface",
#endif
    VK_EXT_DEBUG_REPORT_EXTENSION_NAME
);

#if USE_LAYERS
auto constexpr layers = make_array(
    //"VK_LAYER_LUNARG_api_dump",
    "VK_LAYER_LUNARG_core_validation",
    "VK_LAYER_LUNARG_object_tracker",
    "VK_LAYER_LUNARG_parameter_validation",
    "VK_LAYER_GOOGLE_threading",
    "VK_LAYER_GOOGLE_unique_objects",

    "VK_LAYER_NV_nsight"
);
#endif

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

[[nodiscard]] SwapChainSupportDetails QuerySwapChainSupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);


template<class T>
class VulkanQueue {
public:

    VkQueue handle() { return handle_; }
    VkQueue &handle() const { return handle_; }

    template<class... Args>
    constexpr bool StrictMatching(Args &&... args)
    {
        return static_cast<T *>(this)->StrictMatching(std::forward<Args>(args)...);
    }

    template<class... Args>
    constexpr bool TolerantMatching(Args &&... args)
    {
        return static_cast<T *>(this)->TolerantMatching(std::forward<Args>(args)...);
    }

private:
    VkQueue handle_{VK_NULL_HANDLE};

};



#include "instance.h"
#include "device.h"


class GraphicsQueue final : VulkanQueue<GraphicsQueue> {
public:

    template<class P, std::enable_if_t<std::is_same_v<std::decay_t<P>, VkQueueFamilyProperties>>...>
    constexpr bool StrictMatching(P &&properties)
    {
        return properties.queueCount > 0 && properties.queueFlags == (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
    }

    template<class P, std::enable_if_t<std::is_same_v<std::decay_t<P>, VkQueueFamilyProperties>>...>
    constexpr bool TolerantMatching(P &&properties)
    {
        return properties.queueCount > 0 && (properties.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
    }
};

class TransferQueue final : VulkanQueue<TransferQueue> {
public:

    template<class P, std::enable_if_t<std::is_same_v<std::decay_t<P>, VkQueueFamilyProperties>>...>
    constexpr bool StrictMatching(P &&properties)
    {
        return properties.queueCount > 0 && properties.queueFlags == VK_QUEUE_TRANSFER_BIT;
    }

    template<class P, std::enable_if_t<std::is_same_v<std::decay_t<P>, VkQueueFamilyProperties>>...>
    constexpr bool TolerantMatching(P &&properties)
    {
        return properties.queueCount > 0 && (properties.queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT;
    }
};

class PresentationQueue final : VulkanQueue<PresentationQueue> {
public:

    template<class... Args>
    constexpr bool StrictMatching(/*VulkanDevice const &device,*/VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::uint32_t queueIndex)
    {
        VkBool32 surfaceSupported = 0;
        if (auto result = vkGetPhysicalDeviceSurfaceSupportKHR(/*device.physical_handle()*/physicalDevice, queueIndex, surface, &surfaceSupported); result != VK_SUCCESS)
            throw std::runtime_error("failed to retrieve surface support: "s + std::to_string(result));

        return surfaceSupported == VK_TRUE;
    }
};