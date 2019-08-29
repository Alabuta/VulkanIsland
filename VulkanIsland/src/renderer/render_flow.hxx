#pragma once

#include <vector>
#include <map>
#include <set>

#include "device/device.hxx"
#include "graphics.hxx"
#include "attachments.hxx"
#include "material.hxx"


namespace graphics
{
    struct render_flow_node final {
        std::vector<graphics::attachment> input_attachments;
        std::vector<graphics::attachment> color_attachments;
        std::vector<graphics::attachment> depth_stencil_attachments;

        std::shared_ptr<graphics::material> material;
    };

    class render_flow final {
    public:

        void add_nodes(std::vector<graphics::render_flow_node> const &nodes);

    private:

        std::vector<graphics::render_flow_node> nodes_;
    };
}
