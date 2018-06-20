#include "device.h"
#include "buffer.h"
#include "image.h"

namespace {

[[nodiscard]] std::optional<std::uint32_t>
FindMemoryType(VulkanDevice const &vulkanDevice, std::uint32_t filter, VkMemoryPropertyFlags propertyFlags)
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

MemoryPool::MemoryPool(VulkanDevice &vulkanDevice, VkDeviceSize bufferImageGranularity)
        : vulkanDevice_{vulkanDevice}, bufferImageGranularity_{bufferImageGranularity}
{
    if (kBLOCK_ALLOCATION_SIZE < bufferImageGranularity_)
        throw std::runtime_error("default memory page is less than buffer image granularity size"s);

}

MemoryPool::~MemoryPool()
{
    for (auto &&memoryBlock : memoryBlocks_)
        vkFreeMemory(vulkanDevice_.handle(), memoryBlock.second.handle, nullptr);

    memoryBlocks_.clear();
}

template<class T, typename std::enable_if_t<is_one_of_v<T, VkBuffer, VkImage>>...>
[[nodiscard]] std::shared_ptr<DeviceMemory>
MemoryPool::CheckRequirementsAndAllocate(T buffer, VkMemoryPropertyFlags properties)
{
    VkMemoryDedicatedRequirements memoryDedicatedRequirements{
        VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS,
        nullptr,
        0, 0
    };

    VkMemoryRequirements2 memoryRequirements2{
        VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
        &memoryDedicatedRequirements,{ }
    };

    if constexpr (std::is_same_v<T, VkBuffer>)
    {
        VkBufferMemoryRequirementsInfo2 const bufferMemoryRequirements{
            VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
            nullptr,
            buffer
        };

        vkGetBufferMemoryRequirements2(vulkanDevice_.handle(), &bufferMemoryRequirements, &memoryRequirements2);
    }

    else {
        VkImageMemoryRequirementsInfo2 const imageMemoryRequirements{
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
            nullptr,
            buffer
        };

        vkGetImageMemoryRequirements2(vulkanDevice_.handle(), &imageMemoryRequirements, &memoryRequirements2);
    }

    if (memoryDedicatedRequirements.prefersDedicatedAllocation | memoryDedicatedRequirements.requiresDedicatedAllocation)
        return AllocateMemory(memoryRequirements2, properties);

    else return AllocateMemory(memoryRequirements2.memoryRequirements, properties);
}

template<class R, typename std::enable_if_t<is_one_of_v<std::decay_t<R>, VkMemoryRequirements, VkMemoryRequirements2>>...>
[[nodiscard]] std::shared_ptr<DeviceMemory>
MemoryPool::AllocateMemory(R &&memoryRequirements2, VkMemoryPropertyFlags properties)
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
        if (memoryRequirements.size > kBLOCK_ALLOCATION_SIZE)
            throw std::runtime_error("requested allocation size is bigger than memory page size"s);
    }

    if (auto index = FindMemoryType(vulkanDevice_, memoryRequirements.memoryTypeBits, properties); !index)
        throw std::runtime_error("failed to find suitable memory type"s);

    else memoryTypeIndex = index.value();

    if (memoryBlocks_.find(memoryTypeIndex) == std::end(memoryBlocks_))
        AllocateMemoryBlock(memoryTypeIndex, kSUB_ALLOCATION ? kBLOCK_ALLOCATION_SIZE : memoryRequirements.size);

    decltype(memoryBlocks_)::iterator it_block;
    decltype(Block::availableChunks)::iterator it_chunk;

    auto [it_begin, it_end] = memoryBlocks_.equal_range(memoryTypeIndex);

    if (it_begin == std::end(memoryBlocks_)) {
        it_block = AllocateMemoryBlock(memoryTypeIndex, kSUB_ALLOCATION ? kBLOCK_ALLOCATION_SIZE : memoryRequirements.size);
        it_chunk = it_block->second.availableChunks.lower_bound(kSUB_ALLOCATION ? kBLOCK_ALLOCATION_SIZE : memoryRequirements.size);
    }

    else {
        it_block = std::find_if(it_begin, it_end, [&it_chunk, &memoryRequirements] (auto &&pair)
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

        if (it_block == it_end) {
            it_block = AllocateMemoryBlock(memoryTypeIndex, kSUB_ALLOCATION ? kBLOCK_ALLOCATION_SIZE : memoryRequirements.size);
            it_chunk = it_block->second.availableChunks.lower_bound(kSUB_ALLOCATION ? kBLOCK_ALLOCATION_SIZE : memoryRequirements.size);
        }
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

        availableChunks.erase(Block::Chunk{0, 0});

        std::cout << "Memory pool: ["s << memoryTypeIndex << "]: sub-allocation : "s << memoryRequirements.size / 1024.f << "KB\n"s;

        return std::shared_ptr<DeviceMemory>{
            new DeviceMemory{memoryBlock.handle, memoryTypeIndex, memoryRequirements.size, memoryOffset},
            [this] (DeviceMemory *const ptr_memory)
            {
                DeallocateMemory(*ptr_memory);

                delete ptr_memory;
            }
        };
    }

    else throw std::runtime_error("failed to extract available memory block chunk"s);

    return { };
}

