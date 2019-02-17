#pragma once

#include <optional>
#include <memory>

#include "main.hxx"
#include "device.hxx"
#include "program.hxx"
#include "vertexFormat.hxx"
#include "staging.hxx"

class VulkanImage;
class VulkanImageView;
class VulkanSampler;
class VulkanBuffer;
class VulkanShaderModule;


class IndexBuffer final {
public:

    template<class T, typename std::enable_if_t<is_one_of_v<T, std::uint16_t, std::uint32_t>>...>
    IndexBuffer(std::shared_ptr<VulkanBuffer> buffer, std::size_t size) noexcept : buffer{buffer}, size{size}, type{T{}} { }

    template<class T, typename std::enable_if_t<std::is_same_v<IndexBuffer, std::decay_t<T>>>...>
    bool constexpr operator< (T &&rhs) const noexcept
    {
        return buffer->handle() < rhs.buffer->handle();
    }

private:
    std::shared_ptr<VulkanBuffer> buffer{nullptr};
    std::size_t size{0};

    std::variant<std::uint16_t, std::uint32_t> type;
};

class VertexBuffer final {
public:

    VertexBuffer(std::shared_ptr<VulkanBuffer> deviceBuffer, std::shared_ptr<VulkanBuffer> stagingBuffer, std::size_t sizeInBytes, xformat::vertex_layout &&layout) noexcept
        : deviceBuffer_{deviceBuffer}, stagingBuffer_{stagingBuffer}, sizeInBytes_{sizeInBytes}, layout{std::move(layout)} { }

    VulkanBuffer const &deviceBuffer() const noexcept { return *deviceBuffer_; }

private:

    std::shared_ptr<VulkanBuffer> deviceBuffer_{nullptr};
    std::shared_ptr<VulkanBuffer> stagingBuffer_{nullptr};

    std::size_t sizeInBytes_{0};
    std::size_t offset_{0};

    xformat::vertex_layout layout;
};

struct VertexBufferView final {
    std::shared_ptr<VertexBuffer> buffer;

    std::size_t sizeInBytes{0};
    std::byte *begin;
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
    [[nodiscard]] std::optional<IndexBuffer> CreateIndexBuffer(std::size_t sizeInBytes) noexcept;

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


[[nodiscard]] std::shared_ptr<VulkanBuffer> CreateUniformBuffer(VulkanDevice &device, std::size_t size);
[[nodiscard]] std::shared_ptr<VulkanBuffer> CreateCoherentStorageBuffer(VulkanDevice &device, std::size_t size);
[[nodiscard]] std::shared_ptr<VulkanBuffer> CreateStorageBuffer(VulkanDevice &device, std::size_t size);
