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


template<class T, typename std::enable_if_t<is_one_of_v<T, std::uint16_t, std::uint32_t>>...>
struct IndexBuffer final {
    using type = T;

    std::shared_ptr<VulkanBuffer> buffer;
};

struct VertexBuffer final {
    std::shared_ptr<VulkanBuffer> buffer;
};


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

    template<class T, typename std::enable_if_t<is_one_of_v<T, std::uint16_t, std::uint32_t>>...>
    [[nodiscard]] std::optional<IndexBuffer<T>> CreateIndexBuffer(std::size_t sizeInBytes) noexcept;

    [[nodiscard]] std::optional<VertexBuffer> CreateVertexBuffer(std::size_t sizeInBytes) noexcept;

private:

    VulkanDevice &device_;

    template<class T, std::enable_if_t<is_one_of_v<std::decay_t<T>,
        VulkanImage, VulkanSampler, VulkanImageView, VulkanBuffer, VulkanShaderModule
    >> ...>
    void ReleaseResource(T &&resource) noexcept;

    ResourceManager() = delete;
    ResourceManager(ResourceManager const &) = delete;
    ResourceManager(ResourceManager &&) = delete;

    /*std::vector<IndexBuffer<std::uint16_t>> indexBufferU16_;
    std::vector<IndexBuffer<std::uint32_t>> indexBufferU32_;*/
};


template<class T, typename std::enable_if_t<is_one_of_v<T, std::uint16_t, std::uint32_t>>...>
std::optional<IndexBuffer<T>> ResourceManager::CreateIndexBuffer(std::size_t /*sizeInBytes*/) noexcept
{
    return std::optional<IndexBuffer<T>>();
}
