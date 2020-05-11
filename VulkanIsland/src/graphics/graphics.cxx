#include "graphics.hxx"

namespace graphics
{
    std::size_t constexpr size_in_bytes(graphics::FORMAT format)
    {
        switch (format) {
            case graphics::FORMAT::RG4_UNORM_PACK8:
            case graphics::FORMAT::R8_UNORM:
            case graphics::FORMAT::R8_SNORM:
            case graphics::FORMAT::R8_USCALED:
            case graphics::FORMAT::R8_SSCALED:
            case graphics::FORMAT::R8_UINT:
            case graphics::FORMAT::R8_SINT:
            case graphics::FORMAT::R8_SRGB:
                return 8;

            case graphics::FORMAT::RGBA4_UNORM_PACK16:
            case graphics::FORMAT::BGRA4_UNORM_PACK16:
            case graphics::FORMAT::B5G6R5_UNORM_PACK16:
            case graphics::FORMAT::RGB5A1_UNORM_PACK16:
            case graphics::FORMAT::BGR5A1_UNORM_PACK16:
            case graphics::FORMAT::R5G6B5_UNORM_PACK16:
            case graphics::FORMAT::A1RGB5_UNORM_PACK16:
            case graphics::FORMAT::RG8_UNORM:
            case graphics::FORMAT::RG8_SNORM:
            case graphics::FORMAT::R16_UNORM:
            case graphics::FORMAT::R16_SNORM:
            case graphics::FORMAT::R16_SFLOAT:
            case graphics::FORMAT::RG8_USCALED:
            case graphics::FORMAT::RG8_SSCALED:
            case graphics::FORMAT::R16_USCALED:
            case graphics::FORMAT::R16_SSCALED:
            case graphics::FORMAT::RG8_UINT:
            case graphics::FORMAT::RG8_SINT:
            case graphics::FORMAT::RG8_SRGB:
            case graphics::FORMAT::R16_UINT:
            case graphics::FORMAT::R16_SINT:
            case graphics::FORMAT::D16_UNORM:
            case graphics::FORMAT::GBGR10X6_422_UNORM_4PACK16:
            case graphics::FORMAT::BGRG10X6_422_UNORM_4PACK16:
            case graphics::FORMAT::GBGR12X4_422_UNORM_4PACK16:
            case graphics::FORMAT::BGRG12X4_422_UNORM_4PACK16:
            case graphics::FORMAT:::
            case graphics::FORMAT:::
            case graphics::FORMAT:::
            case graphics::FORMAT:::
                return 16;

            default:
                return 0;
        }
    }
}