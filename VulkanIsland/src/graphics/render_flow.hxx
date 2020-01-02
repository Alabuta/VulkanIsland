#pragma once

#include <vector>
#include <map>
#include <set>

#include "vulkan/device.hxx"
#include "graphics.hxx"
#include "attachments.hxx"
#include "material.hxx"

#include "renderer/config.hxx"
#include "renderer/swapchain.hxx"


namespace graphics
{
    struct render_pipeline_node final {
        renderer::extent extent;

        /*std::vector<graphics::attachment> input_attachments;
        std::vector<graphics::attachment> color_attachments;
        std::vector<graphics::attachment> depth_stencil_attachments;*/

        std::shared_ptr<graphics::material> material;

        graphics::pipeline_states pipeline_states;
    };

    class render_pipeline final {
    public:

    private:

        std::unique_ptr<renderer::swapchain> swapchain_;

        std::vector<std::shared_ptr<graphics::render_pass>> render_passes;

        std::vector<graphics::attachment> attachments_;
        std::vector<std::shared_ptr<resource::framebuffer>> framebuffers_;

        // std::vector<graphics::render_pipeline_node> nodes_;
    };

    class render_pipeline_manager final {
    public:
    
        render_pipeline_manager(vulkan::device const &device, std::shared_ptr<resource::resource_manager> resource_manager);

        [[nodiscard]] graphics::render_pipeline
        create_render_flow(renderer::swapchain const &swapchain, std::vector<graphics::render_pipeline_node> const &nodes);

    private:

        vulkan::device const &device_;

        std::shared_ptr<resource::resource_manager> resource_manager_;
        std::unique_ptr<graphics::render_pass_manager> render_pass_manager_;
    };
}


std::vector<graphics::attachment>
create_attachments(vulkan::device const &device, renderer::config const &renderer_config,
                   resource::resource_manager &resource_manager,renderer::swapchain const &swapchain);

std::vector<graphics::attachment_description>
create_attachment_descriptions(std::vector<graphics::attachment> const &attachments);

std::shared_ptr<graphics::render_pass>
create_render_pass(graphics::render_pass_manager &render_pass_manager,
                   renderer::surface_format surface_format, std::vector<graphics::attachment_description> attachment_descriptions);

std::vector<std::shared_ptr<resource::framebuffer>>
create_framebuffers(resource::resource_manager &resource_manager, renderer::swapchain const &swapchain,
                    std::shared_ptr<graphics::render_pass> render_pass, std::vector<graphics::attachment> const &attachments);
