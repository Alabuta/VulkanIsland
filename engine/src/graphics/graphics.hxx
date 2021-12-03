#pragma once

#include <array>
#include <string>
#include <variant>
#include <cstdint>
#include <optional>
#include <type_traits>

#include <boost/cstdfloat.hpp>

#include "utility/mpl.hxx"


namespace graphics
{
    template<class T>
    struct hash;

    template<class T>
    struct compatibility;
}

namespace graphics
{
    enum struct PRIMITIVE_TOPOLOGY {
        POINTS = 0,
        LINES, LINE_STRIP,
        TRIANGLES, TRIANGLE_STRIP, TRIANGLE_FAN
    };

    enum struct VERTEX_INPUT_RATE {
        PER_VERTEX, PER_INSTANCE
    };

    enum struct SHADER_STAGE {
        VERTEX = 0x01,
        TESS_CONTROL = 0x02,
        TESS_EVAL = 0x04,
        GEOMETRY = 0x08,
        FRAGMENT = 0x10,
        COMPUTE = 0x20,

        ALL_GRAPHICS_SHADER_STAGES = VERTEX | TESS_CONTROL | TESS_EVAL | GEOMETRY | FRAGMENT,
        ALL_SHADER_STAGES = VERTEX | TESS_CONTROL | TESS_EVAL | GEOMETRY | FRAGMENT | COMPUTE
    };

    enum struct PIPELINE_STAGE {
        TOP_OF_PIPE = 0x01,
        DRAW_INDIRECT = 0x02,
        VERTEX_INPUT = 0x04,
        VERTEX_SHADER = 0x08,
        TESSELLATION_CONTROL_SHADER = 0x10,
        TESSELLATION_EVALUATION_SHADER = 0x20,
        GEOMETRY_SHADER = 0x40,
        FRAGMENT_SHADER = 0x80,
        EARLY_FRAGMENT_TESTS = 0x100,
        LATE_FRAGMENT_TESTS = 0x200,
        COLOR_ATTACHMENT_OUTPUT = 0x400,
        COMPUTE_SHADER = 0x800,
        TRANSFER = 0x1000,
        BOTTOM_OF_PIPE = 0x2000,
        HOST = 0x4000,
        ALL_GRAPHICS_PIPELINE_STAGES = 0x8000,
        ALL_COMMANDS = 0x1'0000
    };

    enum struct DESCRIPTOR_TYPE {
        SAMPLER = 0,
        COMBINED_IMAGE_SAMPLER,
        SAMPLED_IMAGE,
        STORAGE_IMAGE,
        UNIFORM_TEXEL_BUFFER,
        STORAGE_TEXEL_BUFFER,
        UNIFORM_BUFFER,
        STORAGE_BUFFER,
        UNIFORM_BUFFER_DYNAMIC,
        STORAGE_BUFFER_DYNAMIC,
        INPUT_ATTACHMENT
    };

    enum struct CULL_MODE {
        NONE, FRONT, BACK, FRONT_AND_BACK = FRONT | BACK
    };

    enum struct POLYGON_FRONT_FACE {
        COUNTER_CLOCKWISE, CLOCKWISE
    };

    enum struct POLYGON_MODE {
        FILL, LINE, POINT
    };

    enum struct COMPARE_OPERATION {
        NEVER,
        LESS,
        EQUAL,
        LESS_OR_EQUAL,
        GREATER,
        NOT_EQUAL,
        GREATER_OR_EQUAL,
        ALWAYS
    };

    enum struct STENCIL_OPERATION {
        KEEP = 0,
        ZERO,
        REPLACE,
        INCREMENT_AND_CLAMP,
        DECREMENT_AND_CLAMP,
        INVERT,
        INCREMENT_AND_WRAP,
        DECREMENT_AND_WRAP
    };

    enum struct LOGIC_OPERATION {
        CLEAR,
        AND,
        AND_REVERSE,
        COPY,
        AND_INVERTED,
        NO_OP,
        XOR,
        OR,
        NOR,
        EQUIVALENT,
        INVERT,
        OR_REVERSE,
        COPY_INVERTED,
        OR_INVERTED,
        NAND,
        SET
    };

