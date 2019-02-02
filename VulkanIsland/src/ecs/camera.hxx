#pragma once

#include <memory>


auto constexpr kPI = 3.14159265358979323846f;

#define GLM_FORCE_SWIZZLE
#define GLM_GTX_polar_coordinates
#define GLM_GTX_quaternion
#define GLM_GTX_transform

#include "math.hxx"

#include "ecs.hxx"


namespace ecs
{
struct Camera final {
    float yFOV{glm::radians(75.f)};
    float znear{.01f}, zfar{1000.f};
    float aspect{1.f};

    glm::vec3 up{0, 1, 0};

    struct data_t {
        glm::mat4 view{1.f};
        glm::mat4 projection{1.f};

        glm::mat4 projectionView{1.f};

        glm::mat4 invertedView{1.f};
        glm::mat4 invertedProjection{1.f};
    } data;

    glm::mat4 world{1.f};
};

class CameraSystem final : public System {
public:

    CameraSystem(entity_registry &registry) noexcept : registry_{registry} { }

    ~CameraSystem() = default;

    void update() override;

private:
    entity_registry &registry_;
};
}
