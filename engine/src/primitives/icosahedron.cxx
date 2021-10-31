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

	static auto const vertices = std::array{
		glm::vec3{-1, t, 0},		glm::vec3{1, t, 0},		glm::vec3{-1, -t, 0},		glm::vec3{1, -t, 0},
		glm::vec3{0, -1, t},		glm::vec3{0, 1, t},		glm::vec3{0, -1, -t},		glm::vec3{0, 1, -t},
		glm::vec3{t, 0, -1},		glm::vec3{t, 0, 1},		glm::vec3{-t, 0, -1},		glm::vec3{-t, 0, 1}
	};

	static auto constexpr indices = std::array{
		std::array{0u, 11u, 5u}, 	    std::array{0u, 5u, 1u}, 	std::array{0u, 1u, 7u}, 		std::array{0u, 7u, 10u}, 	std::array{0u, 10u, 11u},
		std::array{1u, 5u, 9u}, 		std::array{5u, 11u, 4u},	std::array{11u, 10u, 2u},	    std::array{10u, 7u, 6u},	std::array{7u, 1u, 8u},
		std::array{3u, 9u, 4u}, 		std::array{3u, 4u, 2u},	    std::array{3u, 2u, 6u},		    std::array{3u, 6u, 8u},		std::array{3u, 8u, 9u},
		std::array{4u, 9u, 5u}, 		std::array{2u, 4u, 11u},	std::array{6u, 2u, 10u},		std::array{8u, 6u, 7u},		std::array{9u, 8u, 1u}
	};

    void subdive_face(std::uint32_t detail_level, glm::vec3 const &a, glm::vec3 const &b, glm::vec3 const &c)
    {
        auto const columns = detail_level + 1;

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

    void subdive(std::uint32_t detail_level)
    {
        glm::vec3 a, b, c;

        for (auto &&i : indices) {
            a = vertices[i[0]];
            b = vertices[i[1]];
            c = vertices[i[2]];

            subdive_face(detail_level, a, b, c);
        }
    }

    template<class T, class F>
    void generate_vertex_as_triangles(F generator, primitives::icosahedron_create_info const &,
                                      strided_bidirectional_iterator<T> it_begin, std::uint32_t vertex_number)
    {
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
                         strided_bidirectional_iterator<T> it_begin, std::uint32_t vertex_number)
    {
        switch (create_info.topology) {
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLES:
                generate_vertex_as_triangles(generator, create_info, it_begin, vertex_number);
                break;

            case graphics::PRIMITIVE_TOPOLOGY::POINTS:
            case graphics::PRIMITIVE_TOPOLOGY::LINES:
            case graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP:
            default:
                throw resource::exception("unsupported primitive topology"s);
        }
    }

    template<std::size_t N, class T>
    std::array<T, N> generate_position(primitives::icosahedron_create_info const &create_info, std::uint32_t vertex_index)
    {
       /* auto const hsegments = create_info.hsegments;
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

        else */throw resource::exception("unsupported components number"s);
    }

    template<std::size_t N, class T>
    void generate_positions(primitives::icosahedron_create_info const &create_info, graphics::FORMAT format,
                            strided_bidirectional_iterator<std::array<T, N>> it_begin, std::uint32_t vertex_number)
    {
        if constexpr (N == 2 || N == 3) {
            switch (graphics::numeric_format(format)) {
                case graphics::NUMERIC_FORMAT::FLOAT:
                    {
                        auto generator = std::bind(generate_position<N, T>, create_info, std::placeholders::_1);
                        generate_vertex(generator, create_info, it_begin, vertex_number);
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
		return 0;
	}

	void generate_icosahedron(primitives::icosahedron_create_info const &create_info, std::span<std::byte> vertex_buffer)
	{
		
        auto &&vertex_layout = create_info.vertex_layout;

        auto vertex_number = calculate_icosahedron_vertices_count(create_info);
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
                            generate_positions(create_info, attribute.format, it, vertex_number);
                            break;

                        case vertex::SEMANTIC::NORMAL:
                            //generate_normals(attribute.format, it, vertex_number);
                            break;

                        case vertex::SEMANTIC::TEXCOORD_0:
                            //generate_texcoords(create_info, attribute.format, it, vertex_number);
                            break;

                        case vertex::SEMANTIC::COLOR_0:
                            //generate_colors(color, attribute.format, it, vertex_number);
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