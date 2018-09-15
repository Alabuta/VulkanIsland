#pragma once

#include <iomanip>

#include "entityx/entityx.hh"
namespace ex = entityx;

#include "main.hxx"
#include "helpers.hxx"
#include "math.hxx"

struct Transform final {
    glm::mat4 localMatrix;
    glm::mat4 worldMatrix;

    Transform() = default;

    Transform(glm::mat4 localMatrix, glm::mat4 worldMatrix) : localMatrix{localMatrix}, worldMatrix{worldMatrix} { }

    template<class T1, class T2, std::enable_if_t<are_same_v<glm::mat4, T1, T2>>...>
    Transform(T1 &&localMatrix, T2 &&worldMatrix) : localMatrix{std::forward<T1>(localMatrix)}, worldMatrix{std::forward<T2>(worldMatrix)} { }
};

/*struct TransformSytem final : public ex::System<Transform> {
    void update(ex::EntityManager &es, ex::EventManager &events, ex::TimeDelta dt) final
    {
        es.each<Transform>([] (auto &&entity, auto &&transform)
        {
            std::cout << entity << "\n" << transform.localMatrix << "\n\n" << transform.worldMatrix << "\n\n";
        });
    }
};*/
