#include <string>
using namespace std::string_literals;

#include <fmt/format.h>
#include <boost/functional/hash.hpp>

#include "utility/exceptions.hxx"
#include "descriptors.hxx"
#include "graphics_api.hxx"


namespace graphics
{
    std::size_t hash<graphics::descriptor_set_binding>::operator() (graphics::descriptor_set_binding const &binding) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, binding.binding_index);
        boost::hash_combine(seed, binding.descriptor_count);
        boost::hash_combine(seed, binding.descriptor_type);
        boost::hash_combine(seed, binding.shader_stages);

        return seed;
    }

    std::size_t hash<graphics::descriptor_set_layout>::operator() (graphics::descriptor_set_layout const &layout) const
    {
        std::size_t seed = 0;

        hash<graphics::descriptor_set_binding> constexpr hasher;

        for (auto &&binding : layout.descriptor_set_bindings())
            boost::hash_combine(seed, hasher(binding));

        return seed;
    }
}

namespace graphics
{
    descriptor_registry::descriptor_registry(vulkan::device &device) : device_{device}
    {
        std::array<VkDescriptorPoolSize, 3> constexpr descriptor_pool{{
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
        }};

        VkDescriptorPoolCreateInfo const create_info{
            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            nullptr, 0,
            kDESCRIPTOR_SET_MAXIMUM_NUMBER,
            static_cast<std::uint32_t>(std::size(descriptor_pool)), std::data(descriptor_pool)
        };

        if (auto result = vkCreateDescriptorPool(device_.handle(), &create_info, nullptr, &descriptor_pool_); result != VK_SUCCESS)
            throw vulkan::exception(fmt::format("failed to create the descriptor pool: {0:#x}"s, result));
    }

    descriptor_registry::~descriptor_registry()
    {
        vkDestroyDescriptorPool(device_.handle(), descriptor_pool_, nullptr);
    }

    std::shared_ptr<graphics::descriptor_set_layout>
    descriptor_registry::create_descriptor_set_layout(std::vector<graphics::descriptor_set_binding> const &descriptor_set_bindings)
    {
        std::vector<VkDescriptorSetLayoutBinding> layout_bindigns;

        for (auto &&binding : descriptor_set_bindings) {
            layout_bindigns.push_back(VkDescriptorSetLayoutBinding{
                binding.binding_index,
                convert_to::vulkan(binding.descriptor_type),
                binding.descriptor_count,
                static_cast<VkShaderStageFlags>(convert_to::vulkan(binding.shader_stages)),
                nullptr
             });
        }

        VkDescriptorSetLayoutCreateInfo const create_info{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            nullptr, 0,
            static_cast<std::uint32_t>(std::size(layout_bindigns)), std::data(layout_bindigns)
        };

        VkDescriptorSetLayout handle;

        if (auto result = vkCreateDescriptorSetLayout(device_.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
            throw vulkan::exception(fmt::format("failed to create descriptor set layout: {0:#x}"s, result));

        std::shared_ptr<graphics::descriptor_set_layout> descriptor_set_layout;

        descriptor_set_layout.reset(new graphics::descriptor_set_layout{handle}, [this] (graphics::descriptor_set_layout *ptr_descriptor_set_layout)
        {
            vkDestroyDescriptorSetLayout(device_.handle(), ptr_descriptor_set_layout->handle(), nullptr);

            delete ptr_descriptor_set_layout;
        });

        return descriptor_set_layout;
    }
}

