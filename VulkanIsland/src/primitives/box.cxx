#include <array>
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
    std::array<std::uint32_t, 3> calculate_box_faces_vertices_count(primitives::box_create_info const &create_info)
    {
        if (create_info.hsegments * create_info.vsegments * create_info.dsegments < 1)
            throw resource::exception("invalid box segments' values"s);

        std::uint32_t xface_vertices_number, yface_vertices_number, zface_vertices_number;

        auto const hsegments = create_info.hsegments;
        auto const vsegments = create_info.vsegments;
        auto const dsegments = create_info.dsegments;

        bool is_primitive_indexed = create_info.index_buffer_type != graphics::INDEX_TYPE::UNDEFINED;

        if (is_primitive_indexed) {
            xface_vertices_number = (vsegments + 1) * (dsegments + 1);
            yface_vertices_number = (hsegments + 1) * (dsegments + 1);
            zface_vertices_number = (hsegments + 1) * (vsegments + 1);
        }

        else {
            switch (create_info.topology) {
                case graphics::PRIMITIVE_TOPOLOGY::POINTS:
                case graphics::PRIMITIVE_TOPOLOGY::LINES:
                case graphics::PRIMITIVE_TOPOLOGY::TRIANGLES:
                    xface_vertices_number = vsegments * dsegments * 3;
                    yface_vertices_number = hsegments * dsegments * 3;
                    zface_vertices_number = hsegments * vsegments * 3;
                    break;

                case graphics::PRIMITIVE_TOPOLOGY::LINE_STRIP:
                case graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP:
                    xface_vertices_number = (dsegments + 1) * 2 * vsegments + (vsegments - 1) * 2;
                    yface_vertices_number = (hsegments + 1) * 2 * dsegments + (dsegments - 1) * 2;
                    zface_vertices_number = (hsegments + 1) * 2 * vsegments + (vsegments - 1) * 2;
                    /*xface_vertices_number = ((dsegments + 1) * 2 * vsegments + (vsegments - 1) * 2 + 2) * 2;
                    yface_vertices_number = ((hsegments + 1) * 2 * dsegments + (dsegments - 1) * 2 + 2) * 2;
                    zface_vertices_number = ((hsegments + 1) * 2 * vsegments + (vsegments - 1) * 2) * 2 + 2;*/
                    break;

                default:
                    throw resource::exception("unsupported primitive topology"s);
            }
        }

        return {xface_vertices_number, yface_vertices_number, zface_vertices_number};
    }

#if 0
    template<std::size_t N, class T, class F>
    void generate_vertex_as_triangles(F generator, primitives::box_create_info const &create_info,
                         strided_bidirectional_iterator<T> it_begin, std::size_t total_vertex_count)
    {
        auto const hsegments = create_info.hsegments;
        auto const vsegments = create_info.vsegments;
        auto const dsegments = create_info.dsegments;

        auto it_end = std::next(it_begin, static_cast<std::ptrdiff_t>(total_vertex_count));

        auto const faces = std::array{
            std::tuple{hsegments, vsegments, (hsegments + 1) * 2 * vsegments + (vsegments - 1) * 2}
        };

        for (auto [hsegs, vsegs, vertex_count] : faces) {
            auto it_face_end = std::next(it_begin, static_cast<std::ptrdiff_t>(vertex_count));

            auto const vertices_per_strip = (hsegs + 1) * 2;
            auto const extra_vertices_per_strip = static_cast<std::uint32_t>(vsegs > 1) * 2;

            for (auto strip_index = 0u; strip_index < vsegs; ++strip_index) {
                auto it = std::next(it_begin, vertices_per_strip + extra_vertices_per_strip);

                std::generate_n(it, 2, [&, offset = 0u]() mutable
                {
                    auto vertex_index = (strip_index + offset++) * (hsegs + 1);

                    return generator(vertex_index);
                });

                std::generate_n(std::next(it, 2), vertices_per_strip - 2, [&, triangle_index = strip_index * hsegs * 2]() mutable
                {
                    auto quad_index = triangle_index / 2;

                    auto column = quad_index % hsegs + 1;
                    auto row = quad_index / hsegs + (triangle_index % 2);

                    ++triangle_index;

                    auto vertex_index = row * (hsegs + 1) + column;

                    return generator(vertex_index);
                });

                it = std::next(it, vertices_per_strip);

                if (it < it_face_end) {
                    std::generate_n(it, 2, [&, i = 0u]() mutable {
                        auto vertex_index = (strip_index + 1) * (hsegs + 1) + hsegs * (1 - i++);

                        return generator(vertex_index);
                    });
                }

                it_begin = it;
            }
        }
    }
