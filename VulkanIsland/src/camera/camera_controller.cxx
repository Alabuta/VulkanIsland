#include "handlers/mouse_handler.hxx"
#include "camera_controller.hxx"



orbit_controller::orbit_controller(std::shared_ptr<camera> camera, platform::input_manager &input_manager) : camera_{camera}
{
    mouse_handler_ = std::make_shared<mouse_handler>(*this);
    input_manager.mouse().connect_handler(mouse_handler_);
}

void orbit_controller::look_at(glm::vec3 &&eye, glm::vec3 &&target)
{
    target_ = target;
    offset_ = eye;

    auto &&world = camera_->world;

    world = glm::inverse(glm::lookAt(offset_, target_, {0, 1, 0}));
}

void orbit_controller::rotate(float longitude, float latitude)
{
    auto speed = (1.f - damping_) * .0064f;

    polar_delta_.x -= latitude * speed;
    polar_delta_.y += longitude * speed;
}

void orbit_controller::pan(float x, float y)
{
    auto speed = (1.f - damping_) * .0024f;

    pan_delta_.x += x * speed;
    pan_delta_.y -= y * speed;
}

void orbit_controller::dolly(float delta)
{
    auto speed = (1.f - damping_) * 2.f;

    auto dollying = std::pow(.95f, std::abs(delta) * speed);

    scale_ = std::signbit(delta) ? (std::signbit(delta) ? 1.f / dollying : 1.f) : dollying;
}

void orbit_controller::update()
{
    auto &&world = camera_->world;

    auto x_axis = glm::vec3{world[0]};
    auto y_axis = glm::vec3{world[1]};
    // auto zAxis = glm::vec3{world[2]};

    auto position = glm::vec3{world[3]};

    offset_ = position - target_;

    auto radius = glm::length(offset_);
    auto distance = radius * 2.f * std::tan(camera_->vertical_fov * .5f);

    radius = std::clamp(radius * scale_, znear, zfar);

    x_axis *= pan_delta_.x * distance;
    y_axis *= pan_delta_.y * distance;

    pan_offset_ = x_axis + y_axis;

    polar_ = glm::polar(offset_);
    polar_ = glm::clamp(polar_ + polar_delta_, min_polar, max_polar);

    offset_ = glm::euclidean(polar_) * radius;

    /*auto yaw = std::atan2(zAxis.x, zAxis.z);
    orientation_ = glm::angleAxis(yaw, up_);
    zAxis = orientation_ * (directionLerped_ * glm::vec3{1, 0, -1});
    panOffset_ += zAxis * std::max(.1f, std::log(std::abs(distance) + 1.f));*/

    target_ += pan_offset_;

    position = target_ + offset_;

    world = glm::inverse(glm::lookAt(position, target_, up_));


#if 0
    auto orientation = from_two_vec3(camera_->up, glm::vec3{0, 1, 0});
#endif

    apply_damping();
}

void orbit_controller::apply_damping()
{
    polar_delta_ *= damping_;

    pan_delta_ *= damping_;

    scale_ += (1 - scale_) * (1 - damping_);

    direction_lerped_ = glm::mix(direction_lerped_, direction_, 1.f - damping_);
}
