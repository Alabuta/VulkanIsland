#include <string>
using namespace std::string_literals;

#include "vertex.hxx"


namespace graphics
{
    std::uint32_t get_vertex_attribute_semantic_index(graphics::vertex_attribute const &vertex_attribute)
    {
        return std::visit([] (auto semantic)
        {
            using S = std::remove_cvref_t<decltype(semantic)>;
            return S::index;

        }, vertex_attribute.semantic);
    }

    graphics::FORMAT get_vertex_attribute_format(graphics::vertex_attribute const &vertex_attribute)
    {
        auto normalized = vertex_attribute.normalized;

        return std::visit([normalized] (auto &&type) -> graphics::FORMAT
        {
            using T = std::remove_cvref_t<decltype(type)>;

            using A = typename T::type;

            if constexpr (std::is_same_v<A, std::int8_t>) {
                if (normalized) {
                    switch (T::length) {
                        case 1: return graphics::FORMAT::R8_SNORM;
                        case 2: return graphics::FORMAT::RG8_SNORM;
                        case 3: return graphics::FORMAT::RGB8_SNORM;
                        case 4: return graphics::FORMAT::RGBA8_SNORM;
                        default: break;
                    }
                }

                else {
                    switch (T::length) {
                        case 1: return graphics::FORMAT::R8_SINT;
                        case 2: return graphics::FORMAT::RG8_SINT;
                        case 3: return graphics::FORMAT::RGB8_SINT;
                        case 4: return graphics::FORMAT::RGBA8_SINT;
                        default: break;
                    }
                }
            }

            else if constexpr (std::is_same_v<A, std::uint8_t>) {
                if (normalized) {
                    switch (T::length) {
                        case 1: return graphics::FORMAT::R8_UNORM;
                        case 2: return graphics::FORMAT::RG8_UNORM;
                        case 3: return graphics::FORMAT::RGB8_UNORM;
                        case 4: return graphics::FORMAT::RGBA8_UNORM;
                        default: break;
                    }
                }

                else {
                    switch (T::length) {
                        case 1: return graphics::FORMAT::R8_UINT;
                        case 2: return graphics::FORMAT::RG8_UINT;
                        case 3: return graphics::FORMAT::RGB8_UINT;
                        case 4: return graphics::FORMAT::RGBA8_UINT;
                        default: break;
                    }
                }
            }

            else if constexpr (std::is_same_v<A, std::int16_t>) {
                if (normalized) {
                    switch (T::length) {
                        case 1: return graphics::FORMAT::R16_SNORM;
                        case 2: return graphics::FORMAT::RG16_SNORM;
                        case 3: return graphics::FORMAT::RGB16_SNORM;
                        case 4: return graphics::FORMAT::RGBA16_SNORM;
                        default: break;
                    }
                }

                else {
                    switch (T::length) {
                        case 1: return graphics::FORMAT::R16_SINT;
                        case 2: return graphics::FORMAT::RG16_SINT;
                        case 3: return graphics::FORMAT::RGB16_SINT;
                        case 4: return graphics::FORMAT::RGBA16_SINT;
                        default: break;
                    }
                }
            }

            else if constexpr (std::is_same_v<A, std::uint16_t>) {
                if (normalized) {
                    switch (T::length) {
                        case 1: return graphics::FORMAT::R16_UNORM;
                        case 2: return graphics::FORMAT::RG16_UNORM;
                        case 3: return graphics::FORMAT::RGB16_UNORM;
                        case 4: return graphics::FORMAT::RGBA16_UNORM;
                        default: break;
                    }
                }

                else {
                    switch (T::length) {
                        case 1: return graphics::FORMAT::R16_UINT;
                        case 2: return graphics::FORMAT::RG16_UINT;
                        case 3: return graphics::FORMAT::RGB16_UINT;
                        case 4: return graphics::FORMAT::RGBA16_UINT;
                        default: break;
                    }
                }
            }

            else if constexpr (std::is_same_v<A, std::int32_t>) {
                switch (T::length) {
                    case 1: return graphics::FORMAT::R32_SINT;
                    case 2: return graphics::FORMAT::RG32_SINT;
                    case 3: return graphics::FORMAT::RGB32_SINT;
                    case 4: return graphics::FORMAT::RGBA32_SINT;
                    default: break;
                }
            }

            else if constexpr (std::is_same_v<A, std::uint32_t>) {
                switch (T::length) {
                    case 1: return graphics::FORMAT::R32_UINT;
                    case 2: return graphics::FORMAT::RG32_UINT;
                    case 3: return graphics::FORMAT::RGB32_UINT;
                    case 4: return graphics::FORMAT::RGBA32_UINT;
                    default: break;
                }
            }

            else if constexpr (std::is_same_v<A, float>) {
                switch (T::length) {
                    case 1: return graphics::FORMAT::R32_SFLOAT;
                    case 2: return graphics::FORMAT::RG32_SFLOAT;
                    case 3: return graphics::FORMAT::RGB32_SFLOAT;
                    case 4: return graphics::FORMAT::RGBA32_SFLOAT;
                    default: break;
                }
            }

            throw std::runtime_error("undefined format"s);

        }, vertex_attribute.type);
    }
}
