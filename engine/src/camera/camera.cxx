#ifdef _MSC_VER
    #include <execution>
#endif

#include <algorithm>
#include <ranges>

#include "camera/camera.hxx"


void camera_system::update()
{
#ifdef _MSC_VER
    std::for_each(std::execution::par_unseq, std::begin(cameras_), std::end(cameras_), [] (auto &&camera)
#else
    std::ranges::for_each(cameras_, [] (auto &&camera)
#endif
    {
        camera->data.projection = math::reversed_perspective(camera->vertical_fov, camera->aspect, camera->znear, camera->zfar);
        camera->data.inverted_projection = glm::inverse(camera->data.projection);

        camera->data.view = glm::inverse(camera->world);
        camera->data.inverted_view = glm::inverse(camera->data.view);

        camera->data.projection_view = camera->data.projection * camera->data.view;
    });
}
