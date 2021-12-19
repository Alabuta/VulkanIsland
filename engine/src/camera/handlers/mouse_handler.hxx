#pragma once

#define GLM_FORCE_CXX17
#include <glm/glm.hpp>


#include "platform/input/input_manager.hxx"
#include "platform/input/mouse.hxx"

#include "camera/camera_controller.hxx"


class mouse_handler final : public platform::mouse::handler_interface {
public:

    mouse_handler(orbit_controller &controller) : controller_{controller} { }

private:

    orbit_controller &controller_;

    std::function<void(mouse_handler &)> update_handler_{[] (auto &&) { }};

    glm::vec2 delta{0, 0};
    glm::vec2 last{0, 0};

    glm::vec2 dolly_direction{0, -1};

    void on_move(float x, float y) override;
    void on_wheel(float xoffset, float yoffset) override;
    void on_down(handler_interface::buttons_t buttons) override;
    void on_up(handler_interface::buttons_t buttons) override;
};
