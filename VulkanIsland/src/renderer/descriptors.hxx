#pragma once

#include <vector>

#include "graphics.hxx"
#include "utility/mpl.hxx"


namespace graphics
{
    struct descriptor_set_binding final {
        // TODO:: immutable samplers support.

        std::uint32_t binding_index;
        std::uint32_t descriptor_count;

        graphics::DESCRIPTOR_TYPE descriptor_type;
        graphics::SHADER_STAGE shader_stages;

        template<class T> requires std::same_as<std::remove_cvref_t<T>, descriptor_set_binding>
        auto constexpr operator== (T &&rhs) const
        {
            return binding_index == rhs.binding_index &&
                descriptor_count == rhs.descriptor_count &&
                descriptor_type == rhs.descriptor_type &&
                shader_stages == rhs.shader_stages;
        }
    };

    struct descriptor_set_layout final {
        // TODO:: create flags support.

        std::vector<graphics::descriptor_set_binding> descriptor_set_bindings;
    };
}

namespace graphics
{
    template<>
    struct hash<graphics::descriptor_set_binding> {
        std::size_t operator() (graphics::descriptor_set_binding const &binding) const;
    };
}