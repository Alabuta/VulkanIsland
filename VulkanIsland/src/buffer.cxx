#include "device.h"
#include "buffer.h"


DeviceMemoryPool::~DeviceMemoryPool()
{
    for (auto &&memoryBlock : memoryBlocks_)
        vkFreeMemory(vulkanDevice_->handle(), memoryBlock.second.handle, nullptr);

    memoryBlocks_.clear();
}

[[nodiscard]] std::optional<DeviceMemoryPool::DeviceMemory>
DeviceMemoryPool::AllocateMemory(VkMemoryRequirements const &memoryReqirements, VkMemoryPropertyFlags properties)
{
    memory_type_index_t memoryTypeIndex{0};

    std::cout << __FUNCTION__ << '\n';

    if (auto index = FindMemoryType(memoryReqirements.memoryTypeBits, properties); !index)
        throw std::runtime_error("failed to find suitable memory type"s);

    else memoryTypeIndex = index.value();

    if (memoryBlocks_.find(memoryTypeIndex) == std::end(memoryBlocks_))
        AllocateMemoryBlock(memoryTypeIndex, memoryReqirements.size);

    auto [it_begin, it_end] = memoryBlocks_.equal_range(memoryTypeIndex);

    decltype(MemoryBlock::availableChunks)::iterator it_chunk;

    auto it_memoryBlock = std::find_if(it_begin, it_end, [&it_chunk, &memoryReqirements] (auto &&pair)
    {
        auto &&[type, memoryBlock] = pair;

        if (memoryBlock.availableSize < memoryReqirements.size)
            return false;

#if NOT_YET_IMPLEMENTED
        it_chunk = memoryBlock.availableChunks.lower_bound(memoryReqirements.size);

        if (it_chunk == std::end(memoryBlock.availableChunks))
            return false;

        auto [offset, size] = *it_chunk;

        auto aligment = std::min(VkDeviceSize{1}, offset % memoryReqirements.alignment) * (memoryReqirements.alignment - (offset % memoryReqirements.alignment));

        return memoryReqirements.size <= size - aligment;
#else
        it_chunk = memoryBlock.availableChunks.lower_bound(kBLOCK_ALLOCATION_SIZE);
        return it_chunk != std::end(memoryBlock.availableChunks);
#endif
    });

    if (it_memoryBlock == it_end) {
        it_memoryBlock = AllocateMemoryBlock(memoryTypeIndex, memoryReqirements.size);
        it_chunk = it_memoryBlock->second.availableChunks.lower_bound(kBLOCK_ALLOCATION_SIZE);
    }

    //if (auto memoryBlock = memoryBlocks_.extract(it_memoryBlock); memoryBlock) {
    auto &&memoryBlock = it_memoryBlock->second;

        auto &&availableChunks = memoryBlock.availableChunks;

        if (auto node = availableChunks.extract(it_chunk); node) {
            auto &&[offset, size] = node.value();

#if NOT_YET_IMPLEMENTED
            auto aligment = std::min(VkDeviceSize{1}, offset % memoryReqirements.alignment) * (memoryReqirements.alignment - (offset % memoryReqirements.alignment));
#else
            auto aligment = 0;
#endif

            auto memoryOffset = offset + aligment;

            offset += memoryReqirements.size + aligment;
            size -= memoryReqirements.size + aligment;

            availableChunks.insert(std::move(node));

            memoryBlock.availableSize -= memoryReqirements.size + aligment;

            // DeviceMemory deviceMemory{memoryBlock.handle, memoryTypeIndex, memoryReqirements.size, memoryOffset};

            // memoryBlocks_.insert(std::move(memoryBlock));

            return DeviceMemory{memoryBlock.handle, memoryTypeIndex, memoryReqirements.size, memoryOffset};
        }

        else throw std::runtime_error("failed to extract available memory block chunk"s);
    /*}

    else throw std::runtime_error("failed to extract memory block"s);*/

    return std::nullopt;
}

void DeviceMemoryPool::FreeMemory(std::optional<DeviceMemory> &&_memory)
{
    if (_memory == std::nullopt)
        return;

    auto memory = std::move(_memory.value());

    _memory.reset();

    auto [it_begin, it_end] = memoryBlocks_.equal_range(memory.memoryTypeIndex());

    auto it_pair = std::find_if(it_begin, it_end, [handle = memory.handle()] (auto &&pair)
    {
        return handle == pair.second.handle;
    });

    if (it_pair == it_end)
        return;

    auto &&memoryBlock = it_pair->second;

     auto &&availableChunks = it_pair->second.availableChunks;

     auto it_chunk = std::find_if(std::begin(availableChunks), std::end(availableChunks), [&memory] (auto &&chunk)
     {
         return chunk.offset == memory.offset() + memory.size();
     });

     if (it_chunk != std::end(availableChunks)) {
         if (auto node = availableChunks.extract(it_chunk); node) {
             auto &&[offset, size] = node.value();

             offset -= memory.size();
             size += memory.size();

             availableChunks.insert(std::move(node));

             memoryBlock.availableSize += memory.size();
         }
     }

     else {
         it_chunk = std::find_if(std::begin(availableChunks), std::end(availableChunks), [&memory] (auto &&chunk)
         {
             return chunk.offset + chunk.size == memory.offset();
         });

         if (it_chunk == std::end(availableChunks))
             return;

         if (auto node = availableChunks.extract(it_chunk); node) {
             auto &&[offset, size] = node.value();

             size += memory.size();

             availableChunks.insert(std::move(node));

             memoryBlock.availableSize += memory.size();
         }
     }
}

