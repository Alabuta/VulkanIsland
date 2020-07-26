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
    void generate_vertex(F generator, primitives::plane_create_info const &create_info,
                         strided_bidirectional_iterator<std::array<T, N>> it_begin, std::size_t vertex_count)
    {
        auto const hsegments = create_info.hsegments;
        auto const vsegments = create_info.vsegments;

        auto it_end = std::next(it_begin, static_cast<std::ptrdiff_t>(vertex_count));

        std::uint32_t const vertices_per_strip = (hsegments + 1) * 2;
        std::uint32_t const extra_vertices_per_strip = static_cast<std::uint32_t>(vsegments > 1) * 2;

        for (std::uint32_t strip_index = 0u; strip_index < vsegments; ++strip_index) {
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
    }

    template<std::size_t N, class T>
    std::array<T, N>
    generate_position(primitives::plane_create_info const &create_info, std::size_t vertex_index)
    {
        auto const hsegments = create_info.hsegments;
        auto const vsegments = create_info.vsegments;

        auto const width = create_info.width;
        auto const height = create_info.height;

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
    }

    template<std::size_t N, class T>
    std::array<T, N>
    generate_texcoord(primitives::plane_create_info const &create_info, graphics::FORMAT format, std::size_t vertex_index)
    {
        auto const hsegments = create_info.hsegments;
        auto const vsegments = create_info.vsegments;

        if constexpr (N == 2) {
            auto x = static_cast<float>(vertex_index % (hsegments + 1u)) / static_cast<float>(hsegments);
            auto y = 1.f - static_cast<float>(vertex_index / (hsegments + 1u)) / static_cast<float>(vsegments);

            switch (graphics::numeric_format(format)) {
                case graphics::NUMERIC_FORMAT::NORMALIZED:
                    if constexpr (std::is_same_v<T, std::uint16_t>) {
                        auto constexpr type_max = static_cast<float>(std::numeric_limits<T>::max());

                        return std::array<T, N>{static_cast<T>(x * type_max), static_cast<T>(y * type_max)};
                    }

                    else throw resource::exception("unsupported format type"s);

                case graphics::NUMERIC_FORMAT::FLOAT:
                    if constexpr (std::is_floating_point_v<T>)
                        return std::array<T, N>{static_cast<T>(x), static_cast<T>(y)};

                    else throw resource::exception("unsupported format type"s);

                default:
                    throw resource::exception("unsupported numeric format"s);
            }
        }

        else throw resource::exception("unsupported components number"s);
    }

    template<std::size_t N, class T>
    std::array<T, N> generate_color(glm::vec4 const &color, graphics::FORMAT format)
    {
        if constexpr (N == 3 || N == 4) {
            switch (graphics::numeric_format(format)) {
                case graphics::NUMERIC_FORMAT::NORMALIZED:
                    if constexpr (std::is_same_v<T, std::uint8_t>) {
                        auto constexpr type_max = static_cast<float>(std::numeric_limits<T>::max());

                        if constexpr (N == 4) {
                            return std::array<T, N>{
                                static_cast<T>(color.r * type_max),
                                static_cast<T>(color.g * type_max),
                                static_cast<T>(color.b * type_max),
                                static_cast<T>(color.a * type_max)
                            };
                        }

                        else if constexpr (N == 3) {
                            return std::array<T, N>{
                                static_cast<T>(color.r * type_max),
                                static_cast<T>(color.g * type_max),
                                static_cast<T>(color.b * type_max)
                            };
                        }
                    }

                    else throw resource::exception("unsupported format type"s);

                case graphics::NUMERIC_FORMAT::FLOAT:
                    if constexpr (std::is_floating_point_v<T>) {
                        if constexpr (N == 4)
                            return std::array<T, N>{color.r, color.g, color.b, 1};

                        else if constexpr (N == 3)
                            return std::array<T, N>{color.r, color.g, color.b};
                    }

                    else throw resource::exception("unsupported format type"s);

                default:
                    throw resource::exception("unsupported numeric format"s);
            }
        }

        else throw resource::exception("unsupported components number"s);
    }

    template<std::size_t N, class T>
    void generate_positions(primitives::plane_create_info const &create_info, graphics::FORMAT format,
                            strided_bidirectional_iterator<std::array<T, N>> it_begin, std::size_t vertex_count)
    {
        if constexpr (N == 2 || N == 3) {
            switch (graphics::numeric_format(format)) {
                case graphics::NUMERIC_FORMAT::FLOAT:
                    generate_vertex<N, T>(
                        std::bind(generate_position<N, T>, create_info, std::placeholders::_1), create_info, it_begin, vertex_count
                    );
                    break;

                default:
                    throw resource::exception("unsupported numeric format"s);
                    break;
            }
        }

        else throw resource::exception("unsupported components number"s);
    }

    template<std::size_t N, class T>
    void generate_normals(graphics::FORMAT format, strided_bidirectional_iterator<std::array<T, N>> it, std::size_t vertex_count)
    {
        if constexpr (N == 2) {
            switch (graphics::numeric_format(format)) {
                case graphics::NUMERIC_FORMAT::NORMALIZED:
                {
                    if constexpr (mpl::is_one_of_v<T, std::int8_t, std::int16_t>) {
                        std::array<T, 2> oct;

                        math::encode_unit_vector_to_oct_precise(oct, glm::vec3{0, 0, 1});

                        std::fill_n(it, vertex_count, oct);
                    }

                    else throw resource::exception("unsupported format type"s);

                    break;
                }

                default:
                    throw resource::exception("unsupported numeric format"s);
            }
        }

        else if constexpr (N == 3) {
            switch (graphics::numeric_format(format)) {
                case graphics::NUMERIC_FORMAT::SCALED:
                case graphics::NUMERIC_FORMAT::INT:
                case graphics::NUMERIC_FORMAT::FLOAT:
                    std::fill_n(it, vertex_count, std::array<T, 3>{0, 0, 1});
                    break;

                default:
                    throw resource::exception("unsupported numeric format"s);
            }
        }

        else throw resource::exception("unsupported components number"s);
    }

    template<std::size_t N, class T>
    void generate_texcoords(primitives::plane_create_info const &create_info, graphics::FORMAT format,
                            strided_bidirectional_iterator<std::array<T, N>> it_begin, std::size_t vertex_count)
    {
        generate_vertex<N, T>(std::bind(generate_texcoord<N, T>, create_info, format, std::placeholders::_1),
                              create_info, it_begin, vertex_count);
    }

    template<std::size_t N, class T>
    void generate_colors(primitives::plane_create_info const &create_info, glm::vec4 const &color, graphics::FORMAT format,
                         strided_bidirectional_iterator<std::array<T, N>> it_begin, std::size_t vertex_count)
    {
        generate_vertex<N, T>(std::bind(generate_color<N, T>, color, format),
                              create_info, it_begin, vertex_count);
    }

    std::uint32_t calculate_plane_vertices_number(primitives::plane_create_info const &create_info)
    {
        if (create_info.hsegments * create_info.vsegments < 1)
            throw resource::exception("invalid plane segments' values"s);

        bool is_indexed_primitive = create_info.index_buffer_format != graphics::FORMAT::UNDEFINED;

        if (is_indexed_primitive)
            return (create_info.hsegments + 1) * (create_info.vsegments + 1);

        switch (create_info.topology) {
            case graphics::PRIMITIVE_TOPOLOGY::POINTS:
            case graphics::PRIMITIVE_TOPOLOGY::LINES:
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLES:
                return create_info.hsegments * create_info.vsegments * 2 * 3;

            case graphics::PRIMITIVE_TOPOLOGY::LINE_STRIP:
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP:
                return (create_info.hsegments + 1) * 2 * create_info.vsegments + (create_info.vsegments - 1) * 2;

            default:
                throw resource::exception("unsupported primitive topology"s);
        }
    }

    std::uint32_t calculate_plane_indices_number(primitives::plane_create_info const &create_info)
    {
        if (create_info.hsegments * create_info.vsegments < 1)
            throw resource::exception("invalid plane segments' values"s);

        bool is_indexed_primitive = create_info.index_buffer_format != graphics::FORMAT::UNDEFINED;

        if (!is_indexed_primitive)
            return 0;

        switch (create_info.topology) {
            case graphics::PRIMITIVE_TOPOLOGY::POINTS:
            case graphics::PRIMITIVE_TOPOLOGY::LINES:
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLES:
                return create_info.hsegments * create_info.vsegments * 2 * 3;

            case graphics::PRIMITIVE_TOPOLOGY::LINE_STRIP:
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP:
                return (create_info.hsegments + 1) * 2 * create_info.vsegments + (create_info.vsegments - 1) * 2;

            default:
                throw resource::exception("unsupported primitive topology"s);
        }
    }

    void generate_plane_indexed(primitives::plane_create_info const &create_info, std::vector<std::byte>::iterator it_vertex_buffer,
                        std::vector<std::byte>::iterator it_index_buffer, glm::vec4 const &color)
    {
        ;
    }

    void generate_plane(primitives::plane_create_info const &create_info, std::vector<std::byte>::iterator it_vertex_buffer, glm::vec4 const &color)
    {
        auto &&vertex_layout = create_info.vertex_layout;

        std::uint32_t vertex_count = calculate_plane_vertices_number(create_info);
        std::uint32_t vertex_size = static_cast<std::uint32_t>(vertex_layout.size_in_bytes);

        auto &&attributes = vertex_layout.attributes;

        std::size_t offset_in_bytes = 0;

        for (auto &&attribute : attributes) {
            if (auto format_inst = graphics::instantiate_format(attribute.format); format_inst) {
                std::visit([&] (auto &&format_inst)
                {
                    using type = typename std::remove_cvref_t<decltype(format_inst)>;
                    using pointer_type = typename std::add_pointer_t<type>;

                    auto data = reinterpret_cast<pointer_type>(std::addressof(*it_vertex_buffer) + offset_in_bytes);

                    offset_in_bytes += sizeof(type);

                    auto it = strided_bidirectional_iterator<type>{data, vertex_size};

                    switch (attribute.semantic) {
                        case vertex::SEMANTIC::POSITION:
                            generate_positions(create_info, attribute.format, it, vertex_count);
                            break;

                        case vertex::SEMANTIC::NORMAL:
                            generate_normals(attribute.format, it, vertex_count);
                            break;

                        case vertex::SEMANTIC::TEXCOORD_0:
                            generate_texcoords(create_info, attribute.format, it, vertex_count);
                            break;

                        case vertex::SEMANTIC::COLOR_0:
                            generate_colors(create_info, color, attribute.format, it, vertex_count);
                            break;

                        default:
                            break;
                    }

                }, *format_inst);
            }

            else throw resource::exception("unsupported attribute format"s);
        }
    }
    
    void generate_plane(primitives::plane_create_info const &create_info, std::vector<std::byte>::iterator it_vertex_buffer,
                        std::vector<std::byte>::iterator it_index_buffer, glm::vec4 const &color)
    {
        if (create_info.topology != graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP)
            throw resource::exception("unsupported primitive topology"s);

        if (create_info.index_buffer_format != graphics::FORMAT::UNDEFINED && it_index_buffer != std::vector<std::byte>::iterator{})
            primitives::generate_plane_indexed(create_info, it_vertex_buffer, it_index_buffer, color);

        else primitives::generate_plane(create_info, it_vertex_buffer, color);
    }
}
