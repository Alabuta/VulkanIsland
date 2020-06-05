#pragma once

#include <cstddef>
#include <variant>
#include <vector>
#include <unordered_map>

#include "math/math.hxx"
#include "graphics/graphics.hxx"
#include "graphics/vertex.hxx"


struct xformat final {
#if NOT_YET_IMPLEMENTED
    enum class ATTRIBUTE_TYPE : std::uint8_t {
        SCALAR = 0,
        VEC2, VEC3, VEC4
    };

    enum class COMPONENT_TYPE : std::uint8_t {
        I8 = 0, U8,
        I16, U16,
        I32, U32,
        F32,
        F64
    };

    enum class ATTRIBUTE_SEMANTIC : std::uint8_t {
        POSITION = 0,
        NORMAL,
        TEXCOORD_0, TEXCOORD_1,
        TANGENT,
        COLOR_0,
        JOINTS_0,
        WEIGHTS_0
    };


    struct vertex_attribute final {
        ATTRIBUTE_SEMANTIC semantic;
        ATTRIBUTE_TYPE attributeType;
        COMPONENT_TYPE componentType;
        bool normalized;
    };
#endif

    std::vector<graphics::vertex_layout> vertex_layouts;

    struct vertex_buffer final {
        std::size_t vertex_layout_index;

        std::size_t count{0};

        std::vector<std::byte> buffer;
    };

    std::unordered_map<std::size_t, vertex_buffer> vertex_buffers;

#if NOT_YET_IMPLEMETED
    struct index_buffer final {
        std::variant<std::uint16_t, std::uint32_t> type;

        std::size_t count{0};

        std::vector<std::byte> buffer;
    };

    std::vector<index_buffer> index_buffers;
#endif

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

#if NOT_YET_IMPLEMETED
    struct indexed_meshlet final {
        graphics::PRIMITIVE_TOPOLOGY topology;

        std::size_t vertex_buffer_index;
        std::size_t index_buffer_index;

        std::size_t material_index;

        std::uint32_t index_count{0};
        std::uint32_t instance_count{0};
        std::uint32_t first_index{0};
        std::uint32_t vertex_offset{0};
        std::uint32_t first_instance{0};
    };

    std::vector<indexed_meshlet> indexed_meshlets;
#endif

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
