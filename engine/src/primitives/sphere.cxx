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
    template<std::size_t N, class T>
    std::array<T, N> generate_position(primitives::sphere_create_info const &create_info, glm::vec2 const &uv)
    {
        auto const wsegments = create_info.wsegments;
        auto const hsegments = create_info.hsegments;

        glm::vec3 point{
            0,
            std::cos(uv.v * std::numbers::pi),
            0
        };

        point = glm::normalize(point) * create_info.radius;

        if constexpr (N == 4)
            return std::array<T, N>{static_cast<float>(point.x), static_cast<float>(point.y), static_cast<float>(point.z), 1};

        else if constexpr (N == 3)
            return std::array<T, N>{static_cast<float>(point.x), static_cast<float>(point.y), static_cast<float>(point.z)};

        else throw resource::exception("unsupported components number"s);
    }

    template<std::size_t N, class T>
    void generate_positions(primitives::sphere_create_info const &create_info, graphics::FORMAT format,
                            strided_bidirectional_iterator<std::array<T, N>> it_begin, std::uint32_t vertices_count)
    {
        if constexpr (N == 2 || N == 3) {
            switch (graphics::numeric_format(format)) {
                case graphics::NUMERIC_FORMAT::FLOAT:
                    {
                        auto generator = std::bind(generate_position<N, T>, create_info, std::placeholders::_1);

                        if (create_info.index_buffer_type != graphics::INDEX_TYPE::UNDEFINED) {
                            std::generate_n(it_begin, vertices_count, [generator, i = 0u] () mutable
                            {
                                return generator(i++);
                            });
                        }

                        else generate_vertex(generator, create_info, it_begin, vertices_count);
                    }
                    break;

                default:
                    throw resource::exception("unsupported numeric format"s);
            }
        }

        else throw resource::exception("unsupported components number"s);
    }
}


namespace primitives
{
    std::uint32_t calculate_sphere_vertices_count(primitives::sphere_create_info const &create_info)
    {
        auto const wsegments = create_info.wsegments;
        auto const hsegments = create_info.hsegments;

        if (wsegments < 1 || hsegments < 1)
            throw resource::exception("invalid sphere segments' values"s);

        if (create_info.index_buffer_type == graphics::INDEX_TYPE::UNDEFINED)
            throw resource::exception("unsupported primitive topology"s);

        return wsegments * (hsegments - 1) + 2;
    }

    std::uint32_t calculate_sphere_indices_count(primitives::sphere_create_info const &create_info)
    {
        if (create_info.index_buffer_type == graphics::INDEX_TYPE::UNDEFINED)
            return 0;

        auto const wsegments = create_info.wsegments;
        auto const hsegments = create_info.hsegments;

        if (wsegments < 1 || hsegments < 1)
            throw resource::exception("invalid sphere segments' values"s);

        switch (create_info.topology) {
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLES:
                return wsegments * 2 * ((hsegments - 2) + 1) * 3;

            case graphics::PRIMITIVE_TOPOLOGY::POINTS:
            case graphics::PRIMITIVE_TOPOLOGY::LINES:
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP:
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

                    //generate_indices(create_info, it, indices_count);
                }
                break;

            case graphics::INDEX_TYPE::UINT_32:
                {
                    using pointer_type = typename std::add_pointer_t<std::uint32_t>;
                    auto it = reinterpret_cast<pointer_type>(std::to_address(std::data(index_buffer)));

                    //generate_indices(create_info, it, indices_count);
                }
                break;

            default:
                throw resource::exception("unsupported index instance type"s);
        }

        generate_sphere(create_info, vertex_buffer);
    }

    void generate_sphere(primitives::sphere_create_info const &create_info, std::span<std::byte> vertex_buffer)
    {
        auto &&vertex_layout = create_info.vertex_layout;

        auto vertices_count = calculate_sphere_vertices_count(create_info);
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
