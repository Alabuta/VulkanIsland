#pragma once

#include <span>
#include <cstdint>

#include "graphics/vertex.hxx"


namespace primitives
{
    struct plane_create_info final {
        graphics::vertex_layout vertex_layout;

        graphics::PRIMITIVE_TOPOLOGY topology;

        graphics::INDEX_TYPE index_buffer_type;

        float width, height;
        std::uint32_t hsegments, vsegments;
    };

    std::uint32_t calculate_plane_vertices_count(primitives::plane_create_info const &create_info);

    std::uint32_t calculate_plane_indices_count(primitives::plane_create_info const &create_info);

    void generate_plane(primitives::plane_create_info const &create_info, std::span<std::byte> vertex_buffer,
                        glm::vec4 const &color = glm::vec4{1});

    void generate_plane_indexed(primitives::plane_create_info const &create_info, std::span<std::byte> vertex_buffer,
                                std::span<std::byte> index_buffer, glm::vec4 const &color = glm::vec4{1});

    struct box_create_info final {
        graphics::vertex_layout vertex_layout;

        graphics::PRIMITIVE_TOPOLOGY topology;

        graphics::INDEX_TYPE index_buffer_type;

        float width, height, depth;
        std::uint32_t hsegments, vsegments, dsegments;

        std::array<glm::vec4, 6> colors;
    };

    std::uint32_t calculate_box_vertices_count(primitives::box_create_info const &create_info);

    std::uint32_t calculate_box_indices_number(primitives::box_create_info const &create_info);

    void generate_box(primitives::box_create_info const &create_info, std::span<std::byte> vertex_buffer);

    void generate_box_indexed(primitives::box_create_info const &create_info, std::span<std::byte> vertex_buffer,
                              std::span<std::byte> index_buffer);

    struct teapot_create_info final {
        graphics::vertex_layout vertex_layout;

        graphics::PRIMITIVE_TOPOLOGY topology;

        graphics::INDEX_TYPE index_buffer_type;

        std::uint32_t size;
        std::uint32_t segments;

        glm::vec4 color = glm::vec4{1};

        bool bottom = true;
        bool lid = true;
        bool body = true;
    };

    std::uint32_t calculate_teapot_vertices_count(primitives::teapot_create_info const &create_info);

    std::uint32_t calculate_teapot_indices_count(primitives::teapot_create_info const &create_info);

    void generate_teapot(primitives::teapot_create_info const &create_info, std::span<std::byte> vertex_buffer);

    void generate_teapot_indexed(primitives::teapot_create_info const &create_info, std::span<std::byte> vertex_buffer,
                                 std::span<std::byte> index_buffer);

    struct icosahedron_create_info final {
        graphics::vertex_layout vertex_layout;
        graphics::PRIMITIVE_TOPOLOGY topology;

        glm::vec4 color = glm::vec4{1};

        float radius = 1.f;
        std::uint32_t detail = 4;
    };

    std::uint32_t calculate_icosahedron_vertices_count(primitives::icosahedron_create_info const &create_info);

    void generate_icosahedron(primitives::icosahedron_create_info const &create_info, std::span<std::byte> vertex_buffer);
}

