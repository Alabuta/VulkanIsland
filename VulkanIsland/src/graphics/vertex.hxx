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
    enum class eSEMANTIC_INDEX : std::uint32_t {
        POSITION = 0,
        NORMAL,
        TEXCOORD_0,
        TEXCOORD_1,
        TANGENT,
        COLOR_0,
        JOINTS_0,
        WEIGHTS_0,

        MAX
    };

    template<eSEMANTIC_INDEX SI>
    struct semantic {
        static eSEMANTIC_INDEX constexpr semantic_index{SI};
        static auto constexpr index{static_cast<std::uint32_t>(SI)};

        template<eSEMANTIC_INDEX si>
        auto constexpr operator< (semantic<si>) const noexcept { return SI < si; }

        template<eSEMANTIC_INDEX si>
        auto constexpr operator== (semantic<si>) const noexcept { return SI == si; }
    };
}

namespace vertex
{
    struct position final : semantic<eSEMANTIC_INDEX::POSITION> { };
    struct normal final : semantic<eSEMANTIC_INDEX::NORMAL> { };
    struct tex_coord_0 final : semantic<eSEMANTIC_INDEX::TEXCOORD_0> { };
    struct tex_coord_1 final : semantic<eSEMANTIC_INDEX::TEXCOORD_1> { };
    struct tangent final : semantic<eSEMANTIC_INDEX::TANGENT> { };
    struct color_0 final : semantic<eSEMANTIC_INDEX::COLOR_0> { };
    struct joints_0 final : semantic<eSEMANTIC_INDEX::JOINTS_0> { };
    struct weights_0 final : semantic<eSEMANTIC_INDEX::WEIGHTS_0> { };

    using attribute_semantic = std::variant<
        position,
        normal,
        tex_coord_0,
        tex_coord_1,
        tangent,
        color_0,
        joints_0,
        weights_0
    >;
}

namespace graphics
{
    struct vertex_attribute final {
        vertex::attribute_semantic semantic;
        graphics::FORMAT format;

        std::size_t offset_in_bytes{0};

        template<class T> requires mpl::same_as<std::remove_cvref_t<T>, vertex_attribute>
        auto constexpr operator< (T &&rhs) const
        {
            return semantic < rhs.semantic;
        }

        template<class T> requires mpl::same_as<std::remove_cvref_t<T>, vertex_attribute>
        auto constexpr operator== (T &&rhs) const
        {
            return offset_in_bytes == rhs.offset_in_bytes &&
                semantic == rhs.semantic &&
                format == rhs.format;
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
    template<class S, class T>
    void compile_vertex_attributes(std::vector<graphics::vertex_attribute> &attributes, S semantic, T format)
    {
        attributes.push_back(graphics::vertex_attribute{semantic, format, 0});
    }

    template<class S, class T, class... Ts>
    void compile_vertex_attributes(std::vector<graphics::vertex_attribute> &attributes, S semantic, T format, Ts ...args)
    {
        attributes.push_back(graphics::vertex_attribute{semantic, format, 0});

        compile_vertex_attributes(attributes, args...);
    }

    template<class... Ts>
    graphics::vertex_layout create_vertex_layout(Ts ...args)
    {
        graphics::vertex_layout vertex_layout{0, { }};

        auto &&vertex_attributes = vertex_layout.attributes;

        compile_vertex_attributes(vertex_attributes, args...);

        std::sort(std::begin(vertex_attributes), std::end(vertex_attributes));

        for (auto &&vertex_attribute : vertex_attributes) {
            vertex_attribute.offset_in_bytes += vertex_layout.size_in_bytes;

            if (auto format = graphics::instantiate_format(vertex_attribute.format); format) {
                vertex_layout.size_in_bytes += std::visit([] (auto &&format)
                {
                    return sizeof(std::remove_cvref_t<decltype(format)>);
                }, *format);
            }

            else throw graphics::exception("unsupported format");
        }

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
}
