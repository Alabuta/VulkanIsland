#include <unordered_map>
#include <vector>

#include <fmt/format.h>

#include "graphics/graphics_api.hxx"

#include "buffer.hxx"
#include "image.hxx"

#include "memory_manager.hxx"


namespace
{
    std::optional<std::uint32_t>
    find_memory_type_index(vulkan::device const &device, std::uint32_t filter, graphics::MEMORY_PROPERTY_TYPE memory_property_types) noexcept
    {
        VkPhysicalDeviceMemoryProperties memory_properties;
        vkGetPhysicalDeviceMemoryProperties(device.physical_handle(), &memory_properties);

        auto const memory_types = mpl::to_array(memory_properties.memoryTypes);

        auto it_type = std::find_if(std::cbegin(memory_types), std::cend(memory_types), [filter, memory_property_types, i = 0u] (auto type) mutable
        {
            auto const property_flags = convert_to::vulkan(memory_property_types);

            return (filter & (1u << i++)) && (type.propertyFlags & property_flags) == property_flags;
        });

        if (it_type < std::next(std::cbegin(memory_types), memory_properties.memoryTypeCount))
            return static_cast<std::uint32_t>(std::distance(std::cbegin(memory_types), it_type));

        return { };
    }
}

namespace resource
{
    resource::memory_manager::memory_manager(vulkan::device const &device) : device_{device}
    {
        buffer_image_granularity_ = device.device_limits().buffer_image_granularity;

        if (kBLOCK_ALLOCATION_SIZE < buffer_image_granularity_)
            throw std::runtime_error("default memory page size is less than buffer image granularity size"s);
    }

    resource::memory_manager::~memory_manager()
    {
        ;
    }

    template<>
    std::shared_ptr<resource::device_memory>
    memory_manager::allocate_memory(std::shared_ptr<resource::buffer> buffer, graphics::MEMORY_PROPERTY_TYPE memory_property_types)
    {
        auto constexpr linear_memory = true;

        VkMemoryRequirements memory_requirements;
        vkGetBufferMemoryRequirements(device_.handle(), buffer->handle(), &memory_requirements);

        return allocate_memory(std::move(memory_requirements), memory_property_types, linear_memory);
    }

    template<>
    std::shared_ptr<resource::device_memory>
    memory_manager::allocate_memory(std::shared_ptr<resource::image> image, graphics::MEMORY_PROPERTY_TYPE memory_property_types)
    {
        auto const linear_memory = image->tiling() == graphics::IMAGE_TILING::LINEAR;

        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(device_.handle(), image->handle(), &memory_requirements);

        return allocate_memory(std::move(memory_requirements), memory_property_types, linear_memory);
    }

    std::shared_ptr<resource::device_memory>
    memory_manager::allocate_memory(VkMemoryRequirements &&memory_requirements, graphics::MEMORY_PROPERTY_TYPE memory_property_types, bool linear)
    {
        return std::shared_ptr<resource::device_memory>();
    }
}
