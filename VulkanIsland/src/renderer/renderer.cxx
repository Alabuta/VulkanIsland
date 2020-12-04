#include <renderer/renderer.hxx>

#include "graphics/graphics_pipeline.hxx"

namespace renderer
{
    draw_commands_system::draw_commands_system(graphics::vertex_input_state_manager const &vertex_input_state_manager)
        : vertex_input_state_manager_{vertex_input_state_manager}
    { }

    void draw_commands_system::add_draw_command(renderer::nonindexed_draw_command const &draw_command)
    {
        auto &&draw_commands = nonindexed_draw_commands_;

        draw_commands.push_back(draw_command);

        std::stable_sort(std::begin(draw_commands), std::end(draw_commands), [] (auto &&lhs, auto &&rhs)
        {
            if (lhs.vertex_input_binding_index == rhs.vertex_input_binding_index)
                return lhs.vertex_buffer->device_buffer()->handle() < rhs.vertex_buffer->device_buffer()->handle();

            return lhs.vertex_input_binding_index < rhs.vertex_input_binding_index;
        });
    }

    void draw_commands_system::add_draw_command(renderer::indexed_draw_command const &)
    {
        ;
    }
}
