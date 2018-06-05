#pragma once

#include <optional>
#include <vector>
#include <set>
#include <map>

#include "main.h"
#include "device.h"
#include "command_buffer.h"

class BufferPool;

class DeviceMemoryPool final {
public:
    using memory_type_index_t = std::uint32_t;

    class DeviceMemory;

    DeviceMemoryPool(VulkanDevice *vulkanDevice) : vulkanDevice_{vulkanDevice} { }
    ~DeviceMemoryPool();

    template<class T, typename std::enable_if_t<std::is_same_v<T, VkBuffer> || std::is_same_v<T, VkImage>>...>
    [[nodiscard]] auto AllocateMemory(T buffer, VkMemoryPropertyFlags properties)
        -> std::optional<DeviceMemoryPool::DeviceMemory>
    {
        VkMemoryRequirements memoryReqirements;

        if constexpr (std::is_same_v<T, VkBuffer>)
            vkGetBufferMemoryRequirements(vulkanDevice_->handle(), buffer, &memoryReqirements);

        else vkGetImageMemoryRequirements(vulkanDevice_->handle(), buffer, &memoryReqirements);

        return AllocateMemory(memoryReqirements, properties);
    }

    void FreeMemory(std::optional<DeviceMemory> &&memory);

    class DeviceMemory {
    public:

        DeviceMemory(DeviceMemory &&) = default;
        DeviceMemory &operator= (DeviceMemory &&) = default;

        VkDeviceMemory handle() const noexcept { return handle_; }

        VkDeviceSize size() const noexcept { return size_; }
        VkDeviceSize offset() const noexcept { return offset_; }

    private:
        VkDeviceMemory handle_;
        memory_type_index_t memTypeIndex_;
        VkDeviceSize size_, offset_;

        DeviceMemory() = delete;
        DeviceMemory(DeviceMemory const &) = delete;

        DeviceMemory(VkDeviceMemory handle, memory_type_index_t memTypeIndex, VkDeviceSize size, VkDeviceSize offset) noexcept
            : handle_{handle}, memTypeIndex_{memTypeIndex}, size_{size}, offset_{offset} { }

        friend DeviceMemoryPool;
    };

private:

    friend BufferPool;

    static VkDeviceSize constexpr kBLOCK_ALLOCATION_SIZE{0x4'000'000};   // 64 MB

    VulkanDevice *vulkanDevice_;

    struct MemoryBlock {
        VkDeviceMemory handle;
        VkDeviceSize availableSize{0};

        memory_type_index_t memoryType;

        struct comparator {
            using is_transparent = void;

            template<class L, class R, typename std::enable_if_t<are_same_v<MemoryBlock, L, R>>...>
            auto operator() (L &&lhs, R &&rhs) const noexcept
            {
                return lhs.handle < rhs.handle;
            }

            template<class T, typename std::enable_if_t<std::is_same_v<MemoryBlock, std::decay_t<T>>>...>
            auto operator() (T &&block, memory_type_index_t memoryType) const noexcept
            {
                return block.memoryType < memoryType;
            }
        };

        struct MemoryChunk {
            VkDeviceSize offset{0}, size{0};

            struct comparator {
                using is_transparent = void;

                template<class L, class R, typename std::enable_if_t<are_same_v<MemoryChunk, L, R>>...>
                auto operator() (L &&lhs, R &&rhs) const noexcept
                {
                    return lhs.size < rhs.size;
                }

                template<class T, class S, typename std::enable_if_t<std::is_same_v<MemoryChunk, std::decay_t<T>> && std::is_integral_v<S>>...>
                auto operator() (T &&chunk, S size) const noexcept
                {
                    return chunk.size < size;
                }
            };

            MemoryChunk(VkDeviceSize offset, VkDeviceSize size) noexcept : offset{offset}, size{size} { }
        };

        std::set<MemoryChunk, MemoryChunk::comparator> availableChunks;

        MemoryBlock(VkDeviceMemory handle, VkDeviceSize availableSize, memory_type_index_t memoryType)
            : handle{handle}, availableSize{availableSize}, memoryType{memoryType}, availableChunks{{0, availableSize}} { }
    };

    std::set<MemoryBlock, MemoryBlock::comparator> memoryBlocks_;

    [[nodiscard]] std::optional<DeviceMemoryPool::DeviceMemory>
    AllocateMemory(VkMemoryRequirements const &memoryReqirements, VkMemoryPropertyFlags properties);

    auto AllocateMemoryBlock(memory_type_index_t memTypeIndex, VkDeviceSize size)
        -> decltype(memoryBlocks_)::iterator;

    [[nodiscard]] std::optional<DeviceMemoryPool::memory_type_index_t> FindMemoryType(memory_type_index_t filter, VkMemoryPropertyFlags propertyFlags);
};

class BufferPool {
public:

    
    [[nodiscard]] static auto CreateBuffer(VulkanDevice *vulkanDevice, VkBuffer &buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
        ->std::optional<DeviceMemoryPool::DeviceMemory>;

    
    [[nodiscard]] static auto CreateImage(VulkanDevice *vulkanDevice, VkImage &image,
                                       std::uint32_t width, std::uint32_t height, std::uint32_t mipLevels,
                                       VkFormat format, VkImageTiling tiling, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
        ->std::optional<DeviceMemoryPool::DeviceMemory>;

    
    [[nodiscard]] static auto CreateUniformBuffer(VulkanDevice *vulkanDevice, VkBuffer &uboBuffer, std::size_t size)
        -> std::optional<DeviceMemoryPool::DeviceMemory>;
};




template<class Q, class R, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>> && is_container_v<std::decay_t<R>>>...>
void CopyBufferToBuffer(VulkanDevice *vulkanDevice, Q &queue, VkBuffer srcBuffer, VkBuffer dstBuffer, R &&copyRegion, VkCommandPool commandPool)
{
    static_assert(std::is_same_v<typename std::decay_t<R>::value_type, VkBufferCopy>, "'copyRegion' argument does not contain 'VkBufferCopy' elements");

    auto commandBuffer = BeginSingleTimeCommand(vulkanDevice, queue, commandPool);

    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, static_cast<std::uint32_t>(std::size(copyRegion)), std::data(copyRegion));

    EndSingleTimeCommand(vulkanDevice, queue, commandBuffer, commandPool);
}

template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>...>
void CopyBufferToImage(VulkanDevice *vulkanDevice, Q &queue, VkBuffer srcBuffer, VkImage dstImage, std::uint32_t width, std::uint32_t height, VkCommandPool commandPool)
{
    auto commandBuffer = BeginSingleTimeCommand(vulkanDevice, queue, commandPool);

    VkBufferImageCopy const copyRegion{
        0,
        0, 0,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        { 0, 0, 0 },
        { width, height, 1 }
    };

    vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    EndSingleTimeCommand(vulkanDevice, queue, commandBuffer, commandPool);
}