#pragma once

#include <optional>
#include <memory>

#include "main.hxx"
#include "device.hxx"
#include "program.hxx"
#include "commandBuffer.hxx"

class VulkanImage;
class VulkanImageView;
class VulkanSampler;
class VulkanBuffer;
class VulkanShaderModule;

class ResourceManager final {
public:

    ResourceManager(VulkanDevice &device) noexcept : device_{device} { }

    [[nodiscard]] std::shared_ptr<VulkanImage>
    CreateImage(VkFormat format, std::uint16_t width, std::uint16_t height, std::uint32_t mipLevels,
                VkSampleCountFlagBits samplesCount, VkImageTiling tiling, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags);

    [[nodiscard]] std::optional<VulkanImageView>
    CreateImageView(VulkanImage const &image, VkImageViewType type, VkImageAspectFlags aspectFlags) noexcept;

    [[nodiscard]] std::shared_ptr<VulkanSampler>
    CreateImageSampler(std::uint32_t mipLevels) noexcept;
    
    [[nodiscard]] std::shared_ptr<VulkanBuffer>
    CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) noexcept;

    [[nodiscard]] std::shared_ptr<VulkanShaderModule>
    CreateShaderModule(std::vector<std::byte> const &shaderByteCode) noexcept;

private:

    VulkanDevice &device_;

    template<class T, std::enable_if_t<is_one_of_v<std::decay_t<T>,
        VulkanImage, VulkanSampler, VulkanImageView, VulkanBuffer, VulkanShaderModule
    >> ...>
    void ReleaseResource(T &&resource) noexcept;

    ResourceManager() = delete;
    ResourceManager(ResourceManager const &) = delete;
    ResourceManager(ResourceManager &&) = delete;
};
