#pragma once

#include "main.h"
#include "device.h"
#include "buffer.h"
#include "TARGA_loader.h"

class VulkanImage final {
public:

    VulkanImage(VkImage handle, std::shared_ptr<DeviceMemory> memory, VkFormat format, std::uint32_t mipLevels, std::uint16_t width, std::uint16_t height) noexcept :
        handle{handle}, memory{memory}, format{format}, mipLevels{mipLevels}, width{width}, height{height} { }

#if NOT_YET_IMPLEMENTED
    VkImage handle() const noexcept { return handle_; }

    std::shared_ptr<DeviceMemory> memory() const noexcept { return memory_; }
    std::shared_ptr<DeviceMemory> &memory() noexcept { return memory_; }

    VkFormat format() const noexcept { return format_; }

    std::uint32_t mipLevels() const noexcept { return mipLevels_; }

    std::uint16_t width() const noexcept { return width_; }
    std::uint16_t height() const noexcept { return height_; }
#endif

    VkImage handle;
    std::shared_ptr<DeviceMemory> memory;

    VkFormat format{VK_FORMAT_UNDEFINED};

    std::uint32_t mipLevels{1};
    std::uint16_t width{0}, height{0};

    VulkanImage() = default;
};

// template<VkImageViewType ImageViewType>
struct VulkanImageView final {
    // static auto constexpr kImageViewType{ImageViewType};

    VkImageView handle;
    VkFormat format{VK_FORMAT_UNDEFINED};

    VulkanImageView() = default;

    VulkanImageView(VkImageView handle, VkFormat format) noexcept : handle{handle}, format{format} { }
};

struct VulkanSampler final {
    VkSampler handle;

    VulkanSampler() = default;

    VulkanSampler(VkSampler handle) noexcept : handle{handle} { }
};

struct VulkanTexture final {
    VulkanImage image;
    VulkanImageView view;
    //VulkanSampler sampler;

    VulkanTexture() = default;

    VulkanTexture(VulkanImage image, VulkanImageView view) noexcept : image{image}, view{view} { }
};



template<class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
[[nodiscard]] std::optional<VkFormat>
FindSupportedImageFormat(VkPhysicalDevice physicalDevice, T &&candidates, VkImageTiling tiling, VkFormatFeatureFlags features) noexcept
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkFormat>, "iterable object does not contain 'VkFormat' elements");

    auto it_format = std::find_if(std::cbegin(candidates), std::cend(candidates), [physicalDevice, tiling, features] (auto candidate)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, candidate, &properties);

        switch (tiling) {
            case VK_IMAGE_TILING_LINEAR:
                return (properties.linearTilingFeatures & features) == features;

            case VK_IMAGE_TILING_OPTIMAL:
                return (properties.optimalTilingFeatures & features) == features;

            default:
                return false;
        }
    });

    return it_format != std::cend(candidates) ? *it_format : std::optional<VkFormat>();
}

[[nodiscard]] std::optional<VkFormat> FindDepthImageFormat(VkPhysicalDevice physicalDevice) noexcept;


[[nodiscard]] std::optional<VkImage>
CreateImageHandle(VulkanDevice const &vulkanDevice, std::uint32_t width, std::uint32_t height, std::uint32_t mipLevels,
                  VkFormat format, VkImageTiling tiling, VkBufferUsageFlags usage) noexcept;

#if 0
[[nodiscard]] std::optional<VulkanImage>
CreateImage(VulkanDevice &device, VkFormat format, std::uint32_t width, std::uint32_t height, std::uint32_t mipLevels,
            VkImageTiling tiling, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags);

[[nodiscard]] std::optional<VulkanImageView>
CreateImageView(VulkanDevice const &device, VulkanImage const &image, VkImageAspectFlags aspectFlags) noexcept;

[[nodiscard]] std::optional<VulkanSampler> CreateImageSampler(VkDevice device, std::uint32_t mipLevels) noexcept;
#endif

[[nodiscard]] std::optional<VulkanTexture>
CreateTexture(VulkanDevice &device, VkFormat format, std::uint32_t width, std::uint32_t height, std::uint32_t mipLevels, VkImageTiling tiling,
              VkImageAspectFlags aspectFlags, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags);


template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>...>
void GenerateMipMaps(VulkanDevice const &device, Q &queue, VulkanImage const &image, VkCommandPool commandPool) noexcept
{
    auto commandBuffer = BeginSingleTimeCommand(device, queue, commandPool);

    auto width = image.width;
    auto height = image.height;

    VkImageMemoryBarrier barrier{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        0, 0,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        image.handle,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
    };

    for (auto i = 1u; i < image.mipLevels; ++i) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit const imageBlit{
            { VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1 },
            {{ 0, 0, 0 }, {width, height, 1 }},
            { VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1 },
            {{ 0, 0, 0 }, {width / 2, height / 2, 1 }}
        };

        vkCmdBlitImage(commandBuffer, image.handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (width > 1) width /= 2;
        if (height > 1) height /= 2;
    }

    barrier.subresourceRange.baseMipLevel = image.mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    EndSingleTimeCommand(device, queue, commandBuffer, commandPool);
}


template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>...>
bool TransitionImageLayout(VulkanDevice const &device, Q &queue, VulkanImage const &image,
                           VkImageLayout srcLayout, VkImageLayout dstLayout, VkCommandPool commandPool) noexcept
{
    VkImageMemoryBarrier barrier{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        0, 0,
        srcLayout, dstLayout,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        image.handle,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, image.mipLevels, 0, 1 }
    };

    if (dstLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (image.format == VK_FORMAT_D32_SFLOAT_S8_UINT || image.format == VK_FORMAT_D24_UNORM_S8_UINT)
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    VkPipelineStageFlags srcStageFlags, dstStageFlags;

    if (srcLayout == VK_IMAGE_LAYOUT_UNDEFINED && dstLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }

    else if (srcLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && dstLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        srcStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    else if (srcLayout == VK_IMAGE_LAYOUT_UNDEFINED && dstLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }

    else {
        std::cerr << "unsupported layout transition\n"s;
        return false;
    }

    auto commandBuffer = BeginSingleTimeCommand(device, queue, commandPool);

    vkCmdPipelineBarrier(commandBuffer, srcStageFlags, dstStageFlags, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    EndSingleTimeCommand(device, queue, commandBuffer, commandPool);

    return true;
}


[[nodiscard]] std::pair<VkBuffer, std::shared_ptr<DeviceMemory>> StageImage(VulkanDevice &device, RawImage const &rawImage) noexcept;
