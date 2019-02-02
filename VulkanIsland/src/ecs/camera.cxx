#include "camera.hxx"
#include "transform.hxx"


namespace ecs
{
void CameraSystem::update()
{
    auto view = registry_.view<Camera, Transform>();

    view.each([] (auto &&, auto &&)
    {
    });
}
}
