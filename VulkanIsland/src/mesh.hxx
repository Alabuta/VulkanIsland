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
    };

    struct submesh_t {
        PRIMITIVE_TOPOLOGY topology;

        vertices_t  vertices;
        indices_t indices;
    };

    struct mesh_t {
        std::vector<submesh_t> submeshes;
    };

    struct scene_t {
        std::vector<mesh_t> meshes;

        std::vector<std::byte> vertexBuffer;
        std::vector<std::byte> indexBuffer;

        std::vector<vertex_buffer_t> vertexBuffers;
        //std::unordered_map<vertex_layout_t, std::vector<std::byte>, vertex_buffer_t::hash_value> vertexBuffers;
    };
}



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
auto constexpr getFormat(bool normalized = false)
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
