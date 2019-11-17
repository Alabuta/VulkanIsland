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

        template<class T> requires mpl::one_of<T, std::uint16_t, std::uint32_t>
        index_buffer(std::shared_ptr<resource::buffer> buffer, [[maybe_unused]] std::size_t size) noexcept : buffer{buffer}/* , size{size} */, type{T{}} { }

        template<class T> requires mpl::same_as<std::remove_cvref_t<T>, resource::index_buffer>
        bool constexpr operator< (T &&rhs) const noexcept
        {
            return buffer->handle() < rhs.buffer->handle();
        }

    private:
        std::shared_ptr<resource::buffer> buffer{nullptr};
        // std::size_t size{0};

        std::variant<std::uint16_t, std::uint32_t> type;
    };
}

namespace resource
{
    class vertex_buffer final {
    public:

        vertex_buffer(std::shared_ptr<resource::buffer> device_buffer, std::shared_ptr<resource::buffer> staging_buffer,
                     std::size_t capacity_in_bytes, graphics::vertex_layout const &vertex_layout) noexcept
            : device_buffer_{device_buffer}, staging_buffer_{staging_buffer}, capacity_in_bytes_{capacity_in_bytes}, vertex_layout_{vertex_layout} { }

        resource::buffer const &device_buffer() const noexcept { return *device_buffer_; }
        resource::buffer const &staging_buffer() const noexcept { return *staging_buffer_; }

        std::size_t device_buffer_offset() const noexcept { return device_buffer_->memory()->offset() + offset_; }
        std::size_t staging_buffer_offset() const noexcept { return staging_buffer_->memory()->offset() + offset_; }

        std::size_t available_memory_size() const noexcept { return capacity_in_bytes_ - offset_; }

        graphics::vertex_layout const &vertex_layout() const noexcept { return vertex_layout_; }

    private:

        std::shared_ptr<resource::buffer> device_buffer_{nullptr};
        std::shared_ptr<resource::buffer> staging_buffer_{nullptr};

        std::size_t capacity_in_bytes_{0};

        graphics::vertex_layout vertex_layout_;

        std::size_t offset_{0};
        std::size_t staging_buffer_size_in_bytes_{0};

        friend resource::resource_manager;
    };
}
