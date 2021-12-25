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
    template<class T, class F>
    void generate_vertex_as_triangles(F generator, primitives::sphere_create_info const &create_info,
                                      strided_bidirectional_iterator<T> &it, bool index_generator)
    {
        auto const wsegments = create_info.wsegments;
        auto const hsegments = create_info.hsegments;

        auto constexpr pattern = std::array{
            std::array{std::pair{0u, 0u}, std::pair{1u, 1u}, std::pair{1u, 0u}},
            std::array{std::pair{0u, 0u}, std::pair{0u, 1u}, std::pair{1u, 1u}}
        };

        for (auto iy = 0u; iy < hsegments; ++iy) {
            for (auto ix = 0u; ix < wsegments + static_cast<std::uint32_t>(!index_generator); ++ix) {
                auto const g = [&] (auto offset)
                {
                    auto y = iy + std::get<1>(offset);
                    auto x = y == 0 || y == hsegments ? 0 : ix + std::get<0>(offset);
                    return generator(x, y);
                };

                if (iy != 0)
                    it = std::transform(std::cbegin(pattern[0]), std::cend(pattern[0]), it, g);

                if (iy != hsegments - 1)
                    it = std::transform(std::cbegin(pattern[1]), std::cend(pattern[1]), it, g);
            }
        }
    }

    template<class T, class F>
    void generate_vertex(F generator, primitives::sphere_create_info const &create_info,
                         strided_bidirectional_iterator<T> it, bool index_generator)
    {
        switch (create_info.topology) {
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLES:
                generate_vertex_as_triangles(generator, create_info, it, index_generator);
                break;

            case graphics::PRIMITIVE_TOPOLOGY::POINTS:
            case graphics::PRIMITIVE_TOPOLOGY::LINES:
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP:
            default:
                throw resource::exception("unsupported primitive topology"s);
        }
    }

    glm::vec2 generate_uv(primitives::sphere_create_info const &create_info, std::uint32_t ix, std::uint32_t iy)
    {
        auto const wsegments = create_info.wsegments;
        auto const hsegments = create_info.hsegments;

        if (ix == 0) {
            if (iy == 0) {
                return glm::vec2{0, 0};
            }

            else if (iy == hsegments) {
                return glm::vec2{0, 1};
            }
        }

        return glm::vec2{
            static_cast<float>(ix) / static_cast<float>(wsegments),
            static_cast<float>(iy) / static_cast<float>(hsegments)
        };
    }

    glm::vec3 generate_point(primitives::sphere_create_info const &create_info, std::uint32_t ix, std::uint32_t iy)
    {
        auto const uv = generate_uv(create_info, ix, iy);

        glm::vec3 point{
            -std::cos(uv.x * std::numbers::pi_v<float> * 2) * std::sin(uv.y * std::numbers::pi_v<float>),
            std::cos(uv.y * std::numbers::pi_v<float>),
            std::sin(uv.x * std::numbers::pi_v<float> * 2) * std::sin(uv.y * std::numbers::pi_v<float>)
        };

        return glm::normalize(point);
    }

    template<std::size_t N, class T>
    std::array<T, N> generate_position(primitives::sphere_create_info const &create_info, std::uint32_t ix, std::uint32_t iy)
    {
        auto const point = generate_point(create_info, ix, iy) * create_info.radius;

        if constexpr (N == 4)
            return std::array<T, N>{static_cast<T>(point.x), static_cast<T>(point.y), static_cast<T>(point.z), 1};

        else if constexpr (N == 3)
            return std::array<T, N>{static_cast<T>(point.x), static_cast<T>(point.y), static_cast<T>(point.z)};

        else throw resource::exception("unsupported components number"s);
    }
    
    template<std::size_t N, class T>
    std::array<T, N> generate_normal(primitives::sphere_create_info const &create_info, std::uint32_t ix, std::uint32_t iy)
    {
        auto const point = generate_point(create_info, ix, iy);

        if constexpr (N == 2) {
            std::array<T, 2> oct;

            math::encode_unit_vector_to_oct_fast(std::span{oct}, glm::vec3{point});
            return oct;
        }

        else if constexpr (N == 3)
            return std::array<T, N>{static_cast<T>(point.x), static_cast<T>(point.y), static_cast<T>(point.z)};

        else throw resource::exception("unsupported components number"s);
    }
    
    template<std::size_t N, class T>
    std::array<T, N> generate_texcoord(primitives::sphere_create_info const &create_info, graphics::FORMAT format, std::uint32_t ix, std::uint32_t iy)
    {
        if constexpr (N == 2) {
            auto const uv = generate_uv(create_info, ix, iy);

            switch (graphics::numeric_format(format)) {
                case graphics::NUMERIC_FORMAT::NORMALIZED:
                    if constexpr (std::is_same_v<T, std::uint16_t>) {
                        auto constexpr type_max = static_cast<float>(std::numeric_limits<T>::max());

                        return std::array<T, N>{static_cast<T>(uv.x * type_max), static_cast<T>((1.f - uv.y) * type_max)};
                    }

                    else throw resource::exception("unsupported format type"s);

                case graphics::NUMERIC_FORMAT::FLOAT:
                    if constexpr (std::is_floating_point_v<T>)
                        return std::array<T, N>{static_cast<T>(uv.x), static_cast<T>(1.f - uv.y)};

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

                    break;

                case graphics::NUMERIC_FORMAT::FLOAT:
                    if constexpr (std::is_floating_point_v<T>) {
                        if constexpr (N == 4)
                            return std::array<T, N>{static_cast<T>(color.r), static_cast<T>(color.g), static_cast<T>(color.b), 1};

                        else if constexpr (N == 3)
                            return std::array<T, N>{static_cast<T>(color.r), static_cast<T>(color.g), static_cast<T>(color.b)};
                    }

                    else throw resource::exception("unsupported format type"s);

                    break;

                default:
                    throw resource::exception("unsupported numeric format"s);
            }
        }

        else throw resource::exception("unsupported components number"s);
    }

    template<std::size_t N, class T>
    void generate_positions(primitives::sphere_create_info const &create_info, graphics::FORMAT format,
                            strided_bidirectional_iterator<std::array<T, N>> it, std::uint32_t vertices_count)
    {
        if constexpr (N == 2 || N == 3) {
            switch (graphics::numeric_format(format)) {
                case graphics::NUMERIC_FORMAT::FLOAT:
                    {
                        auto const wsegments = create_info.wsegments;

                        auto generator = std::bind(generate_position<N, T>, create_info, std::placeholders::_1, std::placeholders::_2);

                        if (create_info.index_buffer_type != graphics::INDEX_TYPE::UNDEFINED) {
                            std::generate_n(it, vertices_count, [generator, wsegments, i = 0u] () mutable
                            {
                                auto ix = std::max(0u, i - 1) % (wsegments + 1);
                                auto iy = i == 0 ? 0u : (i - 1) / (wsegments + 1) + 1;
                                ++i;
                                return generator(static_cast<std::uint32_t>(ix), static_cast<std::uint32_t>(iy));
                            });
                        }

                        // else generate_vertex(generator, create_info, it, vertices_count);
                        else throw resource::exception("unsupported non-indexed mesh"s);
                    }
                    break;

                default:
                    throw resource::exception("unsupported numeric format"s);
            }
        }

        else throw resource::exception("unsupported components number"s);
    }
    
    template<std::size_t N, class T>
    void generate_normals(primitives::sphere_create_info const &create_info, graphics::FORMAT format,
                          strided_bidirectional_iterator<std::array<T, N>> it, std::uint32_t vertices_count)
    {
        auto const wsegments = create_info.wsegments;

        if constexpr (N == 2) {
            switch (graphics::numeric_format(format)) {
                case graphics::NUMERIC_FORMAT::NORMALIZED:
                {
                    if constexpr (mpl::is_one_of_v<T, std::int8_t, std::int16_t>) {
                        auto generator = std::bind(generate_normal<N, T>, create_info, std::placeholders::_1, std::placeholders::_2);

                        if (create_info.index_buffer_type != graphics::INDEX_TYPE::UNDEFINED) {
                            std::generate_n(it, vertices_count, [generator, wsegments, i = 0u] () mutable
                            {
                                auto ix = std::max(0u, i - 1) % (wsegments + 1);
                                auto iy = i == 0 ? 0 : (i - 1) / (wsegments + 1) + 1;
                                ++i;
                                return generator(static_cast<std::uint32_t>(ix), static_cast<std::uint32_t>(iy));
                            });
                        }

                        // else generate_vertex(generator, create_info, it, vertices_count);
                        else throw resource::exception("unsupported non-indexed mesh"s);
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
                {
                    auto generator = std::bind(generate_normal<N, T>, create_info, std::placeholders::_1, std::placeholders::_2);

                    if (create_info.index_buffer_type != graphics::INDEX_TYPE::UNDEFINED) {
                        std::generate_n(it, vertices_count, [generator, wsegments, i = 0u] () mutable
                        {
                            auto ix = std::max(0u, i - 1) % (wsegments + 1);
                            auto iy = i == 0 ? 0 : (i - 1) / (wsegments + 1) + 1;
                            ++i;
                            return generator(static_cast<std::uint32_t>(ix), static_cast<std::uint32_t>(iy));
                        });
                    }

                    // else generate_vertex(generator, create_info, it, vertices_count);
                    else throw resource::exception("unsupported non-indexed mesh"s);

                    break;
                }

                default:
                    throw resource::exception("unsupported numeric format"s);
            }
        }

        else throw resource::exception("unsupported components number"s);
    }

    template<std::size_t N, class T>
    void generate_texcoords(primitives::sphere_create_info const &create_info, graphics::FORMAT format,
                            strided_bidirectional_iterator<std::array<T, N>> it, std::uint32_t vertices_count)
    {
        auto const wsegments = create_info.wsegments;

        auto generator = std::bind(generate_texcoord<N, T>, create_info, format, std::placeholders::_1, std::placeholders::_2);

        auto is_primitive_indexed = create_info.index_buffer_type != graphics::INDEX_TYPE::UNDEFINED;
        if (is_primitive_indexed) {
            std::generate_n(it, vertices_count, [generator, wsegments, i = 0u] () mutable
            {
                auto ix = std::max(0u, i - 1) % (wsegments + 1);
                auto iy = i == 0 ? 0 : (i - 1) / (wsegments + 1) + 1;
                ++i;
                return generator(static_cast<std::uint32_t>(ix), static_cast<std::uint32_t>(iy));
            });
        }

        // else generate_vertex(generator, create_info, it_begin, vertices_count);
        else throw resource::exception("unsupported non-indexed mesh"s);
    }

    template<std::size_t N, class T>
    void generate_colors(glm::vec4 const &color, graphics::FORMAT format,
                         strided_bidirectional_iterator<std::array<T, N>> it_begin, std::uint32_t vertices_count)
    {
        auto generator = std::bind(generate_color<N, T>, color, format);

        std::generate_n(it_begin, vertices_count, generator);
    }

    template<class T>
    void generate_indices(primitives::sphere_create_info const &create_info, T *buffer_begin, std::uint32_t)
    {
        auto it = strided_bidirectional_iterator<T>{buffer_begin, sizeof(T)};

        auto const wsegments = create_info.wsegments;
        auto const hsegments = create_info.hsegments;

        auto vertices_count = calculate_sphere_vertices_count(create_info);

        switch (create_info.topology) {
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLES:
                generate_vertex([wsegments, hsegments, vertices_count] (auto ix, auto iy)
                {
                    auto vertex_index = iy == 0 ? 0 : (ix % (wsegments + 1)) + ((iy - 1) * (wsegments + 1) + 1);
                    vertex_index = iy == hsegments ? vertices_count - 1 : vertex_index;

                    return static_cast<T>(vertex_index);
                }, create_info, it, true);
                break;

            case graphics::PRIMITIVE_TOPOLOGY::POINTS:
            case graphics::PRIMITIVE_TOPOLOGY::LINES:
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP:
            default:
                throw resource::exception("unsupported primitive topology"s);
        }
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
            throw resource::exception("unsupported primitive index topology"s);

        return wsegments * (hsegments - 1) + hsegments + 1;
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
                return wsegments * ((hsegments - 2) + 1) * 2 * 3;

            case graphics::PRIMITIVE_TOPOLOGY::POINTS:
            case graphics::PRIMITIVE_TOPOLOGY::LINES:
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP:
            default:
                throw resource::exception("unsupported primitive topology"s);
        }
    }

    void generate_sphere_indexed(primitives::sphere_create_info const &create_info, std::span<std::byte> vertex_buffer,
                                std::span<std::byte> index_buffer)
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
                            generate_positions(create_info, attribute.format, it, vertices_count);
                            break;

                        case vertex::SEMANTIC::NORMAL:
                            generate_normals(create_info, attribute.format, it, vertices_count);
                            break;

                        case vertex::SEMANTIC::TEXCOORD_0:
                            generate_texcoords(create_info, attribute.format, it, vertices_count);
                            break;

                        case vertex::SEMANTIC::COLOR_0:
                            generate_colors(create_info.color, attribute.format, it, vertices_count);
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
