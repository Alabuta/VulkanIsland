#pragma once

#include <array>
#include <vector>
#include <variant>

#include <boost/cstdfloat.hpp>

#include "utility/mpl.hxx"
#include "utility/exceptions.hxx"
#include "graphics.hxx"


namespace vertex
{
    enum struct SEMANTIC : std::uint32_t {
        POSITION = 0,
        NORMAL,
        TEXCOORD_0,
        TEXCOORD_1,
        TANGENT,
        COLOR_0,
        JOINTS_0,
        WEIGHTS_0
    };
}

namespace graphics
{
    struct vertex_attribute final {
        vertex::SEMANTIC semantic;
        graphics::FORMAT format;

        template<class T> requires mpl::same_as<std::remove_cvref_t<T>, vertex_attribute>
        auto constexpr operator< (T &&rhs) const
        {
            return semantic < rhs.semantic;
        }

        template<class T> requires mpl::same_as<std::remove_cvref_t<T>, vertex_attribute>
        auto constexpr operator== (T &&rhs) const
        {
            return semantic == rhs.semantic && format == rhs.format;
        }
    };

    struct vertex_layout final {
        std::size_t size_in_bytes{0};

        std::vector<graphics::vertex_attribute> attributes;

        template<class T> requires mpl::same_as<std::remove_cvref_t<T>, vertex_layout>
        auto constexpr operator== (T &&rhs) const
        {
            return size_in_bytes == rhs.size_in_bytes && attributes == rhs.attributes;
        }
    };

    struct vertex_input_binding final {
        std::uint32_t binding_index;
        std::uint32_t stride_in_bytes;

        graphics::VERTEX_INPUT_RATE input_rate;

        template<class T> requires mpl::same_as<std::remove_cvref_t<T>, vertex_input_binding>
        auto constexpr operator== (T &&rhs) const
        {
            return binding_index == rhs.binding_index &&
                stride_in_bytes == rhs.stride_in_bytes &&
                input_rate == rhs.input_rate;
        }
    };

    struct vertex_input_attribute final {
        std::uint32_t location_index;
        std::uint32_t binding_index;
        std::uint32_t offset_in_bytes;

        graphics::FORMAT format;

        template<class T> requires mpl::same_as<std::remove_cvref_t<T>, vertex_input_attribute>
        auto constexpr operator== (T &&rhs) const
        {
            return location_index == rhs.location_index &&
                binding_index == rhs.binding_index &&
                offset_in_bytes == rhs.offset_in_bytes &&
                format == rhs.format;
        }
    };

    std::uint32_t get_vertex_attribute_semantic_index(graphics::vertex_attribute const &vertex_attribute);
}

namespace vertex
{
    std::size_t compile_vertex_attributes(graphics::vertex_layout &vertex_layout, vertex::SEMANTIC semantic, graphics::FORMAT format);

    template<class... Ts>
    std::size_t compile_vertex_attributes(graphics::vertex_layout &vertex_layout, vertex::SEMANTIC semantic, graphics::FORMAT format, Ts ...args)
    {
        auto size_in_bytes = compile_vertex_attributes(vertex_layout, semantic, format);

        return size_in_bytes + compile_vertex_attributes(vertex_layout, args...);
    }

    template<class... Ts>
    graphics::vertex_layout create_vertex_layout(Ts ...args)
    {
        graphics::vertex_layout vertex_layout;

        vertex_layout.size_in_bytes = compile_vertex_attributes(vertex_layout, args...);

        std::sort(std::begin(vertex_layout.attributes), std::end(vertex_layout.attributes));

        return vertex_layout;
    }
}

namespace graphics
{
    template<>
    struct hash<graphics::vertex_attribute> {
        std::size_t operator() (graphics::vertex_attribute const &attribute) const;
    };

    template<>
    struct hash<graphics::vertex_layout> {
        std::size_t operator() (graphics::vertex_layout const &layout) const;
    };

    template<>
    struct hash<graphics::vertex_input_binding> {
        std::size_t operator() (graphics::vertex_input_binding const &binding) const;
    };

    template<>
    struct hash<graphics::vertex_input_attribute> {
        std::size_t operator() (graphics::vertex_input_attribute const &input_attribute) const;
    };

    std::string to_string(vertex::SEMANTIC semantic);

    std::string to_string(graphics::FORMAT format);

    std::string to_string(graphics::vertex_layout const &layout);
}
