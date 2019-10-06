#pragma once

#include <optional>
#include <memory>

#include "main.hxx"
#include "utility/mpl.hxx"
#include "vulkan/device.hxx"

#include "graphics/graphics_api.hxx"
#include "graphics/vertex.hxx"

#include "semaphore.hxx"


class VulkanImage;
class VulkanImageView;
class VulkanSampler;
class VulkanBuffer;

class VertexBuffer;
class IndexBuffer;

namespace resource
{
    class semaphore;
}


template<class T> requires mpl::one_of<std::remove_cvref_t<T>, VulkanImage, VulkanBuffer>
bool IsResourceLinear(T &&resource)
{
    using type = std::remove_cvref_t<T>;
   
    if constexpr (std::is_same_v<type, VulkanBuffer>)
        return true;

    else if constexpr (std::is_same_v<type, VulkanImage>) {
        return resource.tiling() == VK_IMAGE_TILING_LINEAR;
    }

    else return false;
}

class ResourceManager final {
public:

    ResourceManager(vulkan::device &device) noexcept : device_{device} { }

    [[nodiscard]] std::shared_ptr<VulkanImage>
    CreateImage(graphics::FORMAT format, std::uint16_t width, std::uint16_t height, std::uint32_t mipLevels,
                std::uint32_t samples_count, graphics::IMAGE_TILING tiling, graphics::IMAGE_USAGE usageFlags, VkMemoryPropertyFlags propertyFlags);

    [[nodiscard]] std::optional<VulkanImageView>
    CreateImageView(VulkanImage const &image, graphics::IMAGE_VIEW_TYPE view_type, VkImageAspectFlags aspectFlags) noexcept;

    [[nodiscard]] std::shared_ptr<VulkanSampler>
    CreateImageSampler(std::uint32_t mipLevels) noexcept;
    
    [[nodiscard]] std::shared_ptr<VulkanBuffer>
    CreateBuffer(VkDeviceSize size, graphics::BUFFER_USAGE usage, VkMemoryPropertyFlags properties) noexcept;

    template<class T> requires mpl::one_of<T, std::uint16_t, std::uint32_t>
    [[nodiscard]] std::shared_ptr<IndexBuffer> CreateIndexBuffer(std::size_t sizeInBytes) noexcept;

    [[nodiscard]] std::shared_ptr<VertexBuffer> CreateVertexBuffer(graphics::vertex_layout const &layout, std::size_t sizeInBytes) noexcept;

    void StageVertexData(std::shared_ptr<VertexBuffer> vertexBuffer, std::vector<std::byte> const &container) const;

    void TransferStagedVertexData(VkCommandPool transferCommandPool, TransferQueue &transferQueue) const;

    [[nodiscard]] auto &vertex_buffers() const noexcept { return vertexBuffers_; }

    [[nodiscard]] std::shared_ptr<VertexBuffer> vertex_buffer(graphics::vertex_layout const &layout) const;

    [[nodiscard]] std::shared_ptr<resource::semaphore> create_semaphore();

private:

    static auto constexpr kVertexBufferIncreaseValue{4};

    vulkan::device &device_;

    template<class T> requires mpl::one_of<std::remove_cvref_t<T>,
        VulkanImage, VulkanSampler, VulkanImageView, VulkanBuffer, resource::semaphore
    >
    void ReleaseResource(T &&resource) noexcept;

    ResourceManager() = delete;
    ResourceManager(ResourceManager const &) = delete;
    ResourceManager(ResourceManager &&) = delete;
    
    // TODO:: unordered_miltimap
    std::unordered_map<graphics::vertex_layout, std::shared_ptr<VertexBuffer>, graphics::hash<graphics::vertex_layout>> vertexBuffers_;
};


[[nodiscard]] std::shared_ptr<VulkanBuffer> CreateUniformBuffer(vulkan::device &device, std::size_t size);
[[nodiscard]] std::shared_ptr<VulkanBuffer> CreateCoherentStorageBuffer(vulkan::device &device, std::size_t size);
[[nodiscard]] std::shared_ptr<VulkanBuffer> CreateStorageBuffer(vulkan::device &device, std::size_t size);
