#pragma once

#include <vector>
#include <iomanip>
#include <numeric>
#include <unordered_map>

#include <boost/functional/hash.hpp>

#include <entityx/entityx.h>
namespace ex = entityx;

#include "main.hxx"
#include "utility/helpers.hxx"
#include "utility/mpl.hxx"
#include "math.hxx"

#include "vertexFormat.hxx"
#include "renderer/graphics.hxx"
#include "renderer/vertex.hxx"



namespace staging
{
    struct vertex_buffer_t final {
        vertex_layout_t layout;

        std::size_t offset{0};
        std::vector<std::byte> buffer;

        template<class T> requires std::same_as<std::remove_cvref_t<T>, vertex_layout_t>
        vertex_buffer_t(T &&layout) noexcept : layout{std::forward<T>(layout)} { }

        struct hash_value final {
            template<class T> requires std::same_as<std::remove_cvref_t<T>, vertex_buffer_t>
            constexpr std::size_t operator() (T &&vertexBuffer) const noexcept
            {
                std::size_t seed = 0;

                for (auto &&description : vertexBuffer.layout) {
                    boost::hash_combine(seed, description.semantic.index());
                    boost::hash_combine(seed, description.attribute.index());
                }

                return seed;
            }
        };

        template<class T> requires std::same_as<std::remove_cvref_t<T>, vertex_buffer_t>
        constexpr bool operator== (T &&rhs) const noexcept
        {
            if (std::size(buffer) != std::size(rhs.buffer))
                return false;

            return std::equal(std::cbegin(buffer), std::cend(buffer), std::cbegin(rhs.buffer), [] (auto &&lhs, auto &&rhs) {
                return lhs.semantic.index() == rhs.semantic.index() && lhs.attribute.index() == rhs.attribute.index();
            });
        }

#if NOT_YET_IMPLEMENTED
        struct comparator final {
            using is_transparent = void;

            template<class L, class R>
            std::enable_if_t<are_same_v<vertex_buffer_t, L, R>, bool>
                constexpr operator() (L &&lhs, R &&rhs) const noexcept
            {
                if (std::size(buffer) != std::size(rhs.buffer))
                    return false;

                return std::equal(std::cbegin(buffer), std::cend(buffer), std::cbegin(rhs.buffer), [] (auto &&lhs, auto &&rhs)
                {
                    return lhs.semantic.index() == rhs.semantic.index() && lhs.attribute.index() == rhs.attribute.index();
                });
            }

            /*template<class T, class S, typename std::enable_if_t<std::is_same_v<vertex_buffer_t, std::remove_cvref_t<T>> && std::is_integral_v<S>>* = nullptr>
            auto operator() (T &&chunk, S size) const noexcept
            {
                return chunk.size < size;
            }

            template<class S, class T, typename std::enable_if_t<std::is_same_v<vertex_buffer_t, std::remove_cvref_t<T>> && std::is_integral_v<S>>* = nullptr>
            auto operator() (S size, T &&chunk) const noexcept
            {
                return chunk.size < size;
            }*/
        };
#endif
    };

    struct submesh_t final {
        graphics::PRIMITIVE_TOPOLOGY topology;

        vertices_t  vertices;
        indices_t indices;
    };

    struct mesh_t final {
        std::vector<submesh_t> submeshes;
        //std::vector<std::size_t> submeshes;
    };

    struct scene_t final {
        //std::vector<submesh_t> submeshes;
        std::vector<mesh_t> meshes;

        std::vector<std::byte> vertexBuffer;
        std::vector<std::byte> indexBuffer;

        std::vector<vertex_buffer_t> vertex_buffers;
        //std::unordered_map<vertex_layout_t, std::vector<std::byte>, vertex_buffer_t::hash_value> vertexBuffers;
    };
}


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

    struct index_buffer final {
        std::variant<std::uint16_t, std::uint32_t> type;

        std::size_t count{0};

        std::vector<std::byte> buffer;
    };

    std::vector<index_buffer> index_buffers;

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
};

template<class S, class T, class N>
void add_vertex_attributes(std::vector<graphics::vertex_attribute> &attributes, std::size_t offset_in_bytes, S semantic, T type, N normalized)
{
    attributes.push_back(graphics::vertex_attribute{ offset_in_bytes, semantic, type, normalized });
}

template<class S, class T, class N, class... Ts>
void add_vertex_attributes(std::vector<graphics::vertex_attribute> &attributes, std::size_t offset_in_bytes, S semantic, T type, N normalized, Ts... args)
{
    attributes.push_back(graphics::vertex_attribute{ offset_in_bytes, semantic, type, normalized });

    add_vertex_attributes(attributes, offset_in_bytes + sizeof(type), args...);
}

template<class... Ts>
graphics::vertex_layout create_vertex_layout(Ts... args)
{
    graphics::vertex_layout vertex_layout;

    auto &&vertex_attributes = vertex_layout.attributes;

    add_vertex_attributes(vertex_attributes, 0, args...);

    vertex_layout.size_in_bytes = 0;

    for (auto &&vertex_attribute : vertex_attributes) {
        auto size_in_bytes = std::visit([] (auto &&type)
        {
            return sizeof(std::remove_cvref_t<decltype(type)>);

        }, vertex_attribute.type);

        vertex_layout.size_in_bytes += size_in_bytes;
    }

    return vertex_layout;
}


/*struct Mesh final {
    glm::mat4 localMatrix;
    glm::mat4 worldMatrix;

    template<class T1, class T2, std::enable_if_t<are_same_v<glm::mat4, T1, T2>>* = nullptr>
    Mesh(T1 &&localMatrix, T2 &&worldMatrix) : localMatrix{std::forward<T1>(localMatrix)}, worldMatrix{std::forward<T2>(worldMatrix)} {}
};

struct MeshSytem final : public ex::System<Mesh> {
    void update(ex::EntityManager &es, ex::EventManager &events, ex::TimeDelta dt) final
    {
        es.each<Mesh>([] (auto &&entity, auto &&mesh)
        {
            ;
        });
    }
};*/
