#pragma once

#include <vector>
#include <iomanip>
#include <unordered_map>

#include <boost/functional/hash.hpp>

#include "entityx/entityx.hh"
namespace ex = entityx;

#include "main.hxx"
#include "helpers.hxx"
#include "math.hxx"

#include "vertexFormat.hxx"


enum class PRIMITIVE_TOPOLOGY {
    POINTS = 0,
    LINES, LINE_LOOP, LINE_STRIP,
    TRIANGLES, TRIANGLE_STRIP, TRIANGLE_FAN
};

namespace staging
{
    struct vertex_buffer_t final {
        vertex_layout_t layout;

        std::size_t offset{0};
        std::vector<std::byte> buffer;

        template<class T, typename std::enable_if_t<std::is_same_v<vertex_layout_t, std::decay_t<T>>>...>
        vertex_buffer_t(T &&layout) noexcept : layout{std::forward<T>(layout)} { }

        struct hash_value final {
            template<class T, typename std::enable_if_t<std::is_same_v<vertex_buffer_t, std::decay_t<T>>>...>
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

        template<class T, typename std::enable_if_t<std::is_same_v<vertex_buffer_t, std::decay_t<T>>>...>
        constexpr bool operator== (T &&rhs) const noexcept
        {
            if (std::size(buffer) != std::size(rhs.buffer))
                return false;

            return std::equal(std::cbegin(buffer), std::cend(buffer), std::cbegin(rhs.buffer), [] (auto &&lhs, auto &&rhs) {
                return lhs.semantic.index() == rhs.semantic.index() && lhs.attribute.index() == rhs.attribute.index();
            });
        }

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

            /*template<class T, class S, typename std::enable_if_t<std::is_same_v<vertex_buffer_t, std::decay_t<T>> && std::is_integral_v<S>>...>
            auto operator() (T &&chunk, S size) const noexcept
            {
                return chunk.size < size;
            }

            template<class S, class T, typename std::enable_if_t<std::is_same_v<vertex_buffer_t, std::decay_t<T>> && std::is_integral_v<S>>...>
            auto operator() (S size, T &&chunk) const noexcept
            {
                return chunk.size < size;
            }*/
        };
    };

    struct submesh_t final {
        PRIMITIVE_TOPOLOGY topology;

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

        std::vector<vertex_buffer_t> vertexBuffers;
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

    struct vertex_attribute final {
        std::size_t offsetInBytes;

        semantics_t semantic;
        attribute_t type;

        bool normalized;
    };

    struct vertex_layout final {
        std::vector<vertex_attribute> attributes;

        std::size_t sizeInBytes;
    };


    struct hash_value final {
        template<class T, typename std::enable_if_t<std::is_same_v<vertex_attribute, std::decay_t<T>>>...>
        auto constexpr operator() (T &&attribute) const noexcept
        {
            std::size_t seed = 0;

            boost::hash_combine(seed, attribute.offsetInBytes);

            boost::hash_combine(seed, attribute.semantic.index());
            boost::hash_combine(seed, attribute.type.index());

            boost::hash_combine(seed, attribute.normalized);

            return seed;
        }

        template<class T, typename std::enable_if_t<std::is_same_v<vertex_layout, std::decay_t<T>>>...>
        auto constexpr operator() (T &&layout) const noexcept
        {
            std::size_t seed = 0;

            //boost::hash_combine(seed, boost::hash<vertex_attribute>{layout.attributes});

            boost::hash_combine(seed, layout.sizeInBytes);

            return seed;
        }
    };

    struct equal_comparator final {
        template<class T1, class T2, typename std::enable_if_t<are_same_v<struct vertex_attribute, T1, T2>>...>
        auto constexpr operator() (T1 &&lhs, T2 &&rhs) const noexcept
        {
            return lhs.offsetInBytes == rhs.offsetInBytes && lhs.semantic == rhs.semantic && lhs.type == rhs.type && lhs.normalized == rhs.normalized;
        }

