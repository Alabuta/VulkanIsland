#include <array>
#include <tuple>
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


namespace primitives
{
    template<std::size_t N, class T, class F>
    void generate_vertex(F generator, primitives::box_create_info const &create_info,
                         strided_bidirectional_iterator<std::array<T, N>> it_begin, std::size_t vertex_count)
    {
        /*auto const hsegments = create_info.hsegments;
        auto const vsegments = create_info.vsegments;
        auto const dsegments = create_info.dsegments;*/

        auto it_end = std::next(it_begin, static_cast<std::ptrdiff_t>(vertex_count));

        auto const faces = std::array{
            std::tuple{create_info.dsegments, create_info.vsegments}
        };

        for (auto [hsegments, vsegments] : faces) {
            auto const vertices_per_strip = (hsegments + 1) * 2;
            auto const extra_vertices_per_strip = static_cast<std::uint32_t>(vsegments > 1) * 2;

            for (auto strip_index = 0u; strip_index < vsegments; ++strip_index) {
                auto it = std::next(it_begin, strip_index * (vertices_per_strip + extra_vertices_per_strip));

                std::generate_n(it, 2, [&, offset = 0u] () mutable
                {
                    auto vertex_index = (strip_index + offset++) * (hsegments + 1);

                    return generator(vertex_index);
                });

                std::generate_n(std::next(it, 2), vertices_per_strip - 2, [&, triangle_index = strip_index * hsegments * 2] () mutable
                {
                    auto quad_index = triangle_index / 2;

                    auto column = quad_index % hsegments + 1;
                    auto row = quad_index / hsegments + (triangle_index % 2);

                    ++triangle_index;

                    auto vertex_index = row * (hsegments + 1) + column;

                    return generator(vertex_index);
                });

                it = std::next(it, vertices_per_strip);

                if (it < it_end) {
                    std::generate_n(it, 2, [&, i = 0u] () mutable {
                        auto vertex_index = (strip_index + 1) * (hsegments + 1) + hsegments * (1 - i++);

                        return generator(vertex_index);
                    });
                }
            }

            it_begin = it;
        }
    }
    
    template<std::size_t N, class T>
    std::array<T, N>
    generate_position(primitives::box_create_info const &create_info, std::size_t vertex_index)
    {
        auto const hsegments = create_info.hsegments;
        auto const vsegments = create_info.vsegments;
        auto const dsegments = create_info.dsegments;

        auto const width = create_info.width;
        auto const height = create_info.height;
        auto const depth = create_info.depth;

        auto [x0, y0] = std::pair{-width / 2.f, height / 2.f};
        auto [step_x, step_y] = std::pair{width / static_cast<float>(hsegments), -height / static_cast<float>(vsegments)};

        auto x = static_cast<T>(x0 + static_cast<float>(vertex_index % (hsegments + 1u)) * step_x);
        auto y = static_cast<T>(y0 + static_cast<float>(vertex_index / (hsegments + 1u)) * step_y);

        if constexpr (N == 4)
            return std::array<T, N>{x, y, 0, 1};

        else if constexpr (N == 3)
            return std::array<T, N>{x, y, 0};

        else if constexpr (N == 2)
            return std::array<T, N>{x, y};

        else throw resource::exception("unsupported components number"s);

        return { };
    }

    template<std::size_t N, class T>
    void generate_positions(primitives::box_create_info const &create_info, graphics::FORMAT attribute_format,
                            strided_bidirectional_iterator<std::array<T, N>> it_begin, std::size_t vertex_count)
    {
        if constexpr (N == 2 || N == 3) {
            switch (graphics::numeric_format(attribute_format)) {
                case graphics::NUMERIC_FORMAT::FLOAT:
                    generate_vertex<N, T>(
                        std::bind(generate_position<N, T>, create_info, std::placeholders::_1),
                        create_info, it_begin, vertex_count
                    );
                    break;

                default:
                    throw resource::exception("unsupported numeric format"s);
                    break;
            }
        }

        else throw resource::exception("unsupported components number"s);
    }

