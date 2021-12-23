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


namespace {
    // https://github.com/mrdoob/three.js/blob/00a692864f541a3ec194d266e220efd597eb28fa/src/geometries/PolyhedronGeometry.js
	static auto const t = (1.f + std::sqrt(5.f)) / 2.f;

	auto const input_vertices = std::array{
		glm::vec3{-1, t, 0},		glm::vec3{1, t, 0},		glm::vec3{-1, -t, 0},		glm::vec3{1, -t, 0},
		glm::vec3{0, -1, t},		glm::vec3{0, 1, t},		glm::vec3{0, -1, -t},		glm::vec3{0, 1, -t},
		glm::vec3{t, 0, -1},		glm::vec3{t, 0, 1},		glm::vec3{-t, 0, -1},		glm::vec3{-t, 0, 1}
	};

	auto constexpr faces = std::array{
		std::array{0u, 11u, 5u}, 	    std::array{0u, 5u, 1u}, 	std::array{0u, 1u, 7u}, 		std::array{0u, 7u, 10u}, 	std::array{0u, 10u, 11u},
		std::array{1u, 5u, 9u}, 		std::array{5u, 11u, 4u},	std::array{11u, 10u, 2u},	    std::array{10u, 7u, 6u},	std::array{7u, 1u, 8u},
		std::array{3u, 9u, 4u}, 		std::array{3u, 4u, 2u},	    std::array{3u, 2u, 6u},		    std::array{3u, 6u, 8u},		std::array{3u, 8u, 9u},
		std::array{4u, 9u, 5u}, 		std::array{2u, 4u, 11u},	std::array{6u, 2u, 10u},		std::array{8u, 6u, 7u},		std::array{9u, 8u, 1u}
	};

    auto constexpr offsets_pattern = std::array{
        std::array{
            std::pair{0u, 1u}, std::pair{1u, 0u}, std::pair{0u, 0u}
        },
        std::array{
            std::pair{0u, 1u}, std::pair{1u, 1u}, std::pair{1u, 0u}
        }
    };

    // Angle around the Y axis, counter-clockwise when looking from above.
    float azimuth(glm::vec3 const &point)
    {
        return std::atan2(point.z, -point.x);
    }

    // Angle above the XZ plane.
    float inclination(glm::vec3 const &point)
    {
        return std::atan2(-point.y, std::sqrt((point.x * point.x) + (point.z * point.z)));
    }

    glm::vec3 generate_point(std::span<std::uint32_t const, 3> face, std::uint32_t columns, std::uint32_t i, std::uint32_t j)
    {
        auto &&a = input_vertices[face[0]];
        auto &&b = input_vertices[face[1]];
        auto &&c = input_vertices[face[2]];

        auto point = glm::mix(a, c, static_cast<float>(i) / static_cast<float>(columns));

        if (i != columns || j != 0)
        {
            auto bj = glm::mix(b, c, static_cast<float>(i) / static_cast<float>(columns));
            point = glm::mix(point, bj, static_cast<float>(j) / static_cast<float>(columns - i));
        }

        return glm::normalize(point);
    }

    void correct_uv(glm::vec2 &uv, glm::vec3 const &vec, float azimuth)
    {
        if (azimuth < 0 && uv.x == 1)
            uv.x = uv.x - 1.f;

        if (vec.x == 0 && vec.z == 0)
            uv.x = azimuth / 2.f / static_cast<float>(std::numbers::pi) + .5f;
    }

    template<std::size_t N, class T>
    void generate_position(primitives::icosahedron_create_info const &create_info, strided_bidirectional_iterator<std::array<T, N>> &it,
                           std::span<std::uint32_t const, 3> face, std::uint32_t pattern_index, std::uint32_t i, std::uint32_t j)
    {
        auto const columns = create_info.detail + 1;
        auto const radius = create_info.radius;

        it = std::transform(std::cbegin(offsets_pattern[pattern_index]), std::cend(offsets_pattern[pattern_index]), it, [&face, radius, columns, i, j] (auto offsets)
        {
            auto point = generate_point(face, columns, i + std::get<0>(offsets), j + std::get<1>(offsets)) * radius;

            if constexpr (N == 4)
                return std::array<T, N>{static_cast<T>(point.x), static_cast<T>(point.y), static_cast<T>(point.z), 1};

            else if constexpr (N == 3)
                return std::array<T, N>{static_cast<T>(point.x), static_cast<T>(point.y), static_cast<T>(point.z)};

            else throw resource::exception("unsupported components number"s);
        });
    }

