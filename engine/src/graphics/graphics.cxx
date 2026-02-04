#include "graphics.hxx"
#include "vertex.hxx"

import engine_types;

namespace graphics
{
    graphics::NUMERIC_FORMAT numeric_format(graphics::FORMAT format)
    {
        switch (format) {
            case graphics::FORMAT::RG4_UNORM_PACK8:
            case graphics::FORMAT::RGBA4_UNORM_PACK16:
            case graphics::FORMAT::BGRA4_UNORM_PACK16:
            case graphics::FORMAT::B5G6R5_UNORM_PACK16:
            case graphics::FORMAT::RGB5A1_UNORM_PACK16:
            case graphics::FORMAT::BGR5A1_UNORM_PACK16:
            case graphics::FORMAT::R5G6B5_UNORM_PACK16:
            case graphics::FORMAT::A1RGB5_UNORM_PACK16:
            case graphics::FORMAT::R8_UNORM:
            case graphics::FORMAT::R8_SNORM:
            case graphics::FORMAT::RG8_UNORM:
            case graphics::FORMAT::RG8_SNORM:
            case graphics::FORMAT::RGBA8_UNORM:
            case graphics::FORMAT::RGBA8_SNORM:
            case graphics::FORMAT::BGRA8_UNORM:
            case graphics::FORMAT::ABGR8_UNORM_PACK32:
            case graphics::FORMAT::ABGR8_SNORM_PACK32:
            case graphics::FORMAT::A2BGR10_UNORM_PACK32:
            case graphics::FORMAT::R16_UNORM:
            case graphics::FORMAT::R16_SNORM:
            case graphics::FORMAT::RG16_UNORM:
            case graphics::FORMAT::RG16_SNORM:
            case graphics::FORMAT::RGBA16_UNORM:
            case graphics::FORMAT::RGBA16_SNORM:
            case graphics::FORMAT::RGB8_UNORM:
            case graphics::FORMAT::RGB8_SNORM:
            case graphics::FORMAT::BGR8_UNORM:
            case graphics::FORMAT::BGR8_SNORM:
            case graphics::FORMAT::A2RGB10_SNORM_PACK32:
            case graphics::FORMAT::A2BGR10_SNORM_PACK32:
            case graphics::FORMAT::RGB16_UNORM:
            case graphics::FORMAT::RGB16_SNORM:
            case graphics::FORMAT::BC1_RGB_UNORM_BLOCK:
            case graphics::FORMAT::BC1_RGBA_UNORM_BLOCK:
            case graphics::FORMAT::BC2_UNORM_BLOCK:
            case graphics::FORMAT::BC3_UNORM_BLOCK:
            case graphics::FORMAT::BC4_UNORM_BLOCK:
            case graphics::FORMAT::BC4_SNORM_BLOCK:
            case graphics::FORMAT::BC5_UNORM_BLOCK:
            case graphics::FORMAT::BC5_SNORM_BLOCK:
            case graphics::FORMAT::BC7_UNORM_BLOCK:
            case graphics::FORMAT::BGRA8_SNORM:
            case graphics::FORMAT::A2RGB10_UNORM_PACK32:
            case graphics::FORMAT::D16_UNORM:
            case graphics::FORMAT::X8_D24_UNORM_PACK32:
            case graphics::FORMAT::GBGR8_422_UNORM:
            case graphics::FORMAT::BGRG8_422_UNORM:
            case graphics::FORMAT::GBGR10X6_422_UNORM_4PACK16:
            case graphics::FORMAT::BGRG10X6_422_UNORM_4PACK16:
            case graphics::FORMAT::GBGR12X4_422_UNORM_4PACK16:
            case graphics::FORMAT::BGRG12X4_422_UNORM_4PACK16:
            case graphics::FORMAT::GBGR16_422_UNORM:
            case graphics::FORMAT::BGRG16_422_UNORM:
            case graphics::FORMAT::G8_B8_R8_3PLANE_420_UNORM:
            case graphics::FORMAT::G8_B8R8_2PLANE_420_UNORM:
            case graphics::FORMAT::G8_B8_R8_3PLANE_422_UNORM:
            case graphics::FORMAT::G8_B8R8_2PLANE_422_UNORM:
            case graphics::FORMAT::G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
            case graphics::FORMAT::G10X6_BR10X6_2PLANE_420_UNORM_3PACK16:
            case graphics::FORMAT::G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
            case graphics::FORMAT::G10X6_BR10X6_2PLANE_422_UNORM_3PACK16:
            case graphics::FORMAT::G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
            case graphics::FORMAT::G12X4_BR12X4_2PLANE_420_UNORM_3PACK16:
            case graphics::FORMAT::G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
            case graphics::FORMAT::G12X4_BR12X4_2PLANE_422_UNORM_3PACK16:
            case graphics::FORMAT::G16_B16_R16_3PLANE_420_UNORM:
            case graphics::FORMAT::G16_BR16_2PLANE_420_UNORM:
            case graphics::FORMAT::G16_B16_R16_3PLANE_422_UNORM:
            case graphics::FORMAT::G16_BR16_2PLANE_422_UNORM:
            case graphics::FORMAT::G8_B8_R8_3PLANE_444_UNORM:
            case graphics::FORMAT::G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
            case graphics::FORMAT::G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
            case graphics::FORMAT::G16_B16_R16_3PLANE_444_UNORM:
            case graphics::FORMAT::R10X6_UNORM_PACK16:
            case graphics::FORMAT::RG10X6_UNORM_2PACK16:
            case graphics::FORMAT::RGBA10X6_UNORM_4PACK16:
            case graphics::FORMAT::R12X4_UNORM_PACK16:
            case graphics::FORMAT::RG12X4_UNORM_2PACK16:
            case graphics::FORMAT::RGBA12X4_UNORM_4PACK16:
                return graphics::NUMERIC_FORMAT::NORMALIZED;

            case graphics::FORMAT::R8_USCALED:
            case graphics::FORMAT::R8_SSCALED:
            case graphics::FORMAT::RG8_USCALED:
            case graphics::FORMAT::RG8_SSCALED:
            case graphics::FORMAT::RGB8_USCALED:
            case graphics::FORMAT::RGB8_SSCALED:
            case graphics::FORMAT::BGR8_USCALED:
            case graphics::FORMAT::BGR8_SSCALED:
            case graphics::FORMAT::RGBA8_USCALED:
            case graphics::FORMAT::RGBA8_SSCALED:
            case graphics::FORMAT::BGRA8_USCALED:
            case graphics::FORMAT::BGRA8_SSCALED:
            case graphics::FORMAT::ABGR8_USCALED_PACK32:
            case graphics::FORMAT::ABGR8_SSCALED_PACK32:
            case graphics::FORMAT::A2RGB10_USCALED_PACK32:
            case graphics::FORMAT::A2RGB10_SSCALED_PACK32:
            case graphics::FORMAT::A2BGR10_USCALED_PACK32:
            case graphics::FORMAT::A2BGR10_SSCALED_PACK32:
            case graphics::FORMAT::R16_USCALED:
            case graphics::FORMAT::R16_SSCALED:
            case graphics::FORMAT::RG16_USCALED:
            case graphics::FORMAT::RG16_SSCALED:
            case graphics::FORMAT::RGB16_USCALED:
            case graphics::FORMAT::RGB16_SSCALED:
            case graphics::FORMAT::RGBA16_USCALED:
            case graphics::FORMAT::RGBA16_SSCALED:
                return graphics::NUMERIC_FORMAT::SCALED;

            case graphics::FORMAT::D16_UNORM_S8_UINT:
            case graphics::FORMAT::D24_UNORM_S8_UINT:
                return graphics::NUMERIC_FORMAT::NORMALIZED & graphics::NUMERIC_FORMAT::INT;

            case graphics::FORMAT::RGB8_UINT:
            case graphics::FORMAT::RGB8_SINT:
            case graphics::FORMAT::BGR8_UINT:
            case graphics::FORMAT::BGR8_SINT:
            case graphics::FORMAT::A2RGB10_SINT_PACK32:
            case graphics::FORMAT::A2BGR10_SINT_PACK32:
            case graphics::FORMAT::RGB16_UINT:
            case graphics::FORMAT::RGB16_SINT:
            case graphics::FORMAT::R64_UINT:
            case graphics::FORMAT::R64_SINT:
            case graphics::FORMAT::RG64_UINT:
            case graphics::FORMAT::RG64_SINT:
            case graphics::FORMAT::RGB64_UINT:
            case graphics::FORMAT::RGB64_SINT:
            case graphics::FORMAT::RGBA64_UINT:
            case graphics::FORMAT::RGBA64_SINT:
            case graphics::FORMAT::R8_UINT:
            case graphics::FORMAT::R8_SINT:
            case graphics::FORMAT::RG8_UINT:
            case graphics::FORMAT::RG8_SINT:
            case graphics::FORMAT::RGBA8_UINT:
            case graphics::FORMAT::RGBA8_SINT:
            case graphics::FORMAT::ABGR8_UINT_PACK32:
            case graphics::FORMAT::ABGR8_SINT_PACK32:
            case graphics::FORMAT::A2BGR10_UINT_PACK32:
            case graphics::FORMAT::R16_UINT:
            case graphics::FORMAT::R16_SINT:
            case graphics::FORMAT::RG16_UINT:
            case graphics::FORMAT::RG16_SINT:
            case graphics::FORMAT::RGBA16_UINT:
            case graphics::FORMAT::RGBA16_SINT:
            case graphics::FORMAT::RG32_UINT:
            case graphics::FORMAT::RG32_SINT:
            case graphics::FORMAT::RGBA32_UINT:
            case graphics::FORMAT::RGBA32_SINT:
            case graphics::FORMAT::BGRA8_UINT:
            case graphics::FORMAT::BGRA8_SINT:
            case graphics::FORMAT::A2RGB10_UINT_PACK32:
            case graphics::FORMAT::R32_UINT:
            case graphics::FORMAT::R32_SINT:
            case graphics::FORMAT::RGB32_UINT:
            case graphics::FORMAT::RGB32_SINT:
            case graphics::FORMAT::D32_SFLOAT_S8_UINT:
                return graphics::NUMERIC_FORMAT::INT;

            case graphics::FORMAT::E5BGR9_UFLOAT_PACK32:
            case graphics::FORMAT::R16_SFLOAT:
            case graphics::FORMAT::RG16_SFLOAT:
            case graphics::FORMAT::RGBA16_SFLOAT:
            case graphics::FORMAT::RG32_SFLOAT:
            case graphics::FORMAT::RGBA32_SFLOAT:
            case graphics::FORMAT::B10GR11_UFLOAT_PACK32:
            case graphics::FORMAT::RGB16_SFLOAT:
            case graphics::FORMAT::R64_SFLOAT:
            case graphics::FORMAT::RG64_SFLOAT:
            case graphics::FORMAT::RGB64_SFLOAT:
            case graphics::FORMAT::RGBA64_SFLOAT:
            case graphics::FORMAT::BC6H_UFLOAT_BLOCK:
            case graphics::FORMAT::BC6H_SFLOAT_BLOCK:
            case graphics::FORMAT::R32_SFLOAT:
            case graphics::FORMAT::RGB32_SFLOAT:
            case graphics::FORMAT::D32_SFLOAT:
                return graphics::NUMERIC_FORMAT::FLOAT;

            case graphics::FORMAT::RGB8_SRGB:
            case graphics::FORMAT::BGR8_SRGB:
            case graphics::FORMAT::RG8_SRGB:
            case graphics::FORMAT::R8_SRGB:
            case graphics::FORMAT::BC1_RGB_SRGB_BLOCK:
            case graphics::FORMAT::BC1_RGBA_SRGB_BLOCK:
            case graphics::FORMAT::BC2_SRGB_BLOCK:
            case graphics::FORMAT::BC3_SRGB_BLOCK:
            case graphics::FORMAT::BC7_SRGB_BLOCK:
            case graphics::FORMAT::RGBA8_SRGB:
            case graphics::FORMAT::BGRA8_SRGB:
            case graphics::FORMAT::ABGR8_SRGB_PACK32:
                return graphics::NUMERIC_FORMAT::SRGB;

            case graphics::FORMAT::UNDEFINED:
            default:
                return graphics::NUMERIC_FORMAT::UNDEFINED;
        }
    }

