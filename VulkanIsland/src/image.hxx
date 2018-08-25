#pragma once

#include "main.hxx"
#include "device.hxx"
#include "buffer.hxx"
#include "TARGA_loader.hxx"

class VulkanImageView;

class VulkanImage final {
public:

    VulkanImage(std::shared_ptr<DeviceMemory> memory, VkImage handle, VkFormat format,
                std::uint32_t mipLevels, std::uint16_t width, std::uint16_t height) noexcept :
        memory_{memory}, handle_{handle}, format_{format}, mipLevels_{mipLevels}, width_{width}, height_{height} { }

    std::shared_ptr<DeviceMemory> memory() const noexcept { return memory_; }
    std::shared_ptr<DeviceMemory> &memory() noexcept { return memory_; }

    VkImage handle() const noexcept { return handle_; }
    VkFormat format() const noexcept { return format_; }

    std::uint32_t mipLevels() const noexcept { return mipLevels_; }

    std::uint16_t width() const noexcept { return width_; }
    std::uint16_t height() const noexcept { return height_; }

private:
    std::shared_ptr<DeviceMemory> memory_;

    VkImage handle_;
    VkFormat format_{VK_FORMAT_UNDEFINED};

    std::uint32_t mipLevels_{1};
    std::uint16_t width_{0}, height_{0};

    VulkanImage() = delete;
    VulkanImage(VulkanImage const &) = delete;
    VulkanImage(VulkanImage &&) = delete;
};

class VulkanImageView final {
public:
    VkImageView handle_;
    VkImageViewType type_;

    VulkanImageView() = default;
    VulkanImageView(VkImageView handle, VkImageViewType type) noexcept : handle_{handle}, type_{type} { }

    VkImageView handle() const noexcept { return handle_; }
    VkImageViewType type() const noexcept { return type_; }

private:
    //VulkanImageView() = delete;
};

class VulkanSampler final {
public:

    VulkanSampler(VkSampler handle) noexcept : handle_{handle} { }

    VkSampler handle() const noexcept { return handle_; }

private:
    VkSampler handle_;

    VulkanSampler() = delete;
    VulkanSampler(VulkanSampler const &) = delete;
    VulkanSampler(VulkanSampler &&) = delete;
};

struct VulkanTexture final {
    std::shared_ptr<VulkanImage> image;
    VulkanImageView view;

    std::shared_ptr<VulkanSampler> sampler;

    VulkanTexture() = default;

    VulkanTexture(std::shared_ptr<VulkanImage> image, VulkanImageView view, std::shared_ptr<VulkanSampler> sampler) noexcept
        : image{image}, view{view}, sampler{sampler} { }
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
                  VkSampleCountFlagBits samplesCount, VkFormat format, VkImageTiling tiling, VkBufferUsageFlags usage) noexcept;

[[nodiscard]] std::optional<VulkanTexture>
CreateTexture(VulkanDevice &device, VkFormat format, VkImageViewType type,
              std::uint16_t width, std::uint16_t height, std::uint32_t mipLevels, VkSampleCountFlagBits samplesCount, VkImageTiling tiling,
              VkImageAspectFlags aspectFlags, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags);


template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>...>
void GenerateMipMaps(VulkanDevice const &device, Q &queue, VulkanImage const &image, VkCommandPool commandPool) noexcept
{
    auto commandBuffer = BeginSingleTimeCommand(device, queue, commandPool);

    auto width = image.width();
    auto height = image.height();

    VkImageMemoryBarrier barrier{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        0, 0,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        image.handle(),
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
    };

    for (auto i = 1u; i < image.mipLevels(); ++i) {
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

        vkCmdBlitImage(commandBuffer, image.handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image.handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (width > 1) width /= 2;
        if (height > 1) height /= 2;
    }

    barrier.subresourceRange.baseMipLevel = image.mipLevels() - 1;
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
        image.handle(),
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, image.mipLevels(), 0, 1 }
    };

    if (dstLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (image.format() == VK_FORMAT_D32_SFLOAT_S8_UINT || image.format() == VK_FORMAT_D24_UNORM_S8_UINT)
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

