#pragma once

#include <array>
#include <vector>
#include <string_view>
#include <variant>

#include "math.hxx"

enum class ePIXEL_LAYOUT {
    nUNDEFINED = 0, nRED, nRG, nRGB, nBGR, nRGBA, nBGRA
};

using byte_t = std::uint8_t;

using texel_t = std::variant<
    vec<1, std::uint8_t>,
    vec<2, std::uint8_t>,
    vec<3, std::uint8_t>,
    vec<4, std::uint8_t>
>;

using texel_buffer_t = wrap_variant_by_vector<texel_t>::type;

struct RawImage {
    VkFormat format{VK_FORMAT_UNDEFINED};
    VkImageViewType type;

    std::int16_t width{0}, height{0};
    std::uint32_t mipLevels{1};

    texel_buffer_t data;
};

struct TARGA {
    struct header_t {
        byte_t IDLength;
        byte_t colorMapType;
        byte_t imageType;
        std::array<byte_t, 5> colorMapSpec;
        std::array<byte_t, 10> imageSpec;
    } header;

    ePIXEL_LAYOUT pixelLayout{ePIXEL_LAYOUT::nUNDEFINED};

    std::int16_t width{0}, height{0};
    std::uint8_t pixelDepth{0}, colorMapDepth{0};

    texel_buffer_t data;
};

[[nodiscard]] std::optional<RawImage> LoadTARGA(std::string_view name);