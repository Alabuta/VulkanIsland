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
    void generate_vertex(F generator, std::uint32_t hsegments, std::uint32_t vsegments, std::uint32_t dsegments,
                         strided_bidirectional_iterator<std::array<T, N>> it_begin, std::size_t vertex_count)
    {
        auto it_end = std::next(it_begin, static_cast<std::ptrdiff_t>(vertex_count));

        std::uint32_t const vertices_per_strip = (hsegments + 1) * 2;
        std::uint32_t const extra_vertices_per_strip = static_cast<std::uint32_t>(vsegments > 1) * 2;

        for (std::uint32_t strip_index = 0u; strip_index < vsegments; ++strip_index) {
            auto it = std::next(it_begin, strip_index * (vertices_per_strip + extra_vertices_per_strip));

            std::generate_n(it, 2, [&, offset = 0u] () mutable
            {
                auto vertex_index = (strip_index + offset++) * (hsegments + 1);

                return generator(hsegments, vsegments, dsegments, vertex_index);
            });

            std::generate_n(std::next(it, 2), vertices_per_strip - 2, [&, triangle_index = strip_index * hsegments * 2] () mutable
            {
                auto quad_index = triangle_index / 2;

                auto column = quad_index % hsegments + 1;
                auto row = quad_index / hsegments + (triangle_index % 2);

                ++triangle_index;

                auto vertex_index = row * (hsegments + 1) + column;

                return generator(hsegments, vsegments, dsegments, vertex_index);
            });

            it = std::next(it, vertices_per_strip);

            if (it < it_end) {
                std::generate_n(it, 2, [&, i = 0u] () mutable {
                    auto vertex_index = (strip_index + 1) * (hsegments + 1) + hsegments * (1 - i++);

                    return generator(hsegments, vsegments, dsegments, vertex_index);
                });
            }
        }
    }
    
    template<std::size_t N, class T>
    std::array<T, N>
    generate_position(float width, float height, float depth,
                      std::size_t hsegments, std::size_t vsegments, std::size_t dsegments, std::size_t vertex_index)
    {
        /*auto [x0, y0] = std::pair{-width / 2.f, height / 2.f};
        auto [step_x, step_y] = std::pair{width / static_cast<float>(hsegments), -height / static_cast<float>(vsegments)};

        auto x = static_cast<T>(x0 + static_cast<float>(vertex_index % (hsegments + 1u)) * step_x);
        auto y = static_cast<T>(y0 + static_cast<float>(vertex_index / (hsegments + 1u)) * step_y);

        if constexpr (N == 4)
            return std::array<T, N>{x, y, 0, 1};

        else if constexpr (N == 3)
            return std::array<T, N>{x, y, 0};

        else if constexpr (N == 2)
            return std::array<T, N>{x, y};

        else throw resource::exception("unsupported components number"s);*/

        return { };
    }

    
    template<std::size_t N, class T>
    void generate_positions(graphics::FORMAT format, float width, float height, float depth,
                            std::uint32_t hsegments, std::uint32_t vsegments, std::uint32_t dsegments,
                            strided_bidirectional_iterator<std::array<T, N>> it_begin, std::size_t vertex_count)
    {
        using std::placeholders::_1;
        using std::placeholders::_2;
        using std::placeholders::_3;
        using std::placeholders::_4;

        if constexpr (N == 2 || N == 3) {
            switch (graphics::numeric_format(format)) {
                case graphics::NUMERIC_FORMAT::FLOAT:
                    generate_vertex<N, T>(
                        std::bind(generate_position<N, T>, width, height, depth, _1, _2, _3, _4),
                        hsegments, vsegments, dsegments, it_begin, vertex_count
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
    generate_box_indexed(float width, float height, float depth, std::uint32_t hsegments, std::uint32_t vsegments, std::uint32_t dsegments,
                         graphics::vertex_layout const &vertex_layout, glm::vec4 const &color)
    {
        std::size_t vertex_count = (hsegments + 1) * 2 * vsegments + (vsegments - 1) * 2;
        std::size_t vertex_size = vertex_layout.size_in_bytes;

        std::vector<std::byte> bytes(vertex_size * vertex_count);

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
                            generate_positions(attribute.format, width, height, depth, hsegments, vsegments, dsegments, it, vertex_count);
                            break;

                        case vertex::SEMANTIC::NORMAL:
                            //generate_normals(attribute.format, it, vertex_count);
                            break;

                        case vertex::SEMANTIC::TEXCOORD_0:
                            //generate_texcoords(attribute.format, it, hsegments, vsegments, vertex_count);
                            break;

                        case vertex::SEMANTIC::COLOR_0:
                            //generate_colors(color, attribute.format, it, hsegments, vsegments, vertex_count);
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

    std::uint32_t calculate_box_vertices_number(std::uint32_t hsegments, std::uint32_t vsegments, std::uint32_t dsegments)
    {
        std::uint32_t xface_vertices_number = (hsegments + 1) * (dsegments + 1);
        std::uint32_t yface_vertices_number = (vsegments + 1) * (dsegments + 1);
        std::uint32_t zface_vertices_number = (hsegments + 1) * (vsegments + 1);

        return (xface_vertices_number + yface_vertices_number + zface_vertices_number) * 2;
    }

    std::uint32_t calculate_box_indices_number(graphics::PRIMITIVE_TOPOLOGY topology, std::uint32_t hsegments, std::uint32_t vsegments, std::uint32_t dsegments)
    {
        std::uint32_t xface_indices_number = 0;
        std::uint32_t yface_indices_number = 0;
        std::uint32_t zface_indices_number = 0;

        switch (topology) {
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLES:
                xface_indices_number = (hsegments + 1) * (dsegments + 1);
                yface_indices_number = (vsegments + 1) * (dsegments + 1);
                zface_indices_number = (hsegments + 1) * (vsegments + 1);
                break;

            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP:
                break;

            default:
                throw resource::exception("unsupported primitive topology"s);
        }

        return (xface_indices_number + yface_indices_number + zface_indices_number) * 2;
    }
}
