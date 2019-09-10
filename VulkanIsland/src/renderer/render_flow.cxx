#include "render_flow.hxx"


namespace graphics
{
    void render_flow::add_nodes(std::vector<graphics::render_flow_node> const &nodes)
    {
        nodes_ = nodes;
    }

    void render_flow::output_layout(std::vector<graphics::render_flow_output> const &output)
    { }
}