#endif

    template<class T, class F>
    void generate_vertex_as_triangles(F generator, primitives::box_create_info const &create_info,
                                      strided_bidirectional_iterator<T> it_begin, std::uint32_t)
    {
        auto const vertices_per_quad = 2 * 3;

        auto const segments = std::array{
            std::pair{create_info.hsegments, create_info.dsegments},
            std::pair{create_info.hsegments, create_info.dsegments},
            std::pair{create_info.vsegments, create_info.dsegments},
            std::pair{create_info.vsegments, create_info.dsegments},
            std::pair{create_info.hsegments, create_info.vsegments},
            std::pair{create_info.hsegments, create_info.vsegments}
        };

        std::size_t total_offset = 0u;

        for (std::size_t vertex_offset = 0u; auto [hsegments, vsegments] : segments) {
            auto const pattern = std::array{ 0u, hsegments + 1, 1u, 1u, hsegments + 1, hsegments + 2 };

            auto const horizontal_vertices_number = static_cast<std::size_t>(hsegments * vertices_per_quad);

            for (auto vsegment_index = 0u; vsegment_index < vsegments; ++vsegment_index) {
                for (auto hsegment_index = 0u; hsegment_index < hsegments; ++hsegment_index) {
                    auto const offset = total_offset + horizontal_vertices_number * vsegment_index + static_cast<std::size_t>(hsegment_index) * vertices_per_quad;
                    auto it = std::next(it_begin, static_cast<std::ptrdiff_t>(offset));

                    std::transform(std::cbegin(pattern), std::cend(pattern), it, [&] (auto column)
                    {
                        auto vertex_index = vsegment_index * (hsegments + 1) + column + hsegment_index;

                        return generator(vertex_offset + vertex_index);
                    });
                }
            }

            total_offset += vsegments * hsegments * vertices_per_quad;

            vertex_offset += (vsegments + 1) * (hsegments + 1);
        }
    }

    template<class T, class F>
    void generate_vertex(F generator, primitives::box_create_info const &create_info,
                         strided_bidirectional_iterator<T> it_begin, std::uint32_t vertex_number)
    {
        switch (create_info.topology) {
            case graphics::PRIMITIVE_TOPOLOGY::POINTS:
                // generate_vertex_as_points(generator, create_info, it_begin, vertex_number);
                break;

            case graphics::PRIMITIVE_TOPOLOGY::LINES:
                // generate_vertex_as_lines(generator, create_info, it_begin, vertex_number);
                break;

            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLES:
                generate_vertex_as_triangles(generator, create_info, it_begin, vertex_number);
                break;

            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP:
                // generate_vertex_as_triangle_strip(generator, create_info, it_begin, vertex_number);
                break;

            default:
                throw resource::exception("unsupported primitive topology"s);
        }
    }

    template<std::size_t N, class T>
    std::array<T, N> generate_position(std::uint32_t hsegments, std::uint32_t vsegments, float width, float height, glm::mat4 transform, std::size_t vertex_index)
    {
        auto step = glm::vec2{width / static_cast<float>(hsegments), -height / static_cast<float>(vsegments)};
        auto xy = glm::vec2{-width, height} / 2.f + glm::vec2{vertex_index % (hsegments + 1u), vertex_index / (hsegments + 1u)} * step;

        auto pos = glm::vec<4, T>{transform * glm::vec4{xy, 0, 1}};

        if constexpr (N == 4)
            return std::array<T, N>{pos.x, pos.y, pos.z, 1};

        else if constexpr (N == 3)
            return std::array<T, N>{pos.x, pos.y, pos.z};

        else throw resource::exception("unsupported components number"s);
    }
    
    template<std::size_t N, class T>
    std::array<T, N>
    generate_color(glm::vec4 const &color, graphics::FORMAT format)
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
    void generate_positions(primitives::box_create_info const &create_info, graphics::FORMAT attribute_format, std::span<glm::mat4 const, 6> transforms,
                            strided_bidirectional_iterator<std::array<T, N>> it_begin, [[maybe_unused]] std::size_t vertex_count)
    {
        if constexpr (N == 3) {
            switch (graphics::numeric_format(attribute_format)) {
                case graphics::NUMERIC_FORMAT::FLOAT:
                    {
                        auto vertices_number = calculate_box_faces_vertices_count(create_info);

                        auto const dimensions_data = std::array{
                            std::tuple{create_info.dsegments, create_info.vsegments, create_info.depth, create_info.height},
                            std::tuple{create_info.hsegments, create_info.dsegments, create_info.width, create_info.depth},
                            std::tuple{create_info.hsegments, create_info.vsegments, create_info.width, create_info.height}
                        };

                        bool is_primitive_indexed = create_info.index_buffer_type != graphics::INDEX_TYPE::UNDEFINED;

                        for (std::size_t face_index = 0, offset = 0; auto &&transform : transforms) {
                            auto [hsegments, vsegments, width, height] = dimensions_data.at(face_index / 2);
                            auto generator = std::bind(generate_position<N, T>, hsegments, vsegments, width, height, transform, std::placeholders::_1);

                            if (is_primitive_indexed) {
                                std::generate_n(std::next(it_begin, offset), vertices_number.at(face_index / 2), [generator, i = 0u] () mutable
                                {
                                    return generator(i++);
                                });
                            }

                            // else generate_vertex(generator, create_info, it_begin, vertex_count);
                            else throw resource::exception("not yet implemented"s);

                            offset += vertices_number.at(face_index / 2);
                            ++face_index;
                        }
                    }
                    break;

                default:
                    throw resource::exception("unsupported numeric format"s);
                    break;
            }
        }

        else throw resource::exception("unsupported components number"s);
    }

    template<std::size_t N, class T>
    void generate_normals(primitives::box_create_info const &create_info, graphics::FORMAT attribute_format, std::span<glm::mat4 const, 6> transforms,
                          strided_bidirectional_iterator<std::array<T, N>> it_begin, [[maybe_unused]] std::size_t vertex_count)
    {
        auto vertices_number = calculate_box_faces_vertices_count(create_info);

        for (std::size_t face_index = 0, offset = 0; auto &&transform : transforms) {
            auto normal = glm::vec<4, T>{transform * glm::vec4{0, 0, 1, 0}};

            if constexpr (N == 2) {
                switch (graphics::numeric_format(attribute_format)) {
                    case graphics::NUMERIC_FORMAT::NORMALIZED:
                    {
                        if constexpr (mpl::is_one_of_v<T, std::int8_t, std::int16_t>) {
                            std::array<T, 2> oct;

                            math::encode_unit_vector_to_oct_fast(std::span{oct}, glm::vec3{normal});

                            std::fill_n(std::next(it_begin, offset), vertices_number.at(face_index / 2), oct);
                        }

                        else throw resource::exception("unsupported format type"s);

                        break;
                    }

                    default:
                        throw resource::exception("unsupported numeric format"s);
                }
            }

            else if constexpr (N == 3) {
                switch (graphics::numeric_format(attribute_format)) {
                    case graphics::NUMERIC_FORMAT::SCALED:
                    case graphics::NUMERIC_FORMAT::INT:
                    case graphics::NUMERIC_FORMAT::FLOAT:
                        std::fill_n(std::next(it_begin, offset), vertices_number.at(face_index / 2), std::array<T, 3>{normal.x, normal.y, normal.z});
                        break;

                    default:
                        throw resource::exception("unsupported numeric format"s);
                }
            }

            else throw resource::exception("unsupported components number"s);

            offset += vertices_number.at(face_index / 2);
            ++face_index;
        }
    }


    template<std::size_t N, class T>
    void generate_colors(primitives::box_create_info const& create_info, graphics::FORMAT format,
                         strided_bidirectional_iterator<std::array<T, N>> it_begin, [[maybe_unused]] std::uint32_t vertex_number)
    {
        auto vertices_number = calculate_box_faces_vertices_count(create_info);

        for (std::size_t face_index = 0, offset = 0; auto &&color : create_info.colors) {
            auto generator = std::bind(generate_color<N, T>, color, format);

            std::generate_n(std::next(it_begin, offset), vertices_number.at(face_index / 2), generator);

            offset += vertices_number.at(face_index / 2);
            ++face_index;
        }
    }

    template<class T>
    void generate_indices(primitives::box_create_info const &create_info, T *buffer_begin, std::uint32_t indices_number)
    {
        auto it_begin = strided_bidirectional_iterator<T>{buffer_begin, sizeof(T)};

        switch (create_info.topology) {
            case graphics::PRIMITIVE_TOPOLOGY::POINTS:
            case graphics::PRIMITIVE_TOPOLOGY::LINES:
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLES:
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP:
                generate_vertex([] (auto vertex_index)
                {
                    return static_cast<T>(vertex_index);
                }, create_info, it_begin, indices_number);
                break;

            default:
                throw resource::exception("unsupported primitive topology"s);
        }
    }
}

