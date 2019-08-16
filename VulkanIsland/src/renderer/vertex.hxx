#pragma once

#include <array>
#include <variant>

#include <boost/cstdfloat.hpp>


namespace
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

        template<std::uint32_t N2, class T2>
        auto constexpr operator== (static_array<N2, T2>) const noexcept
        {
            return N == N2 && std::is_same_v<T, T2>;
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