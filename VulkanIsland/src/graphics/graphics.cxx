#include "graphics.hxx"


namespace graphics
{
    std::optional<format_instance> constexpr instantiate_format(graphics::FORMAT format)
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

    #if defined(BOOST_FLOAT16_C)
            case graphics::FORMAT::R16_SFLOAT:
                return std::array<boost::float16_t, 1>{ };
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
            case graphics::FORMAT::D24_UNORM_S8_UINT:
                return std::array<std::uint32_t, 1>{ };

    #if defined(BOOST_FLOAT16_C)
            case graphics::FORMAT::RG16_SFLOAT:
                return std::array<boost::float16_t, 2>{ };
    #endif

            case graphics::FORMAT::R32_SFLOAT:
            case graphics::FORMAT::D32_SFLOAT:
                return std::array<boost::float32_t, 1>{ };

            case graphics::FORMAT::RGB16_SNORM:
            case graphics::FORMAT::RGB16_SSCALED:
            case graphics::FORMAT::RGB16_SINT:
                return std::array<std::int16_t, 3>{ };

            case graphics::FORMAT::RGB16_UNORM:
            case graphics::FORMAT::RGB16_USCALED:
            case graphics::FORMAT::RGB16_UINT:
                return std::array<std::uint16_t, 3>{ };

    #if defined(BOOST_FLOAT16_C)
            case graphics::FORMAT::RGB16_SFLOAT:
                return std::array<boost::float16_t, 3>{ };
    #endif

            case graphics::FORMAT::RGBA16_SNORM:
            case graphics::FORMAT::RGBA16_SSCALED:
            case graphics::FORMAT::RGBA16_SINT:
                return std::array<std::int16_t, 4>{ };

            case graphics::FORMAT::RGBA16_UNORM:
            case graphics::FORMAT::RGBA16_USCALED:
            case graphics::FORMAT::RGBA16_UINT:
                return std::array<std::uint16_t, 4>{ };

            case graphics::FORMAT::RG32_SINT:
                return std::array<std::int32_t, 2>{ };

            case graphics::FORMAT::RG32_UINT:
                return std::array<std::uint32_t, 2>{ };

            case graphics::FORMAT::R64_SINT:
                return std::array<std::int64_t, 1>{ };

            case graphics::FORMAT::D32_SFLOAT_S8_UINT:
            case graphics::FORMAT::R64_UINT:
                return std::array<std::uint64_t, 1>{ };

    #if defined(BOOST_FLOAT16_C)
            case graphics::FORMAT::RGBA16_SFLOAT:
                return std::array<boost::float16_t, 4>{ };
    #endif

            case graphics::FORMAT::RG32_SFLOAT:
                return std::array<boost::float32_t, 2>{ };

            case graphics::FORMAT::R64_SFLOAT:
                return std::array<boost::float64_t, 1>{ };

            case graphics::FORMAT::RGB32_SINT:
                return std::array<std::int32_t, 3>{ };

            case graphics::FORMAT::RGB32_UINT:
                return std::array<std::uint32_t, 3>{ };

            case graphics::FORMAT::RGB32_SFLOAT:
                return std::array<boost::float32_t, 3>{ };

            case graphics::FORMAT::RGBA32_SINT:
                return std::array<std::int32_t, 4>{ };

            case graphics::FORMAT::RGBA32_UINT:
                return std::array<std::uint32_t, 4>{ };

            case graphics::FORMAT::RG64_SINT:
                return std::array<std::int64_t, 2>{ };

            case graphics::FORMAT::RG64_UINT:
                return std::array<std::uint64_t, 2>{ };

            case graphics::FORMAT::RG64_SFLOAT:
                return std::array<boost::float64_t, 2>{ };

            case graphics::FORMAT::RGB64_SINT:
                return std::array<std::int64_t, 3>{ };

            case graphics::FORMAT::RGB64_UINT:
                return std::array<std::uint64_t, 3>{ };

            case graphics::FORMAT::RGB64_SFLOAT:
                return std::array<boost::float64_t, 3>{ };

            case graphics::FORMAT::RGBA64_SINT:
                return std::array<std::int64_t, 4>{ };

            case graphics::FORMAT::RGBA64_UINT:
                return std::array<std::uint64_t, 4>{ };

            case graphics::FORMAT::RGBA64_SFLOAT:
                return std::array<boost::float64_t, 4>{ };

            default:
                return { };
        }
    }
}