        template<class T1, class T2, typename std::enable_if_t<are_same_v<vertex_layout, T1, T2>>...>
        auto constexpr operator() (T1 &&lhs, T2 &&rhs) const noexcept
        {
            auto sameSize = lhs.sizeInBytes == rhs.sizeInBytes;;

            return std::equal(std::begin(lhs.attributes), std::end(lhs.attributes), std::begin(rhs.attributes), [] (auto &&lhs, auto &&rhs)
            {
                return true;// lhs.attribute == rhs.attribute;
            }) && sameSize;
        }
    };

#if NOT_YET_IMPLEMENTED
    struct less_comparator final {

        template<class T1, class T2, typename std::enable_if_t<are_same_v<struct vertex_attribute, T1, T2>>...>
        auto constexpr operator() (T1 &&lhs, T2 &&rhs) const noexcept
        {
            return lhs.semantic < rhs.semantic;
        }

        template<class T1, class T2, typename std::enable_if_t<are_same_v<vertex_layout, T1, T2>>...>
        auto constexpr operator() (T1 &&lhs, T2 &&rhs) const noexcept
        {
            return lhs.attribute < rhs.attribute;
        }
    };
#endif


    std::vector<vertex_layout> vertexLayouts;

    struct vertex_buffer final {
        std::size_t vertexLayoutIndex;

        std::size_t count{0};

        std::vector<std::byte> buffer;
    };

    std::vector<vertex_buffer> vertexBuffers;

    struct index_buffer final {
        std::variant<std::uint16_t, std::uint32_t> type;

        std::size_t count{0};

        std::vector<std::byte> buffer;
    };

    std::vector<index_buffer> indexBuffers;

    struct material final {
        PRIMITIVE_TOPOLOGY topology;
    };

    std::vector<material> materials;

    struct non_indexed_meshlet final {
        std::size_t vertexBufferIndex;

        std::size_t materialIndex;

        std::size_t vertexCount{0};
        std::size_t instanceCount{0};
        std::size_t firstVertex{0};
        std::size_t firstInstance{0};
    };

    std::vector<non_indexed_meshlet> nonIndexedMeshlets;

    struct indexed_meshlet final {
        std::size_t vertexBufferIndex;
        std::size_t indexBufferIndex;

        std::size_t materialIndex;

        std::size_t indexCount{0};
        std::size_t instanceCount{0};
        std::size_t firstIndex{0};
        std::size_t vertexOffset{0};
        std::size_t firstInstance{0};
    };

