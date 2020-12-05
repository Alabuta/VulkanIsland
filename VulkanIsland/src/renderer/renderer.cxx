#include <algorithm>
#include <ranges>
#include <tuple>

#include <renderer/renderer.hxx>

#include "graphics/graphics_pipeline.hxx"


namespace renderer
{
    template<class T>
    template<class L, class R> requires mpl::are_same_v<T, std::remove_cvref_t<L>, std::remove_cvref_t<R>>
    constexpr bool draw_commands_holder::comparator<T>::operator() (L &&lhs, R &&rhs) const
    {
        if (lhs.vertex_input_binding_index != rhs.vertex_input_binding_index)
            return lhs.vertex_input_binding_index < rhs.vertex_input_binding_index;

        if (lhs.vertex_buffer->device_buffer()->handle() != rhs.vertex_buffer->device_buffer()->handle())
            return lhs.vertex_buffer->device_buffer()->handle() < rhs.vertex_buffer->device_buffer()->handle();

        return lhs.first_vertex < rhs.first_vertex;
    }

    void draw_commands_holder::add_draw_command(renderer::nonindexed_draw_command const &draw_command)
    {
        nonindexed_draw_commands_.emplace(draw_command);
    }

    void draw_commands_holder::add_draw_command(renderer::indexed_draw_command const &draw_command)
    {
        indexed_draw_commands_.emplace(draw_command);
    }

    std::vector<renderer::nonindexed_primitives_buffers_bind_range> draw_commands_holder::get_nonindexed_primitives_buffers_bind_range() const
    {
        std::vector<renderer::nonindexed_primitives_buffers_bind_range> buffers_bind_range;

        ;

        return buffers_bind_range;
    }
}