namespace primitives
{
    std::uint32_t calculate_box_vertices_number(primitives::box_create_info const &create_info)
    {
        auto vertices_number = calculate_box_faces_vertices_count(create_info);
        return (vertices_number.at(0) + vertices_number.at(1) + vertices_number.at(2)) * 2;
    }

    std::uint32_t calculate_box_indices_number(primitives::box_create_info const &create_info)
    {
        if (create_info.hsegments * create_info.vsegments * create_info.dsegments < 1)
            throw resource::exception("invalid box segments' values"s);

        bool is_primitive_indexed = create_info.index_buffer_type != graphics::INDEX_TYPE::UNDEFINED;

        if (!is_primitive_indexed)
            return 0;

        auto const hsegments = create_info.hsegments;
        auto const vsegments = create_info.vsegments;
        auto const dsegments = create_info.dsegments;

        std::uint32_t xface_indices_number, yface_indices_number, zface_indices_number;

        switch (create_info.topology) {
            case graphics::PRIMITIVE_TOPOLOGY::POINTS:
            case graphics::PRIMITIVE_TOPOLOGY::LINES:
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLES:
                xface_indices_number = hsegments * dsegments * 2 * 3;
                yface_indices_number = vsegments * dsegments * 2 * 3;
                zface_indices_number = hsegments * vsegments * 2 * 3;
                break;

            case graphics::PRIMITIVE_TOPOLOGY::LINE_STRIP:
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP:
                xface_indices_number = ((dsegments + 1) * 2 * vsegments + (vsegments - 1) * 2 + 2) * 2;
                yface_indices_number = ((hsegments + 1) * 2 * dsegments + (dsegments - 1) * 2 + 2) * 2;
                zface_indices_number = ((hsegments + 1) * 2 * vsegments + (vsegments - 1) * 2) * 2 + 2;
                break;

            default:
                throw resource::exception("unsupported primitive topology"s);
        }

        return (xface_indices_number + yface_indices_number + zface_indices_number) * 2;
    }

