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

#include "helpers.hxx"

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


auto constexpr kREVERSED_DEPTH = true;
