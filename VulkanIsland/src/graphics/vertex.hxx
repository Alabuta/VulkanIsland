#pragma once

#include <array>
#include <variant>

#include <boost/cstdfloat.hpp>

#include "utility/mpl.hxx"
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

        MAX_NUMBER
    };

    template<eSEMANTIC_INDEX SI>
    struct semantic {
        static auto constexpr index{static_cast<std::uint32_t>(SI)};

        template<eSEMANTIC_INDEX si>
        auto constexpr operator< (semantic<si>) const noexcept { return SI < si; }

        template<eSEMANTIC_INDEX si>
        auto constexpr operator== (semantic<si>) const noexcept { return SI == si; }
    };

    template<std::uint32_t N, class T>
    struct static_array final : public std::array<T, N> {
        using type = T;
        static auto constexpr length{N};

        template<std::uint32_t N2, class T2> requires mpl::same_as<T, T2>
        auto constexpr operator== (static_array<N2, T2>) const noexcept
        {
            return N == N2;
        }
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

    using attribute_type = std::variant<
        static_array<1, std::int8_t>,
        static_array<2, std::int8_t>,
        static_array<3, std::int8_t>,
        static_array<4, std::int8_t>,

        static_array<1, std::uint8_t>,
        static_array<2, std::uint8_t>,
        static_array<3, std::uint8_t>,
        static_array<4, std::uint8_t>,

        static_array<1, std::int16_t>,
        static_array<2, std::int16_t>,
        static_array<3, std::int16_t>,
        static_array<4, std::int16_t>,

        static_array<1, std::uint16_t>,
        static_array<2, std::uint16_t>,
        static_array<3, std::uint16_t>,
        static_array<4, std::uint16_t>,

        static_array<1, std::int32_t>,
        static_array<2, std::int32_t>,
        static_array<3, std::int32_t>,
        static_array<4, std::int32_t>,

        static_array<1, std::uint32_t>,
        static_array<2, std::uint32_t>,
        static_array<3, std::uint32_t>,
        static_array<4, std::uint32_t>,

    #if defined(BOOST_FLOAT16_C)
        static_array<1, boost::float16_t>,
        static_array<2, boost::float16_t>,
        static_array<3, boost::float16_t>,
        static_array<4, boost::float16_t>
    #endif

        static_array<1, boost::float32_t>,
        static_array<2, boost::float32_t>,
        static_array<3, boost::float32_t>,
        static_array<4, boost::float32_t>
    >;
}

namespace graphics
{
    struct vertex_attribute final {
        std::size_t offset_in_bytes{0};

        vertex::attribute_semantic semantic;
        vertex::attribute_type type;

        bool normalized;

        template<class T> requires mpl::same_as<std::remove_cvref_t<T>, vertex_attribute>
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

    graphics::FORMAT get_vertex_attribute_format(graphics::vertex_attribute const &vertex_attribute);
}

namespace vertex
{
    template<class S, class T, class N>
    void add_vertex_attributes(std::vector<graphics::vertex_attribute> &attributes, std::size_t offset_in_bytes, S semantic, T type, N normalized)
    {
        attributes.push_back(graphics::vertex_attribute{ offset_in_bytes, semantic, type, normalized });
    }

    template<class S, class T, class N, class... Ts>
    void add_vertex_attributes(std::vector<graphics::vertex_attribute> &attributes, std::size_t offset_in_bytes, S semantic, T type, N normalized, Ts... args)
    {
        attributes.push_back(graphics::vertex_attribute{ offset_in_bytes, semantic, type, normalized });

        add_vertex_attributes(attributes, offset_in_bytes + sizeof(type), args...);
    }

    template<class... Ts>
    graphics::vertex_layout create_vertex_layout(Ts... args)
    {
        graphics::vertex_layout vertex_layout;

        auto &&vertex_attributes = vertex_layout.attributes;

        add_vertex_attributes(vertex_attributes, 0, args...);

        vertex_layout.size_in_bytes = 0;

        for (auto &&vertex_attribute : vertex_attributes) {
            auto size_in_bytes = std::visit([] (auto &&type)
            {
                return sizeof(std::remove_cvref_t<decltype(type)>);

            }, vertex_attribute.type);

            vertex_layout.size_in_bytes += size_in_bytes;
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