auto DeviceMemoryPool::AllocateMemoryBlock(memory_type_index_t memoryTypeIndex, VkDeviceSize size)
-> decltype(memoryBlocks_)::iterator
{
    std::cout << __FUNCTION__ << '\n';

    if (size > kBLOCK_ALLOCATION_SIZE)
        throw std::runtime_error("requested allocation size is bigger than memory block allocation size"s);

    VkMemoryAllocateInfo const memAllocInfo{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        kBLOCK_ALLOCATION_SIZE,
        memoryTypeIndex
    };

    VkDeviceMemory handle;

    if (auto result = vkAllocateMemory(vulkanDevice_->handle(), &memAllocInfo, nullptr, &handle); result != VK_SUCCESS)
        throw std::runtime_error("failed to allocate block from device memory pool"s);

    return memoryBlocks_.emplace(std::piecewise_construct, 
                                 std::forward_as_tuple(memoryTypeIndex), std::forward_as_tuple(handle, kBLOCK_ALLOCATION_SIZE, memoryTypeIndex));
}

[[nodiscard]] std::optional<DeviceMemoryPool::memory_type_index_t>
DeviceMemoryPool::FindMemoryType(memory_type_index_t filter, VkMemoryPropertyFlags propertyFlags)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(vulkanDevice_->physical_handle(), &memoryProperties);

    auto const memoryTypes = to_array(memoryProperties.memoryTypes);

    auto it_type = std::find_if(std::cbegin(memoryTypes), std::cend(memoryTypes), [filter, propertyFlags, i = 0u] (auto type) mutable
    {
        return (filter & (1 << i++)) && (type.propertyFlags & propertyFlags) == propertyFlags;
    });

    if (it_type < std::next(std::cbegin(memoryTypes), memoryProperties.memoryTypeCount))
        return static_cast<std::uint32_t>(std::distance(std::cbegin(memoryTypes), it_type));

    return { };
}


[[nodiscard]] auto BufferPool::CreateBuffer(VulkanDevice *vulkanDevice, VkBuffer &buffer,
                                            VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    -> std::optional<DeviceMemoryPool::DeviceMemory>
{
    VkBufferCreateInfo const bufferCreateInfo{
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        nullptr, 0,
        size,
        usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0, nullptr
    };

    if (auto result = vkCreateBuffer(vulkanDevice->handle(), &bufferCreateInfo, nullptr, &buffer); result != VK_SUCCESS)
        throw std::runtime_error("failed to create buffer: "s + std::to_string(result));

    if (auto deviceMemory = vulkanDevice->memoryPool()->AllocateMemory(buffer, properties); deviceMemory) {
        if (auto result = vkBindBufferMemory(vulkanDevice->handle(), buffer, deviceMemory->handle(), deviceMemory->offset()); result != VK_SUCCESS)
            throw std::runtime_error("failed to bind buffer memory: "s + std::to_string(result));

        return std::move(deviceMemory);
    }

    else throw std::runtime_error("failed to allocate buffer memory"s);
}



[[nodiscard]] auto BufferPool::CreateImage(VulkanDevice *vulkanDevice, VkImage &image, std::uint32_t width, std::uint32_t height, std::uint32_t mipLevels,
                                           VkFormat format, VkImageTiling tiling, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    -> std::optional<DeviceMemoryPool::DeviceMemory>
{
    VkImageCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr, 0,
        VK_IMAGE_TYPE_2D,
        format,
        { width, height, 1 },
        mipLevels,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        tiling,
        usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0, nullptr,
        VK_IMAGE_LAYOUT_UNDEFINED
    };

    if (auto result = vkCreateImage(vulkanDevice->handle(), &createInfo, nullptr, &image); result != VK_SUCCESS)
        throw std::runtime_error("failed to create image: "s + std::to_string(result));

    if (auto deviceMemory = vulkanDevice->memoryPool()->AllocateMemory(image, properties); deviceMemory) {
        if (auto result = vkBindImageMemory(vulkanDevice->handle(), image, deviceMemory->handle(), deviceMemory->offset()); result != VK_SUCCESS)
            throw std::runtime_error("failed to bind image buffer memory: "s + std::to_string(result));

        return std::move(deviceMemory);
    }

    else throw std::runtime_error("failed to allocate image buffer memory"s);
}



[[nodiscard]] auto BufferPool::CreateUniformBuffer(VulkanDevice *vulkanDevice, VkBuffer &uboBuffer, std::size_t size)
    -> std::optional<DeviceMemoryPool::DeviceMemory>
{
    auto constexpr usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    return CreateBuffer(vulkanDevice, uboBuffer, size, usageFlags, propertyFlags);
}