    enum struct BLEND_FACTOR {
        ZERO,
        ONE,
        SRC_COLOR,
        ONE_MINUS_SRC_COLOR,
        DST_COLOR,
        ONE_MINUS_DST_COLOR,
        SRC_ALPHA,
        ONE_MINUS_SRC_ALPHA,
        DST_ALPHA,
        ONE_MINUS_DST_ALPHA,
        CONSTANT_COLOR,
        ONE_MINUS_CONSTANT_COLOR,
        CONSTANT_ALPHA,
        ONE_MINUS_CONSTANT_ALPHA,
        SRC_ALPHA_SATURATE,
        SRC1_COLOR,
        ONE_MINUS_SRC1_COLOR,
        SRC1_ALPHA,
        ONE_MINUS_SRC1_ALPHA
    };

    enum struct BLEND_OPERATION {
        ADD,
        SUBTRACT
    };

    enum struct COLOR_COMPONENT {
        R = 0x01,
        G = 0x02,
        B = 0x04,
        A = 0x08,

        RGB = R | G | B,
        RGBA = R | G | B | A
    };

    enum struct IMAGE_LAYOUT {
        UNDEFINED = 0,
        GENERAL,
        COLOR_ATTACHMENT,
        DEPTH_STENCIL_ATTACHMENT,
        DEPTH_STENCIL_READ_ONLY,
        SHADER_READ_ONLY,
        TRANSFER_SOURCE,
        TRANSFER_DESTINATION,
        PREINITIALIZED,
        DEPTH_READ_ONLY_STENCIL_ATTACHMENT,
        DEPTH_ATTACHMENT_STENCIL_READ_ONLY,

        PRESENT_SOURCE
    };

    enum struct IMAGE_TILING {
        OPTIMAL = 0,
        LINEAR
    };

    enum struct IMAGE_TYPE {
        TYPE_1D = 0,
        TYPE_2D,
        TYPE_3D
    };

    enum struct IMAGE_VIEW_TYPE {
        TYPE_1D = 0,
        TYPE_2D,
        TYPE_3D,
        TYPE_CUBE,
        TYPE_1D_ARRAY,
        TYPE_2D_ARRAY,
        TYPE_CUBE_ARRAY
    };

    enum struct TEXTURE_FILTER {
        NEAREST = 0,
        LINEAR = 1,
        CUBIC = 2
    };

    enum struct TEXTURE_MIPMAP_MODE {
        NEAREST = 0,
        LINEAR = 1,
    };

    enum struct MEMORY_PROPERTY_TYPE {
        DEVICE_LOCAL = 0x01,
        HOST_VISIBLE = 0x02,
        HOST_COHERENT = 0x04,
        HOST_CACHED = 0x08,
        LAZILY_ALLOCATED = 0x10
    };

    enum struct FORMAT {
        UNDEFINED = 0,

        RG4_UNORM_PACK8,
        RGBA4_UNORM_PACK16,
        BGRA4_UNORM_PACK16,
        B5G6R5_UNORM_PACK16,
        RGB5A1_UNORM_PACK16,
        BGR5A1_UNORM_PACK16,
        E5BGR9_UFLOAT_PACK32,

        R5G6B5_UNORM_PACK16,
        A1RGB5_UNORM_PACK16,

        R8_UNORM,
        R8_SNORM,
        RG8_UNORM,
        RG8_SNORM,
        RGBA8_UNORM,
        RGBA8_SNORM,
        BGRA8_UNORM,
        ABGR8_UNORM_PACK32,
        ABGR8_SNORM_PACK32,
        A2BGR10_UNORM_PACK32,
        R16_UNORM,
        R16_SNORM,
        R16_SFLOAT,
        RG16_UNORM,
        RG16_SNORM,
        RG16_SFLOAT,
        RGBA16_UNORM,
        RGBA16_SNORM,
        RGBA16_SFLOAT,
        RG32_SFLOAT,
        RGBA32_SFLOAT,
        B10GR11_UFLOAT_PACK32,