    template<std::size_t N, class T>
    void generate_normal(primitives::icosahedron_create_info const &create_info, strided_bidirectional_iterator<std::array<T, N>> &it,
                         std::span<std::uint32_t const, 3> face, std::uint32_t pattern_index, std::uint32_t i, std::uint32_t j)
    {
        auto const columns = create_info.detail + 1;

        it = std::transform(std::cbegin(offsets_pattern[pattern_index]), std::cend(offsets_pattern[pattern_index]), it, [&face, columns, i, j] (auto offsets)
        {
            auto point = generate_point(face, columns, i + std::get<0>(offsets), j + std::get<1>(offsets));

            if constexpr (N == 2) {
                std::array<T, 2> oct;

                math::encode_unit_vector_to_oct_precise(std::span{ oct }, glm::vec3{ point });

                return oct;
            }

            else if constexpr (N == 3)
                return std::array<T, N>{static_cast<T>(point.x), static_cast<T>(point.y), static_cast<T>(point.z)};

            else throw resource::exception("unsupported components number"s);
        });
    }

    template<std::size_t N, class T>
    void generate_texcoord(primitives::icosahedron_create_info const &create_info, strided_bidirectional_iterator<std::array<T, N>> &it, graphics::FORMAT format,
                           std::span<std::uint32_t const, 3> face, std::uint32_t pattern_index, std::uint32_t i, std::uint32_t j)
    {
        if constexpr (N == 2) {
            auto const columns = create_info.detail + 1;

            std::array<glm::vec3, 3> points;
            std::transform(std::cbegin(offsets_pattern[pattern_index]), std::cend(offsets_pattern[pattern_index]), std::begin(points), [&face, columns, i, j] (auto offsets)
            {
                return generate_point(face, columns, i + std::get<0>(offsets), j + std::get<1>(offsets));
            });

            auto const centoroid = std::accumulate(std::cbegin(points), std::cend(points), glm::vec3{0}) / 3.f;
            auto const centoroid_azimuth = azimuth(centoroid);

            std::array<glm::vec2, 3> uvs;
            std::transform(std::cbegin(points), std::cend(points), std::begin(uvs), [centoroid_azimuth] (auto &&point)
            {
                auto uv = glm::vec2{azimuth(point) / 2.f / std::numbers::pi + .5f, 1.f - (inclination(point) / std::numbers::pi + .5f)};

                correct_uv(uv, point, centoroid_azimuth);

                return uv;
            });

            auto [min, max] = std::ranges::minmax(uvs, [] (auto &&lhs, auto &&rhs)
            {
                return lhs.x < rhs.x;
            });

            if (min.x < .1f && max.x > .9f) {
                if (uvs[0].x < .2f)
                    uvs[0].x += 1;

                if (uvs[1].x < .2f)
                    uvs[1].x += 1;

                if (uvs[2].x < .2f)
                    uvs[2].x += 1;
            }

            it = std::transform(std::cbegin(uvs), std::cend(uvs), it, [format] ([[maybe_unused]] auto &&uv)
            {
                switch (graphics::numeric_format(format)) {
                    case graphics::NUMERIC_FORMAT::NORMALIZED:
                        if constexpr (std::is_same_v<T, std::uint16_t>) {
                            auto constexpr type_max = static_cast<float>(std::numeric_limits<T>::max());

                            return std::array<T, N>{static_cast<T>(uv.x * type_max), static_cast<T>(uv.y * type_max)};
                        }

                    case graphics::NUMERIC_FORMAT::FLOAT:
                        if constexpr (std::is_floating_point_v<T>)
                            return std::array<T, N>{static_cast<T>(uv.x), static_cast<T>(uv.y)};

                    default:
                        break;
                }

                return std::array<T, N>{};
            });
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
                            return std::array<T, N>{color.r, color.g, color.b, 1};

                        else if constexpr (N == 3)
                            return std::array<T, N>{color.r, color.g, color.b};
                    }

                    else throw resource::exception("unsupported format type"s);

                    break;

                default:
                    throw resource::exception("unsupported numeric format"s);
            }
        }

        else throw resource::exception("unsupported components number"s);
    }

    template<std::size_t N, class T, class F>
    void generate_vertex_as_triangles(F generator, primitives::icosahedron_create_info const &create_info,
                                      strided_bidirectional_iterator<std::array<T, N>>)
    {
        auto const columns = create_info.detail + 1;

        for (auto &&face : faces) {
            for (auto i = 0u; i < columns; ++i) {
                for (auto j = 0u; j < 2 * (columns - i) - 1; ++j) {
                    auto const pattern_index = j % 2;

                    generator(face, pattern_index, i, j / 2);
                }
            }
        }
    }

    template<class T, class F>
    void generate_vertex(F generator, primitives::icosahedron_create_info const &create_info,
                         strided_bidirectional_iterator<T> it_begin)
    {
        switch (create_info.topology) {
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLES:
                generate_vertex_as_triangles(generator, create_info, it_begin);
                break;

            case graphics::PRIMITIVE_TOPOLOGY::POINTS:
            case graphics::PRIMITIVE_TOPOLOGY::LINES:
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP:
            default:
                throw resource::exception("unsupported primitive topology"s);
        }
    }

    template<std::size_t N, class T>
    void generate_positions(primitives::icosahedron_create_info const &create_info, graphics::FORMAT format,
                            strided_bidirectional_iterator<std::array<T, N>> it)
    {
        if constexpr (N == 3 || N == 4) {
            switch (graphics::numeric_format(format)) {
                case graphics::NUMERIC_FORMAT::FLOAT:
                    {
                        auto generator = std::bind(generate_position<N, T>, create_info, it, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
                        generate_vertex(generator, create_info, it);
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
    void generate_normals(primitives::icosahedron_create_info const &create_info, graphics::FORMAT format,
                          strided_bidirectional_iterator<std::array<T, N>> it)
    {
        if constexpr (N == 2) {
            switch (graphics::numeric_format(format)) {
                case graphics::NUMERIC_FORMAT::NORMALIZED:
                {
                    if constexpr (mpl::is_one_of_v<T, std::int8_t, std::int16_t>) {
                        auto generator = std::bind(generate_normal<N, T>, create_info, it, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
                        generate_vertex(generator, create_info, it);
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
                case graphics::NUMERIC_FORMAT::FLOAT:
                {
                    auto generator = std::bind(generate_normal<N, T>, create_info, it, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
                    generate_vertex(generator, create_info, it);

                    break;
                }

                default:
                    throw resource::exception("unsupported numeric format"s);
            }
        }

        else throw resource::exception("unsupported components number"s);
    }

    template<std::size_t N, class T>
    void generate_texcoords(primitives::icosahedron_create_info const &create_info, graphics::FORMAT format,
                            strided_bidirectional_iterator<std::array<T, N>> it)
    {
        auto generator = std::bind(generate_texcoord<N, T>, create_info, it, format, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
        generate_vertex(generator, create_info, it);
    }

    template<std::size_t N, class T>
    void generate_colors(primitives::icosahedron_create_info const &create_info, graphics::FORMAT format,
                         strided_bidirectional_iterator<std::array<T, N>> it_begin, std::uint32_t vertices_count)
    {
        auto generator = std::bind(generate_color<N, T>, create_info.color, format);

        std::generate_n(it_begin, vertices_count, generator);
    }
}

namespace primitives {

	std::uint32_t calculate_icosahedron_vertices_count(primitives::icosahedron_create_info const &create_info)
	{
        auto const columns = create_info.detail + 1;

        std::uint32_t vertices_count = 0;

        for (auto i = 0u; i < columns; ++i)
            for (auto j = 0u; j < 2 * (columns - i) - 1; ++j)
                vertices_count += 3;

        return static_cast<std::uint32_t>(std::size(faces)) * vertices_count;
	}

	void generate_icosahedron(primitives::icosahedron_create_info const &create_info, std::span<std::byte> vertex_buffer)
	{
        auto &&vertex_layout = create_info.vertex_layout;

        auto vertices_count = calculate_icosahedron_vertices_count(create_info);
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
                            std::fill_n(it, calculate_icosahedron_vertices_count(create_info), type{0});
                            generate_positions(create_info, attribute.format, it);
                            break;

                        case vertex::SEMANTIC::NORMAL:
                            generate_normals(create_info, attribute.format, it);
                            break;

                        case vertex::SEMANTIC::TEXCOORD_0:
                            generate_texcoords(create_info, attribute.format, it);
                            break;

                        case vertex::SEMANTIC::COLOR_0:
                            generate_colors(create_info, attribute.format, it, vertices_count);
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