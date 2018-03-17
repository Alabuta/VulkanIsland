
#include "main.h"

#include <iostream>
#include <string>
#include <string_view>

using namespace std::string_literals;
using namespace std::string_view_literals;

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif


[[nodiscard]] inline VKAPI_ATTR VkBool32 VKAPI_CALL
DebugCallback([[maybe_unused]] VkDebugReportFlagsEXT flags, [[maybe_unused]] VkDebugReportObjectTypeEXT objectType,
              [[maybe_unused]] std::uint64_t object, [[maybe_unused]] std::size_t location,
              std::int32_t messageCode,
              [[maybe_unused]] const char *pLayerPrefix, [[maybe_unused]] const char *pMessage, [[maybe_unused]] void *pUserData)
{
    std::cout << messageCode << std::endl;

    return VK_FALSE;
}

[[nodiscard]] VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(
    VkInstance instance, VkDebugReportCallbackCreateInfoEXT const *pCreateInfo, VkAllocationCallbacks const *pAllocator, VkDebugReportCallbackEXT *pCallback)
{
    auto func = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));

    if (func)
        return func(instance, pCreateInfo, pAllocator, pCallback);

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(
    VkInstance instance, VkDebugReportCallbackEXT callback, VkAllocationCallbacks const *pAllocator)
{
    auto func = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));

    if (func)
        func(instance, callback, pAllocator);
}

void CreateDebugReportCallback(VkInstance instance, VkDebugReportCallbackEXT &callback)
{
    VkDebugReportCallbackCreateInfoEXT constexpr createInfo{
        VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
        nullptr,
        VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT,
        DebugCallback,
        nullptr
    };

    if (auto result = vkCreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback); result != VK_SUCCESS)
        throw std::runtime_error("failed to set up debug callback: "s + std::to_string(result));
}