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
    template<std::size_t N, class T>
    std::array<T, N>
    generate_position(float width, float height, std::size_t hsegments, std::size_t vsegments, std::size_t vertex_index)
    {
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
    generate_texcoord(graphics::FORMAT format, std::size_t hsegments, std::size_t vsegments, std::size_t vertex_index)
    {
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
    std::array<T, N> generate_color(glm::vec4 const &color, graphics::FORMAT format, std::size_t, std::size_t, std::size_t)
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

    template<std::size_t N, class T, class F>
    void generate_vertex(F generator, std::uint32_t hsegments, std::uint32_t vsegments,
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

                return generator(hsegments, vsegments, vertex_index);
            });

            std::generate_n(std::next(it, 2), vertices_per_strip - 2, [&, triangle_index = strip_index * hsegments * 2] () mutable
            {
                auto quad_index = triangle_index / 2;

                auto column = quad_index % hsegments + 1;
                auto row = quad_index / hsegments + (triangle_index % 2);

                ++triangle_index;

                auto vertex_index = row * (hsegments + 1) + column;

                return generator(hsegments, vsegments, vertex_index);
            });

            it = std::next(it, vertices_per_strip);

            if (it < it_end) {
                std::generate_n(it, 2, [&, i = 0u] () mutable {
                    auto vertex_index = (strip_index + 1) * (hsegments + 1) + hsegments * (1 - i++);

                    return generator(hsegments, vsegments, vertex_index);
                });
            }
        }
    }

    template<std::size_t N, class T>
    void generate_positions([[maybe_unused]] graphics::FORMAT format,
                            [[maybe_unused]] float width, [[maybe_unused]] float height,
                            [[maybe_unused]] std::uint32_t hsegments, [[maybe_unused]] std::uint32_t vsegments,
                            [[maybe_unused]] strided_bidirectional_iterator<std::array<T, N>> it_begin,
                            [[maybe_unused]] std::size_t vertex_count)
    {
        using std::placeholders::_1;
        using std::placeholders::_2;
        using std::placeholders::_3;

        if constexpr (N == 2 || N == 3) {
            switch (graphics::numeric_format(format)) {
                case graphics::NUMERIC_FORMAT::FLOAT:
                    generate_vertex<N, T>(
                        std::bind(generate_position<N, T>, width, height, _1, _2, _3),
                        hsegments, vsegments, it_begin, vertex_count
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
    void generate_normals([[maybe_unused]] graphics::FORMAT format,
                          [[maybe_unused]] strided_bidirectional_iterator<std::array<T, N>> it,
                          [[maybe_unused]] std::size_t vertex_count)
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
    void generate_texcoords(graphics::FORMAT format,
                            strided_bidirectional_iterator<std::array<T, N>> it_begin,
                            std::uint32_t hsegments, std::uint32_t vsegments, std::size_t vertex_count)
    {
        using std::placeholders::_1;
        using std::placeholders::_2;
        using std::placeholders::_3;

        generate_vertex<N, T>(std::bind(generate_texcoord<N, T>, format, _1, _2, _3),
                              hsegments, vsegments, it_begin, vertex_count);
    }

    template<std::size_t N, class T>
    void generate_colors(glm::vec4 const &color, graphics::FORMAT format,
                         strided_bidirectional_iterator<std::array<T, N>> it_begin,
                         std::uint32_t hsegments, std::uint32_t vsegments, std::size_t vertex_count)
    {
        using std::placeholders::_1;
        using std::placeholders::_2;
        using std::placeholders::_3;

        generate_vertex<N, T>(std::bind(generate_color<N, T>, color, format, _1, _2, _3),
                              hsegments, vsegments, it_begin, vertex_count);
    }
    
    std::vector<std::byte>
    generate_plane(float width, float height, std::uint32_t hsegments, std::uint32_t vsegments,
                   graphics::vertex_layout const &vertex_layout, glm::vec4 const &color)
    {
        std::size_t vertex_count = (hsegments + 1) * 2 * vsegments + (vsegments - 1) * 2;
        std::size_t vertex_size = vertex_layout.size_in_bytes;

        std::vector<std::byte> bytes(vertex_size * vertex_count);

        auto &&attributes = vertex_layout.attributes;

        for (auto &&attribute : attributes) {
            auto attribute_semantic = std::visit([] (auto semantic)
            {
                return semantic.semantic_index;
            }, attribute.semantic);

            if (auto format_inst = graphics::instantiate_format(attribute.format); format_inst) {
                std::visit([&] (auto &&format_inst)
                {
                    using type = typename std::remove_cvref_t<decltype(format_inst)>;
                    using pointer_type = typename std::add_pointer_t<type>;

                    auto data = reinterpret_cast<pointer_type>(std::data(bytes) + attribute.offset_in_bytes);

                    auto it = strided_bidirectional_iterator<type>{data, vertex_size};

                    switch (attribute_semantic) {
                        case vertex::eSEMANTIC_INDEX::POSITION:
                            generate_positions(attribute.format, width, height, hsegments, vsegments, it, vertex_count);
                            break;

                        case vertex::eSEMANTIC_INDEX::NORMAL:
                            generate_normals(attribute.format, it, vertex_count);
                            break;

                        case vertex::eSEMANTIC_INDEX::TEXCOORD_0:
                            generate_texcoords(attribute.format, it, hsegments, vsegments, vertex_count);
                            break;

                        case vertex::eSEMANTIC_INDEX::COLOR_0:
                            generate_colors(color, attribute.format, it, hsegments, vsegments, vertex_count);
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
}
