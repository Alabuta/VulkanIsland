#pragma once

#include <vector>

#include "vertex.hxx"


namespace graphics
{
    struct vertex_attribute final {
        std::size_t offset_in_bytes{0};

        vertex::attribute_semantic semantic;
        vertex::attribute_type type;

        bool normalized;

        template<class T, typename std::enable_if_t<std::is_same_v<vertex_attribute, std::decay_t<T>>>* = nullptr>
        auto constexpr operator== (T &&rhs) const
        {
            return offset_in_bytes == rhs.offset_in_bytes &&
                normalized == rhs.normalized &&
                semantic == rhs.semantic &&
                type == rhs.type;
        }
    };

    struct vertex_layout final {
        std::size_t size_in_bytes{0};

        std::vector<graphics::vertex_attribute> attributes;

        template<class T, typename std::enable_if_t<std::is_same_v<vertex_layout, std::decay_t<T>>>* = nullptr>
        auto constexpr operator== (T &&rhs) const
        {
            return size_in_bytes == rhs.size_in_bytes && attributes == rhs.attributes;
        }
    };
}
