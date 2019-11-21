#include "render_flow.hxx"


namespace graphics
{
    render_pipeline_manager::render_pipeline_manager(std::shared_ptr<resource::resource_manager> resource_manager, renderer::config const &renderer_config)
        : resource_manager_{resource_manager}, renderer_config_{renderer_config} { }
    
    graphics::render_pipeline
    render_pipeline_manager::create_render_flow(std::vector<graphics::render_pipeline_node> const &, std::vector<graphics::render_pipeline_output> const &)
    {
        return graphics::render_pipeline();
    }
}
