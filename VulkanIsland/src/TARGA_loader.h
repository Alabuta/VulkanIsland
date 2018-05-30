#pragma once

#include <array>
#include <vector>
#include <string_view>
#include <variant>

struct alignas(sizeof(std::uint32_t)) RGBA
{
    union {
        std::array<std::byte, 4> channels;
        std::uint32_t value;
    };
};

enum class ePIXEL_LAYOUT {
    nINVALID = 0, nRED, nRG, nRGB, nBGR, nRGBA, nBGRA
};

using byte_t = std::uint8_t;

template<std::size_t N, class T>
struct vec {
    static auto constexpr size = N;
    using value_type = T;

    std::array<T, N> array;

    vec() = default;

    template<class... Ts, typename = std::enable_if_t<std::conjunction_v<std::is_arithmetic<Ts>...> && sizeof...(Ts) == size>>
    constexpr vec(Ts... values) : array{static_cast<typename std::decay_t<decltype(array)>::value_type>(values)...} { }
};

using texel_t = std::variant<
    vec<1, std::uint8_t>,
    vec<2, std::uint8_t>,
    vec<3, std::uint8_t>,
    vec<4, std::uint8_t>
>;

using texel_buffer_t = wrap_variant_by_vector<texel_t>::type;

struct Image {
    ePIXEL_LAYOUT pixelLayout = ePIXEL_LAYOUT::nINVALID;
    std::int16_t width = 0, height = 0;
    std::uint8_t pixelDepth = 0;

    texel_buffer_t data;
};

[[nodiscard]] std::optional<Image> LoadTARGA(std::string_view name);