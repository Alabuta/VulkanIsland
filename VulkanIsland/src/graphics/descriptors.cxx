#include <string>
using namespace std::string_literals;

#include <fmt/format.h>
#include <boost/functional/hash.hpp>

#include "descriptors.hxx"


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

        for (auto &&binding : layout.descriptor_set_bindings)
            boost::hash_combine(seed, hasher(binding));

        return seed;
    }
}

namespace graphics
{
    descriptor_registry::descriptor_registry(vulkan::device &device) noexcept : device_{device}
    {
        std::array<VkDescriptorPoolSize, 3> constexpr descriptor_pool{{
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
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
            throw std::runtime_error(fmt::format("failed to create the descriptor pool: {0:#x}\n"s, result));
    }

    std::shared_ptr<descriptor_set_layout>
    descriptor_registry::create_descriptor_set_layout(std::vector<graphics::descriptor_set_binding> const &descriptor_set_bindings)
    {

        return std::shared_ptr<descriptor_set_layout>();
    }
}