        R8_USCALED,
        R8_SSCALED,
        RG8_USCALED,
        RG8_SSCALED,
        RGB8_UNORM,
        RGB8_SNORM,
        RGB8_USCALED,
        RGB8_SSCALED,
        RGB8_UINT,
        RGB8_SINT,
        RGB8_SRGB,
        BGR8_UNORM,
        BGR8_SNORM,
        BGR8_USCALED,
        BGR8_SSCALED,
        BGR8_UINT,
        BGR8_SINT,
        BGR8_SRGB,
        RGBA8_USCALED,
        RGBA8_SSCALED,
        BGRA8_USCALED,
        BGRA8_SSCALED,
        ABGR8_USCALED_PACK32,
        ABGR8_SSCALED_PACK32,
        A2RGB10_SNORM_PACK32,
        A2RGB10_USCALED_PACK32,
        A2RGB10_SSCALED_PACK32,
        A2RGB10_SINT_PACK32,
        A2BGR10_SNORM_PACK32,
        A2BGR10_USCALED_PACK32,
        A2BGR10_SSCALED_PACK32,
        A2BGR10_SINT_PACK32,
        R16_USCALED,
        R16_SSCALED,
        RG16_USCALED,
        RG16_SSCALED,
        RGB16_UNORM,
        RGB16_SNORM,
        RGB16_USCALED,
        RGB16_SSCALED,
        RGB16_UINT,
        RGB16_SINT,
        RGB16_SFLOAT,
        RGBA16_USCALED,
        RGBA16_SSCALED,
        R64_UINT,
        R64_SINT,
        R64_SFLOAT,
        RG64_UINT,
        RG64_SINT,
        RG64_SFLOAT,
        RGB64_UINT,
        RGB64_SINT,
        RGB64_SFLOAT,
        RGBA64_UINT,
        RGBA64_SINT,
        RGBA64_SFLOAT,

        R8_UINT,
        R8_SINT,
        RG8_UINT,
        RG8_SINT,
        RG8_SRGB,
        RGBA8_UINT,
        RGBA8_SINT,
        ABGR8_UINT_PACK32,
        ABGR8_SINT_PACK32,
        A2BGR10_UINT_PACK32,
        R16_UINT,
        R16_SINT,
        RG16_UINT,
        RG16_SINT,
        RGBA16_UINT,
        RGBA16_SINT,
        RG32_UINT,
        RG32_SINT,
        RGBA32_UINT,
        RGBA32_SINT,

        R8_SRGB,
        BC1_RGB_UNORM_BLOCK,
        BC1_RGB_SRGB_BLOCK,
        BC1_RGBA_UNORM_BLOCK,
        BC1_RGBA_SRGB_BLOCK,
        BC2_UNORM_BLOCK,
        BC2_SRGB_BLOCK,
        BC3_UNORM_BLOCK,
        BC3_SRGB_BLOCK,
        BC4_UNORM_BLOCK,
        BC4_SNORM_BLOCK,
        BC5_UNORM_BLOCK,
        BC5_SNORM_BLOCK,
        BC6H_UFLOAT_BLOCK,
        BC6H_SFLOAT_BLOCK,
        BC7_UNORM_BLOCK,
        BC7_SRGB_BLOCK,

        RGBA8_SRGB,
        BGRA8_SRGB,
        ABGR8_SRGB_PACK32,

        BGRA8_SNORM,

        BGRA8_UINT,
        BGRA8_SINT,
        A2RGB10_UINT_PACK32,

        A2RGB10_UNORM_PACK32,

        R32_UINT,
        R32_SINT,

        R32_SFLOAT,

        RGB32_UINT,
        RGB32_SINT,

        RGB32_SFLOAT,

        D16_UNORM,
        D16_UNORM_S8_UINT,
        D24_UNORM_S8_UINT,
        X8_D24_UNORM_PACK32,
        D32_SFLOAT,
        D32_SFLOAT_S8_UINT,

        GBGR8_422_UNORM,
        BGRG8_422_UNORM,
        GBGR10X6_422_UNORM_4PACK16,
        BGRG10X6_422_UNORM_4PACK16,
        GBGR12X4_422_UNORM_4PACK16,
        BGRG12X4_422_UNORM_4PACK16,
        GBGR16_422_UNORM,
        BGRG16_422_UNORM,

        G8_B8_R8_3PLANE_420_UNORM,
        G8_B8R8_2PLANE_420_UNORM,
        G8_B8_R8_3PLANE_422_UNORM,
        G8_B8R8_2PLANE_422_UNORM,
        G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,
        G10X6_BR10X6_2PLANE_420_UNORM_3PACK16,
        G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,
        G10X6_BR10X6_2PLANE_422_UNORM_3PACK16,
        G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,
        G12X4_BR12X4_2PLANE_420_UNORM_3PACK16,
        G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,
        G12X4_BR12X4_2PLANE_422_UNORM_3PACK16,
        G16_B16_R16_3PLANE_420_UNORM,
        G16_BR16_2PLANE_420_UNORM,
        G16_B16_R16_3PLANE_422_UNORM,
        G16_BR16_2PLANE_422_UNORM,

