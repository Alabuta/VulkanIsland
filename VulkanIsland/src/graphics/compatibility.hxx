#pragma once

#include <set>
#include <ranges>
#include <algorithm>

#include "graphics.hxx"
#include "render_pass.hxx"
#include "attachments.hxx"


namespace graphics
{
    template<>
    struct compatibility<graphics::render_pass> {
        auto operator() (graphics::render_pass const &, graphics::render_pass const &) const
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

            if (lhs == rhs)
                return true;

            auto const check = std::set{ lhs, rhs };

            auto /*constexpr*/ bit8 = std::set{
                FORMAT::RG4_UNORM_PACK8,
                FORMAT::R8_UNORM,
                FORMAT::R8_SNORM,
                FORMAT::R8_USCALED,
                FORMAT::R8_SSCALED,
                FORMAT::R8_UINT,
                FORMAT::R8_SINT,
                FORMAT::R8_SRGB
            };

            if (std::ranges::includes(bit8, check))
                return true;

            auto /*constexpr*/ bit16 = std::set{
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

            if (std::ranges::includes(bit16, check))
                return true;

            auto /*constexpr*/ bit24 = std::set{
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

            if (std::ranges::includes(bit24, check))
                return true;

            auto /*constexpr*/ bit32 = std::set{
                FORMAT::E5BGR9_UFLOAT_PACK32,
                FORMAT::RGBA8_UNORM,
                FORMAT::RGBA8_SNORM,
                FORMAT::BGRA8_UNORM,
                FORMAT::ABGR8_UNORM_PACK32,
                FORMAT::ABGR8_SNORM_PACK32,
                FORMAT::A2BGR10_UNORM_PACK32,
                FORMAT::RG16_UNORM,
                FORMAT::RG16_SNORM,
                FORMAT::RG16_SFLOAT,
                FORMAT::B10GR11_UFLOAT_PACK32,
                FORMAT::RGBA8_USCALED,
                FORMAT::RGBA8_SSCALED,
                FORMAT::BGRA8_USCALED,
                FORMAT::BGRA8_SSCALED,
                FORMAT::ABGR8_USCALED_PACK32,
                FORMAT::ABGR8_SSCALED_PACK32,
                FORMAT::A2RGB10_SNORM_PACK32,
                FORMAT::A2RGB10_USCALED_PACK32,
                FORMAT::A2RGB10_SSCALED_PACK32,
                FORMAT::A2RGB10_SINT_PACK32,
                FORMAT::A2BGR10_SNORM_PACK32,
                FORMAT::A2BGR10_USCALED_PACK32,
                FORMAT::A2BGR10_SSCALED_PACK32,
                FORMAT::A2BGR10_SINT_PACK32,
                FORMAT::RG16_USCALED,
                FORMAT::RG16_SSCALED,
                FORMAT::RGBA8_UINT,
                FORMAT::RGBA8_SINT,
                FORMAT::ABGR8_UINT_PACK32,
                FORMAT::ABGR8_SINT_PACK32,
                FORMAT::A2BGR10_UINT_PACK32,
                FORMAT::RG16_UINT,
                FORMAT::RG16_SINT,
                FORMAT::RGBA8_SRGB,
                FORMAT::BGRA8_SRGB,
                FORMAT::ABGR8_SRGB_PACK32,
                FORMAT::BGRA8_SNORM,
                FORMAT::BGRA8_UINT,
                FORMAT::BGRA8_SINT,
                FORMAT::A2RGB10_UINT_PACK32,
                FORMAT::A2RGB10_UNORM_PACK32,
                FORMAT::R32_UINT,
                FORMAT::R32_SINT,
                FORMAT::R32_SFLOAT
                // FORMAT::R10X6G10X6_UNORM_2PACK16,
                // FORMAT::R12X4G12X4_UNORM_2PACK16
            };

            if (std::ranges::includes(bit32, check))
                return true;

            auto /*constexpr*/ bit48 = std::set{
                FORMAT::RGB16_UNORM,
                FORMAT::RGB16_SNORM,
                FORMAT::RGB16_USCALED,
                FORMAT::RGB16_SSCALED,
                FORMAT::RGB16_UINT,
                FORMAT::RGB16_SINT,
                FORMAT::RGB16_SFLOAT
            };

            if (std::ranges::includes(bit48, check))
                return true;

            auto /*constexpr*/ bit64 = std::set{
                FORMAT::RGBA16_UNORM,
                FORMAT::RGBA16_SNORM,
                FORMAT::RGBA16_SFLOAT,
                FORMAT::RG32_SFLOAT,
                FORMAT::RGBA16_USCALED,
                FORMAT::RGBA16_SSCALED,
                FORMAT::R64_UINT,
                FORMAT::R64_SINT,
                FORMAT::R64_SFLOAT,
                FORMAT::RGBA16_UINT,
                FORMAT::RGBA16_SINT,
                FORMAT::RG32_UINT,
                FORMAT::RG32_SINT
            };

            if (std::ranges::includes(bit64, check))
                return true;

            auto /*constexpr*/ bit96 = std::set{
                FORMAT::RGB32_UINT,
                FORMAT::RGB32_SINT,
                FORMAT::RGB32_SFLOAT
            };

            if (std::ranges::includes(bit96, check))
                return true;

            auto /*constexpr*/ bit128 = std::set{
                FORMAT::RGBA32_SFLOAT,
                FORMAT::RGBA32_UINT,
                FORMAT::RGBA32_SINT,
                FORMAT::RG64_UINT,
                FORMAT::RG64_SINT,
                FORMAT::RG64_SFLOAT
            };

            if (std::ranges::includes(bit128, check))
                return true;

            auto /*constexpr*/ bit192 = std::set{
                FORMAT::RGB64_UINT,
                FORMAT::RGB64_SINT,
                FORMAT::RGB64_SFLOAT
            };

            if (std::ranges::includes(bit192, check))
                return true;

            auto /*constexpr*/ bit256 = std::set{
                FORMAT::RGBA64_UINT,
                FORMAT::RGBA64_SINT,
                FORMAT::RGBA64_SFLOAT
            };

            if (std::ranges::includes(bit256, check))
                return true;

            return false;
        }
    };
}
