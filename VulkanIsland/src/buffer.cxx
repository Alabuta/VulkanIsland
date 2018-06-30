#include "device.hxx"
#include "buffer.hxx"
#include "image.hxx"

namespace {
[[nodiscard]] std::optional<std::uint32_t>
FindMemoryType(VulkanDevice const &vulkanDevice, std::uint32_t filter, VkMemoryPropertyFlags propertyFlags) noexcept
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(vulkanDevice.physical_handle(), &memoryProperties);

    auto const memoryTypes = to_array(memoryProperties.memoryTypes);

    auto it_type = std::find_if(std::cbegin(memoryTypes), std::cend(memoryTypes), [filter, propertyFlags, i = 0u] (auto type) mutable
    {
        return (filter & (1 << i++)) && (type.propertyFlags & propertyFlags) == propertyFlags;
    });

    if (it_type < std::next(std::cbegin(memoryTypes), memoryProperties.memoryTypeCount))
        return static_cast<std::uint32_t>(std::distance(std::cbegin(memoryTypes), it_type));

    return { };
}
}


MemoryManager::MemoryManager(VulkanDevice const &vulkanDevice, VkDeviceSize bufferImageGranularity)
        : vulkanDevice_{vulkanDevice}, bufferImageGranularity_{bufferImageGranularity}
{
    if (kBLOCK_ALLOCATION_SIZE < bufferImageGranularity_)
        throw std::runtime_error("default memory page is less than buffer image granularity size"s);
}

MemoryManager::~MemoryManager()
{
    for (auto &&[type, pool] : pools_)
        for (auto &&[handle, block] : pool.blocks)
            vkFreeMemory(vulkanDevice_.handle(), handle, nullptr);

    pools_.clear();
}


