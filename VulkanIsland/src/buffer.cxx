#include <unordered_map>

#include "buffer.h"

class DeviceMemoryPool final {
public:


    template<class T, typename std::enable_if_t<std::is_same_v<T, VkBuffer> || std::is_same_v<T, VkImage>>...>
    [[nodiscard]] static std::optional<VkDeviceMemory> AllocateMemory(VulkanDevice *vulkanDevice, T buffer, VkMemoryPropertyFlags properties)
    {
        VkMemoryRequirements memoryReqirements;

        if constexpr (std::is_same_v<T, VkBuffer>)
            vkGetBufferMemoryRequirements(vulkanDevice->handle(), buffer, &memoryReqirements);

        else vkGetImageMemoryRequirements(vulkanDevice->handle(), buffer, &memoryReqirements);

        std::uint32_t memTypeIndex = 0;

        if (auto index = FindMemoryType(vulkanDevice, memoryReqirements.memoryTypeBits, properties); !index)
            throw std::runtime_error("failed to find suitable memory type"s);

        else memTypeIndex = index.value();

        VkMemoryAllocateInfo const memAllocInfo{
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            nullptr,
            memoryReqirements.size,
            memTypeIndex
        };

        VkDeviceMemory deviceMemory;

        if (auto result = vkAllocateMemory(vulkanDevice->handle(), &memAllocInfo, nullptr, &deviceMemory); result != VK_SUCCESS)
            return { };

        return deviceMemory;
    }

private:

    std::unordered_map<std::uint32_t, VkDeviceMemory> pool_;

    [[nodiscard]] static std::optional<std::uint32_t> FindMemoryType(VulkanDevice *vulkanDevice, std::uint32_t filter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(vulkanDevice->physical_handle(), &memoryProperties);

        auto const memoryTypes = to_array(memoryProperties.memoryTypes);

        auto it_type = std::find_if(std::cbegin(memoryTypes), std::cend(memoryTypes), [filter, properties, i = 0u] (auto type) mutable
        {
            return (filter & (1 << i++)) && (type.propertyFlags & properties) == properties;
        });

        if (it_type < std::next(std::cbegin(memoryTypes), memoryProperties.memoryTypeCount))
            return static_cast<std::uint32_t>(std::distance(std::cbegin(memoryTypes), it_type));

        return { };
    }
};


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

    if (auto result = DeviceMemoryPool::AllocateMemory(vulkanDevice, buffer, properties); !result)
        throw std::runtime_error("failed to allocate buffer memory"s);

    else deviceMemory = result.value();

    if (auto result = vkBindBufferMemory(vulkanDevice->handle(), buffer, deviceMemory, 0); result != VK_SUCCESS)
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

    if (auto result = DeviceMemoryPool::AllocateMemory(vulkanDevice, image, properties); !result)
        throw std::runtime_error("failed to allocate image buffer memory"s);

    else deviceMemory = result.value();

    if (auto result = vkBindImageMemory(vulkanDevice->handle(), image, deviceMemory, 0); result != VK_SUCCESS)
        throw std::runtime_error("failed to bind image buffer memory: "s + std::to_string(result));
}



void CreateUniformBuffer(VulkanDevice *vulkanDevice, VkBuffer &uboBuffer, VkDeviceMemory &uboBufferMemory, std::size_t size)
{
    CreateBuffer(vulkanDevice, uboBuffer, uboBufferMemory, size,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}
