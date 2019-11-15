#pragma once

#include <memory>

#include "main.hxx"
#include "utility/mpl.hxx"
#include "vulkan/device.hxx"

#include "vulkan/device.hxx"


namespace resource
{
    class buffer;
    class image;

    class memory_manager;
}

namespace resource
{
    class device_memory final {
    public:

        VkDeviceMemory handle() const noexcept { return handle_; }

        std::size_t size() const noexcept { return size_; }
        std::size_t offset() const noexcept { return offset_; }

        std::uint32_t type_index() const noexcept { return type_index_; }
        VkMemoryPropertyFlags properties() const noexcept { return properties_; }

    private:

        VkDeviceMemory handle_;

        std::size_t size_, offset_;

        std::uint32_t type_index_;
        VkMemoryPropertyFlags properties_;

        bool linear_;

        friend resource::memory_manager;
    };

    class memory_manager final {
    public:

        memory_manager(vulkan::device const &device);
        ~memory_manager();

        template<class T, typename std::enable_if_t<mpl::is_one_of_v<T, std::shared_ptr<resource::buffer>, std::shared_ptr<resource::image>>>* = nullptr>
        std::shared_ptr<resource::device_memory> allocate_memory(T buffer, graphics::MEMORY_PROPERTY_TYPE memory_property_types);

    private:

        static std::size_t constexpr kBLOCK_ALLOCATION_SIZE{0x800'0000};   // 128 MB

        vulkan::device const &device_;

        std::size_t total_allocated_size_{0}, buffer_image_granularity_{0};

        std::shared_ptr<resource::device_memory>
        allocate_memory(VkMemoryRequirements &&memory_requirements, graphics::MEMORY_PROPERTY_TYPE, bool linear);
    };
}
