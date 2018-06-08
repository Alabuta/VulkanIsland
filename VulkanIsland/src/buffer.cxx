#include "device.h"
#include "buffer.h"




MemoryPool::~MemoryPool()
{
    for (auto &&memoryBlock : memoryBlocks_)
        vkFreeMemory(vulkanDevice_->handle(), memoryBlock.second.handle, nullptr);

    memoryBlocks_.clear();
}

template<class R, typename std::enable_if_t<std::is_same_v<std::decay_t<R>, VkMemoryRequirements> || std::is_same_v<std::decay_t<R>, VkMemoryRequirements2>>...>
[[nodiscard]] std::optional<MemoryPool::DeviceMemory>
MemoryPool::AllocateMemory(R &&memoryRequirements2, VkMemoryPropertyFlags properties)
{
    memory_type_index_t memoryTypeIndex{0};

    auto &&memoryRequirements = [] (auto &&memoryRequirements2)
    {
        if constexpr (std::is_same_v<std::decay_t<decltype(memoryRequirements2)>, VkMemoryRequirements>)
            return memoryRequirements2;

        else return memoryRequirements2.memoryRequirements;

    } (memoryRequirements2);

    if constexpr (std::is_same_v<std::decay_t<R>, VkMemoryRequirements>) {
        if (memoryRequirements.size > kBLOCK_ALLOCATION_SIZE)
            throw std::runtime_error("requested allocation size is bigger than memory page size"s);
    }

    if (auto index = FindMemoryType(memoryRequirements.memoryTypeBits, properties); !index)
        throw std::runtime_error("failed to find suitable memory type"s);

    else memoryTypeIndex = index.value();

    if (memoryBlocks_.find(memoryTypeIndex) == std::end(memoryBlocks_))
        AllocateMemoryBlock(memoryTypeIndex, kBLOCK_ALLOCATION_SIZE);

    auto [it_begin, it_end] = memoryBlocks_.equal_range(memoryTypeIndex);

    decltype(MemoryBlock::availableChunks)::iterator it_chunk;

    auto it_memoryBlock = std::find_if(it_begin, it_end, [&it_chunk, &memoryRequirements] (auto &&pair)
    {
        auto &&[type, memoryBlock] = pair;

        if (memoryBlock.availableSize < memoryRequirements.size)
            return false;

        auto &&availableChunks = memoryBlock.availableChunks;

        if constexpr (std::is_same_v<std::decay_t<R>, VkMemoryRequirements>) {
            auto [it_chunk_begin, it_chunk_end] = availableChunks.equal_range(memoryRequirements.size);

            it_chunk = std::find_if(it_chunk_begin, it_chunk_end, [&memoryRequirements] (auto &&chunk)
            {
                auto alignedOffset = ((chunk.offset + memoryRequirements.alignment - 1) / memoryRequirements.alignment) * memoryRequirements.alignment;

                return alignedOffset + memoryRequirements.size <= chunk.offset + chunk.size;
            });

            return it_chunk != it_chunk_end;
        }

        else {
            it_chunk = std::begin(availableChunks);

            return availableChunks.size() == 1 && it_chunk->offset == 0;
        }
    });

    if (it_memoryBlock == it_end)
        std::cout << "!!!!!!\n";

    if (it_memoryBlock == it_end) {
        if constexpr (std::is_same_v<std::decay_t<R>, VkMemoryRequirements>) {
            it_memoryBlock = AllocateMemoryBlock(memoryTypeIndex, kBLOCK_ALLOCATION_SIZE);
            it_chunk = it_memoryBlock->second.availableChunks.lower_bound(kBLOCK_ALLOCATION_SIZE);
        }

        else it_memoryBlock = AllocateMemoryBlock(memoryTypeIndex, memoryRequirements.size);
    }

    auto &&memoryBlock = it_memoryBlock->second;
    auto &&availableChunks = memoryBlock.availableChunks;

    if constexpr (std::is_same_v<std::decay_t<R>, VkMemoryRequirements>) {
        if (auto node = availableChunks.extract(it_chunk); node) {
            auto &&[offset, size] = node.value();

            auto alignedOffset = ((offset + memoryRequirements.alignment - 1) / memoryRequirements.alignment) * memoryRequirements.alignment;

            size -= memoryRequirements.size + alignedOffset - offset;
            offset = alignedOffset + memoryRequirements.size;

            availableChunks.insert(std::move(node));

            if (alignedOffset > offset)
                availableChunks.emplace(offset, alignedOffset - offset);

            memoryBlock.availableSize -= memoryRequirements.size;

            return DeviceMemory{memoryBlock.handle, memoryTypeIndex, memoryRequirements.size, alignedOffset};
        }

        else throw std::runtime_error("failed to extract available memory block chunk"s);
    }

    else {
        availableChunks.emplace(0, memoryRequirements.size);
        memoryBlock.availableSize = 0;

        return DeviceMemory{memoryBlock.handle, memoryTypeIndex, memoryRequirements.size, 0};
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<MemoryPool::DeviceMemory>
MemoryPool::AllocateDedicatedMemory(VkMemoryRequirements2 const &memoryRequirements2, VkMemoryPropertyFlags properties)
{
    memory_type_index_t memoryTypeIndex{0};

    std::cout << __FUNCTION__ << '\n';

    auto &&memoryRequirements = memoryRequirements2.memoryRequirements;

    if (auto index = FindMemoryType(memoryRequirements.memoryTypeBits, properties); !index)
        throw std::runtime_error("failed to find suitable memory type"s);

    else memoryTypeIndex = index.value();

    auto [it_begin, it_end] = memoryBlocks_.equal_range(memoryTypeIndex);

    auto it_memoryBlock = std::find_if(it_begin, it_end, [&memoryRequirements] (auto &&pair)
    {
        auto &&[type, memoryBlock] = pair;

        return memoryBlock.availableSize >= memoryRequirements.size;
    });

    if (it_memoryBlock == it_end)
        std::cout << "!!!!!!\n";

    if (it_memoryBlock == it_end)
        it_memoryBlock = AllocateMemoryBlock(memoryTypeIndex, memoryRequirements.size);

    auto &&memoryBlock = it_memoryBlock->second;
    auto &&availableChunks = memoryBlock.availableChunks;

    availableChunks.emplace(0, memoryRequirements.size);

    memoryBlock.availableSize = 0;

    return DeviceMemory{memoryBlock.handle, memoryTypeIndex, memoryRequirements.size, 0};
}

void MemoryPool::FreeMemory(std::optional<DeviceMemory> &&_memory)
{
    if (_memory == std::nullopt)
        return;

    std::cout << __FUNCTION__ << '\n';

    auto memory = std::move(_memory.value());

    _memory.reset();

    auto[it_begin, it_end] = memoryBlocks_.equal_range(memory.memoryTypeIndex());

    auto it_memoryBlock = std::find_if(it_begin, it_end, [handle = memory.handle()](auto &&pair)
    {
        return handle == pair.second.handle;
    });

    if (it_memoryBlock == it_end)
        return;

    auto &&memoryBlock = it_memoryBlock->second;
    auto &&availableChunks = it_memoryBlock->second.availableChunks;

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

auto MemoryPool::AllocateMemoryBlock(memory_type_index_t memoryTypeIndex, VkDeviceSize size)
-> decltype(memoryBlocks_)::iterator
{
    std::cout << __FUNCTION__ << '\n';

    VkMemoryAllocateInfo const memAllocInfo{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        size,
        memoryTypeIndex
    };

    VkDeviceMemory handle;

    if (auto result = vkAllocateMemory(vulkanDevice_->handle(), &memAllocInfo, nullptr, &handle); result != VK_SUCCESS)
        throw std::runtime_error("failed to allocate block from device memory pool"s);

    std::cout << memoryBlocks_.size() + 1 << '\n';

    return memoryBlocks_.emplace(std::piecewise_construct, 
                                 std::forward_as_tuple(memoryTypeIndex), std::forward_as_tuple(handle, size, memoryTypeIndex));
}

[[nodiscard]] std::optional<MemoryPool::memory_type_index_t>
MemoryPool::FindMemoryType(memory_type_index_t filter, VkMemoryPropertyFlags propertyFlags)
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
    -> std::optional<MemoryPool::DeviceMemory>
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
    -> std::optional<MemoryPool::DeviceMemory>
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
    -> std::optional<MemoryPool::DeviceMemory>
{
    auto constexpr usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    return CreateBuffer(vulkanDevice, uboBuffer, size, usageFlags, propertyFlags);
}

