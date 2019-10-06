#pragma once

#include <iomanip>

#include "main.hxx"
#include "utility/helpers.hxx"
#include "math/math.hxx"
#include "utility/mpl.hxx"

#include "ecs.hxx"

struct Transform final {
    glm::mat4 localMatrix;
    glm::mat4 worldMatrix;

    Transform() noexcept = default;

    template<class T1, class T2> requires mpl::all_same<glm::mat4, T1, T2>
    Transform(T1 &&localMatrix, T2 &&worldMatrix) noexcept : localMatrix{std::forward<T1>(localMatrix)}, worldMatrix{std::forward<T2>(worldMatrix)} { }
};


namespace ecs
{
class TransformSystem final : public System {
public:

    TransformSystem(entity_registry &registry) noexcept : registry{registry} { }
    ~TransformSystem() = default;

    void update() override;

private:
    entity_registry &registry;
};
}

/*struct TransformSytem final : public ex::System<Transform> {
    void update(ex::EntityManager &es, ex::EventManager &events, ex::TimeDelta dt) final
    {
        es.each<Transform>([] (auto &&entity, auto &&transform)
        {
            std::cout << entity << "\n" << transform.localMatrix << "\n\n" << transform.worldMatrix << "\n\n";
        });
    }
};*/
