#pragma once

#include <vector>
#include <map>
#include <set>

#include "vulkan/device.hxx"
#include "graphics.hxx"
#include "attachments.hxx"
#include "material.hxx"


namespace graphics
{
    struct render_pipeline_output final {
        graphics::attachment_reference attachment_reference;
    };

    struct render_pipeline_node final {
        std::uint32_t width, height;

        /*std::vector<graphics::attachment> input_attachments;
        std::vector<graphics::attachment> color_attachments;
        std::vector<graphics::attachment> depth_stencil_attachments;*/

        std::shared_ptr<graphics::material> material;

        graphics::pipeline_states pipeline_states;
    };

    class render_pipeline final {
    public:

    private:

        std::vector<graphics::render_pipeline_node> nodes_;
    };

    class render_pipeline_manager final {
    public:

        [[nodiscard]] graphics::render_pipeline
        create_render_flow(std::vector<graphics::render_pipeline_node> const &nodes, std::vector<graphics::render_pipeline_output> const &output);

    private:

        ;
    };
}
