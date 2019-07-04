#pragma once

#include "math.hxx"


struct Camera {
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