        G8_B8_R8_3PLANE_444_UNORM,
        G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,
        G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,
        G16_B16_R16_3PLANE_444_UNORM,

        R10X6_UNORM_PACK16,
        RG10X6_UNORM_2PACK16,
        RGBA10X6_UNORM_4PACK16,
        R12X4_UNORM_PACK16,
        RG12X4_UNORM_2PACK16,
        RGBA12X4_UNORM_4PACK16
    };

    enum struct INDEX_TYPE {
        UNDEFINED = 0,
        UINT_16,
        UINT_32
    };

    enum struct NUMERIC_FORMAT {
        UNDEFINED = 0,
        NORMALIZED = 0x01,
        SCALED = 0x02,
        INT = 0x04,
        FLOAT = 0x08,
        SRGB = 0x10
    };

    enum struct FORMAT_FEATURE {
        SAMPLED_IMAGE = 0x00001,
        STORAGE_IMAGE = 0x00002,
        STORAGE_IMAGE_ATOMIC = 0x00004,
        UNIFORM_TEXEL_BUFFER = 0x00008,
        STORAGE_TEXEL_BUFFER = 0x00010,
        STORAGE_TEXEL_BUFFER_ATOMIC = 0x00020,
        VERTEX_BUFFER = 0x00040,
        COLOR_ATTACHMENT = 0x00080,
        COLOR_ATTACHMENT_BLEND = 0x00100,
        DEPTH_STENCIL_ATTACHMENT = 0x00200,
        BLIT_SOURCE = 0x00400,
        BLIT_DESTINATION = 0x00800,
        SAMPLED_IMAGE_FILTER_LINEAR = 0x01000,
        TRANSFER_SOURCE = 0x04000,
        TRANSFER_DESTINATION = 0x08000,
        MIDPOINT_CHROMA_SAMPLES = 0x2'0000,
        SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER = 0x4'0000,
        SAMPLED_IMAGE_YCBCR_CONVERSION_SEPARATE_RECONSTRUCTION_FILTER = 0x8'0000,
        SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT = 0x0010'0000,
        SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_FORCEABLE = 0x0020'0000,
        DISJOINT = 0x0040'0000,
        COSITED_CHROMA_SAMPLES = 0x0080'0000,
        SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMAGE = 0x02000
    };

    enum struct ATTACHMENT_LOAD_TREATMENT {
        LOAD = 0,
        CLEAR,
        DONT_CARE
    };

    enum struct ATTACHMENT_STORE_TREATMENT {
        STORE = 0,
        DONT_CARE
    };

    enum struct COLOR_SPACE {
        SRGB_NONLINEAR = 0,
        EXTENDED_SRGB_LINEAR,
        EXTENDED_SRGB_NONLINEAR,
        BT709_LINEAR,
        BT709_NONLINEAR,
        ADOBE_RGB_LINEAR,
        ADOBE_RGB_NONLINEAR,
        HDR10_ST2084,
        HDR10_HLG,
        PASS_THROUGH
    };

    enum struct BUFFER_USAGE {
        TRANSFER_SOURCE = 0x01,
        TRANSFER_DESTINATION = 0x02,
        UNIFORM_TEXEL_BUFFER = 0x04,
        STORAGE_TEXEL_BUFFER = 0x08,
        UNIFORM_BUFFER = 0x10,
        STORAGE_BUFFER = 0x20,
        INDEX_BUFFER = 0x40,
        VERTEX_BUFFER = 0x80,
        INDIRECT_BUFFER = 0x100
    };

    enum struct IMAGE_USAGE {
        TRANSFER_SOURCE = 0x01,
        TRANSFER_DESTINATION = 0x02,
        SAMPLED = 0x04,
        STORAGE = 0x08,
        COLOR_ATTACHMENT = 0x10,
        DEPTH_STENCIL_ATTACHMENT = 0x20,
        TRANSIENT_ATTACHMENT = 0x40,
        INPUT_ATTACHMENT = 0x80
    };

    enum struct QUEUE_CAPABILITY {
        GRAPHICS = 0x01,
        COMPUTE = 0x02,
        TRANSFER = 0x04
    };

    enum struct PRESENTATION_MODE {
        IMMEDIATE = 0,
        MAILBOX,
        FIFO,
        FIFO_RELAXED
    };

    enum struct IMAGE_ASPECT {
        COLOR_BIT = 0x01,
        DEPTH_BIT = 0x02,
        STENCIL_BIT = 0x04
    };