    std::optional<graphics::format_instance> instantiate_format(graphics::FORMAT format)
    {
        switch (format) {
            case graphics::FORMAT::R8_SNORM:
            case graphics::FORMAT::R8_SSCALED:
            case graphics::FORMAT::R8_SINT:
                return std::array<std::int8_t, 1>{ };

            case graphics::FORMAT::RG4_UNORM_PACK8:
            case graphics::FORMAT::R8_UNORM:
            case graphics::FORMAT::R8_USCALED:
            case graphics::FORMAT::R8_UINT:
            case graphics::FORMAT::R8_SRGB:
                return std::array<std::uint8_t, 1>{ };

            case graphics::FORMAT::RG8_SNORM:
            case graphics::FORMAT::RG8_SSCALED:
            case graphics::FORMAT::RG8_SINT:
                return std::array<std::int8_t, 2>{ };

            case graphics::FORMAT::RG8_UNORM:
            case graphics::FORMAT::RG8_USCALED:
            case graphics::FORMAT::RG8_UINT:
            case graphics::FORMAT::RG8_SRGB:
                return std::array<std::uint8_t, 2>{ };

            case graphics::FORMAT::R16_SNORM:
            case graphics::FORMAT::R16_SSCALED:
            case graphics::FORMAT::R16_SINT:
                return std::array<std::int16_t, 1>{ };

            case graphics::FORMAT::RGBA4_UNORM_PACK16:
            case graphics::FORMAT::BGRA4_UNORM_PACK16:
            case graphics::FORMAT::B5G6R5_UNORM_PACK16:
            case graphics::FORMAT::RGB5A1_UNORM_PACK16:
            case graphics::FORMAT::BGR5A1_UNORM_PACK16:
            case graphics::FORMAT::R5G6B5_UNORM_PACK16:
            case graphics::FORMAT::A1RGB5_UNORM_PACK16:
            case graphics::FORMAT::R16_UNORM:
            case graphics::FORMAT::R16_USCALED:
            case graphics::FORMAT::R16_UINT:
            case graphics::FORMAT::D16_UNORM:
            case graphics::FORMAT::R10X6_UNORM_PACK16:
            case graphics::FORMAT::R12X4_UNORM_PACK16:
                return std::array<std::uint16_t, 1>{ };

            case graphics::FORMAT::R16_SFLOAT:
    #if defined(BOOST_FLOAT16_C)
                return std::array<float16, 1>{ };
    #else
                return { };
    #endif

            case graphics::FORMAT::RGB8_SNORM:
            case graphics::FORMAT::RGB8_SSCALED:
            case graphics::FORMAT::RGB8_SINT:
            case graphics::FORMAT::BGR8_SNORM:
            case graphics::FORMAT::BGR8_SSCALED:
            case graphics::FORMAT::BGR8_SINT:
                return std::array<std::int8_t, 3>{ };

            case graphics::FORMAT::RGB8_UNORM:
            case graphics::FORMAT::RGB8_USCALED:
            case graphics::FORMAT::RGB8_UINT:
            case graphics::FORMAT::BGR8_UNORM:
            case graphics::FORMAT::BGR8_USCALED:
            case graphics::FORMAT::BGR8_UINT:
            case graphics::FORMAT::RGB8_SRGB:
            case graphics::FORMAT::BGR8_SRGB:
            case graphics::FORMAT::D16_UNORM_S8_UINT:
                return std::array<std::uint8_t, 3>{ };

            case graphics::FORMAT::RGBA8_SNORM:
            case graphics::FORMAT::RGBA8_SSCALED:
            case graphics::FORMAT::BGRA8_SSCALED:
            case graphics::FORMAT::RGBA8_SINT:
            case graphics::FORMAT::BGRA8_SNORM:
            case graphics::FORMAT::BGRA8_SINT:
                return std::array<std::int8_t, 4>{ };

            case graphics::FORMAT::RGBA8_UNORM:
            case graphics::FORMAT::BGRA8_UNORM:
            case graphics::FORMAT::RGBA8_USCALED:
            case graphics::FORMAT::BGRA8_USCALED:
            case graphics::FORMAT::RGBA8_UINT:
            case graphics::FORMAT::BGRA8_UINT:
            case graphics::FORMAT::RGBA8_SRGB:
            case graphics::FORMAT::BGRA8_SRGB:
                return std::array<std::uint8_t, 4>{ };

            case graphics::FORMAT::RG16_SNORM:
            case graphics::FORMAT::RG16_SSCALED:
            case graphics::FORMAT::RG16_SINT:
                return std::array<std::int16_t, 2>{ };

            case graphics::FORMAT::RG16_UNORM:
            case graphics::FORMAT::RG16_USCALED:
            case graphics::FORMAT::RG16_UINT:
            case graphics::FORMAT::D24_UNORM_S8_UINT:
            case graphics::FORMAT::RG10X6_UNORM_2PACK16:
            case graphics::FORMAT::RG12X4_UNORM_2PACK16:
                return std::array<std::uint16_t, 2>{ };

            case graphics::FORMAT::ABGR8_SNORM_PACK32:
            case graphics::FORMAT::A2RGB10_SNORM_PACK32:
            case graphics::FORMAT::A2BGR10_SNORM_PACK32:
            case graphics::FORMAT::ABGR8_SSCALED_PACK32:
            case graphics::FORMAT::A2RGB10_SSCALED_PACK32:
            case graphics::FORMAT::A2RGB10_SINT_PACK32:
            case graphics::FORMAT::A2BGR10_SSCALED_PACK32:
            case graphics::FORMAT::A2BGR10_SINT_PACK32:
            case graphics::FORMAT::ABGR8_SINT_PACK32:
            case graphics::FORMAT::R32_SINT:
                return std::array<std::int32_t, 1>{ };

            case graphics::FORMAT::E5BGR9_UFLOAT_PACK32:
            case graphics::FORMAT::ABGR8_UNORM_PACK32:
            case graphics::FORMAT::A2BGR10_UNORM_PACK32:
            case graphics::FORMAT::A2RGB10_UNORM_PACK32:
            case graphics::FORMAT::X8_D24_UNORM_PACK32:
            case graphics::FORMAT::B10GR11_UFLOAT_PACK32:
            case graphics::FORMAT::ABGR8_USCALED_PACK32:
            case graphics::FORMAT::A2RGB10_USCALED_PACK32:
            case graphics::FORMAT::A2BGR10_USCALED_PACK32:
            case graphics::FORMAT::ABGR8_UINT_PACK32:
            case graphics::FORMAT::A2BGR10_UINT_PACK32:
            case graphics::FORMAT::A2RGB10_UINT_PACK32:
            case graphics::FORMAT::ABGR8_SRGB_PACK32:
            case graphics::FORMAT::R32_UINT:
                return std::array<std::uint32_t, 1>{ };

            case graphics::FORMAT::RG16_SFLOAT:
    #if defined(BOOST_FLOAT16_C)
                return std::array<float16, 2>{ };
    #else
                return { };
    #endif

            case graphics::FORMAT::R32_SFLOAT:
            case graphics::FORMAT::D32_SFLOAT:
                return std::array<float32, 1>{ };

            case graphics::FORMAT::RGB16_SNORM:
            case graphics::FORMAT::RGB16_SSCALED:
            case graphics::FORMAT::RGB16_SINT:
                return std::array<std::int16_t, 3>{ };

            case graphics::FORMAT::RGB16_UNORM:
            case graphics::FORMAT::RGB16_USCALED:
            case graphics::FORMAT::RGB16_UINT:
                return std::array<std::uint16_t, 3>{ };

            case graphics::FORMAT::RGB16_SFLOAT:
    #if defined(BOOST_FLOAT16_C)
                return std::array<float16, 3>{ };
    #else
                return { };
    #endif

            case graphics::FORMAT::RGBA16_SNORM:
            case graphics::FORMAT::RGBA16_SSCALED:
            case graphics::FORMAT::RGBA16_SINT:
                return std::array<std::int16_t, 4>{ };

            case graphics::FORMAT::RGBA16_UNORM:
            case graphics::FORMAT::RGBA16_USCALED:
            case graphics::FORMAT::RGBA16_UINT:
            case graphics::FORMAT::RGBA10X6_UNORM_4PACK16:
            case graphics::FORMAT::RGBA12X4_UNORM_4PACK16:
                return std::array<std::uint16_t, 4>{ };

            case graphics::FORMAT::RG32_SINT:
                return std::array<std::int32_t, 2>{ };

            case graphics::FORMAT::RG32_UINT:
                return std::array<std::uint32_t, 2>{ };

            case graphics::FORMAT::R64_SINT:
                return std::array<std::int64_t, 1>{ };

            case graphics::FORMAT::R64_UINT:
                return std::array<std::uint64_t, 1>{ };

            case graphics::FORMAT::RGBA16_SFLOAT:
    #if defined(BOOST_FLOAT16_C)
                return std::array<float16, 4>{ };
    #else
                return { };
    #endif

            case graphics::FORMAT::RG32_SFLOAT:
            case graphics::FORMAT::D32_SFLOAT_S8_UINT:
                return std::array<float32, 2>{ };

            case graphics::FORMAT::R64_SFLOAT:
                return std::array<float64, 1>{ };

            case graphics::FORMAT::RGB32_SINT:
                return std::array<std::int32_t, 3>{ };

            case graphics::FORMAT::RGB32_UINT:
                return std::array<std::uint32_t, 3>{ };

            case graphics::FORMAT::RGB32_SFLOAT:
                return std::array<float32, 3>{ };

            case graphics::FORMAT::RGBA32_SINT:
                return std::array<std::int32_t, 4>{ };

            case graphics::FORMAT::RGBA32_UINT:
                return std::array<std::uint32_t, 4>{ };

            case graphics::FORMAT::RGBA32_SFLOAT:
                return std::array<float32, 4>{ };

            case graphics::FORMAT::RG64_SINT:
                return std::array<std::int64_t, 2>{ };

            case graphics::FORMAT::RG64_UINT:
                return std::array<std::uint64_t, 2>{ };

            case graphics::FORMAT::RG64_SFLOAT:
                return std::array<float64, 2>{ };

            case graphics::FORMAT::RGB64_SINT:
                return std::array<std::int64_t, 3>{ };

            case graphics::FORMAT::RGB64_UINT:
                return std::array<std::uint64_t, 3>{ };

            case graphics::FORMAT::RGB64_SFLOAT:
                return std::array<float64, 3>{ };

            case graphics::FORMAT::RGBA64_SINT:
                return std::array<std::int64_t, 4>{ };

            case graphics::FORMAT::RGBA64_UINT:
                return std::array<std::uint64_t, 4>{ };

            case graphics::FORMAT::RGBA64_SFLOAT:
                return std::array<float64, 4>{ };

            case graphics::FORMAT::UNDEFINED:

            case graphics::FORMAT::GBGR8_422_UNORM:
            case graphics::FORMAT::BGRG8_422_UNORM:
            case graphics::FORMAT::GBGR10X6_422_UNORM_4PACK16:
            case graphics::FORMAT::BGRG10X6_422_UNORM_4PACK16:
            case graphics::FORMAT::GBGR12X4_422_UNORM_4PACK16:
            case graphics::FORMAT::BGRG12X4_422_UNORM_4PACK16:
            case graphics::FORMAT::GBGR16_422_UNORM:
            case graphics::FORMAT::BGRG16_422_UNORM:

            case graphics::FORMAT::G8_B8_R8_3PLANE_420_UNORM:
            case graphics::FORMAT::G8_B8R8_2PLANE_420_UNORM:
            case graphics::FORMAT::G8_B8_R8_3PLANE_422_UNORM:
            case graphics::FORMAT::G8_B8R8_2PLANE_422_UNORM:
            case graphics::FORMAT::G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
            case graphics::FORMAT::G10X6_BR10X6_2PLANE_420_UNORM_3PACK16:
            case graphics::FORMAT::G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
            case graphics::FORMAT::G10X6_BR10X6_2PLANE_422_UNORM_3PACK16:
            case graphics::FORMAT::G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
            case graphics::FORMAT::G12X4_BR12X4_2PLANE_420_UNORM_3PACK16:
            case graphics::FORMAT::G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
            case graphics::FORMAT::G12X4_BR12X4_2PLANE_422_UNORM_3PACK16:
            case graphics::FORMAT::G16_B16_R16_3PLANE_420_UNORM:
            case graphics::FORMAT::G16_BR16_2PLANE_420_UNORM:
            case graphics::FORMAT::G16_B16_R16_3PLANE_422_UNORM:
            case graphics::FORMAT::G16_BR16_2PLANE_422_UNORM:

            case graphics::FORMAT::G8_B8_R8_3PLANE_444_UNORM:
            case graphics::FORMAT::G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
            case graphics::FORMAT::G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
            case graphics::FORMAT::G16_B16_R16_3PLANE_444_UNORM:

            case graphics::FORMAT::BC1_RGB_UNORM_BLOCK:
            case graphics::FORMAT::BC1_RGB_SRGB_BLOCK:
            case graphics::FORMAT::BC1_RGBA_UNORM_BLOCK:
            case graphics::FORMAT::BC1_RGBA_SRGB_BLOCK:
            case graphics::FORMAT::BC2_UNORM_BLOCK:
            case graphics::FORMAT::BC2_SRGB_BLOCK:
            case graphics::FORMAT::BC3_UNORM_BLOCK:
            case graphics::FORMAT::BC3_SRGB_BLOCK:
            case graphics::FORMAT::BC4_UNORM_BLOCK:
            case graphics::FORMAT::BC4_SNORM_BLOCK:
            case graphics::FORMAT::BC5_UNORM_BLOCK:
            case graphics::FORMAT::BC5_SNORM_BLOCK:
            case graphics::FORMAT::BC6H_UFLOAT_BLOCK:
            case graphics::FORMAT::BC6H_SFLOAT_BLOCK:
            case graphics::FORMAT::BC7_UNORM_BLOCK:
            case graphics::FORMAT::BC7_SRGB_BLOCK:

            default:
                return { };
        }
    }

    std::size_t size_bytes(graphics::FORMAT format)
    {
        if (auto format_inst = graphics::instantiate_format(format); format_inst) {
            return std::visit([] (auto &&f)
            {
                return sizeof(std::remove_cvref_t<decltype(f)>);

            }, *format_inst);
        }

        return 0;
    }

    std::size_t size_bytes(graphics::INDEX_TYPE index_type)
    {
        switch (index_type) {
            case graphics::INDEX_TYPE::UINT_16:
                return sizeof(std::uint16_t);

            case graphics::INDEX_TYPE::UINT_32:
                return sizeof(std::uint32_t);

            case graphics::INDEX_TYPE::UNDEFINED:
            default:
                return 0;
        }
    }
}
