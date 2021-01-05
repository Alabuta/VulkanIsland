#pragma once

#include <memory>

#include "main.hxx"
#include "utility/mpl.hxx"
#include "vulkan/device.hxx"


namespace resource
{
    class buffer;
    class image;

    class memory_manager;
    struct memory_allocator;
}

namespace resource
{
    struct memory_requirements final {
        std::size_t size, alignment;
        std::uint32_t memory_type_bits;
    };

    class memory_block final {
    public:

        VkDeviceMemory handle() const noexcept { return handle_; }

        std::size_t size() const noexcept { return size_; }
        std::size_t offset() const noexcept { return offset_; }

        std::uint32_t type_index() const noexcept { return type_index_; }
        graphics::MEMORY_PROPERTY_TYPE properties() const noexcept { return properties_; }

        bool is_linear() const noexcept { return is_linear_; }

    private:

        VkDeviceMemory handle_;

        std::size_t size_, offset_;

        std::uint32_t type_index_;
        graphics::MEMORY_PROPERTY_TYPE properties_;

        bool is_linear_;

        memory_block(VkDeviceMemory handle, std::size_t size, std::size_t offset, std::uint32_t type_index,
                      graphics::MEMORY_PROPERTY_TYPE properties, bool is_linear) noexcept;

        friend resource::memory_allocator;
    };

    class memory_manager final {
    public:

        static std::size_t constexpr kPAGE_ALLOCATION_SIZE{0x1000'0000}; // 256 MB

        memory_manager(vulkan::device const &device);

        template<class T>
        requires mpl::is_one_of_v<std::remove_cvref_t<T>, resource::buffer, resource::image>
        std::shared_ptr<resource::memory_block>
        allocate_memory(T &&resource, graphics::MEMORY_PROPERTY_TYPE memory_property_types);

    private:

        vulkan::device const &device_;

        std::shared_ptr<resource::memory_allocator> allocator_;

        std::shared_ptr<resource::memory_block>
        allocate_buffer_memory(resource::buffer const &buffer, graphics::MEMORY_PROPERTY_TYPE memory_property_types);

        std::shared_ptr<resource::memory_block>
        allocate_image_memory(resource::image const &image, graphics::MEMORY_PROPERTY_TYPE memory_property_types);
    };

    template<class T>
    requires mpl::is_one_of_v<std::remove_cvref_t<T>, resource::buffer, resource::image>
    std::shared_ptr<resource::memory_block>
    memory_manager::allocate_memory(T &&resource, graphics::MEMORY_PROPERTY_TYPE memory_property_types)
    {
        using resource_type = typename std::remove_cvref_t<T>;

        if constexpr (std::is_same_v<resource_type, resource::buffer>)
            return allocate_buffer_memory(resource, memory_property_types);

        else if constexpr (std::is_same_v<resource_type, resource::image>)
            return allocate_image_memory(resource, memory_property_types);

        else return { };
    }
}
