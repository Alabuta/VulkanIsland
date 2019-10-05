#pragma once

#include "main.hxx"
#include "device/device.hxx"
#include "buffer.hxx"
#include "loaders/loaderTARGA.hxx"
#include "graphics/graphics.hxx"
#include "graphics/graphics_api.hxx"


class VulkanImageView;

class VulkanImage final {
public:

    VulkanImage(std::shared_ptr<DeviceMemory> memory, VkImage handle, graphics::FORMAT format, graphics::IMAGE_TILING tiling,
                std::uint32_t mipLevels, std::uint16_t width, std::uint16_t height) noexcept :
        memory_{memory}, handle_{handle}, format_{format}, tiling_{tiling}, mipLevels_{mipLevels}, width_{width}, height_{height} { }

    std::shared_ptr<DeviceMemory> memory() const noexcept { return memory_; }
    std::shared_ptr<DeviceMemory> &memory() noexcept { return memory_; }

    VkImage handle() const noexcept { return handle_; }
    graphics::FORMAT format() const noexcept { return format_; }
    graphics::IMAGE_TILING tiling() const noexcept { return tiling_; }

    std::uint32_t mipLevels() const noexcept { return mipLevels_; }

    std::uint16_t width() const noexcept { return width_; }
    std::uint16_t height() const noexcept { return height_; }

private:
    std::shared_ptr<DeviceMemory> memory_;

    VkImage handle_{VK_NULL_HANDLE};
    graphics::FORMAT format_{graphics::FORMAT::UNDEFINED};
    graphics::IMAGE_TILING tiling_{graphics::IMAGE_TILING::LINEAR};

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


[[nodiscard]] std::optional<graphics::FORMAT>
FindSupportedImageFormat(VulkanDevice const &device, std::vector<graphics::FORMAT> const &candidates, graphics::IMAGE_TILING tiling, VkFormatFeatureFlags features) noexcept;

[[nodiscard]] std::optional<graphics::FORMAT> FindDepthImageFormat(VulkanDevice const &device) noexcept;


[[nodiscard]] std::optional<VulkanTexture>
CreateTexture(VulkanDevice &device, graphics::FORMAT format, graphics::IMAGE_VIEW_TYPE view_type,
              std::uint16_t width, std::uint16_t height, std::uint32_t mipLevels, std::uint32_t samplesCount, graphics::IMAGE_TILING tiling,
              VkImageAspectFlags aspectFlags, graphics::IMAGE_USAGE usageFlags, VkMemoryPropertyFlags propertyFlags);
