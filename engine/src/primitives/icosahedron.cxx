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

	static auto constexpr input_indices = std::array{
		std::array{0u, 11u, 5u}, 	    std::array{0u, 5u, 1u}, 	std::array{0u, 1u, 7u}, 		std::array{0u, 7u, 10u}, 	std::array{0u, 10u, 11u},
		std::array{1u, 5u, 9u}, 		std::array{5u, 11u, 4u},	std::array{11u, 10u, 2u},	    std::array{10u, 7u, 6u},	std::array{7u, 1u, 8u},
		std::array{3u, 9u, 4u}, 		std::array{3u, 4u, 2u},	    std::array{3u, 2u, 6u},		    std::array{3u, 6u, 8u},		std::array{3u, 8u, 9u},
		std::array{4u, 9u, 5u}, 		std::array{2u, 4u, 11u},	std::array{6u, 2u, 10u},		std::array{8u, 6u, 7u},		std::array{9u, 8u, 1u}
	};

    template<std::size_t N, class T>
    std::array<T, N> generate_position(primitives::icosahedron_create_info const &create_info, std::uint32_t face_index, std::uint32_t i, std::uint32_t j)
    {
        auto &&a = input_vertices[input_indices[face_index][0]];
        auto &&b = input_vertices[input_indices[face_index][1]];
        auto &&c = input_vertices[input_indices[face_index][2]];

        auto const columns = create_info.detail + 1;

        auto pos = glm::mix(a, c, static_cast<float>(i) / columns);

        if (i != 0 || j != columns)
        {
            auto bj = glm::mix(b, c, static_cast<float>(i) / columns);
            pos = glm::mix(pos, bj, static_cast<float>(j) / (columns - i));
        }

        pos = glm::normalize(pos) * create_info.radius;

        if constexpr (N == 4)
            return std::array<T, N>{static_cast<T>(pos.x), static_cast<T>(pos.y), static_cast<T>(pos.z), 1};

        else if constexpr (N == 3)
            return std::array<T, N>{static_cast<T>(pos.x), static_cast<T>(pos.y), static_cast<T>(pos.z)};

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
        for (auto &&i : input_indices) {
            subdive_face(create_info.detail, std::span{i});
        }
    }

    template<class T, class F>
    void generate_vertex_as_triangles(F generator, primitives::icosahedron_create_info const &create_info,
                                      strided_bidirectional_iterator<T> it)
    {
        auto const columns = create_info.detail + 1;

        for (auto face_index = 0u; [[maybe_unused]] auto &&face : input_indices) {
            for (auto i = 0u; i < columns; ++i) {
                for (auto j = 0u; j < 2 * (columns - i) - 1; ++j) {
                    auto const k = j / 2;

                    if (j % 2 == 0) {
                        *it = generator(face_index, i, k + 1);
                        *++it = generator(face_index, i + 1, k);
                        *++it = generator(face_index, i, k);
                    }

                    else {
                        *it = generator(face_index, i, k + 1);
                        *++it = generator(face_index, i + 1, k + 1);
                        *++it = generator(face_index, i + 1, k);
                    }
                }
            }

            ++face_index;
        }

        /*auto const hsegments = create_info.hsegments;
        auto const vsegments = create_info.vsegments;

        auto const pattern = std::array{0u, hsegments + 1, 1u, 1u, hsegments + 1, hsegments + 2};

        auto const vertices_per_quad = 2 * 3;
        auto const horizontal_vertices_number = hsegments * vertices_per_quad;

        for (auto vsegment_index = 0u; vsegment_index < vsegments; ++vsegment_index) {
            for (auto hsegment_index = 0u; hsegment_index < hsegments; ++hsegment_index) {
                auto const offset = horizontal_vertices_number * vsegment_index + hsegment_index * vertices_per_quad;
                auto it = std::next(it_begin, static_cast<std::ptrdiff_t>(offset));

                std::transform(std::cbegin(pattern), std::cend(pattern), it, [&] (auto column)
                {
                    auto vertex_index = vsegment_index * (hsegments + 1) + column + hsegment_index;

                    return generator(vertex_index);
                });
            }
        }*/
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
                            strided_bidirectional_iterator<std::array<T, N>> it_begin)
    {
        auto const columns = create_info.detail + 1;

        if constexpr (N == 3 || N == 4) {
            switch (graphics::numeric_format(format)) {
                case graphics::NUMERIC_FORMAT::FLOAT:
                    {
                        auto generator = std::bind(generate_position<N, T>, create_info, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
                        generate_vertex(generator, create_info, it_begin);
                    }
                    break;

                default:
                    throw resource::exception("unsupported numeric format"s);
                    break;
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

        return static_cast<std::uint32_t>(std::size(input_indices)) * vertices_count;
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
                            //generate_normals(attribute.format, it, vertex_count);
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