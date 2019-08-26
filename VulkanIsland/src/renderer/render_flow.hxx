#pragma once

#include <vector>
#include <map>
#include <set>

#include "device/device.hxx"
#include "graphics.hxx"
#include "attachments.hxx"


namespace graphics
{
    struct render_flow_node final {
        std::vector<graphics::attachment> output_attachments;
    };

    class render_flow final {
    public:

        ;

    private:

        std::vector<render_flow_node> nodes_;
    };
}