    void generate_box_indexed(primitives::box_create_info const &create_info, std::span<std::byte> vertex_buffer,
                              std::span<std::byte> index_buffer)
    {
        auto indices_number = calculate_box_indices_number(create_info);

        switch (create_info.index_buffer_type) {
            case graphics::INDEX_TYPE::UINT_16:
            {
                using pointer_type = typename std::add_pointer_t<std::uint16_t>;
                auto it = reinterpret_cast<pointer_type>(std::to_address(std::data(index_buffer)));

                generate_indices(create_info, it, indices_number);
            }
            break;

            case graphics::INDEX_TYPE::UINT_32:
            {
                using pointer_type = typename std::add_pointer_t<std::uint32_t>;
                auto it = reinterpret_cast<pointer_type>(std::to_address(std::data(index_buffer)));

                generate_indices(create_info, it, indices_number);
            }
            break;

            default:
                throw resource::exception("unsupported index instance type"s);
        }

        generate_box(create_info, vertex_buffer);
    }

    void generate_box(primitives::box_create_info const &create_info, std::span<std::byte> vertex_buffer)
    {
        auto &&vertex_layout = create_info.vertex_layout;

        auto vertex_number = calculate_box_vertices_number(create_info);
        auto vertex_size = static_cast<std::uint32_t>(vertex_layout.size_bytes);

        auto &&attributes = vertex_layout.attributes;

        auto const transforms = std::array{
            glm::translate(glm::rotate(glm::mat4{1.f}, glm::radians(+90.f), glm::vec3{0, 1, 0}), glm::vec3{0, 0, create_info.width / 2.f}),
            glm::translate(glm::rotate(glm::mat4{1.f}, glm::radians(-90.f), glm::vec3{0, 1, 0}), glm::vec3{0, 0, create_info.width / 2.f}),
            glm::translate(glm::rotate(glm::mat4{1.f}, glm::radians(-90.f), glm::vec3{1, 0, 0}), glm::vec3{0, 0, create_info.height / 2.f}),
            glm::translate(glm::rotate(glm::mat4{1.f}, glm::radians(+90.f), glm::vec3{1, 0, 0}), glm::vec3{0, 0, create_info.height / 2.f}),
            glm::translate(glm::rotate(glm::mat4{1.f}, glm::radians(360.f), glm::vec3{0, 1, 0}), glm::vec3{0, 0, create_info.depth / 2.f}),
            glm::translate(glm::rotate(glm::mat4{1.f}, glm::radians(180.f), glm::vec3{0, 1, 0}), glm::vec3{0, 0, create_info.depth / 2.f})
        };

        for (std::size_t offset_in_bytes = 0; auto && attribute : attributes) {
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
                            generate_positions(create_info, attribute.format, std::span{transforms}, it, vertex_number);
                            break;

                        case vertex::SEMANTIC::NORMAL:
                            generate_normals(create_info, attribute.format, std::span{transforms}, it, vertex_number);
                            break;

                        case vertex::SEMANTIC::TEXCOORD_0:
                            //generate_texcoords(create_info, attribute.format, it, vertex_number);
                            break;

                        case vertex::SEMANTIC::COLOR_0:
                            generate_colors(create_info, attribute.format, it, vertex_number);
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
