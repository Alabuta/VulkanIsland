
#pragma once

#include <memory>


auto constexpr kPI = 3.14159265358979323846f;

#define GLM_FORCE_CXX17
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_SWIZZLE
#define GLM_GTX_polar_coordinates
#define GLM_GTX_quaternion
#define GLM_GTX_transform

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/polar_coordinates.hpp> 
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>

#include "../input/input_manager.hxx"


struct Camera {
    float yFOV{glm::radians(75.f)};
    float znear{.01f}, zfar{100.f};
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


class CameraSystem {
public:

    std::shared_ptr<Camera> createCamera()
    {
        auto camera = std::make_shared<Camera>();

        cameras_.push_back(std::move(camera));

        return cameras_.back();
    }

    void update();

private:

    std::vector<std::shared_ptr<Camera>> cameras_;
};