    enum struct MEMORY_ACCESS_TYPE {
        INDIRECT_COMMAND_READ = 0x01,
        INDEX_READ = 0x02,
        VERTEX_ATTRIBUTE_READ = 0x04,
        UNIFORM_READ = 0x08,
        INPUT_ATTACHMENT_READ = 0x10,
        SHADER_READ = 0x020,
        SHADER_WRITE = 0x040,
        COLOR_ATTACHMENT_READ = 0x080,
        COLOR_ATTACHMENT_WRITE = 0x0100,
        DEPTH_STENCIL_ATTACHMENT_READ = 0x0200,
        DEPTH_STENCIL_ATTACHMENT_WRITE = 0x0400,
        TRANSFER_READ = 0x0800,
        TRANSFER_WRITE = 0x1000,
        HOST_READ = 0x02000,
        HOST_WRITE = 0x04000,
        MEMORY_READ = 0x08000,
        MEMORY_WRITE = 0x1'0000
    };

    enum struct RESOURCE_SHARING_MODE {
        EXCLUSIVE = 0,
        CONCURRENT
    };
}

namespace graphics
{
    template<mpl::enumeration T>
    auto constexpr operator| (T lhs, T rhs)
    {
        using E = std::underlying_type_t<T>;
        return static_cast<T>(static_cast<E>(lhs) | static_cast<E>(rhs));
    }

    template<mpl::enumeration T>
    auto constexpr operator& (T lhs, T rhs)
    {
        using E = std::underlying_type_t<T>;
        return static_cast<T>(static_cast<E>(lhs) & static_cast<E>(rhs));
    }
}

namespace renderer
{
    struct extent final {
        std::uint32_t width, height;

        template<class T> requires std::same_as<std::remove_cvref_t<T>, renderer::extent>
        auto constexpr operator== (T &&rhs) const
        {
            return width == rhs.width && height == rhs.height;
        }
    };
}

namespace graphics
{
    using format_instance = std::variant<
        std::array<std::int8_t, 1>,
        std::array<std::int8_t, 2>,
        std::array<std::int8_t, 3>,
        std::array<std::int8_t, 4>,

        std::array<std::uint8_t, 1>,
        std::array<std::uint8_t, 2>,
        std::array<std::uint8_t, 3>,
        std::array<std::uint8_t, 4>,

        std::array<std::int16_t, 1>,
        std::array<std::int16_t, 2>,
        std::array<std::int16_t, 3>,
        std::array<std::int16_t, 4>,

        std::array<std::uint16_t, 1>,
        std::array<std::uint16_t, 2>,
        std::array<std::uint16_t, 3>,
        std::array<std::uint16_t, 4>,

        std::array<std::int32_t, 1>,
        std::array<std::int32_t, 2>,
        std::array<std::int32_t, 3>,
        std::array<std::int32_t, 4>,

        std::array<std::uint32_t, 1>,
        std::array<std::uint32_t, 2>,
        std::array<std::uint32_t, 3>,
        std::array<std::uint32_t, 4>,

    #if defined(BOOST_FLOAT16_C)
        std::array<boost::float16_t, 1>,
        std::array<boost::float16_t, 2>,
        std::array<boost::float16_t, 3>,
        std::array<boost::float16_t, 4>
    #endif

        std::array<boost::float32_t, 1>,
        std::array<boost::float32_t, 2>,
        std::array<boost::float32_t, 3>,
        std::array<boost::float32_t, 4>,

        std::array<std::int64_t, 1>,
        std::array<std::int64_t, 2>,
        std::array<std::int64_t, 3>,
        std::array<std::int64_t, 4>,

        std::array<std::uint64_t, 1>,
        std::array<std::uint64_t, 2>,
        std::array<std::uint64_t, 3>,
        std::array<std::uint64_t, 4>,

        std::array<boost::float64_t, 1>,
        std::array<boost::float64_t, 2>,
        std::array<boost::float64_t, 3>,
        std::array<boost::float64_t, 4>
    >;

    using index_type_instance = std::variant<
        std::uint16_t,
        std::uint32_t
    >;

    graphics::NUMERIC_FORMAT numeric_format(graphics::FORMAT format);

    std::optional<graphics::format_instance> instantiate_format(graphics::FORMAT format);

    std::size_t size_bytes(graphics::FORMAT format);
    std::size_t size_bytes(graphics::INDEX_TYPE index_type);
    std::size_t size_bytes(graphics::vertex_layout const &vertex_layout);
}
