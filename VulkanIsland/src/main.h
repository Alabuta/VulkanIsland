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

#ifdef _MSC_VER
#include <filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#endif

#include "helpers.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

#ifdef _MSC_VER
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#define USE_WIN32 0

#if USE_WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include "config.h"



auto constexpr kVULKAN_VERSION = VK_API_VERSION_1_1;

VkApplicationInfo constexpr app_info{
    VK_STRUCTURE_TYPE_APPLICATION_INFO,
    nullptr,
    "VulkanIsland", VK_MAKE_VERSION(1, 0, 0),
    "VulkanIsland", VK_MAKE_VERSION(1, 0, 0),
    kVULKAN_VERSION
};

auto constexpr extensions = make_array(
    // VK_KHR_MAINTENANCE1_EXTENSION_NAME,

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
);

auto constexpr layers = make_array(
#if TEMPORARILY_DISABLED
    "VK_LAYER_NV_nsight",
#endif

    //"VK_LAYER_LUNARG_api_dump",
    "VK_LAYER_LUNARG_assistant_layer",
    "VK_LAYER_LUNARG_core_validation",
    "VK_LAYER_LUNARG_object_tracker",
    "VK_LAYER_LUNARG_parameter_validation",
    "VK_LAYER_GOOGLE_threading",
    "VK_LAYER_GOOGLE_unique_objects"
);

auto constexpr deviceExtensions = make_array(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

[[nodiscard]] VkImageView CreateImageView(VkDevice device, VkImage &image, VkFormat format, VkImageAspectFlags aspectFlags, std::uint32_t mipLevels);

