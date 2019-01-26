#pragma once

#include <iomanip>

#include "entityx/entityx.hh"
namespace ex = entityx;

#include <entt/entity/registry.hpp>

#include "main.hxx"
#include "helpers.hxx"
#include "math.hxx"

struct Transform final {
    glm::mat4 localMatrix;
    glm::mat4 worldMatrix;

    Transform() noexcept = default;

    template<class T1, class T2, std::enable_if_t<are_same_v<glm::mat4, T1, T2>>...>
    Transform(T1 &&localMatrix, T2 &&worldMatrix) noexcept : localMatrix{std::forward<T1>(localMatrix)}, worldMatrix{std::forward<T2>(worldMatrix)} { }
};

template<class... Ts>
class System {
public:

    virtual void update(entt::registry<> &registry) = 0;

    virtual ~System() = default;
};

class TransformSystem final : public System<Transform> {
public:

    ~TransformSystem() = default;

    void update(entt::registry<> &registry) override
    {
        registry.view<Transform>().each([] (auto &&transfom)
        {
            ;
        });
    }

private:

    std::vector<Transform> transforms;
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
