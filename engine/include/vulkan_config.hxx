#pragma once

#include <array>

#include <volk.h>

#include "config.hxx"


namespace vulkan_config
{
    #define USE_DEBUG_UTILS 0

    auto constexpr extensions = std::array{
    #if USE_DEBUG_UTILS
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    #else
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
    #endif
        // VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME,

    #ifdef _WIN32
        #pragma warning(suppress: 4668)
        #if USE_WIN32
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        #else
            "VK_KHR_win32_surface",
        #endif
    #else
        "VK_KHR_xcb_surface",
    #endif
        VK_KHR_SURFACE_EXTENSION_NAME
        //VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME
    };

    auto constexpr layers = std::array{
         "VK_LAYER_KHRONOS_validation"
         //"VK_LAYER_LUNARG_standard_validation"
    };

    VkApplicationInfo constexpr application_info{
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,
        "engine", VK_MAKE_VERSION(PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, 0),
        "engine", VK_MAKE_VERSION(PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, 0),
        VK_API_VERSION_1_2
    };
}
