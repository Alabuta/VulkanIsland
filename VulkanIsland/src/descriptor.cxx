#include <string>
#include <string_view>
#include "descriptor.hxx"

#if 0
std::optional<VulkanDescriptorPool> DescriptorsManager::CreateDescriptorPool()
{
    std::optional<VulkanDescriptorPool> descriptorPool;

    std::array<VkDescriptorPoolSize, 2> constexpr poolSizes{{
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    }};

    VkDescriptorPoolCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        nullptr, 0,
        1,
        static_cast<std::uint32_t>(std::size(poolSizes)), std::data(poolSizes)
    };

    VkDescriptorPool handle;

    if (auto result = vkCreateDescriptorPool(device_.handle(), &createInfo, nullptr, &handle); result != VK_SUCCESS)
        std::cerr << "failed to create descriptor pool: "s  << result << '\n';

    else descriptorPool.emplace(handle);

    return descriptorPool;
}
#endif

struct DescriptorPoolDescription {

};

class DescriptorPoolManager {
public:

    DescriptorPoolManager(VulkanDevice &device, std::vector<VkDescriptorPoolSize> &&poolSizes)
        : device_{device}, poolSizes_{poolSizes} { };

private:

    [[maybe_unused]] VulkanDevice &device_;
    std::vector<VkDescriptorPoolSize> poolSizes_;
};

std::optional<VkDescriptorPool> CreateDescriptorPool(VulkanDevice const &device)
{
    std::optional<VkDescriptorPool> descriptorPool;

    std::array<VkDescriptorPoolSize, 2> constexpr poolSizes{{
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    }};

    VkDescriptorPoolCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        nullptr, 0,
        1,
        static_cast<std::uint32_t>(std::size(poolSizes)), std::data(poolSizes)
    };

    VkDescriptorPool handle;

    if (auto result = vkCreateDescriptorPool(device.handle(), &createInfo, nullptr, &handle); result != VK_SUCCESS)
        std::cerr << "failed to create descriptor pool: "s << result << '\n';

    else descriptorPool.emplace(handle);

    return descriptorPool;
}



std::optional<VkDescriptorSetLayout> CreateDescriptorSetLayout(VulkanDevice const &device)
{
    std::optional<VkDescriptorSetLayout> descriptorSetLayout;

    std::array<VkDescriptorSetLayoutBinding, 2> constexpr layoutBindings{{
        {
            0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        },
        {
            1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1, VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        }
    }};

    VkDescriptorSetLayoutCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        nullptr, 0,
        static_cast<std::uint32_t>(std::size(layoutBindings)), std::data(layoutBindings)
    };

    VkDescriptorSetLayout handle;

    if (auto result = vkCreateDescriptorSetLayout(device.handle(), &createInfo, nullptr, &handle); result != VK_SUCCESS)
        std::cerr << "failed to create descriptor set layout: "s << result << '\n';

    else descriptorSetLayout.emplace(handle);

    return descriptorSetLayout;
}
