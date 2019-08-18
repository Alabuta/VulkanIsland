#pragma once

#include <vector>

#include "graphics.hxx"
#include "pipelines.hxx"


namespace graphics
{
    enum class DESCRIPTOR_TYPE {
        SAMPLER = 0,
        COMBINED_IMAGE_SAMPLER,
        SAMPLED_IMAGE,
        STORAGE_IMAGE,
        UNIFORM_TEXEL_BUFFER,
        STORAGE_TEXEL_BUFFER,
        UNIFORM_BUFFER,
        STORAGE_BUFFER,
        UNIFORM_BUFFER_DYNAMIC,
        STORAGE_BUFFER_DYNAMIC,
        INPUT_ATTACHMENT
    };

    struct descriptor_set_binding final {
        std::uint32_t binding_index;
        std::uint32_t descriptor_count;

        graphics::DESCRIPTOR_TYPE descriptor_type;
        graphics::PIPELINE_SHADER_STAGE shader_stages;

        // TODO:: immutable samplers support.

        template<class T, typename std::enable_if_t<std::is_same_v<descriptor_set_binding, std::decay_t<T>>>* = nullptr>
        auto constexpr operator== (T &&rhs) const
        {
            return binding_index == rhs.binding_index &&
                descriptor_count == rhs.descriptor_count &&
                descriptor_type == rhs.descriptor_type &&
                shader_stages == rhs.shader_stages;
        }
    };

    struct descriptor_set_layout final {
        // TODO:: additional create flags support.

        std::vector<graphics::descriptor_set_binding> descriptor_set_bindings;
    };
}

namespace graphics
{
    template<>
    struct hash<graphics::descriptor_set_binding> {
        std::size_t operator() (graphics::descriptor_set_binding const &binding) const
        {
            std::size_t seed = 0;

            boost::hash_combine(seed, binding.binding_index);
            boost::hash_combine(seed, binding.descriptor_count);
            boost::hash_combine(seed, binding.descriptor_type);
            boost::hash_combine(seed, binding.shader_stages);

            return seed;
        }
    };
}
