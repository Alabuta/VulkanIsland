#pragma once

#include <optional>
#include <memory>

#include "main.h"
#include "device.h"
#include "image.h"
#include "command_buffer.h"

class ResourceManager final {
public:

    ResourceManager(VulkanDevice &device) noexcept : device_{device}, deleter_{std::make_unique<deleter_t>(*this)} { }

    [[nodiscard]] std::shared_ptr<VulkanImage>
    CreateImage(VkFormat format, std::uint16_t width, std::uint16_t height, std::uint32_t mipLevels,
                VkImageTiling tiling, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags);

    [[nodiscard]] std::optional<VulkanImageView>
    CreateImageView(VulkanImage const &image, VkImageAspectFlags aspectFlags) noexcept;

    [[nodiscard]] std::optional<VulkanSampler>
    CreateImageSampler(std::uint32_t mipLevels) noexcept;

private:

    VulkanDevice &device_;

    class deleter_t final {
    public:

        deleter_t(ResourceManager &resourceManager) noexcept : resourceManager_{resourceManager} { }

        auto operator() (VulkanImage *ptr_image) const
        {
            resourceManager_.FreeImage(*ptr_image);

            delete ptr_image;
        }

    private:
        ResourceManager &resourceManager_;

        deleter_t() = delete;
    };

    std::unique_ptr<deleter_t> deleter_;

    void FreeImage(VulkanImage const &image) noexcept;

    ResourceManager() = delete;
    ResourceManager(ResourceManager const &) = delete;
    ResourceManager(ResourceManager &&) = delete;
};
