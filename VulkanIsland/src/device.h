#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "instance.h"

#if 0
template<bool USE_DEBUG_LAYERS>
class VulkanInstance<USE_DEBUG_LAYERS>::VulkanDevice final {
public:

    /*VulkanDevice() = default;
    VulkanDevice(VulkanDevice &&) = default;

    VulkanDevice(VulkanDevice const &) = default;

    VulkanDevice(VkInstance instance, VkSurfaceKHR surface);*/

    /*operator VkPhysicalDevice &()
    {
        return physical_device_;
    }*/

    VkPhysicalDevice &physical_handle()
    {
        return physical_device_;
    }

    //~VulkanDevice();

public:

    VkPhysicalDevice physical_device_;
    VkDevice device_;
};
#endif