#include "render_flow.hxx"


namespace graphics
{
    graphics::render_pipeline
    render_pipeline_manager::create_render_flow(std::vector<graphics::render_pipeline_node> const &, std::vector<graphics::render_pipeline_output> const &)
    {
        return graphics::render_pipeline();
    }
}
