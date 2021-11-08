#if 0
#include <tuple>
#include <ranges>
#include <variant>
#include <functional>

#include <string>
using namespace std::string_literals;

#include "utility/mpl.hxx"
#include "utility/helpers.hxx"
#include "utility/exceptions.hxx"

#include "math/math.hxx"
#include "math/pack-unpack.hxx"

#include "graphics/graphics.hxx"

#include "primitives/primitives.hxx"


namespace
{
	;
}


namespace primitives
{
    std::uint32_t calculate_sphere_vertices_count(primitives::sphere_create_info const &create_info)
    {
        auto const hsegments = create_info.hsegments;
        auto const vsegments = create_info.vsegments;

        if (hsegments < 1 || vsegments < 1)
            throw resource::exception("invalid plane segments' values"s);

        bool is_primitive_indexed = create_info.index_buffer_type != graphics::INDEX_TYPE::UNDEFINED;

        if (is_primitive_indexed)
            return (hsegments + 1) * (vsegments + 1);

        switch (create_info.topology) {
            case graphics::PRIMITIVE_TOPOLOGY::POINTS:
                return (hsegments + 1) * (vsegments + 1);

            case graphics::PRIMITIVE_TOPOLOGY::LINES:
                return (hsegments - 1) * 2 + (vsegments - 1) * 2 + 4 * 2;

            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLES:
                return hsegments * vsegments * 2 * 3;

            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP:
                return (hsegments + 1) * 2 * vsegments + (vsegments - 1) * 2;

            default:
                throw resource::exception("unsupported primitive topology"s);
        }
    }

    std::uint32_t calculate_sphere_indices_count(primitives::sphere_create_info const &create_info)
    {
        auto const hsegments = create_info.hsegments;
        auto const vsegments = create_info.vsegments;

        if (hsegments * vsegments < 1)
            throw resource::exception("invalid plane segments' values"s);

        bool is_primitive_indexed = create_info.index_buffer_type != graphics::INDEX_TYPE::UNDEFINED;

        if (!is_primitive_indexed)
            return 0;

        switch (create_info.topology) {
            case graphics::PRIMITIVE_TOPOLOGY::POINTS:
                return (hsegments + 1) * (vsegments + 1);

            case graphics::PRIMITIVE_TOPOLOGY::LINES:
                return (hsegments - 1) * 2 + (vsegments - 1) * 2 + 4 * 2;

            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLES:
                return hsegments * vsegments * 2 * 3;

            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP:
                return (hsegments + 1) * 2 * vsegments + (vsegments - 1) * 2;

            default:
                throw resource::exception("unsupported primitive topology"s);
        }
    }

    void generate_sphere_indexed(primitives::sphere_create_info const &create_info, std::span<std::byte> vertex_buffer,
                                std::span<std::byte> index_buffer, glm::vec4 const &color)
    {
        auto indices_count = calculate_sphere_indices_count(create_info);

        switch (create_info.index_buffer_type) {
            case graphics::INDEX_TYPE::UINT_16:
                {
                    using pointer_type = typename std::add_pointer_t<std::uint16_t>;
                    auto it = reinterpret_cast<pointer_type>(std::to_address(std::data(index_buffer)));

                    generate_indices(create_info, it, indices_count);
                }
                break;

            case graphics::INDEX_TYPE::UINT_32:
                {
                    using pointer_type = typename std::add_pointer_t<std::uint32_t>;
                    auto it = reinterpret_cast<pointer_type>(std::to_address(std::data(index_buffer)));

                    generate_indices(create_info, it, indices_count);
                }
                break;

            default:
                throw resource::exception("unsupported index instance type"s);
        }

        generate_sphere(create_info, vertex_buffer, color);
    }

    void generate_sphere(primitives::sphere_create_info const &create_info, std::span<std::byte> vertex_buffer, glm::vec4 const &color)
    {
        auto &&vertex_layout = create_info.vertex_layout;

        auto vertices_count = calculate_plane_vertices_count(create_info);
        auto vertex_size = static_cast<std::uint32_t>(vertex_layout.size_bytes);

        auto &&attributes = vertex_layout.attributes;

        for (std::size_t offset_in_bytes = 0; auto &&attribute : attributes) {
            if (auto format_inst = graphics::instantiate_format(attribute.format); format_inst) {
                std::visit([&] <typename T> (T &&)
                {
                    using type = typename std::remove_cvref_t<T>;
                    using pointer_type = typename std::add_pointer_t<type>;

                    auto data = reinterpret_cast<pointer_type>(std::to_address(std::data(vertex_buffer)) + offset_in_bytes);

                    offset_in_bytes += sizeof(type);

                    auto it = strided_bidirectional_iterator<type>{data, vertex_size};

                    switch (attribute.semantic) {
                        case vertex::SEMANTIC::POSITION:
                            //generate_positions(create_info, attribute.format, it, vertices_count);
                            break;

                        case vertex::SEMANTIC::NORMAL:
                            //generate_normals(attribute.format, it, vertices_count);
                            break;

                        case vertex::SEMANTIC::TEXCOORD_0:
                            //generate_texcoords(create_info, attribute.format, it, vertices_count);
                            break;

                        case vertex::SEMANTIC::COLOR_0:
                            //generate_colors(color, attribute.format, it, vertices_count);
                            break;

                        default:
                            break;
                    }

                }, *format_inst);
            }

            else throw resource::exception("unsupported attribute format"s);
        }
    }
}
#endif
