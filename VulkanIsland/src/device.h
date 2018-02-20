#pragma once

#include "instance.h"


class VulkanDevice final {
public:

    //VulkanDevice() = default;
    VulkanDevice(VulkanDevice const &) = default;

    VulkanDevice(VulkanInstance const &instance, VkSurfaceKHR surface);
    ~VulkanDevice();

    VkPhysicalDevice &physical_handle()
    {
        return physical_device_;
    }

public:
    VulkanDevice(VulkanDevice &&) = delete;

    VkPhysicalDevice physical_device_;
    VkDevice device_;

    void PickPhysicalDevice();
};
