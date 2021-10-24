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
    auto constexpr teapot_patches = std::array{
        /*rim*/
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        3, 16, 17, 18, 7, 19, 20, 21, 11, 22, 23, 24, 15, 25, 26, 27,
        18, 28, 29, 30, 21, 31, 32, 33, 24, 34, 35, 36, 27, 37, 38, 39,
        30, 40, 41, 0, 33, 42, 43, 4, 36, 44, 45, 8, 39, 46, 47, 12,
        /*body*/
        12, 13, 14, 15, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
        15, 25, 26, 27, 51, 60, 61, 62, 55, 63, 64, 65, 59, 66, 67, 68,
        27, 37, 38, 39, 62, 69, 70, 71, 65, 72, 73, 74, 68, 75, 76, 77,
        39, 46, 47, 12, 71, 78, 79, 48, 74, 80, 81, 52, 77, 82, 83, 56,
        56, 57, 58, 59, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
        59, 66, 67, 68, 87, 96, 97, 98, 91, 99, 100, 101, 95, 102, 103, 104,
        68, 75, 76, 77, 98, 105, 106, 107, 101, 108, 109, 110, 104, 111, 112, 113,
        77, 82, 83, 56, 107, 114, 115, 84, 110, 116, 117, 88, 113, 118, 119, 92,
        /*handle*/
        120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135,
        123, 136, 137, 120, 127, 138, 139, 124, 131, 140, 141, 128, 135, 142, 143, 132,
        132, 133, 134, 135, 144, 145, 146, 147, 148, 149, 150, 151, 68, 152, 153, 154,
        135, 142, 143, 132, 147, 155, 156, 144, 151, 157, 158, 148, 154, 159, 160, 68,
        /*spout*/
        161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176,
        164, 177, 178, 161, 168, 179, 180, 165, 172, 181, 182, 169, 176, 183, 184, 173,
        173, 174, 175, 176, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196,
        176, 183, 184, 173, 188, 197, 198, 185, 192, 199, 200, 189, 196, 201, 202, 193,
        /*lid*/
        203, 203, 203, 203, 204, 205, 206, 207, 208, 208, 208, 208, 209, 210, 211, 212,
        203, 203, 203, 203, 207, 213, 214, 215, 208, 208, 208, 208, 212, 216, 217, 218,
        203, 203, 203, 203, 215, 219, 220, 221, 208, 208, 208, 208, 218, 222, 223, 224,
        203, 203, 203, 203, 221, 225, 226, 204, 208, 208, 208, 208, 224, 227, 228, 209,
        209, 210, 211, 212, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240,
        212, 216, 217, 218, 232, 241, 242, 243, 236, 244, 245, 246, 240, 247, 248, 249,
        218, 222, 223, 224, 243, 250, 251, 252, 246, 253, 254, 255, 249, 256, 257, 258,
        224, 227, 228, 209, 252, 259, 260, 229, 255, 261, 262, 233, 258, 263, 264, 237,
        /*bottom*/
        265, 265, 265, 265, 266, 267, 268, 269, 270, 271, 272, 273, 92, 119, 118, 113,
        265, 265, 265, 265, 269, 274, 275, 276, 273, 277, 278, 279, 113, 112, 111, 104,
        265, 265, 265, 265, 276, 280, 281, 282, 279, 283, 284, 285, 104, 103, 102, 95,
        265, 265, 265, 265, 282, 286, 287, 266, 285, 288, 289, 270, 95, 94, 93, 92
    };

    auto constexpr teapot_vertices = std::array{
        1.4, 0, 2.4,
        1.4, -0.784, 2.4,
        0.784, -1.4, 2.4,
        0, -1.4, 2.4,
        1.3375, 0, 2.53125,
        1.3375, -0.749, 2.53125,
        0.749, -1.3375, 2.53125,
        0, -1.3375, 2.53125,
        1.4375, 0, 2.53125,
        1.4375, -0.805, 2.53125,
        0.805, -1.4375, 2.53125,
        0, -1.4375, 2.53125,
        1.5, 0, 2.4,
        1.5, -0.84, 2.4,
        0.84, -1.5, 2.4,
        0, -1.5, 2.4,
        -0.784, -1.4, 2.4,
        -1.4, -0.784, 2.4,
        -1.4, 0, 2.4,
        -0.749, -1.3375, 2.53125,
        -1.3375, -0.749, 2.53125,
        -1.3375, 0, 2.53125,
        -0.805, -1.4375, 2.53125,
        -1.4375, -0.805, 2.53125,
        -1.4375, 0, 2.53125,
        -0.84, -1.5, 2.4,
        -1.5, -0.84, 2.4,
        -1.5, 0, 2.4,
        -1.4, 0.784, 2.4,
        -0.784, 1.4, 2.4,
        0, 1.4, 2.4,
        -1.3375, 0.749, 2.53125,
        -0.749, 1.3375, 2.53125,
        0, 1.3375, 2.53125,
        -1.4375, 0.805, 2.53125,
        -0.805, 1.4375, 2.53125,
        0, 1.4375, 2.53125,
        -1.5, 0.84, 2.4,
        -0.84, 1.5, 2.4,
        0, 1.5, 2.4,
        0.784, 1.4, 2.4,
        1.4, 0.784, 2.4,
        0.749, 1.3375, 2.53125,
        1.3375, 0.749, 2.53125,
        0.805, 1.4375, 2.53125,
        1.4375, 0.805, 2.53125,
        0.84, 1.5, 2.4,
        1.5, 0.84, 2.4,
        1.75, 0, 1.875,
        1.75, -0.98, 1.875,
        0.98, -1.75, 1.875,
        0, -1.75, 1.875,
        2, 0, 1.35,
        2, -1.12, 1.35,
        1.12, -2, 1.35,
        0, -2, 1.35,
        2, 0, 0.9,
        2, -1.12, 0.9,
        1.12, -2, 0.9,
        0, -2, 0.9,
        -0.98, -1.75, 1.875,
        -1.75, -0.98, 1.875,
        -1.75, 0, 1.875,
        -1.12, -2, 1.35,
        -2, -1.12, 1.35,
        -2, 0, 1.35,
        -1.12, -2, 0.9,
        -2, -1.12, 0.9,
        -2, 0, 0.9,
        -1.75, 0.98, 1.875,
        -0.98, 1.75, 1.875,
        0, 1.75, 1.875,
        -2, 1.12, 1.35,
        -1.12, 2, 1.35,
        0, 2, 1.35,
        -2, 1.12, 0.9,
        -1.12, 2, 0.9,
        0, 2, 0.9,
        0.98, 1.75, 1.875,
        1.75, 0.98, 1.875,
        1.12, 2, 1.35,
        2, 1.12, 1.35,
        1.12, 2, 0.9,
        2, 1.12, 0.9,
        2, 0, 0.45,
        2, -1.12, 0.45,
        1.12, -2, 0.45,
        0, -2, 0.45,
        1.5, 0, 0.225,
        1.5, -0.84, 0.225,
        0.84, -1.5, 0.225,
        0, -1.5, 0.225,
        1.5, 0, 0.15,
        1.5, -0.84, 0.15,
        0.84, -1.5, 0.15,
        0, -1.5, 0.15,
        -1.12, -2, 0.45,
        -2, -1.12, 0.45,
        -2, 0, 0.45,
        -0.84, -1.5, 0.225,
        -1.5, -0.84, 0.225,
        -1.5, 0, 0.225,
        -0.84, -1.5, 0.15,
        -1.5, -0.84, 0.15,
        -1.5, 0, 0.15,
        -2, 1.12, 0.45,
        -1.12, 2, 0.45,
        0, 2, 0.45,
        -1.5, 0.84, 0.225,
        -0.84, 1.5, 0.225,
        0, 1.5, 0.225,
        -1.5, 0.84, 0.15,
        -0.84, 1.5, 0.15,
        0, 1.5, 0.15,
        1.12, 2, 0.45,
        2, 1.12, 0.45,
        0.84, 1.5, 0.225,
        1.5, 0.84, 0.225,
        0.84, 1.5, 0.15,
        1.5, 0.84, 0.15,
        -1.6, 0, 2.025,
        -1.6, -0.3, 2.025,
        -1.5, -0.3, 2.25,
        -1.5, 0, 2.25,
        -2.3, 0, 2.025,
        -2.3, -0.3, 2.025,
        -2.5, -0.3, 2.25,
        -2.5, 0, 2.25,
        -2.7, 0, 2.025,
        -2.7, -0.3, 2.025,
        -3, -0.3, 2.25,
        -3, 0, 2.25,
        -2.7, 0, 1.8,
        -2.7, -0.3, 1.8,
        -3, -0.3, 1.8,
        -3, 0, 1.8,
        -1.5, 0.3, 2.25,
        -1.6, 0.3, 2.025,
        -2.5, 0.3, 2.25,
        -2.3, 0.3, 2.025,
        -3, 0.3, 2.25,
        -2.7, 0.3, 2.025,
        -3, 0.3, 1.8,
        -2.7, 0.3, 1.8,
        -2.7, 0, 1.575,
        -2.7, -0.3, 1.575,
        -3, -0.3, 1.35,
        -3, 0, 1.35,
        -2.5, 0, 1.125,
        -2.5, -0.3, 1.125,
        -2.65, -0.3, 0.9375,
        -2.65, 0, 0.9375,
        -2, -0.3, 0.9,
        -1.9, -0.3, 0.6,
        -1.9, 0, 0.6,
        -3, 0.3, 1.35,
        -2.7, 0.3, 1.575,
        -2.65, 0.3, 0.9375,
        -2.5, 0.3, 1.125,
        -1.9, 0.3, 0.6,
        -2, 0.3, 0.9,
        1.7, 0, 1.425,
        1.7, -0.66, 1.425,
        1.7, -0.66, 0.6,
        1.7, 0, 0.6,
        2.6, 0, 1.425,
        2.6, -0.66, 1.425,
        3.1, -0.66, 0.825,
        3.1, 0, 0.825,
        2.3, 0, 2.1,
        2.3, -0.25, 2.1,
        2.4, -0.25, 2.025,
        2.4, 0, 2.025,
        2.7, 0, 2.4,
        2.7, -0.25, 2.4,
        3.3, -0.25, 2.4,
        3.3, 0, 2.4,
        1.7, 0.66, 0.6,
        1.7, 0.66, 1.425,
        3.1, 0.66, 0.825,
        2.6, 0.66, 1.425,
        2.4, 0.25, 2.025,
        2.3, 0.25, 2.1,
        3.3, 0.25, 2.4,
        2.7, 0.25, 2.4,
        2.8, 0, 2.475,
        2.8, -0.25, 2.475,
        3.525, -0.25, 2.49375,
        3.525, 0, 2.49375,
        2.9, 0, 2.475,
        2.9, -0.15, 2.475,
        3.45, -0.15, 2.5125,
        3.45, 0, 2.5125,
        2.8, 0, 2.4,
        2.8, -0.15, 2.4,
        3.2, -0.15, 2.4,
        3.2, 0, 2.4,
        3.525, 0.25, 2.49375,
        2.8, 0.25, 2.475,
        3.45, 0.15, 2.5125,
        2.9, 0.15, 2.475,
        3.2, 0.15, 2.4,
        2.8, 0.15, 2.4,
        0, 0, 3.15,
        0.8, 0, 3.15,
        0.8, -0.45, 3.15,
        0.45, -0.8, 3.15,
        0, -0.8, 3.15,
        0, 0, 2.85,
        0.2, 0, 2.7,
        0.2, -0.112, 2.7,
        0.112, -0.2, 2.7,
        0, -0.2, 2.7,
        -0.45, -0.8, 3.15,
        -0.8, -0.45, 3.15,
        -0.8, 0, 3.15,
        -0.112, -0.2, 2.7,
        -0.2, -0.112, 2.7,
        -0.2, 0, 2.7,
        -0.8, 0.45, 3.15,
        -0.45, 0.8, 3.15,
        0, 0.8, 3.15,
        -0.2, 0.112, 2.7,
        -0.112, 0.2, 2.7,
        0, 0.2, 2.7,
        0.45, 0.8, 3.15,
        0.8, 0.45, 3.15,
        0.112, 0.2, 2.7,
        0.2, 0.112, 2.7,
        0.4, 0, 2.55,
        0.4, -0.224, 2.55,
        0.224, -0.4, 2.55,
        0, -0.4, 2.55,
        1.3, 0, 2.55,
        1.3, -0.728, 2.55,
        0.728, -1.3, 2.55,
        0, -1.3, 2.55,
        1.3, 0, 2.4,
        1.3, -0.728, 2.4,
        0.728, -1.3, 2.4,
        0, -1.3, 2.4,
        -0.224, -0.4, 2.55,
        -0.4, -0.224, 2.55,
        -0.4, 0, 2.55,
        -0.728, -1.3, 2.55,
        -1.3, -0.728, 2.55,
        -1.3, 0, 2.55,
        -0.728, -1.3, 2.4,
        -1.3, -0.728, 2.4,
        -1.3, 0, 2.4,
        -0.4, 0.224, 2.55,
        -0.224, 0.4, 2.55,
        0, 0.4, 2.55,
        -1.3, 0.728, 2.55,
        -0.728, 1.3, 2.55,
        0, 1.3, 2.55,
        -1.3, 0.728, 2.4,
        -0.728, 1.3, 2.4,
        0, 1.3, 2.4,
        0.224, 0.4, 2.55,
        0.4, 0.224, 2.55,
        0.728, 1.3, 2.55,
        1.3, 0.728, 2.55,
        0.728, 1.3, 2.4,
        1.3, 0.728, 2.4,
        0, 0, 0,
        1.425, 0, 0,
        1.425, 0.798, 0,
        0.798, 1.425, 0,
        0, 1.425, 0,
        1.5, 0, 0.075,
        1.5, 0.84, 0.075,
        0.84, 1.5, 0.075,
        0, 1.5, 0.075,
        -0.798, 1.425, 0,
        -1.425, 0.798, 0,
        -1.425, 0, 0,
        -0.84, 1.5, 0.075,
        -1.5, 0.84, 0.075,
        -1.5, 0, 0.075,
        -1.425, -0.798, 0,
        -0.798, -1.425, 0,
        0, -1.425, 0,
        -1.5, -0.84, 0.075,
        -0.84, -1.5, 0.075,
        0, -1.5, 0.075,
        0.798, -1.425, 0,
        1.425, -0.798, 0,
        0.84, -1.5, 0.075,
        1.5, -0.84, 0.075
    };
}


namespace primitives
{
    std::uint32_t calculate_teapot_vertices_number(primitives::teapot_create_info const &create_info)
    {
        auto vertices_number = calculate_teapot_faces_vertices_count(create_info);
        return (vertices_number.at(0) + vertices_number.at(1) + vertices_number.at(2)) * 2;
    }

    std::uint32_t calculate_teapot_indices_number(primitives::teapot_create_info const &create_info)
    {
        if (create_info.hsegments * create_info.vsegments * create_info.dsegments < 1)
            throw resource::exception("invalid teapot segments' values"s);

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

    void generate_teapot_indexed(primitives::teapot_create_info const &create_info, std::span<std::byte> vertex_buffer,
                              std::span<std::byte> index_buffer)
    {
        auto indices_number = calculate_teapot_indices_number(create_info);

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

        generate_teapot(create_info, vertex_buffer);
    }

    void generate_teapot(primitives::teapot_create_info const &create_info, std::span<std::byte> vertex_buffer)
    {
        auto &&vertex_layout = create_info.vertex_layout;

        auto vertex_number = calculate_teapot_vertices_number(create_info);
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
                            generate_texcoords(create_info, attribute.format, it, vertex_number);
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
