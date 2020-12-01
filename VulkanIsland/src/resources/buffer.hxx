#pragma once

#include <span>
#include <memory>

#include "utility/mpl.hxx"
#include "vulkan/device.hxx"
#include "graphics/vertex.hxx"
#include "resource_manager.hxx"


namespace resource
{
    class buffer final {
    public:

        VkBuffer handle() const noexcept { return handle_; }

        std::shared_ptr<resource::memory_block> memory() const noexcept { return memory_; }

        std::size_t size_bytes() const noexcept { return size_bytes_; }
        std::size_t capacity_bytes() const noexcept { return capacity_bytes_; }

        graphics::BUFFER_USAGE usage() const noexcept { return usage_; }
        graphics::RESOURCE_SHARING_MODE sharing_mode() const noexcept { return sharing_mode_; }

    private:

        VkBuffer handle_{VK_NULL_HANDLE};

        std::shared_ptr<resource::memory_block> memory_;

        std::size_t size_bytes_{0};
        std::size_t capacity_bytes_{0};

        graphics::BUFFER_USAGE usage_;
        graphics::RESOURCE_SHARING_MODE sharing_mode_;

        buffer(VkBuffer handle, std::shared_ptr<resource::memory_block> memory, std::size_t size_bytes,
               graphics::BUFFER_USAGE usage, graphics::RESOURCE_SHARING_MODE sharing_mode);

        buffer() = delete;
        buffer(buffer const &) = delete;
        buffer(buffer &&) = delete;

        friend resource::resource_manager;
    };
}

namespace resource
{
    class staging_buffer final {
    public:

        VkBuffer handle() const noexcept { return handle_; }

        std::shared_ptr<resource::memory_block> memory() const noexcept { return memory_; }

        std::span<std::byte> mapped_ptr() const noexcept { return mapped_ptr_; }

        graphics::BUFFER_USAGE usage() const noexcept { return usage_; }
        graphics::RESOURCE_SHARING_MODE sharing_mode() const noexcept { return sharing_mode_; }

    private:

        VkBuffer handle_{VK_NULL_HANDLE};

        std::shared_ptr<resource::memory_block> memory_;

        std::span<std::byte> mapped_ptr_;

        graphics::BUFFER_USAGE usage_;
        graphics::RESOURCE_SHARING_MODE sharing_mode_;

        staging_buffer(VkBuffer handle, std::shared_ptr<resource::memory_block> memory, std::span<std::byte> mapped_ptr,
                       graphics::BUFFER_USAGE usage, graphics::RESOURCE_SHARING_MODE sharing_mode);

        staging_buffer() = delete;
        staging_buffer(staging_buffer const &) = delete;
        staging_buffer(staging_buffer &&) = delete;

        friend resource::resource_manager;
    };

    class index_buffer final {
    public:

        index_buffer(std::shared_ptr<resource::buffer> device_buffer, std::size_t offset_bytes, std::size_t available_size, graphics::INDEX_TYPE index_type);

        std::shared_ptr<resource::buffer> const &device_buffer() const { return device_buffer_; }

        std::size_t offset_bytes() const noexcept { return offset_bytes_; }
        std::size_t available_size() const noexcept { return available_size_; }

        graphics::INDEX_TYPE index_type() const noexcept { return index_type_; }

    private:

        std::shared_ptr<resource::buffer> device_buffer_{nullptr};

        std::size_t offset_bytes_{0};
        std::size_t available_size_{0};

        graphics::INDEX_TYPE index_type_{graphics::INDEX_TYPE::UNDEFINED};

        index_buffer() = delete;
        index_buffer(index_buffer const &) = delete;
        index_buffer(index_buffer &&) = delete;

        friend resource::resource_manager;
    };

    class vertex_buffer final {
    public:

        vertex_buffer(std::shared_ptr<resource::buffer> device_buffer, std::size_t offset_bytes, std::size_t available_size, graphics::vertex_layout const &vertex_layout);

        std::shared_ptr<resource::buffer> const &device_buffer() const { return device_buffer_; }

        std::size_t offset_bytes() const noexcept { return offset_bytes_; }
        std::size_t available_size() const noexcept { return available_size_; }

        graphics::vertex_layout const &vertex_layout() const noexcept { return vertex_layout_; }

    private:

        std::shared_ptr<resource::buffer> device_buffer_{nullptr};

        std::size_t offset_bytes_{0};
        std::size_t available_size_{0};

        graphics::vertex_layout vertex_layout_;

        vertex_buffer() = delete;
        vertex_buffer(vertex_buffer const &) = delete;
        vertex_buffer(vertex_buffer &&) = delete;

        friend resource::resource_manager;
    };
}

#if NOT_YET_IMPLEMENTED
namespace resource
{
    template<>
    struct hash<resource::buffer> {
        std::size_t operator() (std::shared_ptr<resource::buffer> const buffer) const;
    };
}
#endif
