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
    #if TEMPORARILY_DISABLED
        "VK_LAYER_NV_nsight",
        "VK_LAYER_LUNARG_api_dump",
    #endif

        "VK_LAYER_LUNARG_assistant_layer",
        "VK_LAYER_LUNARG_core_validation",
        "VK_LAYER_LUNARG_object_tracker",
        "VK_LAYER_LUNARG_parameter_validation",
        "VK_LAYER_GOOGLE_threading",
        "VK_LAYER_GOOGLE_unique_objects"
    };

    VkApplicationInfo constexpr application_info{
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,
        "VulkanIsland", VK_MAKE_VERSION(PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, 0),
        "VulkanIsland", VK_MAKE_VERSION(PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, 0),
        VK_API_VERSION_1_1
    };
}