auto MemoryPool::AllocateMemoryBlock(std::uint32_t memoryTypeIndex, VkDeviceSize size)
-> decltype(memoryBlocks_)::iterator
{
    VkMemoryAllocateInfo const memAllocInfo{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        size,
        memoryTypeIndex
    };

    VkDeviceMemory handle;

    if (auto result = vkAllocateMemory(vulkanDevice_.handle(), &memAllocInfo, nullptr, &handle); result != VK_SUCCESS)
        throw std::runtime_error("failed to allocate block from device memory pool"s);

    allocatedSize_ += size;

    std::cout << "Memory pool: ["s << memoryTypeIndex << "]: #"s << memoryBlocks_.size() + 1 << " page allocation: "s;
    std::cout << size / 1024.f << " KB/"s << allocatedSize_ / std::pow(2.f, 20.f) << "MB\n"s;

    return memoryBlocks_.emplace(std::piecewise_construct, 
                                 std::forward_as_tuple(memoryTypeIndex), std::forward_as_tuple(handle, size, memoryTypeIndex));
}

void MemoryPool::DeallocateMemory(DeviceMemory const &memory)
{
    auto [it_begin, it_end] = memoryBlocks_.equal_range(memory.typeIndex());

    if (it_begin == std::end(memoryBlocks_)) {
        std::cerr << "Memory pool: dead chunk encountered.\n"s;
        return;
    }

    auto it_block = std::find_if(it_begin, it_end, [handle = memory.handle()](auto &&pair)
    {
        return handle == pair.second.handle;
    });

    if (it_block == it_end) {
        std::cerr << "Memory pool: dead chunk encountered.\n"s;
        return;
    }

    std::cout << "Memory pool: ["s << memory.typeIndex() << "]: releasing chunk: "s << memory.size() / 1024.f << "KB.\n"s;

    auto &&memoryBlock = it_block->second;
    auto &&availableChunks = it_block->second.availableChunks;

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
        auto[offsetA, sizeA] = *it_chunk;
        auto[offsetB, sizeB] = *it_adjacent;

        availableChunks.erase(it_chunk);
        availableChunks.erase(it_adjacent);

        it_chunk = availableChunks.emplace(std::min(offsetA, offsetB), sizeA + sizeB);

        it_adjacent = FindAdjacent(std::begin(availableChunks), std::end(availableChunks), it_chunk);
    }

    memoryBlock.availableSize += memory.size();
}

[[nodiscard]] auto BufferPool::CreateBuffer(VulkanDevice &device, VkBuffer &buffer,
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


    if (auto memory = device.memoryPool().AllocateMemory(buffer, properties); !memory)
        throw std::runtime_error("failed to allocate buffer memory"s);

    else {
        if (auto result = vkBindBufferMemory(device.handle(), buffer, memory->handle(), memory->offset()); result != VK_SUCCESS)
            throw std::runtime_error("failed to bind buffer memory: "s + std::to_string(result));

        return memory;
    }

    return { };
}

[[nodiscard]] auto BufferPool::CreateImage(VulkanDevice &vulkanDevice, VkImage &image, std::uint32_t width, std::uint32_t height, std::uint32_t mipLevels,
                                           VkFormat format, VkImageTiling tiling, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    -> std::shared_ptr<DeviceMemory>
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

    if (auto result = vkCreateImage(vulkanDevice.handle(), &createInfo, nullptr, &image); result != VK_SUCCESS)
        throw std::runtime_error("failed to create image: "s + std::to_string(result));

    if (auto deviceMemory = vulkanDevice.memoryPool().AllocateMemory(image, properties); deviceMemory) {
        if (auto result = vkBindImageMemory(vulkanDevice.handle(), image, deviceMemory->handle(), deviceMemory->offset()); result != VK_SUCCESS)
            throw std::runtime_error("failed to bind image buffer memory: "s + std::to_string(result));

        return deviceMemory;
    }

    else throw std::runtime_error("failed to allocate image buffer memory"s);
}



[[nodiscard]] auto BufferPool::CreateUniformBuffer(VulkanDevice &device, VkBuffer &uboBuffer, std::size_t size)
    -> std::shared_ptr<DeviceMemory>
{
    auto constexpr usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    return CreateBuffer(device, uboBuffer, size, usageFlags, propertyFlags);
}


[[nodiscard]] std::optional<VkImage>
BufferPool::CreateImageHandle(VulkanDevice &vulkanDevice, std::uint32_t width, std::uint32_t height, std::uint32_t mipLevels,
                              VkFormat format, VkImageTiling tiling, VkBufferUsageFlags usage)
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

    VkImage handle;

    if (auto result = vkCreateImage(vulkanDevice.handle(), &createInfo, nullptr, &handle); result != VK_SUCCESS)
        throw std::runtime_error("failed to create image: "s + std::to_string(result));

#if USE_DEBUG_MARKERS
    VkDebugMarkerObjectNameInfoEXT const info{
        VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT,
        nullptr,
        VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
        (std::uint64_t)handle,//static_cast<std::uint64_t>(handle),
        "image"
    };

    auto vkDebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(vulkanDevice.handle(), "vkDebugMarkerSetObjectNameEXT");

    if (auto result = vkDebugMarkerSetObjectNameEXT(vulkanDevice.handle(), &info); result != VK_SUCCESS)
        throw std::runtime_error("failed to set the image debug marker: "s + std::to_string(result));
#endif

    return handle;
}
