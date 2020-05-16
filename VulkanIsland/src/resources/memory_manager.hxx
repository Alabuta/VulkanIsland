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
    class device_memory final {
    public:

        VkDeviceMemory handle() const noexcept { return handle_; }

        std::size_t size() const noexcept { return size_; }
        std::size_t offset() const noexcept { return offset_; }

        std::uint32_t type_index() const noexcept { return type_index_; }
        graphics::MEMORY_PROPERTY_TYPE properties() const noexcept { return properties_; }

        bool linear() const noexcept { return linear_; }

    private:

        VkDeviceMemory handle_;

        std::size_t size_, offset_;

        std::uint32_t type_index_;
        graphics::MEMORY_PROPERTY_TYPE properties_;

        bool linear_;

        device_memory(VkDeviceMemory handle, std::size_t size, std::size_t offset, std::uint32_t type_index,
                      graphics::MEMORY_PROPERTY_TYPE properties, bool linear) noexcept;

        friend resource::memory_allocator;
    };

    class memory_manager final {
    public:

        memory_manager(vulkan::device const &device);

        template<class T>
        requires mpl::is_one_of_v<std::remove_cvref_t<T>, resource::buffer, resource::image>
        std::shared_ptr<resource::device_memory>
        allocate_memory(T &&resource, graphics::MEMORY_PROPERTY_TYPE memory_property_types);

    private:

        vulkan::device const &device_;

        std::shared_ptr<resource::memory_allocator> allocator_;

        std::shared_ptr<resource::device_memory>
        allocate_buffer_memory(resource::buffer const &buffer, graphics::MEMORY_PROPERTY_TYPE memory_property_types);

        std::shared_ptr<resource::device_memory>
        allocate_image_memory(resource::image const &image, graphics::MEMORY_PROPERTY_TYPE memory_property_types);
    };

    template<class T>
    requires mpl::is_one_of_v<std::remove_cvref_t<T>, resource::buffer, resource::image>
    std::shared_ptr<resource::device_memory>
    memory_manager::allocate_memory(T &&resource, graphics::MEMORY_PROPERTY_TYPE memory_property_types)
    {
        using resource_type = typename std::remove_cvref_t<T>;

        if constexpr (std::is_same_v<resource_type, resource::buffer>) {
            return allocate_buffer_memory(resource, memory_property_types);
        }

        else if constexpr (std::is_same_v<resource_type, resource::image>)  {
            return allocate_image_memory(resource, memory_property_types);
        }

        else return { };
    }
}
