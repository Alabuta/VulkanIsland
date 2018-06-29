#pragma once

#include <optional>
#include <vector>
#include <memory>
#include <set>
#include <map>
#include <unordered_map>

#include "main.h"
#include "device.h"
#include "command_buffer.h"

class DeviceMemory;

class MemoryManager final {
public:

    MemoryManager(VulkanDevice const &vulkanDevice, VkDeviceSize bufferImageGranularity);
    ~MemoryManager();

    template<class T, typename std::enable_if_t<is_one_of_v<T, VkBuffer, VkImage>>...>
    [[nodiscard]] std::shared_ptr<DeviceMemory> AllocateMemory(T buffer, VkMemoryPropertyFlags properties, bool linear = true)
    {
        return CheckRequirementsAndAllocate(buffer, properties, linear);
    }

private:
    static VkDeviceSize constexpr kBLOCK_ALLOCATION_SIZE{0x10'000'000};   // 256 MB

    VulkanDevice const &vulkanDevice_;
    VkDeviceSize totalAllocatedSize_{0}, bufferImageGranularity_{0};

    struct Pool final {
        std::uint32_t memoryTypeIndex{0};
        VkDeviceSize allocatedSize{0};

        struct Block final {
            VkDeviceSize availableSize{0};

            struct Chunk final {
                VkDeviceSize offset{0}, size{0};

                Chunk(VkDeviceSize offset, VkDeviceSize size) noexcept : offset{offset}, size{size} { }

                struct comparator final {
                    using is_transparent = void;

                    template<class L, class R>
                    std::enable_if_t<are_same_v<Chunk, L, R>, bool>
                    operator() (L &&lhs, R &&rhs) const noexcept
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
            };

            std::multiset<Chunk, Chunk::comparator> availableChunks;

            Block(VkDeviceSize availableSize) noexcept : availableSize{availableSize}, availableChunks{{0, availableSize}} { }
        };

        std::unordered_map<VkDeviceMemory, Block> blocks;

        Pool(std::uint32_t memoryTypeIndex) noexcept : memoryTypeIndex{memoryTypeIndex} { }
    };

    std::unordered_map<std::uint32_t, Pool> pools_;

    template<class T, typename std::enable_if_t<is_one_of_v<T, VkBuffer, VkImage>>...>
    [[nodiscard]] std::shared_ptr<DeviceMemory> CheckRequirementsAndAllocate(T buffer, VkMemoryPropertyFlags properties, bool linear);

    template<class R, typename std::enable_if_t<is_one_of_v<std::decay_t<R>, VkMemoryRequirements, VkMemoryRequirements2>>...>
    [[nodiscard]] std::shared_ptr<DeviceMemory> AllocateMemory(R &&memoryRequirements, VkMemoryPropertyFlags properties);

    auto AllocateMemoryBlock(std::uint32_t memoryTypeIndex, VkDeviceSize size) -> std::optional<decltype(Pool::blocks)::iterator>;

    void DeallocateMemory(DeviceMemory const &deviceMemory);
};

template<class T, typename std::enable_if_t<is_one_of_v<T, VkBuffer, VkImage>>...>
[[nodiscard]] std::shared_ptr<DeviceMemory>
MemoryManager::CheckRequirementsAndAllocate(T buffer, VkMemoryPropertyFlags properties, [[maybe_unused]] bool linear)
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

    if constexpr (std::is_same_v<T, VkBuffer>) {
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


class DeviceMemory final {
public:

    VkDeviceMemory handle() const noexcept { return handle_; }

    VkDeviceSize size() const noexcept { return size_; }
    VkDeviceSize offset() const noexcept { return offset_; }

    std::uint32_t typeIndex() const noexcept { return typeIndex_; }

private:
    VkDeviceMemory handle_;
    VkDeviceSize size_, offset_;

    std::uint32_t typeIndex_;

    DeviceMemory(VkDeviceMemory handle, std::uint32_t typeIndex, VkDeviceSize size, VkDeviceSize offset) noexcept
        : handle_{handle}, size_{size}, offset_{offset}, typeIndex_{typeIndex} { }

    DeviceMemory() = delete;

    DeviceMemory(DeviceMemory const &) = delete;
    DeviceMemory &operator= (DeviceMemory const &) = delete;

    DeviceMemory(DeviceMemory &&) = delete;
    DeviceMemory &operator= (DeviceMemory &&) = delete;

    friend MemoryManager;
};

class VulkanBuffer final {
public:

    VulkanBuffer(std::shared_ptr<DeviceMemory> memory, VkBuffer handle) : memory_{memory}, handle_{handle} { }

    std::shared_ptr<DeviceMemory> memory() const noexcept { return memory_; }
    std::shared_ptr<DeviceMemory> &memory() noexcept { return memory_; }

    VkBuffer handle() const noexcept { return handle_; }

private:
    std::shared_ptr<DeviceMemory> memory_;
    VkBuffer handle_;

    VulkanBuffer() = delete;
    VulkanBuffer(VulkanBuffer const &) = delete;
    VulkanBuffer(VulkanBuffer &&) = delete;
};


class BufferPool {
public:

    [[nodiscard]] static auto CreateBuffer(VulkanDevice &device, VkBuffer &buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
        ->std::shared_ptr<DeviceMemory>;

    [[nodiscard]] static auto CreateUniformBuffer(VulkanDevice &device, VkBuffer &uboBuffer, std::size_t size)
        ->std::shared_ptr<DeviceMemory>;
};



[[nodiscard]] std::optional<VkBuffer>
CreateBufferHandle(VulkanDevice const &device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) noexcept;


template<class Q, class R, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>> && is_container_v<std::decay_t<R>>>...>
void CopyBufferToBuffer(VulkanDevice const &device, Q &queue, VkBuffer srcBuffer, VkBuffer dstBuffer, R &&copyRegion, VkCommandPool commandPool)
{
    static_assert(std::is_same_v<typename std::decay_t<R>::value_type, VkBufferCopy>, "'copyRegion' argument does not contain 'VkBufferCopy' elements");

    auto commandBuffer = BeginSingleTimeCommand(device, queue, commandPool);

    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, static_cast<std::uint32_t>(std::size(copyRegion)), std::data(copyRegion));

    EndSingleTimeCommand(device, queue, commandBuffer, commandPool);
}

template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>...>
void CopyBufferToImage(VulkanDevice const &device, Q &queue, VkBuffer srcBuffer, VkImage dstImage, std::uint32_t width, std::uint32_t height, VkCommandPool commandPool)
{
    auto commandBuffer = BeginSingleTimeCommand(device, queue, commandPool);

    VkBufferImageCopy const copyRegion{
        0,
        0, 0,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        { 0, 0, 0 },
        { width, height, 1 }
    };

    vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    EndSingleTimeCommand(device, queue, commandBuffer, commandPool);
}