template<class R, typename std::enable_if_t<is_one_of_v<std::decay_t<R>, VkMemoryRequirements, VkMemoryRequirements2>>...>
std::shared_ptr<DeviceMemory>
MemoryManager::AllocateMemory(R &&memoryRequirements2, VkMemoryPropertyFlags properties)
{
    std::uint32_t memoryTypeIndex{0};

    auto constexpr kSUB_ALLOCATION = std::is_same_v<std::decay_t<R>, VkMemoryRequirements>;

    auto &&memoryRequirements = [] (auto &&memoryRequirements2)
    {
        if constexpr (std::is_same_v<std::decay_t<decltype(memoryRequirements2)>, VkMemoryRequirements>)
            return memoryRequirements2;

        else return memoryRequirements2.memoryRequirements;

    } (memoryRequirements2);

    if constexpr (kSUB_ALLOCATION) {
        if (memoryRequirements.size > kBLOCK_ALLOCATION_SIZE) {
            std::cerr << "requested allocation size is bigger than memory page size\n"s;
            return { };
        }
    }

    if (auto index = FindMemoryType(vulkanDevice_, memoryRequirements.memoryTypeBits, properties); !index) {
        std::cerr << "failed to find suitable memory type\n"s;
        return { };
    }

    else memoryTypeIndex = index.value();

    if (pools_.count(memoryTypeIndex) < 1)
        pools_.emplace(memoryTypeIndex, memoryTypeIndex);

    auto &&pool = pools_.at(memoryTypeIndex);

    decltype(Pool::blocks)::iterator it_block;
    decltype(Pool::Block::availableChunks)::iterator it_chunk;

    it_block = std::find_if(std::begin(pool.blocks), std::end(pool.blocks), [&it_chunk, &memoryRequirements] (auto &&pair)
    {
        auto &&[handle, memoryBlock] = pair;

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

    if (it_block == std::end(pool.blocks)) {
        if (auto result = AllocateMemoryBlock(memoryTypeIndex, kSUB_ALLOCATION ? kBLOCK_ALLOCATION_SIZE : memoryRequirements.size); !result)
            return { };

        else it_block = result.value();

        it_chunk = it_block->second.availableChunks.lower_bound(kSUB_ALLOCATION ? kBLOCK_ALLOCATION_SIZE : memoryRequirements.size);

        if (it_chunk == std::end(it_block->second.availableChunks))
            return { };
    }

    auto &&memoryBlock = it_block->second;
    auto &&availableChunks = memoryBlock.availableChunks;

    if (auto node = availableChunks.extract(it_chunk); node) {
        auto &&[offset, size] = node.value();

        auto memoryOffset = offset;

        if constexpr (kSUB_ALLOCATION) {
            auto alignedOffset = ((offset + memoryRequirements.alignment - 1) / memoryRequirements.alignment) * memoryRequirements.alignment;

            size -= memoryRequirements.size + alignedOffset - offset;
            offset = alignedOffset + memoryRequirements.size;

            availableChunks.insert(std::move(node));

            if (alignedOffset > offset)
                availableChunks.emplace(offset, alignedOffset - offset);

            memoryBlock.availableSize -= memoryRequirements.size;
            memoryOffset = alignedOffset;
        }

        else {
            auto const wastedMemoryRatio = 100.f - memoryRequirements.size / static_cast<float>(memoryBlock.availableSize) * 100.f;

            if (wastedMemoryRatio > 1.f)
                std::cerr << "Memory pool: wasted memory ratio: "s << wastedMemoryRatio << "%\n"s;

            availableChunks.emplace(memoryRequirements.size, memoryBlock.availableSize - memoryRequirements.size);
            memoryBlock.availableSize -= memoryRequirements.size;
        }

        availableChunks.erase(Pool::Block::Chunk{0, 0});

        std::cout << "Memory pool: ["s << memoryTypeIndex << "]: sub-allocation : "s << memoryRequirements.size / 1024.f << "KB\n"s;

        return std::shared_ptr<DeviceMemory>{
            new DeviceMemory{it_block->first, memoryTypeIndex, memoryRequirements.size, memoryOffset},
            [this] (DeviceMemory *const ptr_memory)
            {
                DeallocateMemory(*ptr_memory);

                delete ptr_memory;
            }
        };
    }

    else std::cerr << "failed to extract available memory block chunk\n"s;

    return { };
}

auto MemoryManager::AllocateMemoryBlock(std::uint32_t memoryTypeIndex, VkDeviceSize size)
-> std::optional<decltype(Pool::blocks)::iterator>
{
    if (pools_.count(memoryTypeIndex) < 1) {
        std::cerr << "failed to find instantiated memory pool for type index: "s << memoryTypeIndex << '\n';
        return { };
    }

    auto &&pool = pools_.at(memoryTypeIndex);

    VkMemoryAllocateInfo const memAllocInfo{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        size,
        memoryTypeIndex
    };

    VkDeviceMemory handle;

    if (auto result = vkAllocateMemory(vulkanDevice_.handle(), &memAllocInfo, nullptr, &handle); result != VK_SUCCESS) {
        std::cerr << "failed to allocate block from device memory pool\n"s;
        return { };
    }

    totalAllocatedSize_ += size;
    pool.allocatedSize += size;

    auto it = pool.blocks.try_emplace(handle, size).first;

    std::cout << "Memory pool: ["s << memoryTypeIndex << "]: #"s << std::size(pool.blocks) << " page allocation: "s;
    std::cout << size / 1024.f << " KB/"s << totalAllocatedSize_ / std::pow(2.f, 20.f) << "MB\n"s;

    return it;
}

void MemoryManager::DeallocateMemory(DeviceMemory const &memory)
{
    if (pools_.count(memory.typeIndex()) < 1) {
        std::cerr << "Memory pool: dead chunk encountered.\n"s;
        return;
    }

    auto &&pool = pools_.at(memory.typeIndex());

    if (pool.blocks.count(memory.handle()) < 1) {
        std::cerr << "Memory pool: dead chunk encountered.\n"s;
        return;
    }

    auto &&block = pool.blocks.at(memory.handle());
    auto &&availableChunks = block.availableChunks;

    std::cout << "Memory pool: ["s << memory.typeIndex() << "]: releasing chunk: "s << memory.size() / 1024.f << "KB.\n"s;

    auto it_chunk = availableChunks.emplace(memory.offset(), memory.size());

    auto FindAdjacent = [] (auto begin, auto end, auto it_chunk) constexpr
    {
        return std::find_if(begin, end, [it_chunk] (auto &&chunk)
        {
            return chunk.offset + chunk.size == it_chunk->offset || it_chunk->offset + it_chunk->size == chunk.offset;
        });
    };

    auto it_adjacent = FindAdjacent(std::begin(availableChunks), std::end(availableChunks), it_chunk);

    while (it_adjacent != std::end(availableChunks)) {
        auto [offsetA, sizeA] = *it_chunk;
        auto [offsetB, sizeB] = *it_adjacent;

        availableChunks.erase(it_chunk);
        availableChunks.erase(it_adjacent);

        it_chunk = availableChunks.emplace(std::min(offsetA, offsetB), sizeA + sizeB);

        it_adjacent = FindAdjacent(std::begin(availableChunks), std::end(availableChunks), it_chunk);
    }

    block.availableSize += memory.size();
}



auto CreateBuffer(VulkanDevice &device, VkBuffer &buffer,
                              VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    -> std::shared_ptr<DeviceMemory>
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
        throw std::runtime_error("failed to create buffer: "s + std::to_string(result));

    if (auto memory = device.memoryManager().AllocateMemory(buffer, properties); !memory)
        throw std::runtime_error("failed to allocate buffer memory"s);

    else {
        if (auto result = vkBindBufferMemory(device.handle(), buffer, memory->handle(), memory->offset()); result != VK_SUCCESS)
            throw std::runtime_error("failed to bind buffer memory: "s + std::to_string(result));

        return memory;
    }

    return { };
}




std::optional<VkBuffer>
CreateBufferHandle(VulkanDevice const &device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) noexcept
{
    std::optional<VkBuffer> buffer;

    VkBufferCreateInfo const bufferCreateInfo{
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        nullptr, 0,
        size,
        usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0, nullptr
    };

    VkBuffer handle;

    if (auto result = vkCreateBuffer(device.handle(), &bufferCreateInfo, nullptr, &handle); result != VK_SUCCESS)
        std::cerr << "failed to create buffer: "s << result << '\n';

    else buffer.emplace(handle);

    return buffer;
}