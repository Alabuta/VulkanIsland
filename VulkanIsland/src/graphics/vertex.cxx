#include <string>
using namespace std::string_literals;

#include <boost/functional/hash.hpp>

#include <fmt/format.h>

#include "vertex.hxx"


namespace vertex
{
    std::size_t compile_vertex_attributes(graphics::vertex_layout &vertex_layout, vertex::SEMANTIC semantic, graphics::FORMAT format)
    {
        vertex_layout.attributes.push_back(graphics::vertex_attribute{semantic, format});

        if (auto format_inst = graphics::instantiate_format(format); format_inst) {
            return std::visit([] (auto &&format_inst)
            {
                return sizeof(std::remove_cvref_t<decltype(format_inst)>);

            }, *format_inst);
        }

        else throw graphics::exception("unsupported format");
    }
}

namespace graphics
{
    std::uint32_t get_vertex_attribute_semantic_index(graphics::vertex_attribute const &vertex_attribute)
    {
        return static_cast<std::uint32_t>(vertex_attribute.semantic);
        /*return std::visit([] (auto semantic)
        {
            using S = std::remove_cvref_t<decltype(semantic)>;
            return S::index;

        }, vertex_attribute.semantic);*/
    }
}

namespace graphics
{
    std::size_t hash<graphics::vertex_attribute>::operator() (graphics::vertex_attribute const &attribute) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, attribute.semantic);
        //boost::hash_combine(seed, attribute.semantic.index());
        boost::hash_combine(seed, attribute.format);

        return seed;
    }

    std::size_t hash<graphics::vertex_layout>::operator() (graphics::vertex_layout const &layout) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, layout.size_in_bytes);

        graphics::hash<graphics::vertex_attribute> constexpr attribute_hasher;

        for (auto &&attribute : layout.attributes)
            boost::hash_combine(seed, attribute_hasher(attribute));

        return seed;
    }

    std::size_t hash<graphics::vertex_input_binding>::operator() (graphics::vertex_input_binding const &binding) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, binding.binding_index);
        boost::hash_combine(seed, binding.stride_in_bytes);
        boost::hash_combine(seed, binding.input_rate);

        return seed;
    }

    std::size_t hash<graphics::vertex_input_attribute>::operator() (graphics::vertex_input_attribute const &input_attribute) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, input_attribute.location_index);
        boost::hash_combine(seed, input_attribute.binding_index);
        boost::hash_combine(seed, input_attribute.offset_in_bytes);
        boost::hash_combine(seed, input_attribute.format);

        return seed;
    }
}

namespace graphics
{
    template<>
    std::string to_string(vertex::SEMANTIC const &semantic)
    {
        /*return std::visit([] (auto semantic)
        {*/
            switch (semantic) {
                case vertex::SEMANTIC::POSITION:
                    return "position"s;

                case vertex::SEMANTIC::NORMAL:
                    return "normal"s;

                case vertex::SEMANTIC::TEXCOORD_0:
                    return "texcoord_0"s;

                case vertex::SEMANTIC::TEXCOORD_1:
                    return "texcoord_1"s;

                case vertex::SEMANTIC::TANGENT:
                    return "tangent"s;

                case vertex::SEMANTIC::COLOR_0:
                    return "color_0"s;

                case vertex::SEMANTIC::JOINTS_0:
                    return "joints_0"s;

                case vertex::SEMANTIC::WEIGHTS_0:
                    return "weights_0"s;

                default:
                    return ""s;
            }

        //}, semantic);
    }

