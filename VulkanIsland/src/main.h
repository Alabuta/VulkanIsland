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

template<class T, class U, typename std::enable_if_t<is_iterable_v<std::decay_t<T>> && std::is_same_v<std::decay_t<U>, VkQueueFamilyProperties>>...>
[[nodiscard]] std::optional<std::uint32_t> GetRequiredQueueFamilyIndex(T &&queueFamilies, U &&requiredQueue)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkQueueFamilyProperties>, "iterable object does not contain VkQueueFamilyProperties elements");

    // Strict matching.
    auto it_family = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [&requiredQueue] (auto &&queueFamily)
    {
        return queueFamily.queueCount > 0 && queueFamily.queueFlags == requiredQueue.queueFlags;
    });

    if (it_family != queueFamilies.cend())
        return static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_family));

    // Tolerant matching.
    it_family = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [&requiredQueue] (auto &&queueFamily)
    {
        return queueFamily.queueCount > 0 && (queueFamily.queueFlags & requiredQueue.queueFlags) == requiredQueue.queueFlags;
    });

    if (it_family != queueFamilies.cend())
        return static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_family));

    return {};
}

template<class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
[[nodiscard]] std::optional<std::uint32_t> GetPresentationQueueFamilyIndex(T &&queueFamilies, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkQueueFamilyProperties>, "iterable object does not contain VkQueueFamilyProperties elements");

    auto it_presentationQueue = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [physicalDevice, surface, size = queueFamilies.size()](auto queueFamily)
    {
        std::vector<std::uint32_t> queueFamiliesIndices(size);
        std::iota(queueFamiliesIndices.begin(), queueFamiliesIndices.end(), 0);

        return std::find_if(queueFamiliesIndices.crbegin(), queueFamiliesIndices.crend(), [physicalDevice, surface] (auto queueIndex)
        {
            VkBool32 surfaceSupported = 0;
            if (auto result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueIndex, surface, &surfaceSupported); result != VK_SUCCESS)
                throw std::runtime_error("failed to retrieve surface support: "s + std::to_string(result));

            return surfaceSupported != VK_TRUE;

        }) != queueFamiliesIndices.crend();
    });

    if (it_presentationQueue != queueFamilies.cend())
        return static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_presentationQueue));

    return {};
}

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

[[nodiscard]] SwapChainSupportDetails QuerySwapChainSupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

#include "instance.h"
#include "device.h"