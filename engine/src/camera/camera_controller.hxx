#pragma once

#include <memory>

#define GLM_FORCE_SWIZZLE
#define GLM_GTX_polar_coordinates
#define GLM_GTX_quaternion
#define GLM_GTX_transform

#include "math/math.hxx"

#include "camera/camera.hxx"
#include "platform/input/input_manager.hxx"


class mouse_handler;

class orbit_controller final {
public:

    orbit_controller(std::shared_ptr<camera> camera, platform::input_manager &input_manager);

    void look_at(glm::vec3 &&eye, glm::vec3 &&target);

    void rotate(float x, float y);
    void pan(float x, float y);
    void dolly(float delta);

    void update();

private:
    std::shared_ptr<camera> camera_;

    std::shared_ptr<mouse_handler> mouse_handler_;

    glm::vec3 offset_{4};
    glm::vec2 polar_{0, 0}, polar_delta_{0, 0};
    glm::vec3 pan_offset_{0}, pan_delta_{0};
    glm::vec3 direction_{0}, direction_lerped_{0};

    glm::vec3 target_{0};

    float damping_{.5f};

    // glm::quat orientation_;

    float scale_{1.f};

    // latitude ands longitude
    glm::vec2 min_polar{-std::numbers::pi_v<float> * .49f, -std::numbers::pi_v<float>};
    glm::vec2 max_polar{+std::numbers::pi_v<float> * .49f, +std::numbers::pi_v<float>};

    float znear{.01f}, zfar{1000.f};

    glm::vec3 const up_{0, 1, 0};

    void apply_damping();
};
