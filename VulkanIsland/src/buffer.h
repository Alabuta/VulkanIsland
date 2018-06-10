#pragma once

#include <optional>
#include <vector>
#include <memory>
#include <set>
#include <map>

#include "main.h"
#include "device.h"
#include "command_buffer.h"

class BufferPool;

class DeviceMemory;

class MemoryPool final {
public:

    MemoryPool(VulkanDevice &vulkanDevice);
    ~MemoryPool();

    template<class T, typename std::enable_if_t<std::is_same_v<T, VkBuffer> || std::is_same_v<T, VkImage>, int> = 0>
    [[nodiscard]] std::shared_ptr<DeviceMemory> AllocateMemory(T buffer, VkMemoryPropertyFlags properties)
    {
        return CheckRequirementsAndAllocate(buffer, properties);
    }

private:

    static VkDeviceSize constexpr kBLOCK_ALLOCATION_SIZE{0x10'000'000};   // 256 MB

    VulkanDevice &vulkanDevice_;
    VkDeviceSize allocatedSize_{0};

    struct Block final {
        VkDeviceMemory handle;
        VkDeviceSize availableSize{0};

        std::uint32_t memoryTypeIndex;

        struct Chunk final {
            VkDeviceSize offset{0}, size{0};

            struct comparator final {
                using is_transparent = void;

                template<class L, class R, typename std::enable_if_t<are_same_v<Chunk, L, R>>...>
                auto operator() (L &&lhs, R &&rhs) const noexcept
                {
                    return lhs.size < rhs.size;
                }

                template<class T, class S, typename std::enable_if_t<std::is_same_v<Chunk, std::decay_t<T>> && std::is_integral_v<S>>...>
                auto operator() (T &&chunk, S size) const noexcept
                {
                    return chunk.size < size;
                }

                template<class S, class T, typename std::enable_if_t<std::is_same_v<Chunk, std::decay_t<T>> && std::is_integral_v<S>>...>
                auto operator() (S size, T &&chunk) const noexcept
                {
                    return chunk.size < size;
                }
            };

            Chunk(VkDeviceSize offset, VkDeviceSize size) noexcept : offset{offset}, size{size} { }
        };

        std::multiset<Chunk, Chunk::comparator> availableChunks;

        Block(VkDeviceMemory handle, VkDeviceSize availableSize, std::uint32_t memoryTypeIndex)
            : handle{handle}, availableSize{availableSize}, memoryTypeIndex{memoryTypeIndex}, availableChunks{{0, availableSize}} { }
    };

    std::multimap<std::uint32_t, Block> memoryBlocks_;

    template<class R, typename std::enable_if_t<std::is_same_v<std::decay_t<R>, VkMemoryRequirements> || std::is_same_v<std::decay_t<R>, VkMemoryRequirements2>>...>
    [[nodiscard]] auto AllocateMemory(R &&memoryRequirements, VkMemoryPropertyFlags properties) -> std::shared_ptr<DeviceMemory>;

    template<class T, typename std::enable_if_t<std::is_same_v<T, VkBuffer> || std::is_same_v<T, VkImage>>...>
    [[nodiscard]] auto CheckRequirementsAndAllocate(T buffer, VkMemoryPropertyFlags properties) -> std::shared_ptr<DeviceMemory>;

    auto AllocateMemoryBlock(std::uint32_t memTypeIndex, VkDeviceSize size) -> decltype(memoryBlocks_)::iterator;

    void DeallocateMemory(DeviceMemory &&deviceMemory);

    friend BufferPool;
};


class DeviceMemory final {
public:

    DeviceMemory(DeviceMemory &&) = default;
    DeviceMemory &operator= (DeviceMemory &&) = default;

    VkDeviceMemory handle() const noexcept { return handle_; }

    VkDeviceSize size() const noexcept { return size_; }
    VkDeviceSize offset() const noexcept { return offset_; }

    std::uint32_t typeIndex() const noexcept { return typeIndex_; }

private:
    VkDeviceMemory handle_;
    VkDeviceSize size_, offset_;

    std::uint32_t typeIndex_;

    DeviceMemory(VkDeviceMemory handle, std::uint32_t typeIndex, VkDeviceSize size, VkDeviceSize offset) noexcept
        : handle_{handle}, typeIndex_{typeIndex}, size_{size}, offset_{offset} { }

    DeviceMemory() = delete;
    DeviceMemory(DeviceMemory const &) = delete;
    DeviceMemory &operator= (DeviceMemory const &) = delete;

    friend MemoryPool;
};

class BufferPool {
public:
    
    [[nodiscard]] static auto CreateBuffer(VulkanDevice *vulkanDevice, VkBuffer &buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
        ->std::shared_ptr<DeviceMemory>;

    [[nodiscard]] static auto CreateImage(VulkanDevice *vulkanDevice, VkImage &image,
                                       std::uint32_t width, std::uint32_t height, std::uint32_t mipLevels,
                                       VkFormat format, VkImageTiling tiling, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
        ->std::shared_ptr<DeviceMemory>;

    [[nodiscard]] static auto CreateUniformBuffer(VulkanDevice *vulkanDevice, VkBuffer &uboBuffer, std::size_t size)
        -> std::shared_ptr<DeviceMemory>;
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