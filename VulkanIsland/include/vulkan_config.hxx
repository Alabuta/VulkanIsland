#pragma once

#include <array>

#ifndef GLFW_INCLUDE_VULKAN
    #define GLFW_INCLUDE_VULKAN
    #include <GLFW/glfw3.h>
#endif

#include "config.hxx"


namespace vulkan_config
{
    auto constexpr extensions = std::array{
        VK_KHR_SURFACE_EXTENSION_NAME,
    #ifdef _MSC_VER
        #if USE_WIN32
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        #else
            "VK_KHR_win32_surface",
        #endif
    #else
        "VK_KHR_xcb_surface",
    #endif
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME
    };

    auto constexpr layers = std::array{
        // "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_LUNARG_standard_validation"
    };

    VkApplicationInfo constexpr application_info{
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,
        "VulkanIsland", VK_MAKE_VERSION(PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, 0),
        "VulkanIsland", VK_MAKE_VERSION(PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, 0),
        VK_API_VERSION_1_1
    };
}