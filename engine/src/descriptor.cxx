#include <span>
#include <string>
#include <string_view>

#include "descriptor.hxx"


#if 0
std::optional<VulkanDescriptorPool> DescriptorsManager::create_descriptor_pool()
{
    std::optional<VulkanDescriptorPool> descriptor_pool;

    std::array<VkDescriptorPoolSize, 2> constexpr pool_sizes{{
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    }};

    VkDescriptorPoolCreateInfo const create_info{
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        nullptr, 0,
        1,
        static_cast<std::uint32_t>(std::size(pool_sizes)), std::data(pool_sizes)
    };

    VkDescriptorPool handle;

    if (auto result = vkCreateDescriptorPool(device_.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
        std::cerr << "failed to create descriptor pool: "s  << result << '\n';

    else descriptor_pool.emplace(handle);

    return descriptor_pool;
}
#endif


std::optional<VkDescriptorPool> create_descriptor_pool(vulkan::device const &device)
{
    std::array<VkDescriptorPoolSize, 3> constexpr pool_sizes{{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1 },
//#if TEMPORARILY_DISABLED
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
//#endif
    }};

    VkDescriptorPoolCreateInfo const create_info{
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        nullptr, 0,
        3,
        static_cast<std::uint32_t>(std::size(pool_sizes)), std::data(pool_sizes)
    };

    VkDescriptorPool handle;

    if (auto result = vkCreateDescriptorPool(device.handle(), &create_info, nullptr, &handle); result == VK_SUCCESS)
        return handle;

    else throw vulkan::exception(fmt::format("failed to create descriptor pool: {0:#x}", result));
}

#if OBSOLETE
std::optional<VkDescriptorSetLayout> create_descriptor_set_layout(vulkan::device const &device)
{
    std::array<VkDescriptorSetLayoutBinding, 2> constexpr layout_bindings{{
        {
            0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        },
        {
            1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
            1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        }
#if TEMPORARILY_DISABLED
        {
            2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1, VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        },
#endif
    }};

    VkDescriptorSetLayoutCreateInfo const create_info{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        nullptr, 0,
        static_cast<std::uint32_t>(std::size(layout_bindings)), std::data(layout_bindings)
    };

    VkDescriptorSetLayout handle;

    if (auto result = vkCreateDescriptorSetLayout(device.handle(), &create_info, nullptr, &handle); result == VK_SUCCESS)
        return handle;

    else throw vulkan::exception(fmt::format("failed to create descriptor set layout: {0:#x}", result));
}
#endif

std::vector<VkDescriptorSet>
create_descriptor_sets(vulkan::device const &device, VkDescriptorPool descriptor_pool, std::span<VkDescriptorSetLayout const> const descriptor_set_layouts)
{
    VkDescriptorSetAllocateInfo const allocate_info{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        descriptor_pool,
        static_cast<std::uint32_t>(std::size(descriptor_set_layouts)), std::data(descriptor_set_layouts)
    };

    std::vector<VkDescriptorSet> handles(std::size(descriptor_set_layouts));

    if (auto result = vkAllocateDescriptorSets(device.handle(), &allocate_info, std::data(handles)); result == VK_SUCCESS)
        return handles;

    else throw vulkan::exception(fmt::format("failed to allocate descriptor set(s): {0:#x}", result));
}

std::optional<VkDescriptorSetLayout> create_descriptor_set_layout2(vulkan::device const &device, std::span<const VkDescriptorSetLayoutBinding> const layout_bindings)
{
    VkDescriptorSetLayoutCreateInfo const create_info{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        nullptr, 0,
        static_cast<std::uint32_t>(std::size(layout_bindings)), std::data(layout_bindings)
    };

    VkDescriptorSetLayout handle;

    if (auto result = vkCreateDescriptorSetLayout(device.handle(), &create_info, nullptr, &handle); result == VK_SUCCESS)
        return handle;

    else throw vulkan::exception(fmt::format("failed to create descriptor set layout: {0:#x}", result));
}

std::optional<VkDescriptorSetLayout> create_view_resources_descriptor_set_layout(vulkan::device const &device)
{
    std::array<VkDescriptorSetLayoutBinding, 2> constexpr layout_bindings{{
        {
            0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        },
        {
            1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        }
    }};

    return create_descriptor_set_layout2(device, layout_bindings);
}

std::optional<VkDescriptorSetLayout> create_object_resources_descriptor_set_layout(vulkan::device const &device)
{
    std::array<VkDescriptorSetLayoutBinding, 1> constexpr layout_bindings{
        {
            0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
            1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        }
    };

    return create_descriptor_set_layout2(device, layout_bindings);
}

std::optional<VkDescriptorSetLayout> create_image_resources_descriptor_set_layout(vulkan::device const &device)
{
    std::array<VkDescriptorSetLayoutBinding, 1> constexpr layout_bindings{{
        {
            0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1, VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        }
        /*{
            0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            1, VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        },
        {
            1, VK_DESCRIPTOR_TYPE_SAMPLER,
            1, VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        }*/
    }};

    return create_descriptor_set_layout2(device, layout_bindings);
}
