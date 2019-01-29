#include "transform.hxx"

namespace ecs
{
void Transform::update()
{
    auto view = registry.view<Transform>();

    view.each([this, &view] (auto &&transform)
    {
        ;
    });
}
}
