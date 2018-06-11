#pragma once

#include "main.h"
#include "device.h"
#include "buffer.h"

struct VulkanImage final {
    VkImage handle;
    std::shared_ptr<DeviceMemory> memory;

    std::uint32_t mipLevels{1};
    std::uint16_t width{0}, height{0};

    VulkanImage() = default;

    VulkanImage(VkImage handle, std::shared_ptr<DeviceMemory> memory, std::uint32_t mipLevels, std::uint16_t width, std::uint16_t height) :
        handle{handle}, memory{memory}, mipLevels{mipLevels}, width{width}, height{height} { }
};

struct VulkanImageView final {
    VkImageView handle;

    //std::uint32_t mipLevels{1};
};

struct VulkanSampler final {
    VkSampler handle;
};

struct VulkanTexture final {
    VulkanImage image;
    VulkanSampler sampler;
    VulkanImageView view;
};