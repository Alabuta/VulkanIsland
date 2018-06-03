#pragma once

#include <vector>
#include <set>
#include <map>

#include "main.h"
#include "device.h"
#include "command_buffer.h"

class DeviceMemoryPool final {
public:
    using memory_type_index_t = std::uint32_t;

    DeviceMemoryPool(VulkanDevice *vulkanDevice) : vulkanDevice_{vulkanDevice} { }
    ~DeviceMemoryPool();

    class DeviceMemory {
    public:

        DeviceMemory(VkDeviceMemory handle, memory_type_index_t memTypeIndex, VkDeviceSize size, VkDeviceSize offset) noexcept
            : handle_{handle}, memTypeIndex_{memTypeIndex}, size_{size}, offset_{offset} { }

        DeviceMemory(DeviceMemory &&) = default;

        VkDeviceMemory handle() const noexcept { return handle_; }

        VkDeviceSize size() const noexcept { return size_; }
        VkDeviceSize offset() const noexcept { return offset_; }

    private:
        VkDeviceMemory handle_;
        memory_type_index_t memTypeIndex_;
        VkDeviceSize size_, offset_;

        DeviceMemory(DeviceMemory const &) = delete;
    };

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

private:

    static VkDeviceSize constexpr kBLOCK_ALLOCATION_SIZE{0x4'000'000};   // 64 MB

    VulkanDevice *vulkanDevice_;

    struct MemoryBlock {
        VkDeviceMemory handle;
        VkDeviceSize availableSize{0};

        struct MemoryChunk {
            VkDeviceSize offset{0}, size{0};

            MemoryChunk(VkDeviceSize offset, VkDeviceSize size) noexcept : offset{offset}, size{size} { }

            struct comparator {
                using is_transparent = void;

                template<class L, class R, typename std::enable_if_t<are_same_v<MemoryChunk, L, R>>...>
                auto operator() (L &&lhs, R &&rhs) const noexcept
                {
                    return lhs.size < rhs.size;
                }

                template<class T, class S, typename std::enable_if_t<std::is_same_v<MemoryChunk, std::decay_t<T>> && std::is_integral_v<S>>...>
                auto operator() (T &&lhs, S size) const noexcept
                {
                    return lhs.size < size;
                }
            };
        };

        std::set<MemoryChunk, MemoryChunk::comparator> availableChunks;

        MemoryBlock(VkDeviceMemory handle, VkDeviceSize availableSize) : handle{handle}, availableSize{availableSize}, availableChunks{{0, availableSize}} { }
    };

    std::multimap<memory_type_index_t, MemoryBlock> memoryBlocks_;

    [[nodiscard]] std::optional<DeviceMemoryPool::DeviceMemory>
    AllocateMemory(VkMemoryRequirements const &memoryReqirements, VkMemoryPropertyFlags properties);

    auto AllocateMemoryBlock(memory_type_index_t memTypeIndex, VkDeviceSize size)
        -> decltype(memoryBlocks_)::iterator;

    [[nodiscard]] std::optional<DeviceMemoryPool::memory_type_index_t> FindMemoryType(memory_type_index_t filter, VkMemoryPropertyFlags propertyFlags);
};


/*[[nodiscard]]*/ auto CreateBuffer(VulkanDevice *vulkanDevice, VkBuffer &buffer, VkDeviceMemory &deviceMemory,
                                VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    ->std::optional<DeviceMemoryPool::DeviceMemory>;

/*[[nodiscard]]*/ auto CreateImage(VulkanDevice *vulkanDevice, VkImage &image, VkDeviceMemory &deviceMemory,
                               std::uint32_t width, std::uint32_t height, std::uint32_t mipLevels,
                               VkFormat format, VkImageTiling tiling, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    -> std::optional<DeviceMemoryPool::DeviceMemory>;


std::optional<DeviceMemoryPool::DeviceMemory>
CreateUniformBuffer(VulkanDevice *vulkanDevice, VkBuffer &uboBuffer, VkDeviceMemory &uboBufferMemory, std::size_t size);



template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>...>
void CopyBufferToBuffer(VulkanDevice *vulkanDevice, Q &queue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool commandPool)
{
    auto commandBuffer = BeginSingleTimeCommand(vulkanDevice, queue, commandPool);

    VkBufferCopy const copyRegion{ 0, 0, size };

    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

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