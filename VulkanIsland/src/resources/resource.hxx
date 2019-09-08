#pragma once

#include <optional>
#include <memory>

#include "main.hxx"
#include "device/device.hxx"
#include "program.hxx"
#include "vertexFormat.hxx"
#include "staging.hxx"
#include "renderer/graphics_api.hxx"


class VulkanImage;
class VulkanImageView;
class VulkanSampler;
class VulkanBuffer;
class VulkanShaderModule;

class VertexBuffer;
class IndexBuffer;


template<class T, std::enable_if_t<is_one_of_v<std::decay_t<T>, VulkanImage, VulkanBuffer>>...>
bool IsResourceLinear(T &&resource)
{
    using type = std::decay_t<T>;
   
    if constexpr (std::is_same_v<type, VulkanBuffer>)
        return true;

    else if constexpr (std::is_same_v<type, VulkanImage>) {
        return resource.tiling() == VK_IMAGE_TILING_LINEAR;
    }

    else return false;
}

class ResourceManager final {
public:

    ResourceManager(VulkanDevice &device) noexcept : device_{device} { }

    [[nodiscard]] std::shared_ptr<VulkanImage>
    CreateImage(graphics::FORMAT format, std::uint16_t width, std::uint16_t height, std::uint32_t mipLevels,
                std::uint32_t samplesCount, graphics::IMAGE_TILING tiling, graphics::IMAGE_USAGE usageFlags, VkMemoryPropertyFlags propertyFlags);

    [[nodiscard]] std::optional<VulkanImageView>
    CreateImageView(VulkanImage const &image, VkImageViewType type, VkImageAspectFlags aspectFlags) noexcept;

    [[nodiscard]] std::shared_ptr<VulkanSampler>
    CreateImageSampler(std::uint32_t mipLevels) noexcept;
    
    [[nodiscard]] std::shared_ptr<VulkanBuffer>
    CreateBuffer(VkDeviceSize size, graphics::BUFFER_USAGE usage, VkMemoryPropertyFlags properties) noexcept;

    [[nodiscard]] std::shared_ptr<VulkanShaderModule>
    CreateShaderModule(std::vector<std::byte> const &shaderByteCode) noexcept;

    template<class T, typename std::enable_if_t<is_one_of_v<T, std::uint16_t, std::uint32_t>>* = nullptr>
    [[nodiscard]] std::shared_ptr<IndexBuffer> CreateIndexBuffer(std::size_t sizeInBytes) noexcept;

    [[nodiscard]] std::shared_ptr<VertexBuffer> CreateVertexBuffer(xformat::vertex_layout const &layout, std::size_t sizeInBytes) noexcept;

    void StageVertexData(std::shared_ptr<VertexBuffer> vertexBuffer, std::vector<std::byte> const &container) const;

    void TransferStagedVertexData(VkCommandPool transferCommandPool, TransferQueue &transferQueue) const;

    [[nodiscard]] auto &vertexBuffers() const noexcept { return vertexBuffers_; }

private:

    static auto constexpr kVertexBufferIncreaseValue{4};

    VulkanDevice &device_;

    template<class T, std::enable_if_t<is_one_of_v<std::decay_t<T>,
        VulkanImage, VulkanSampler, VulkanImageView, VulkanBuffer, VulkanShaderModule
    >>...>
    void ReleaseResource(T &&resource) noexcept;

    ResourceManager() = delete;
    ResourceManager(ResourceManager const &) = delete;
    ResourceManager(ResourceManager &&) = delete;
    
    // TODO:: unordered_miltimap
    std::unordered_map<xformat::vertex_layout, std::shared_ptr<VertexBuffer>, xformat::hash_value, xformat::equal_comparator> vertexBuffers_;
};


[[nodiscard]] std::shared_ptr<VulkanBuffer> CreateUniformBuffer(VulkanDevice &device, std::size_t size);
[[nodiscard]] std::shared_ptr<VulkanBuffer> CreateCoherentStorageBuffer(VulkanDevice &device, std::size_t size);
[[nodiscard]] std::shared_ptr<VulkanBuffer> CreateStorageBuffer(VulkanDevice &device, std::size_t size);
