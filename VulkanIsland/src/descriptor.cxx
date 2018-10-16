#include "descriptor.hxx"

std::shared_ptr<VulkanDescriptorPool> DescriptorsManager::CreateDescriptorPool()
{
    std::shared_ptr<VulkanDescriptorPool> descriptorPool;

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

    else descriptorPool = std::make_shared<VulkanDescriptorPool>(handle);

    return descriptorPool;
}