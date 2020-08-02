#pragma once

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

        std::shared_ptr<resource::device_memory> memory() const noexcept { return memory_; }
        std::shared_ptr<resource::device_memory> &memory() noexcept { return memory_; }

    private:

        VkBuffer handle_{VK_NULL_HANDLE};

        std::shared_ptr<resource::device_memory> memory_;

        buffer(VkBuffer handle, std::shared_ptr<resource::device_memory> memory) : handle_{handle}, memory_{memory} { }

        buffer() = delete;
        buffer(buffer const &) = delete;
        buffer(buffer &&) = delete;

        friend resource::resource_manager;
    };
}

namespace resource
{
    class index_buffer final {
    public:

        index_buffer(std::shared_ptr<resource::buffer> device_buffer, std::shared_ptr<resource::buffer> staging_buffer, graphics::FORMAT format);

        resource::buffer const &device_buffer() const { return *device_buffer_; }
        resource::buffer const &staging_buffer() const { return *staging_buffer_; }

        std::size_t device_memory_offset() const { return device_buffer_->memory()->offset() + device_buffer_offset_; }
        std::size_t staging_memory_offset() const { return staging_buffer_->memory()->offset() + staging_buffer_offset_; }

        std::size_t available_device_buffer_size() const { return device_buffer_size_ - device_buffer_offset_; }
        std::size_t available_staging_buffer_size() const { return staging_buffer_size_ - staging_buffer_offset_; }

        graphics::FORMAT format() const noexcept { return format_; }

    private:

        std::shared_ptr<resource::buffer> device_buffer_{nullptr};
        std::shared_ptr<resource::buffer> staging_buffer_{nullptr};

        std::size_t device_buffer_offset_{0};
        std::size_t device_buffer_size_{0};

        std::size_t staging_buffer_offset_{0};
        std::size_t staging_buffer_size_{0};

        graphics::FORMAT format_{graphics::FORMAT::UNDEFINED};

        index_buffer() = delete;
        index_buffer(index_buffer const &) = delete;
        index_buffer(index_buffer &&) = delete;

        friend resource::resource_manager;
    };
}

namespace resource
{
    class vertex_buffer final {
    public:

        vertex_buffer(std::shared_ptr<resource::buffer> device_buffer, std::shared_ptr<resource::buffer> staging_buffer,
                      graphics::vertex_layout const &vertex_layout);

        resource::buffer const &device_buffer() const { return *device_buffer_; }
        resource::buffer const &staging_buffer() const { return *staging_buffer_; }

        std::size_t device_memory_offset() const { return device_buffer_->memory()->offset() + device_buffer_offset_; }
        std::size_t staging_memory_offset() const { return staging_buffer_->memory()->offset() + staging_buffer_offset_; }

        std::size_t available_device_buffer_size() const { return device_buffer_size_ - device_buffer_offset_; }
        std::size_t available_staging_buffer_size() const { return staging_buffer_size_ - staging_buffer_offset_; }

        graphics::vertex_layout const &vertex_layout() const noexcept { return vertex_layout_; }

    private:

        std::shared_ptr<resource::buffer> device_buffer_{nullptr};
        std::shared_ptr<resource::buffer> staging_buffer_{nullptr};

        std::size_t device_buffer_offset_{0};
        std::size_t device_buffer_size_{0};

        std::size_t staging_buffer_offset_{0};
        std::size_t staging_buffer_size_{0};

        graphics::vertex_layout vertex_layout_;

        vertex_buffer() = delete;
        vertex_buffer(vertex_buffer const &) = delete;
        vertex_buffer(vertex_buffer &&) = delete;

        friend resource::resource_manager;
    };
}
