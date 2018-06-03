#include "device.h"
#include "buffer.h"


[[nodiscard]] std::optional<std::pair<VkDeviceMemory, VkDeviceSize>>
DeviceMemoryPool::AllocateMemory(VkMemoryRequirements const &memoryReqirements, VkMemoryPropertyFlags properties)
{
    std::uint32_t memTypeIndex = 0;

    if (auto index = FindMemoryType(memoryReqirements.memoryTypeBits, properties); !index)
        throw std::runtime_error("failed to find suitable memory type"s);

    else memTypeIndex = index.value();

    if (memoryBlocks_.find(memTypeIndex) == std::end(memoryBlocks_))
        AllocateMemoryBlock(memTypeIndex, memoryReqirements.size);

    auto [it_begin, it_end] = memoryBlocks_.equal_range(memTypeIndex);

    decltype(MemoryBlock::availableChunks)::iterator it_chunk;

    auto it_memoryBlock = std::find_if(it_begin, it_end, [&it_chunk, &memoryReqirements](auto &&pair)
    {
        auto &&[memTypeIndex, memoryBlock] = pair;

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
        it_memoryBlock = AllocateMemoryBlock(memTypeIndex, memoryReqirements.size);
        it_chunk = it_memoryBlock->second.availableChunks.lower_bound(kBLOCK_ALLOCATION_SIZE);
    }

    VkDeviceSize memoryOffset;

    auto &&availableChunks = it_memoryBlock->second.availableChunks;

    if (auto node = availableChunks.extract(it_chunk); node) {
        auto &&[offset, size] = node.value();

#if NOT_YET_IMPLEMENTED
        auto aligment = std::min(VkDeviceSize{1}, offset % memoryReqirements.alignment) * (memoryReqirements.alignment - (offset % memoryReqirements.alignment));
#else
        auto aligment = 0;
#endif

        memoryOffset = offset + aligment;

        offset += memoryReqirements.size + aligment;
        size -= memoryReqirements.size + aligment;

        availableChunks.insert(std::move(node));

        it_memoryBlock->second.availableSize -= memoryReqirements.size + aligment;
    }

    else throw std::runtime_error("failed to extract available memory block chunk"s);

    return std::make_pair(it_memoryBlock->second.handle, memoryOffset);
}

DeviceMemoryPool::~DeviceMemoryPool()
{
    for (auto &&memoryBlock : memoryBlocks_)
        vkFreeMemory(vulkanDevice_->handle(), memoryBlock.second.handle, nullptr);

    memoryBlocks_.clear();
}

auto DeviceMemoryPool::AllocateMemoryBlock(std::uint32_t memTypeIndex, VkDeviceSize size)
-> decltype(memoryBlocks_)::iterator
{
    if (size > kBLOCK_ALLOCATION_SIZE)
        throw std::runtime_error("requested allocation size is bigger than memory block allocation size"s);

    VkMemoryAllocateInfo const memAllocInfo{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        kBLOCK_ALLOCATION_SIZE,
        memTypeIndex
    };

    VkDeviceMemory deviceMemory;

    if (auto result = vkAllocateMemory(vulkanDevice_->handle(), &memAllocInfo, nullptr, &deviceMemory); result != VK_SUCCESS)
        throw std::runtime_error("failed to allocate block from device memory pool"s);

    return memoryBlocks_.emplace(std::piecewise_construct,
                                 std::forward_as_tuple(memTypeIndex), std::forward_as_tuple(deviceMemory, kBLOCK_ALLOCATION_SIZE));
}

[[nodiscard]] std::optional<std::uint32_t>
DeviceMemoryPool::FindMemoryType(std::uint32_t filter, VkMemoryPropertyFlags propertyFlags)
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


void CreateBuffer(VulkanDevice *vulkanDevice,
                  VkBuffer &buffer, VkDeviceMemory &deviceMemory, VkDeviceSize size,
                  VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
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

    VkDeviceSize memoryOffset;

    if (auto result = vulkanDevice->memoryPool()->AllocateMemory(buffer, properties); !result)
        throw std::runtime_error("failed to allocate buffer memory"s);

    else std::tie(deviceMemory, memoryOffset) = result.value();

    if (auto result = vkBindBufferMemory(vulkanDevice->handle(), buffer, deviceMemory, memoryOffset); result != VK_SUCCESS)
        throw std::runtime_error("failed to bind buffer memory: "s + std::to_string(result));
}



void CreateImage(VulkanDevice *vulkanDevice,
                 VkImage &image, VkDeviceMemory &deviceMemory, std::uint32_t width, std::uint32_t height, std::uint32_t mipLevels,
                 VkFormat format, VkImageTiling tiling, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
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

    VkDeviceSize memoryOffset;

    if (auto result = vulkanDevice->memoryPool()->AllocateMemory(image, properties); !result)
        throw std::runtime_error("failed to allocate image buffer memory"s);

    else std::tie(deviceMemory, memoryOffset) = result.value();

    if (auto result = vkBindImageMemory(vulkanDevice->handle(), image, deviceMemory, memoryOffset); result != VK_SUCCESS)
        throw std::runtime_error("failed to bind image buffer memory: "s + std::to_string(result));
}



void CreateUniformBuffer(VulkanDevice *vulkanDevice, VkBuffer &uboBuffer, VkDeviceMemory &uboBufferMemory, std::size_t size)
{
    CreateBuffer(vulkanDevice, uboBuffer, uboBufferMemory, size,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}
