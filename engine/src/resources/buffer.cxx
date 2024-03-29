#include "buffer.hxx"


namespace resource
{
    buffer::buffer(VkBuffer handle, std::shared_ptr<resource::memory_block> memory, std::size_t size_bytes, graphics::BUFFER_USAGE usage,
                   graphics::RESOURCE_SHARING_MODE sharing_mode)
        : handle_{handle}, memory_{memory}, size_bytes_{size_bytes}, usage_{usage}, sharing_mode_{sharing_mode}
    { }

    staging_buffer::staging_buffer(std::shared_ptr<resource::buffer> buffer, std::span<std::byte> mapped_range, std::size_t offset_bytes)
        : buffer_{buffer}, mapped_ptr_{mapped_range}, offset_bytes_{offset_bytes}
    { }

    index_buffer::index_buffer(std::shared_ptr<resource::buffer> device_buffer, std::size_t offset_bytes, std::size_t available_size, graphics::INDEX_TYPE index_type)
        : device_buffer_{device_buffer}, offset_bytes_{offset_bytes}, available_size_{available_size}, index_type_{index_type}
    { }

    vertex_buffer::vertex_buffer(
        std::shared_ptr<resource::buffer> device_buffer, std::size_t offset_bytes, std::size_t available_size, graphics::vertex_layout const &vertex_layout)
        : device_buffer_{device_buffer}, offset_bytes_{offset_bytes}, available_size_{available_size}, vertex_layout_{vertex_layout}
    { }
}

#ifdef NOT_YET_IMPLEMENTED
namespace resource
{
    std::size_t hash<resource::buffer>::operator() (std::shared_ptr<resource::buffer> const buffer) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, buffer->usage());
        boost::hash_combine(seed, buffer->sharing_mode());

        return seed;
    }
}
#endif

#ifdef OBSOLETE
std::shared_ptr<DeviceMemory>
CreateBuffer(vulkan::device &device, VkBuffer &buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo const bufferCreateInfo{
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        nullptr, 0,
        size,
        usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0, nullptr
    };

    if (auto result = vkCreateBuffer(device.handle(), &bufferCreateInfo, nullptr, &buffer); result != VK_SUCCESS)
        throw vulkan::exception("failed to create buffer: "s + std::to_string(result));

    if (auto memory = device.memory_manager().AllocateMemory(buffer, properties); !memory)
        throw memory::exception("failed to allocate buffer memory"s);

    else {
        if (auto result = vkBindBufferMemory(device.handle(), buffer, memory->handle(), memory->offset()); result != VK_SUCCESS)
            throw vulkan::exception("failed to bind buffer memory: "s + std::to_string(result));

        return memory;
    }

    return { };
}
#endif
