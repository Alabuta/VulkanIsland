#include "transform.hxx"

namespace ecs
{
void TransformSystem::update()
{
    auto view = registry.view<Transform>();

    view.each([/* this, &view */] (auto &&)
    {
        ;
    });
}
}
