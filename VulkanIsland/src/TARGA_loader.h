#pragma once

#include <array>
#include <vector>
#include <string_view>

struct alignas(sizeof(std::uint32_t)) RGBA
{
    union {
        std::array<std::byte, 4> channels;
        std::uint32_t value;
    };
};

struct Image {
    std::vector<RGBA> data;

    std::int32_t bpp{0};
    std::int32_t width{0}, height{0};
    std::uint32_t type{0}, format{0};

    // std::uint8_t BytesPerPixel() const;
};

[[nodiscard]] std::optional<std::pair<std::int32_t, std::int32_t>> LoadTARGA(std::string_view name, std::vector<std::byte> &pixels);