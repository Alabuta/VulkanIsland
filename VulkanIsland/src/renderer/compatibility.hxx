#pragma once

#include <set>
#include <array>
#include <algorithm>

#include "graphics.hxx"
#include "render_pass.hxx"
#include "attachments.hxx"


namespace graphics
{
    template<>
    struct compatibility<graphics::render_pass> {
        auto operator() (graphics::render_pass const &lhs, graphics::render_pass const &rhs) const
        {
            // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#renderpass-compatibility

            /* Two render passes are compatible if their corresponding color, input, resolve, and depth/stencil
            attachment references are compatible and if they are otherwise identical except for:
                Initial and final image layout in attachment descriptions
                Load and store operations in attachment descriptions
                Image layout in attachment references

            As an additional special case, if two render passes have a single subpass, they are compatible even
            if they have different resolve attachment references or depth/stencil resolve modes but satisfy the other compatibility conditions. */

            return false;
        }
    };

    template<>
    struct compatibility<graphics::FORMAT> {
        auto operator() (graphics::FORMAT const &lhs, graphics::FORMAT const &rhs) const
        {
            using namespace graphics;

            auto const check = std::set{ lhs, rhs };

            auto constexpr _8bit = std::array{
                FORMAT::RG4_UNORM_PACK8,
                FORMAT::R8_UNORM,
                FORMAT::R8_SNORM,
                FORMAT::R8_USCALED,
                FORMAT::R8_SSCALED,
                FORMAT::R8_UINT,
                FORMAT::R8_SINT,
                FORMAT::R8_SRGB
            };

            if (std::includes(std::cbegin(_8bit), std::cend(_8bit), std::cbegin(check), std::cend(check)))
                return true;

            auto constexpr _16bit = std::array{
                FORMAT::RGBA4_UNORM_PACK16,
                FORMAT::BGRA4_UNORM_PACK16,
                FORMAT::B5G6R5_UNORM_PACK16,
                FORMAT::RGB5A1_UNORM_PACK16,
                FORMAT::BGR5A1_UNORM_PACK16,
                FORMAT::R5G6B5_UNORM_PACK16,
                FORMAT::A1RGB5_UNORM_PACK16,
                FORMAT::RG8_UNORM,
                FORMAT::RG8_SNORM,
                FORMAT::R16_UNORM,
                FORMAT::R16_SNORM,
                FORMAT::R16_SFLOAT,
                FORMAT::RG8_USCALED,
                FORMAT::RG8_SSCALED,
                FORMAT::R16_USCALED,
                FORMAT::R16_SSCALED,
                FORMAT::RG8_UINT,
                FORMAT::RG8_SINT,
                FORMAT::RG8_SRGB,
                FORMAT::R16_UINT,
                FORMAT::R16_SINT,
                FORMAT::R10X6_UNORM_PACK16,
                FORMAT::R12X4_UNORM_PACK16
            };

            if (std::includes(std::cbegin(_16bit), std::cend(_16bit), std::cbegin(check), std::cend(check)))
                return true;

            auto constexpr _24bit = std::array{
                FORMAT::RGB8_UNORM,
                FORMAT::RGB8_SNORM,
                FORMAT::RGB8_USCALED,
                FORMAT::RGB8_SSCALED,
                FORMAT::RGB8_UINT,
                FORMAT::RGB8_SINT,
                FORMAT::RGB8_SRGB,
                FORMAT::BGR8_UNORM,
                FORMAT::BGR8_SNORM,
                FORMAT::BGR8_USCALED,
                FORMAT::BGR8_SSCALED,
                FORMAT::BGR8_UINT,
                FORMAT::BGR8_SINT,
                FORMAT::BGR8_SRGB
            };

            if (std::includes(std::cbegin(_24bit), std::cend(_24bit), std::cbegin(check), std::cend(check)))
                return true;

            return false;
        }
    };
}
