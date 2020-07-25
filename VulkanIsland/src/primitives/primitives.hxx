#pragma once

#include <vector>
#include <cstdint>

#include "graphics/vertex.hxx"


namespace primitives
{
    struct plane_create_info final {
        graphics::vertex_layout vertex_layout;

        graphics::PRIMITIVE_TOPOLOGY topology;

        graphics::FORMAT index_buffer_format;

        float width, height;
        unsigned hsegments, vsegments;
    };

    struct box_create_info final {
        graphics::vertex_layout vertex_layout;

        graphics::PRIMITIVE_TOPOLOGY topology;

        graphics::FORMAT index_buffer_format;

        float width, height, depth;
        unsigned hsegments, vsegments, dsegments;
    };

    std::vector<std::byte>
    generate_plane(primitives::plane_create_info const &plane_create_info, glm::vec4 const &color = glm::vec4{1});

    /*std::vector<std::byte>
    generate_plane_indexed(float width, float height, std::uint32_t hsegments, std::uint32_t vsegments,
                           graphics::vertex_layout const &vertex_layout, glm::vec4 const &color = glm::vec4{1});*/

    std::vector<std::byte>
    generate_box_indexed(primitives::box_create_info const &create_info, glm::vec4 const &color = glm::vec4{1});

    std::uint32_t calculate_box_vertices_number(primitives::box_create_info const &create_info);

    std::uint32_t calculate_box_indices_number(primitives::box_create_info const &create_info);
}

