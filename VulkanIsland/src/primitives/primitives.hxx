#pragma once

#include <vector>
#include <cstdint>

#include "graphics/vertex.hxx"


namespace primitives
{
    std::vector<std::byte>
    generate_plane(float width, float height, std::uint32_t hsegments, std::uint32_t vsegments,
                   graphics::vertex_layout const &vertex_layout, glm::vec4 const &color = glm::vec4{1});

    /*std::vector<std::byte>
    generate_plane_indexed(float width, float height, std::uint32_t hsegments, std::uint32_t vsegments,
                           graphics::vertex_layout const &vertex_layout, glm::vec4 const &color = glm::vec4{1});*/

    std::vector<std::byte>
    generate_box_indexed(float width, float height, float depth, std::uint32_t hsegments, std::uint32_t vsegments, std::uint32_t dsegments,
                         graphics::vertex_layout const &vertex_layout, glm::vec4 const &color = glm::vec4{1});

    std::uint32_t calculate_box_vertices_number(std::uint32_t hsegments, std::uint32_t vsegments, std::uint32_t dsegments);

    std::uint32_t calculate_box_indices_number(graphics::PRIMITIVE_TOPOLOGY topology, std::uint32_t hsegments, std::uint32_t vsegments, std::uint32_t dsegments);
}