    std::vector<std::byte>
    generate_box_indexed(primitives::box_create_info const &create_info, glm::vec4 const &)
    {
        if (create_info.topology != graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP)
            throw resource::exception("unsupported primitive topology"s);

        auto &&vertex_layout = create_info.vertex_layout;

        std::uint32_t vertex_number = calculate_box_vertices_number(create_info);
        std::uint32_t vertex_size = static_cast<std::uint32_t>(vertex_layout.size_in_bytes);

        //std::uint32_t indices_number = calculate_box_indices_number(create_info);

        std::vector<std::byte> bytes(vertex_size * vertex_number);

        auto &&attributes = vertex_layout.attributes;

        std::size_t offset_in_bytes = 0;

        for (auto &&attribute : attributes) {
            if (auto format_inst = graphics::instantiate_format(attribute.format); format_inst) {
                std::visit([&] (auto &&format_inst)
                {
                    using type = typename std::remove_cvref_t<decltype(format_inst)>;
                    using pointer_type = typename std::add_pointer_t<type>;

                    auto data = reinterpret_cast<pointer_type>(std::data(bytes) + offset_in_bytes);

                    offset_in_bytes += sizeof(type);

                    auto it = strided_bidirectional_iterator<type>{data, vertex_size};

                    switch (attribute.semantic) {
                        case vertex::SEMANTIC::POSITION:
                            generate_positions(create_info, attribute.format, it, vertex_number);
                            break;

                        case vertex::SEMANTIC::NORMAL:
                            //generate_normals(attribute.format, it, vertex_number);
                            break;

                        case vertex::SEMANTIC::TEXCOORD_0:
                            //generate_texcoords(attribute.format, it, hsegments, vsegments, vertex_number);
                            break;

                        case vertex::SEMANTIC::COLOR_0:
                            //generate_colors(color, attribute.format, it, hsegments, vsegments, vertex_number);
                            break;

                        default:
                            break;
                    }

                }, *format_inst);
            }

            else throw resource::exception("unsupported attribute format"s);
        }

        return bytes;
    }

    std::uint32_t calculate_box_vertices_number(primitives::box_create_info const &create_info)
    {
        if (create_info.hsegments * create_info.vsegments * create_info.dsegments < 1)
            throw resource::exception("invalid box segments' values"s);

        std::uint32_t xface_vertices_number = (create_info.hsegments + 1) * (create_info.dsegments + 1);
        std::uint32_t yface_vertices_number = (create_info.vsegments + 1) * (create_info.dsegments + 1);
        std::uint32_t zface_vertices_number = (create_info.hsegments + 1) * (create_info.vsegments + 1);

        return (xface_vertices_number + yface_vertices_number + zface_vertices_number) * 2;
    }

    std::uint32_t calculate_box_indices_number(primitives::box_create_info const &create_info)
    {
        if (create_info.hsegments * create_info.vsegments * create_info.dsegments < 1)
            throw resource::exception("invalid box segments' values"s);

        auto const hsegments = create_info.hsegments;
        auto const vsegments = create_info.vsegments;
        auto const dsegments = create_info.dsegments;

        std::uint32_t xface_indices_number = 0;
        std::uint32_t yface_indices_number = 0;
        std::uint32_t zface_indices_number = 0;

        switch (create_info.topology) {
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLES:
                xface_indices_number = (vsegments + 1) * (dsegments + 1) * 2;
                yface_indices_number = (hsegments + 1) * (dsegments + 1) * 2;
                zface_indices_number = (hsegments + 1) * (vsegments + 1) * 2;
                break;

            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP:
                xface_indices_number = ((dsegments + 1) * 2 * vsegments + (vsegments - 1) * 2 + 2) * 2;
                yface_indices_number = ((hsegments + 1) * 2 * dsegments + (dsegments - 1) * 2 + 2) * 2;
                zface_indices_number = ((hsegments + 1) * 2 * vsegments + (vsegments - 1) * 2) * 2 + 2;
                break;

            default:
                throw resource::exception("unsupported primitive topology"s);
        }

        return xface_indices_number + yface_indices_number + zface_indices_number;
    }
}