    std::vector<indexed_meshlet> indexedMeshlets;
};


/*struct Mesh final {
    glm::mat4 localMatrix;
    glm::mat4 worldMatrix;

    template<class T1, class T2, std::enable_if_t<are_same_v<glm::mat4, T1, T2>>...>
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


auto constexpr get_primitive_topology(PRIMITIVE_TOPOLOGY mode)
{
    switch (mode) {
        case PRIMITIVE_TOPOLOGY::POINTS:
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

        case PRIMITIVE_TOPOLOGY::LINES:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

        case PRIMITIVE_TOPOLOGY::LINE_STRIP:
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;

        case PRIMITIVE_TOPOLOGY::TRIANGLES:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        case PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

        case PRIMITIVE_TOPOLOGY::TRIANGLE_FAN:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;

        default:
            return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
    }
}

template<std::size_t N, class T>
auto constexpr getFormat([[maybe_unused]] bool normalized = false)
{
    if constexpr (std::is_same_v<T, std::int8_t>) {
        if (normalized) {
            switch (N) {
                case 1: return VK_FORMAT_R8_SNORM;
                case 2: return VK_FORMAT_R8G8_SNORM;
                case 3: return VK_FORMAT_R8G8B8_SNORM;
                case 4: return VK_FORMAT_R8G8B8A8_SNORM;
                default: return VK_FORMAT_UNDEFINED;
            }
        }

        else {
            switch (N) {
                case 1: return VK_FORMAT_R8_SINT;
                case 2: return VK_FORMAT_R8G8_SINT;
                case 3: return VK_FORMAT_R8G8B8_SINT;
                case 4: return VK_FORMAT_R8G8B8A8_SINT;
                default: return VK_FORMAT_UNDEFINED;
            }
        }
    }

    else if constexpr (std::is_same_v<T, std::uint8_t>) {
        if (normalized) {
            switch (N) {
                case 1: return VK_FORMAT_R8_UNORM;
                case 2: return VK_FORMAT_R8G8_UNORM;
                case 3: return VK_FORMAT_R8G8B8_UNORM;
                case 4: return VK_FORMAT_R8G8B8A8_UNORM;
                default: return VK_FORMAT_UNDEFINED;
            }
        }

        else {
            switch (N) {
                case 1: return VK_FORMAT_R8_UINT;
                case 2: return VK_FORMAT_R8G8_UINT;
                case 3: return VK_FORMAT_R8G8B8_UINT;
                case 4: return VK_FORMAT_R8G8B8A8_UINT;
                default: return VK_FORMAT_UNDEFINED;
            }
        }
    }

    else if constexpr (std::is_same_v<T, std::int16_t>) {
        if (normalized) {
            switch (N) {
                case 1: return VK_FORMAT_R16_SNORM;
                case 2: return VK_FORMAT_R16G16_SNORM;
                case 3: return VK_FORMAT_R16G16B16_SNORM;
                case 4: return VK_FORMAT_R16G16B16A16_SNORM;
                default: return VK_FORMAT_UNDEFINED;
            }
        }

        else {
            switch (N) {
                case 1: return VK_FORMAT_R16_SINT;
                case 2: return VK_FORMAT_R16G16_SINT;
                case 3: return VK_FORMAT_R16G16B16_SINT;
                case 4: return VK_FORMAT_R16G16B16A16_SINT;
                default: return VK_FORMAT_UNDEFINED;
            }
        }
    }

    else if constexpr (std::is_same_v<T, std::uint16_t>) {
        if (normalized) {
            switch (N) {
                case 1: return VK_FORMAT_R16_UNORM;
                case 2: return VK_FORMAT_R16G16_UNORM;
                case 3: return VK_FORMAT_R16G16B16_UNORM;
                case 4: return VK_FORMAT_R16G16B16A16_UNORM;
                default: return VK_FORMAT_UNDEFINED;
            }
        }

        else {
            switch (N) {
                case 1: return VK_FORMAT_R16_UINT;
                case 2: return VK_FORMAT_R16G16_UINT;
                case 3: return VK_FORMAT_R16G16B16_UINT;
                case 4: return VK_FORMAT_R16G16B16A16_UINT;
                default: return VK_FORMAT_UNDEFINED;
            }
        }
    }

    else if constexpr (std::is_same_v<T, std::int32_t>) {
        switch (N) {
            case 1: return VK_FORMAT_R32_SINT;
            case 2: return VK_FORMAT_R32G32_SINT;
            case 3: return VK_FORMAT_R32G32B32_SINT;
            case 4: return VK_FORMAT_R32G32B32A32_SINT;
            default: return VK_FORMAT_UNDEFINED;
        }
    }

    else if constexpr (std::is_same_v<T, std::uint32_t>) {
        switch (N) {
            case 1: return VK_FORMAT_R32_UINT;
            case 2: return VK_FORMAT_R32G32_UINT;
            case 3: return VK_FORMAT_R32G32B32_UINT;
            case 4: return VK_FORMAT_R32G32B32A32_UINT;
            default: return VK_FORMAT_UNDEFINED;
        }
    }

    else if constexpr (std::is_same_v<T, std::float_t>) {
        switch (N) {
            case 1: return VK_FORMAT_R32_SFLOAT;
            case 2: return VK_FORMAT_R32G32_SFLOAT;
            case 3: return VK_FORMAT_R32G32B32_SFLOAT;
            case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
            default: return VK_FORMAT_UNDEFINED;
        }
    }

    else return VK_FORMAT_UNDEFINED;
}
