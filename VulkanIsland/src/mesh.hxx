#pragma once

#include <vector>
#include <iomanip>

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
    struct submesh_t {
        PRIMITIVE_TOPOLOGY topology;

        vertices_t  vertices;
        indices_t_ indices;
    };

    struct mesh_t {
        std::vector<submesh_t> submeshes;
    };

    struct scene_t {
        std::vector<mesh_t> meshes;

        std::vector<std::byte> buffer;
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
