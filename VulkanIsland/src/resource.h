#pragma once

#include <optional>
#include <memory>

#include "main.h"
#include "device.h"
#include "image.h"
#include "command_buffer.h"

class ResourceManager final {
public:

    ResourceManager(VulkanDevice &device) noexcept : device_{device} { }

    [[nodiscard]] std::optional<VulkanImage>
    CreateImage(VkFormat format, std::uint16_t width, std::uint16_t height, std::uint32_t mipLevels,
                VkImageTiling tiling, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags);


    [[nodiscard]] std::optional<VulkanImageView>
    CreateImageView(VulkanImage const &image, VkImageAspectFlags aspectFlags) noexcept;


    [[nodiscard]] std::optional<VulkanSampler>
    CreateImageSampler(std::uint32_t mipLevels) noexcept;

private:

    VulkanDevice &device_;

    ResourceManager() = delete;
    ResourceManager(ResourceManager const &) = delete;
    ResourceManager(ResourceManager &&) = delete;
};
