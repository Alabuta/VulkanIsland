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

    std::uint32_t calculate_plane_vertices_number(primitives::plane_create_info const &create_info);

    std::uint32_t calculate_plane_indices_number(primitives::plane_create_info const &create_info);

    void generate_plane(primitives::plane_create_info const &plane_create_info, std::vector<std::byte>::iterator it_vertex_buffer,
                        glm::vec4 const &color = glm::vec4{1});

    void generate_plane_indexed(primitives::plane_create_info const &plane_create_info, std::vector<std::byte>::iterator it_vertex_buffer,
                                std::vector<std::byte>::iterator it_index_buffer, glm::vec4 const &color = glm::vec4{1});

    struct box_create_info final {
        graphics::vertex_layout vertex_layout;

        graphics::PRIMITIVE_TOPOLOGY topology;

        graphics::FORMAT index_buffer_format;

        float width, height, depth;
        unsigned hsegments, vsegments, dsegments;
    };

    std::uint32_t calculate_box_vertices_number(primitives::box_create_info const &create_info);

    std::uint32_t calculate_box_indices_number(primitives::box_create_info const &create_info);

    std::vector<std::byte>
    generate_box_indexed(primitives::box_create_info const &create_info, glm::vec4 const &color = glm::vec4{1});
}

