#ifdef _MSC_VER
    #define USE_EXECUTION_POLICIES
    #include <execution>
#endif

#include <iomanip>

#include "CameraController.hxx"


void CameraSystem::update()
{
#ifdef _MSC_VER
    std::for_each(std::execution::par_unseq, std::begin(cameras_), std::end(cameras_), [] (auto &&camera)
#else
    std::for_each(std::begin(cameras_), std::end(cameras_), [] (auto &&camera)
#endif
    {
        camera->data.projection = glm::infinitePerspective(camera->yFOV, camera->aspect, camera->znear);
        camera->data.invertedProjection = glm::inverse(camera->data.projection);

        camera->data.view = glm::inverse(camera->world);
        camera->data.invertedView = glm::inverse(camera->data.view);

        camera->data.projectionView = camera->data.projection * camera->data.view;
    });
}