    template<>
    std::string to_string(graphics::FORMAT const &format)
    {
        switch (format) {
            // Signed and usigned byte integer formats.
            case graphics::FORMAT::R8_SNORM:			return "r8i_norm"s;
            case graphics::FORMAT::RG8_SNORM:			return "rg8i_norm"s;
            case graphics::FORMAT::RGB8_SNORM:			return "rgb8i_norm"s;
            case graphics::FORMAT::RGBA8_SNORM:			return "rgba8i_norm"s;
            case graphics::FORMAT::R8_UNORM:			return "r8ui_norm"s;
            case graphics::FORMAT::RG8_UNORM:			return "rg8ui_norm"s;
            case graphics::FORMAT::RGB8_UNORM:			return "rgb8ui_norm"s;
            case graphics::FORMAT::RGBA8_UNORM:			return "rgba8ui_norm"s;
            case graphics::FORMAT::BGRA8_UNORM:			return "bgra8ui_norm"s;
            case graphics::FORMAT::RGBA8_SRGB:			return "rgba8_srgb"s;
            case graphics::FORMAT::BGRA8_SRGB:			return "bgra8_srgb"s;
            case graphics::FORMAT::R8_SINT:			    return "r8i"s;
            case graphics::FORMAT::RG8_SINT:			return "rg8i"s;
            case graphics::FORMAT::RGB8_SINT:			return "rgb8i"s;
            case graphics::FORMAT::RGBA8_SINT:			return "rgba8i"s;
            case graphics::FORMAT::R8_UINT:			    return "r8ui"s;
            case graphics::FORMAT::RG8_UINT:			return "rg8ui"s;
            case graphics::FORMAT::RGB8_UINT:			return "rgb8ui"s;
            case graphics::FORMAT::RGBA8_UINT:			return "rgba8ui"s;

            // Signed and unsigned two byte integer formats.
            case graphics::FORMAT::R16_SNORM:			return "r16i_norm"s;
            case graphics::FORMAT::RG16_SNORM:			return "rg16i_norm"s;
            case graphics::FORMAT::RGB16_SNORM:			return "rgb16i_norm"s;
            case graphics::FORMAT::RGBA16_SNORM:		return "rgba16i_norm"s;
            case graphics::FORMAT::R16_UNORM:			return "r16ui_norm"s;
            case graphics::FORMAT::RG16_UNORM:			return "rg16ui_norm"s;
            case graphics::FORMAT::RGB16_UNORM:			return "rgb16ui_norm"s;
            case graphics::FORMAT::RGBA16_UNORM:		return "rgba16ui_norm"s;
            case graphics::FORMAT::R16_SINT:			return "r16i"s;
            case graphics::FORMAT::RG16_SINT:			return "rg16i"s;
            case graphics::FORMAT::RGB16_SINT:			return "rgb16i"s;
            case graphics::FORMAT::RGBA16_SINT:			return "rgba16i"s;
            case graphics::FORMAT::R16_UINT:			return "r16ui"s;
            case graphics::FORMAT::RG16_UINT:			return "rg16ui"s;
            case graphics::FORMAT::RGB16_UINT:			return "rgb16ui"s;
            case graphics::FORMAT::RGBA16_UINT:			return "rgba16ui"s;

            // Signed and unsigned four byte integer formats.
            case graphics::FORMAT::R32_SINT:			return "r32i"s;
            case graphics::FORMAT::RG32_SINT:			return "rg32i"s;
            case graphics::FORMAT::RGB32_SINT:			return "rgb32i"s;
            case graphics::FORMAT::RGBA32_SINT:			return "rgba32i"s;
            case graphics::FORMAT::R32_UINT:			return "r32ui"s;
            case graphics::FORMAT::RG32_UINT:			return "rg32ui"s;
            case graphics::FORMAT::RGB32_UINT:			return "rgb32ui"s;
            case graphics::FORMAT::RGBA32_UINT:			return "rgba32ui"s;

            // Two byte float formats.
            case graphics::FORMAT::R16_SFLOAT:			return "r16f"s;
            case graphics::FORMAT::RG16_SFLOAT:			return "rg16f"s;
            case graphics::FORMAT::RGB16_SFLOAT:		return "rgb16f"s;
            case graphics::FORMAT::RGBA16_SFLOAT:		return "rgba16f"s;

            // Four byte float formats.
            case graphics::FORMAT::R32_SFLOAT:			return "r32f"s;
            case graphics::FORMAT::RG32_SFLOAT:			return "rg32f"s;
            case graphics::FORMAT::RGB32_SFLOAT:		return "rgb32f"s;
            case graphics::FORMAT::RGBA32_SFLOAT:		return "rgba32f"s;

            // Depth-stenci formats.
            case graphics::FORMAT::D16_UNORM:			return "d16ui_norm"s;
            case graphics::FORMAT::D16_UNORM_S8_UINT:	return "d16ui_norm_s8ui"s;
            case graphics::FORMAT::D24_UNORM_S8_UINT:	return "d24ui_norm_s8ui"s;
            case graphics::FORMAT::X8_D24_UNORM_PACK32:	return "x8_d24ui_norm_pack32"s;
            case graphics::FORMAT::D32_SFLOAT:			return "d32f"s;
            case graphics::FORMAT::D32_SFLOAT_S8_UINT:	return "d32f_s8ui"s;

            // Special formats.
            case graphics::FORMAT::B10GR11_UFLOAT_PACK32:	return "b10gr11uf_pack32"s;

            case graphics::FORMAT::UNDEFINED:
            default:
                return ""s;
        }
    }

    template<>
    std::string to_string(graphics::vertex_layout const &layout)
    {
        std::string str;

        for (auto &&attribute : layout.attributes) {
            auto semantic = graphics::to_string(attribute.semantic);
            auto format = graphics::to_string(attribute.format);

            if (str.empty())
                str = fmt::format("{}:{}"s, semantic, format);

            else str = fmt::format("{}-{}:{}"s, str, semantic, format);
        }

        return str;
    }
}
