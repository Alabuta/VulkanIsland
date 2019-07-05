#pragma once

#include "main.hxx"
#include "device/device.hxx"
#include "buffer.hxx"
#include "loaders/loaderTARGA.hxx"


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

    VkImage handle_{VK_NULL_HANDLE};
    VkFormat format_{VK_FORMAT_UNDEFINED};

    std::uint32_t mipLevels_{1};
    std::uint16_t width_{0}, height_{0};

    VulkanImage() = delete;
    VulkanImage(VulkanImage const &) = delete;
    VulkanImage(VulkanImage &&) = delete;
};

class VulkanImageView final {
public:
    VkImageView handle_{VK_NULL_HANDLE};
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
    VkSampler handle_{VK_NULL_HANDLE};

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


template<class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>* = nullptr>
[[nodiscard]] std::optional<VkFormat>
FindSupportedImageFormat(VulkanDevice const &device, T &&candidates, VkImageTiling tiling, VkFormatFeatureFlags features) noexcept
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkFormat>, "iterable object does not contain 'VkFormat' elements");

    auto physicalDevice = device.physical_handle();

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

[[nodiscard]] std::optional<VkFormat> FindDepthImageFormat(VulkanDevice const &device) noexcept;


[[nodiscard]] std::optional<VulkanTexture>
CreateTexture(VulkanDevice &device, VkFormat format, VkImageViewType type,
              std::uint16_t width, std::uint16_t height, std::uint32_t mipLevels, VkSampleCountFlagBits samplesCount, VkImageTiling tiling,
              VkImageAspectFlags aspectFlags, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags);
