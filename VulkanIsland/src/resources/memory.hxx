#pragma once

#include <optional>
#include <vector>
#include <memory>
#include <set>
#include <map>
#include <unordered_map>

#include <boost/functional/hash.hpp>

#include "main.hxx"
#include "device/device.hxx"


class DeviceMemory;

class MemoryManager final {
public:

    MemoryManager(VulkanDevice const &vulkanDevice, VkDeviceSize bufferImageGranularity);
    ~MemoryManager();

    template<class T, typename std::enable_if_t<is_one_of_v<T, VkBuffer, VkImage>>* = nullptr>
    [[nodiscard]] std::shared_ptr<DeviceMemory> AllocateMemory(T buffer, VkMemoryPropertyFlags properties, bool linear)
    {
        return CheckRequirementsAndAllocate(buffer, properties, linear);
    }

private:
    static VkDeviceSize constexpr kBLOCK_ALLOCATION_SIZE{0x400'0000};   // 64 MB

    VulkanDevice const &vulkanDevice_;
    VkDeviceSize totalAllocatedSize_{0}, bufferImageGranularity_{0};

    struct Pool final {
        std::uint32_t memoryTypeIndex{0};
        VkDeviceSize allocatedSize{0};
        VkMemoryPropertyFlags properties{0};

        struct hash_value final {
            template<class T, typename std::enable_if_t<std::is_same_v<Pool, std::decay_t<T>>>* = nullptr>
            constexpr std::size_t operator() (T &&pool) const noexcept
            {
                std::size_t seed = 0;

                boost::hash_combine(seed, pool.memoryTypeIndex);
                boost::hash_combine(seed, pool.properties);

                return seed;
            }
        };

        template<class T, typename std::enable_if_t<std::is_same_v<Pool, std::decay_t<T>>>* = nullptr>
        constexpr bool operator== (T &&rhs) const noexcept
        {
            return memoryTypeIndex == rhs.memoryTypeIndex && properties == rhs.properties;
        }

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

                    template<class T, class S, typename std::enable_if_t<std::is_same_v<Chunk, std::decay_t<T>> && std::is_integral_v<S>>* = nullptr>
                    auto operator() (T &&chunk, S size) const noexcept
                    {
                        return chunk.size < size;
                    }

                    template<class S, class T, typename std::enable_if_t<std::is_same_v<Chunk, std::decay_t<T>> && std::is_integral_v<S>>* = nullptr>
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

        Pool(std::uint32_t memoryTypeIndex, VkMemoryPropertyFlags properties) noexcept : memoryTypeIndex{memoryTypeIndex}, properties{properties} { }
    };

    std::unordered_map<std::size_t, Pool> pools_;

    template<class T, typename std::enable_if_t<is_one_of_v<T, VkBuffer, VkImage>>...>
    [[nodiscard]] std::shared_ptr<DeviceMemory>
    CheckRequirementsAndAllocate(T resource, VkMemoryPropertyFlags properties, bool linear);

    [[nodiscard]] std::shared_ptr<DeviceMemory>
    AllocateMemory(VkMemoryRequirements const &memoryRequirements, VkMemoryPropertyFlags properties, bool linear);

    auto AllocateMemoryBlock(std::uint32_t memoryTypeIndex, VkDeviceSize size, VkMemoryPropertyFlags properties) -> std::optional<decltype(Pool::blocks)::iterator>;

    void DeallocateMemory(DeviceMemory const &deviceMemory);
};

template<class T, typename std::enable_if_t<is_one_of_v<T, VkBuffer, VkImage>>...>
[[nodiscard]] std::shared_ptr<DeviceMemory>
MemoryManager::CheckRequirementsAndAllocate(T resource, VkMemoryPropertyFlags properties, bool linear)
{
#if TEMPORALY_DISABLED
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
#endif

    VkMemoryRequirements memoryRequirements;

    if constexpr (std::is_same_v<T, VkBuffer>)
        vkGetBufferMemoryRequirements(vulkanDevice_.handle(), resource, &memoryRequirements);

    else vkGetImageMemoryRequirements(vulkanDevice_.handle(), resource, &memoryRequirements);

    return AllocateMemory(memoryRequirements, properties, linear);
}


class DeviceMemory final {
public:

    VkDeviceMemory handle() const noexcept { return handle_; }

    VkDeviceSize size() const noexcept { return size_; }
    VkDeviceSize offset() const noexcept { return offset_; }

    std::uint32_t typeIndex() const noexcept { return typeIndex_; }
    VkMemoryPropertyFlags properties() const noexcept { return properties_; }

    std::size_t seed() const noexcept { return seed_; }

private:
    VkDeviceMemory handle_;
    VkDeviceSize size_, offset_;

    std::uint32_t typeIndex_;
    VkMemoryPropertyFlags properties_;

    std::size_t seed_{0};

    bool linear_;

    DeviceMemory(VkDeviceMemory handle, std::uint32_t typeIndex, VkMemoryPropertyFlags properties,
                 VkDeviceSize size, VkDeviceSize offset, bool linear) noexcept
        : handle_{handle}, size_{size}, offset_{offset}, typeIndex_{typeIndex}, properties_{properties}, linear_{linear}
    {
        boost::hash_combine(seed_, typeIndex_);
        boost::hash_combine(seed_, properties_); 
    }

    DeviceMemory() = delete;

    DeviceMemory(DeviceMemory const &) = delete;
    DeviceMemory &operator= (DeviceMemory const &) = delete;

    DeviceMemory(DeviceMemory &&) = delete;
    DeviceMemory &operator= (DeviceMemory &&) = delete;

    friend MemoryManager;
};
