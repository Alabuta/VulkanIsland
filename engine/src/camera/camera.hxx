#pragma once

#include <memory>

#include "math/math.hxx"


struct camera {
    float vertical_fov{glm::radians(75.f)};
    float znear{.01f}, zfar{1000.f};
    float aspect{1.f};

    glm::vec3 up{0, 1, 0};

    struct data_t {
        glm::mat4 view{1.f};
        glm::mat4 projection{1.f};

        glm::mat4 projection_view{1.f};

        glm::mat4 inverted_view{1.f};
        glm::mat4 inverted_projection{1.f};
    } data;

    glm::mat4 world{1.f};
};

class camera_system {
public:

    std::shared_ptr<camera> create_camera()
    {
        cameras_.push_back(std::make_shared<camera>());

        return cameras_.back();
    }

    void update();

private:

    std::vector<std::shared_ptr<camera>> cameras_;
};
