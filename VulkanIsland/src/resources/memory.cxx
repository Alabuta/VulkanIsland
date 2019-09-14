#include <boost/align.hpp>
#include <boost/align/align.hpp>

#include "memory.hxx"
#include "commandBuffer.hxx"


namespace {
[[nodiscard]] std::optional<std::uint32_t>
FindMemoryType(VulkanDevice const &vulkanDevice, std::uint32_t filter, VkMemoryPropertyFlags propertyFlags) noexcept
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(vulkanDevice.physical_handle(), &memoryProperties);

    auto const memoryTypes = mpl::to_array(memoryProperties.memoryTypes);

    auto it_type = std::find_if(std::cbegin(memoryTypes), std::cend(memoryTypes), [filter, propertyFlags, i = 0u] (auto type) mutable
    {
        return (filter & (1u << i++)) && (type.propertyFlags & propertyFlags) == propertyFlags;
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


std::shared_ptr<DeviceMemory>
MemoryManager::AllocateMemory(VkMemoryRequirements const &memoryRequirements2, VkMemoryPropertyFlags properties, bool linear)
{
    std::uint32_t memoryTypeIndex{0};

    using R = VkMemoryRequirements;

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

    std::size_t seed = 0;

    boost::hash_combine(seed, memoryTypeIndex);
    boost::hash_combine(seed, properties);
    boost::hash_combine(seed, linear);

    if (pools_.count(seed) < 1)
        pools_.try_emplace(seed, memoryTypeIndex, properties, linear);

    auto &&pool = pools_.at(seed);

    typename decltype(Pool::blocks)::iterator it_block;
    typename decltype(Pool::Block::availableChunks)::iterator it_chunk;

    it_block = std::find_if(std::begin(pool.blocks), std::end(pool.blocks), [&it_chunk, &memoryRequirements] (auto &&pair)
    {
        auto &&[handle, memoryBlock] = pair;

        if (memoryBlock.availableSize < memoryRequirements.size)
            return false;

        auto &&availableChunks = memoryBlock.availableChunks;

        if constexpr (std::is_same_v<std::decay_t<R>, VkMemoryRequirements>) {
            //auto [it_chunk_begin, it_chunk_end] = availableChunks.equal_range(memoryRequirements.size);
            auto it_chunk_begin = availableChunks.lower_bound(memoryRequirements.size);
            auto it_chunk_end = availableChunks.upper_bound(memoryRequirements.size);

            it_chunk = std::find_if(it_chunk_begin, it_chunk_end, [&memoryRequirements] (auto &&chunk)
            {
                auto alignedOffset = boost::alignment::align_up(chunk.offset, memoryRequirements.alignment);

                // if (linear)
                    //alignedOffset = boost::alignment::align_up(alignedOffset, imageGranularity);

                auto fits = alignedOffset + memoryRequirements.size <= chunk.offset + chunk.size;;

                return fits;
            });

            return it_chunk != it_chunk_end;
        }

        else {
            throw std::runtime_error("unimplemented case"s);

            it_chunk = std::begin(availableChunks);

            return availableChunks.size() == 1 && it_chunk->offset == 0;
        }
    });

    if (it_block == std::end(pool.blocks)) {
        if (auto result = AllocateMemoryBlock(
            memoryTypeIndex, kSUB_ALLOCATION ? kBLOCK_ALLOCATION_SIZE : memoryRequirements.size, properties, linear); !result)
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
            auto alignedOffset = boost::alignment::align_up(offset, memoryRequirements.alignment);

            // if (linear)
                //alignedOffset = boost::alignment::align_up(alignedOffset, imageGranularity);

            size -= memoryRequirements.size + alignedOffset - offset;
            offset = alignedOffset + memoryRequirements.size;

            availableChunks.insert(std::move(node));

            if (alignedOffset > offset)
                availableChunks.emplace(offset, alignedOffset - offset);

            memoryBlock.availableSize -= memoryRequirements.size;
            memoryOffset = alignedOffset;
        }

        else {
            // auto const sizeInBytes = memoryRequirements.size + linear ? imageGranularity : 0u;
            auto const wastedMemoryRatio = 100.f - static_cast<float>(memoryRequirements.size) / static_cast<float>(memoryBlock.availableSize) * 100.f;

            if (wastedMemoryRatio > 1.f)
                std::cerr << "Memory type index ["s << memoryTypeIndex << "]: wasted ratio is "s << wastedMemoryRatio << "%\n"s;

            availableChunks.emplace(memoryRequirements.size, memoryBlock.availableSize - memoryRequirements.size);
            memoryBlock.availableSize -= memoryRequirements.size;
        }

        availableChunks.erase(Pool::Block::Chunk{0, 0});

        std::cout << "Memory type index ["s << memoryTypeIndex << "]: sub-allocation : "s << static_cast<float>(memoryRequirements.size) / 1024.f << "KB\n"s;

        return std::shared_ptr<DeviceMemory>{
            new DeviceMemory{it_block->first, memoryTypeIndex, properties, memoryRequirements.size, memoryOffset, linear},
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

auto MemoryManager::AllocateMemoryBlock(std::uint32_t memoryTypeIndex, VkDeviceSize size, VkMemoryPropertyFlags properties, bool linear)
-> std::optional<decltype(Pool::blocks)::iterator>
{
    std::size_t seed = 0;

    boost::hash_combine(seed, memoryTypeIndex);
    boost::hash_combine(seed, properties);
    boost::hash_combine(seed, linear);

    if (pools_.count(seed) < 1) {
        std::cerr << "failed to find instantiated memory pool for type index: "s << memoryTypeIndex << '\n';
        return { };
    }

    auto &&pool = pools_.at(seed);

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

    std::cout << "Memory type index ["s << memoryTypeIndex << "]: "s << std::size(pool.blocks) << "th page allocation: "s;
    std::cout << static_cast<float>(size) / 1024.f << " KB/"s << static_cast<float>(totalAllocatedSize_) / std::pow(2.f, 20.f) << "MB\n"s;

    return it;
}

void MemoryManager::DeallocateMemory(DeviceMemory const &memory)
{
    if (pools_.count(memory.seed()) < 1) {
        std::cerr << "Memory manager: dead chunk encountered.\n"s;
        return;
    }

    auto &&pool = pools_.at(memory.seed());

    if (pool.blocks.count(memory.handle()) < 1) {
        std::cerr << "Memory manager: dead chunk encountered.\n"s;
        return;
    }

    auto &&block = pool.blocks.at(memory.handle());
    auto &&availableChunks = block.availableChunks;

    std::cout << "Memory type index ["s << memory.typeIndex() << "]"s;
    std::cout << ": releasing chunk "s << static_cast<float>(memory.size()) / 1024.f << "KB.\n"s;

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
