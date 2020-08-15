#pragma once

#include <cstddef>
#include <variant>
#include <vector>
#include <unordered_map>

#include "math/math.hxx"
#include "graphics/graphics.hxx"
#include "graphics/vertex.hxx"


namespace loader
{
    struct vertex_buffer_description final {
        std::size_t offset;
        std::size_t size_in_bytes;

        std::uint32_t vertex_layout_index;
    };

    struct index_buffer_description final {
        std::size_t offset;
        std::size_t size_in_bytes;

        graphics::FORMAT format;
    };

    struct transforms_buffer final {
        std::size_t offset;
        std::size_t size_in_bytes;
    };

    struct scene_description final {
        std::vector<loader::vertex_buffer_description> vertex_buffer_descriptions;
        //std::vector<loader::index_buffer_description> index_buffer_descriptions;

        loader::transforms_buffer transforms_buffer;

        std::vector<graphics::vertex_layout> vertex_layouts;

        struct material final {
            std::uint32_t technique;
            std::string name;
        };

        std::vector<material> materials;

        struct non_indexed_meshlet final {
            graphics::PRIMITIVE_TOPOLOGY topology;

            std::size_t vertex_buffer_index;

            std::size_t material_index;

            std::uint32_t vertex_count{0};
            std::uint32_t instance_count{0};
            std::uint32_t first_vertex{0};
            std::uint32_t first_instance{0};
        };

        std::vector<non_indexed_meshlet> non_indexed_meshlets;

        struct mesh final {
            std::vector<std::size_t> meshlets;
        };

        std::vector<mesh> meshes;

        struct scene_node final {
            std::size_t transform_index;
            std::size_t mesh_index;
        };

        std::vector<scene_node> scene_nodes;
    };
}


struct xformat final {
    std::vector<graphics::vertex_layout> vertex_layouts;

    struct vertex_buffer final {
        std::size_t vertex_layout_index;

        std::size_t count{0};
        std::vector<std::byte> buffer;
    };

    std::unordered_map<std::int64_t, vertex_buffer> vertex_buffers;

    struct index_buffer final {
        graphics::FORMAT format;

        std::size_t count{0};
        std::vector<std::byte> buffer;
    };

    std::unordered_map<std::int64_t, index_buffer> index_buffers;

    struct material final {
        std::uint32_t technique;
        std::string name;
    };

    std::vector<material> materials;

    /*struct non_indexed_meshlet final {
        graphics::PRIMITIVE_TOPOLOGY topology;

        std::size_t vertex_buffer_index;

        std::size_t material_index;

        std::uint32_t vertex_count{0};
        std::uint32_t instance_count{0};
        std::uint32_t first_vertex{0};
        std::uint32_t first_instance{0};
    };

    std::vector<non_indexed_meshlet> non_indexed_meshlets;*/

    struct meshlet final {
        graphics::PRIMITIVE_TOPOLOGY topology;

        std::int64_t vertex_buffer_index{-1};
        std::int64_t index_buffer_index{-1};

        std::size_t material_index;

        std::uint32_t vertex_count{0};
        std::uint32_t index_count{0};
        std::uint32_t instance_count{0};
        std::uint32_t first_vertex{0};
        std::uint32_t first_index{0};
        std::uint32_t vertex_offset{0};
        std::uint32_t first_instance{0};
    };

    std::vector<meshlet> meshlets;

    std::vector<glm::mat4> transforms;

    struct mesh final {
        std::vector<std::size_t> meshlets;
    };

    std::vector<mesh> meshes;

    struct scene_node final {
        std::size_t transform_index;
        std::size_t mesh_index;
    };

    std::vector<scene_node> scene_nodes;
};
