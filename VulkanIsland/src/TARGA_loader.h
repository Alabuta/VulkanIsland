#pragma once

#include <array>
#include <vector>
#include <string_view>
#include <variant>


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

struct Image1 {
    VkImage handle;
    VkFormat format{VK_FORMAT_UNDEFINED};

    std::int16_t width{0}, height{0};
    std::uint32_t mipLevels;
};

struct TARGA {
    struct header_t {
        byte_t IDLength;
        byte_t colorMapType;
        byte_t imageType;
        std::array<byte_t, 5> colorMapSpec;
        std::array<byte_t, 10> imageSpec;
    } header;

    ePIXEL_LAYOUT pixelLayout{ePIXEL_LAYOUT::nINVALID};

    std::int16_t width{0}, height{0};
    std::uint8_t pixelDepth{0}, colorMapDepth{0};

    texel_buffer_t data;
};

[[nodiscard]] std::optional<Image> LoadTARGA(std::string_view name);