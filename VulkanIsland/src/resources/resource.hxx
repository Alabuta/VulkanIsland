#pragma once

#include <optional>
#include <memory>

#include "main.hxx"
#include "utility/mpl.hxx"
#include "vulkan/device.hxx"

#include "graphics/graphics_api.hxx"
#include "graphics/vertex.hxx"
#include "graphics/render_pass.hxx"

#include "memory.hxx"
#include "semaphore.hxx"


namespace resource
{
    class image;
    class image_view;

    class sampler;

    class buffer;

    class vertex_buffer;
    class index_buffer;

    class semaphore;
}


template<class T> requires mpl::one_of<std::remove_cvref_t<T>, resource::image, resource::buffer>
bool IsResourceLinear(T &&resource)
{
    using type = std::remove_cvref_t<T>;
   
    if constexpr (std::is_same_v<type, resource::buffer>)
        return true;

    else if constexpr (std::is_same_v<type, resource::image>)
        return resource.tiling() == VK_IMAGE_TILING_LINEAR;

    else return false;
}

class ResourceManager final {
public:

    ResourceManager(vulkan::device &device, MemoryManager &memory_manager) noexcept : device_{device}, memory_manager_{memory_manager} { }

    [[nodiscard]] std::shared_ptr<resource::image>
    CreateImage(graphics::FORMAT format, std::uint16_t width, std::uint16_t height, std::uint32_t mip_levels,
                std::uint32_t samples_count, graphics::IMAGE_TILING tiling, graphics::IMAGE_USAGE usageFlags, VkMemoryPropertyFlags propertyFlags);

    [[nodiscard]] std::shared_ptr<resource::image_view>
    CreateImageView(std::shared_ptr<resource::image> image, graphics::IMAGE_VIEW_TYPE view_type, VkImageAspectFlags aspectFlags) noexcept;

    [[nodiscard]] std::shared_ptr<resource::sampler>
    CreateImageSampler(std::uint32_t mip_levels) noexcept;
    
    [[nodiscard]] std::shared_ptr<resource::buffer>
    CreateBuffer(VkDeviceSize size, graphics::BUFFER_USAGE usage, VkMemoryPropertyFlags properties) noexcept;

    template<class T> requires mpl::one_of<T, std::uint16_t, std::uint32_t>
    [[nodiscard]] std::shared_ptr<resource::index_buffer> CreateIndexBuffer(std::size_t sizeInBytes) noexcept;

    [[nodiscard]] std::shared_ptr<resource::vertex_buffer> CreateVertexBuffer(graphics::vertex_layout const &layout, std::size_t sizeInBytes) noexcept;

    void StageVertexData(std::shared_ptr<resource::vertex_buffer> vertexBuffer, std::vector<std::byte> const &container) const;

    void TransferStagedVertexData(VkCommandPool transferCommandPool, graphics::transfer_queue const &transfer_queue) const;

    [[nodiscard]] auto &vertex_buffers() const noexcept { return vertexBuffers_; }

    [[nodiscard]] std::shared_ptr<resource::vertex_buffer> vertex_buffer(graphics::vertex_layout const &layout) const;

    [[nodiscard]] std::shared_ptr<resource::semaphore> create_semaphore();

private:

    static auto constexpr kVertexBufferIncreaseValue{4};

    vulkan::device &device_;
    MemoryManager &memory_manager_;

    template<class T> requires mpl::one_of<std::remove_cvref_t<T>,
        resource::image, resource::sampler, resource::image_view, resource::buffer, resource::semaphore
    >
    void ReleaseResource(T &&resource) noexcept;

    ResourceManager() = delete;
    ResourceManager(ResourceManager const &) = delete;
    ResourceManager(ResourceManager &&) = delete;
    
    // TODO:: unordered_miltimap
    std::unordered_map<graphics::vertex_layout, std::shared_ptr<resource::vertex_buffer>, graphics::hash<graphics::vertex_layout>> vertexBuffers_;
};


[[nodiscard]] std::shared_ptr<resource::buffer> CreateUniformBuffer(ResourceManager &resource_manager, std::size_t size);
[[nodiscard]] std::shared_ptr<resource::buffer> CreateCoherentStorageBuffer(ResourceManager &resource_manager, std::size_t size);
[[nodiscard]] std::shared_ptr<resource::buffer> CreateStorageBuffer(ResourceManager &resource_manager, std::size_t size);
