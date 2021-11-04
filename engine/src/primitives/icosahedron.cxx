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
	static auto const t = (1.f + std::sqrt(5.f)) / 2.f;

	static auto const input_vertices = std::array{
		glm::vec3{-1, t, 0},		glm::vec3{1, t, 0},		glm::vec3{-1, -t, 0},		glm::vec3{1, -t, 0},
		glm::vec3{0, -1, t},		glm::vec3{0, 1, t},		glm::vec3{0, -1, -t},		glm::vec3{0, 1, -t},
		glm::vec3{t, 0, -1},		glm::vec3{t, 0, 1},		glm::vec3{-t, 0, -1},		glm::vec3{-t, 0, 1}
	};

	static auto constexpr faces = std::array{
		std::array{0u, 11u, 5u}, 	    std::array{0u, 5u, 1u}, 	std::array{0u, 1u, 7u}, 		std::array{0u, 7u, 10u}, 	std::array{0u, 10u, 11u},
		std::array{1u, 5u, 9u}, 		std::array{5u, 11u, 4u},	std::array{11u, 10u, 2u},	    std::array{10u, 7u, 6u},	std::array{7u, 1u, 8u},
		std::array{3u, 9u, 4u}, 		std::array{3u, 4u, 2u},	    std::array{3u, 2u, 6u},		    std::array{3u, 6u, 8u},		std::array{3u, 8u, 9u},
		std::array{4u, 9u, 5u}, 		std::array{2u, 4u, 11u},	std::array{6u, 2u, 10u},		std::array{8u, 6u, 7u},		std::array{9u, 8u, 1u}
	};

    static auto constexpr offsets_pattern = std::array{
        std::array{
            std::pair{0, 1}, std::pair{1, 0}, std::pair{0, 0}
        },
        std::array{
            std::pair{0, 1}, std::pair{1, 1}, std::pair{1, 0}
        }
    };

    glm::vec3 generate_point(primitives::icosahedron_create_info const &create_info, std::span<std::uint32_t const, 3> face, std::uint32_t i, std::uint32_t j)
    {
        auto &&a = input_vertices[face[0]];
        auto &&b = input_vertices[face[1]];
        auto &&c = input_vertices[face[2]];

        auto const columns = create_info.detail + 1;

        auto point = glm::mix(a, c, static_cast<float>(i) / columns);

        if (i != columns || j != 0)
        {
            auto bj = glm::mix(b, c, static_cast<float>(i) / columns);
            point = glm::mix(point, bj, static_cast<float>(j) / (columns - i));
        }

        return glm::normalize(point);
    }

    template<std::size_t N, class T>
    std::array<T, N> generate_position(primitives::icosahedron_create_info const &create_info, std::span<std::uint32_t const, 3> face, std::uint32_t i, std::uint32_t j)
    {
        auto point = generate_point(create_info, face, i, j) * create_info.radius;

        if constexpr (N == 4)
            return std::array<T, N>{static_cast<T>(point.x), static_cast<T>(point.y), static_cast<T>(point.z), 1};

        else if constexpr (N == 3)
            return std::array<T, N>{static_cast<T>(point.x), static_cast<T>(point.y), static_cast<T>(point.z)};

        else throw resource::exception("unsupported components number"s);
    }

    template<std::size_t N, class T>
    std::array<T, N> generate_normal(primitives::icosahedron_create_info const &create_info, std::span<std::uint32_t const, 3> face, std::uint32_t i, std::uint32_t j)
    {
        auto point = generate_point(create_info, face, i, j);

        if constexpr (N == 2) {
            std::array<T, 2> oct;
            math::encode_unit_vector_to_oct_fast(std::span{oct}, glm::vec3{point});

            return oct;
        }

        else if constexpr (N == 3)
            return std::array<T, N>{static_cast<T>(point.x), static_cast<T>(point.y), static_cast<T>(point.z)};

        else throw resource::exception("unsupported components number"s);
    }

    template<class T>
    void subdive_face(primitives::icosahedron_create_info const &create_info, strided_bidirectional_iterator<T> it_begin, std::span<std::uint32_t const, 3> indices)
    {
        auto &&a = input_vertices[indices[0]];
        auto &&b = input_vertices[indices[1]];
        auto &&c = input_vertices[indices[2]];

        auto const columns = create_info.detail + 1;

        glm::vec3 aj{0}, bj{0};

        auto get_vertex = [&] (std::uint32_t i, std::uint32_t j)
        {
            aj = glm::mix(a, c, static_cast<float>(i) / columns);

            if (i == 0 && j == columns)
                return aj;

            else
            {
                bj = glm::mix(b, c, static_cast<float>(i) / columns);
                return glm::mix(aj, bj, static_cast<float>(j) / (columns - i));
            }
        };

        for (auto i = 0u; i < columns; ++i) {
            for (auto j = 0u; j < 2 * (columns - i) - 1; ++j) {
                auto const k = j / 2;

                if (j % 2 == 0) {
                    ;
                }

                else {
                    ;
                }
            }
        }
    }

    template<class T>
    void subdive(primitives::icosahedron_create_info const &create_info, strided_bidirectional_iterator<T> it_begin)
    {
        for (auto &&i : faces) {
            subdive_face(create_info.detail, std::span{i});
        }
    }

    template<std::size_t N, class T, class F>
    void generate_vertex_as_triangles(F generator, primitives::icosahedron_create_info const &create_info,
                                      strided_bidirectional_iterator<std::array<T, N>> it)
    {
        auto const columns = create_info.detail + 1;

        for (auto &&face : faces) {
            for (auto i = 0u; i < columns; ++i) {
                for (auto j = 0u; j < 2 * (columns - i) - 1; ++j) {
                    auto const k = j / 2;
                    auto const pattern_index = j % 2;

                    std::transform(std::cbegin(offsets_pattern[pattern_index]), std::cend(offsets_pattern[pattern_index]), it, [&face, generator, i, k] (auto offsets)
                    {
                        return generator(face, i + std::get<0>(offsets), k + std::get<1>(offsets));
                    });

                    std::advance(it, std::size(offsets_pattern[pattern_index]));
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
        auto const columns = create_info.detail + 1;

        if constexpr (N == 3 || N == 4) {
            switch (graphics::numeric_format(format)) {
                case graphics::NUMERIC_FORMAT::FLOAT:
                    {
                        auto generator = std::bind(generate_position<N, T>, create_info, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
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
                        /*auto generate_normal = [] (glm::vec<N, T> const &point)
                        {
                            std::array<T, 2> oct;
                            math::encode_unit_vector_to_oct_fast(std::span{oct}, point.x, point.y, point.z);

                            return oct;
                        };*/

                        auto generator = std::bind(generate_normal<N, T>, create_info, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
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
                case graphics::NUMERIC_FORMAT::SCALED:
                case graphics::NUMERIC_FORMAT::INT:
                case graphics::NUMERIC_FORMAT::FLOAT:
                    {
                        /*auto generate_normal = [] (glm::vec<N, T> const &point)
                        {
                            return std::array<T, N>{static_cast<T>(point.x), static_cast<T>(point.y), static_cast<T>(point.z)};
                        };*/
                        auto generator = std::bind(generate_normal<N, T>, create_info, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
                        generate_vertex(generator, create_info, it);
                    }
                    break;

                default:
                    throw resource::exception("unsupported numeric format"s);
            }
        }

        else throw resource::exception("unsupported components number"s);
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

        //auto vertex_count = calculate_icosahedron_vertices_count(create_info);
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
                            generate_positions(create_info, attribute.format, it);
                            break;

                        case vertex::SEMANTIC::NORMAL:
                            generate_normals(create_info, attribute.format, it);
                            break;

                        case vertex::SEMANTIC::TEXCOORD_0:
                            //generate_texcoords(create_info, attribute.format, it, vertex_count);
                            break;

                        case vertex::SEMANTIC::COLOR_0:
                            //generate_colors(color, attribute.format, it, vertex_count);
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