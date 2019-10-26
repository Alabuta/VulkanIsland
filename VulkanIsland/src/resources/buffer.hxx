#pragma once

#include <concepts>
#include <memory>

#include "graphics/vertex.hxx"
#include "memory.hxx"

class ResourceManager;


namespace resource
{
    class buffer final {
    public:

        buffer(std::shared_ptr<DeviceMemory> memory, VkBuffer handle) : memory_{memory}, handle_{handle} { }

        std::shared_ptr<DeviceMemory> memory() const noexcept { return memory_; }
        std::shared_ptr<DeviceMemory> &memory() noexcept { return memory_; }

        VkBuffer handle() const noexcept { return handle_; }

    private:
        std::shared_ptr<DeviceMemory> memory_;
        VkBuffer handle_;

        buffer() = delete;
        buffer(buffer const &) = delete;
        buffer(buffer &&) = delete;
    };
}

namespace resource
{
    class index_buffer final {
    public:

        template<class T> requires mpl::one_of<T, std::uint16_t, std::uint32_t>
        index_buffer(std::shared_ptr<resource::buffer> buffer, [[maybe_unused]] std::size_t size) noexcept : buffer{buffer}/* , size{size} */, type{T{}} { }

        template<class T> requires std::same_as<std::remove_cvref_t<T>, resource::index_buffer>
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

        vertex_buffer(std::shared_ptr<resource::buffer> deviceBuffer, std::shared_ptr<resource::buffer> stagingBuffer,
                     std::size_t capacityInBytes, graphics::vertex_layout const &vertexLayout) noexcept
            : deviceBuffer_{deviceBuffer}, stagingBuffer_{stagingBuffer}, capacityInBytes_{capacityInBytes}, vertexLayout_{vertexLayout} { }

        resource::buffer const &deviceBuffer() const noexcept { return *deviceBuffer_; }
        resource::buffer const &stagingBuffer() const noexcept { return *stagingBuffer_; }

        std::size_t deviceBufferOffset() const noexcept { return deviceBuffer_->memory()->offset() + offset_; }
        std::size_t stagingBufferOffset() const noexcept { return stagingBuffer_->memory()->offset() + offset_; }

        std::size_t availableMemorySize() const noexcept { return capacityInBytes_ - offset_; }

        graphics::vertex_layout const &vertexLayout() const noexcept { return vertexLayout_; }

    private:

        std::shared_ptr<resource::buffer> deviceBuffer_{nullptr};
        std::shared_ptr<resource::buffer> stagingBuffer_{nullptr};

        std::size_t capacityInBytes_{0};

        graphics::vertex_layout vertexLayout_;

        std::size_t offset_{0};
        std::size_t stagingBufferSizeInBytes_{0};

        friend ResourceManager;
    };
}
