#pragma once

#include <array>
#include <vector>
#include <string_view>
#include <variant>

#include "math.hxx"
#include "utility/mpl.hxx"
#include "renderer/graphics.hxx"


enum class PIXEL_LAYOUT {
    nUNDEFINED = 0, nRED, nRG, nRGB, nBGR, nRGBA, nBGRA
};

using byte_t = std::uint8_t;

using texel_t = std::variant<
    math::vec<1, std::uint8_t>,
    math::vec<2, std::uint8_t>,
    math::vec<3, std::uint8_t>,
    math::vec<4, std::uint8_t>
>;

using texel_buffer_t = mpl::wrap_variant_by_vector<texel_t>::type;

struct RawImage {
    graphics::FORMAT format{graphics::FORMAT::UNDEFINED};
    graphics::IMAGE_VIEW_TYPE view_type;

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

    PIXEL_LAYOUT pixelLayout{PIXEL_LAYOUT::nUNDEFINED};

    std::int16_t width{0}, height{0};
    std::uint8_t pixelDepth{0}, colorMapDepth{0};

    texel_buffer_t data;
};

[[nodiscard]] std::optional<RawImage> LoadTARGA(std::string_view name);