#pragma once

#include <memory>


auto constexpr kPI = 3.14159265358979323846f;

#define GLM_FORCE_SWIZZLE
#define GLM_GTX_polar_coordinates
#define GLM_GTX_quaternion
#define GLM_GTX_transform

#include "math.hxx"

#include "camera/camera.hxx"
#include "input/inputManager.hxx"

class MouseHandler;

class OrbitController final {
public:

    OrbitController(std::shared_ptr<Camera> camera, InputManager &inputManager);

    void lookAt(glm::vec3 &&eye, glm::vec3 &&target);

    void rotate(float x, float y);
    void pan(float x, float y);
    void dolly(float delta);

    void update();

private:
    std::shared_ptr<Camera> camera_;

    std::shared_ptr<MouseHandler> mouseHandler_;

    glm::vec3 offset_{4};
    glm::vec2 polar_{0, 0}, polarDelta_{0, 0};
    glm::vec3 panOffset_{0}, panDelta_{0};
    glm::vec3 direction_{0}, directionLerped_{0};

    glm::vec3 target_{0};

    float damping_{.5f};

    // glm::quat orientation_;

    float scale_{1.f};

    // latitude ands longitude
    glm::vec2 minPolar{-kPI * .49f, -kPI};
    glm::vec2 maxPolar{+kPI * .49f, +kPI};

    float minZ{.01f}, maxZ{1000.f};

    glm::vec3 const up_{0, 1, 0};

    void applyDamping();
};
