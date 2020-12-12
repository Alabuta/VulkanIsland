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
        if constexpr (std::is_same_v<T, renderer::nonindexed_draw_command>) {
            if (lhs.vertex_input_binding_index != rhs.vertex_input_binding_index)
                return lhs.vertex_input_binding_index < rhs.vertex_input_binding_index;

            if (lhs.vertex_buffer->device_buffer()->handle() != rhs.vertex_buffer->device_buffer()->handle())
                return lhs.vertex_buffer->device_buffer()->handle() < rhs.vertex_buffer->device_buffer()->handle();

            return lhs.first_vertex < rhs.first_vertex;
        }

        else return false;
    }

    void draw_commands_holder::add_draw_command(renderer::nonindexed_draw_command const &draw_command)
    {
        nonindexed_draw_commands_.push_back(draw_command);
    }

    void draw_commands_holder::add_draw_command(renderer::indexed_draw_command const &draw_command)
    {
        indexed_draw_commands_.push_back(draw_command);
    }

    std::vector<renderer::nonindexed_primitives_buffers_bind_range>
    draw_commands_holder::get_nonindexed_primitives_buffers_bind_range()
    {
        auto &&draw_commands = nonindexed_draw_commands_;

        std::stable_sort(std::begin(draw_commands), std::end(draw_commands), comparator<renderer::nonindexed_draw_command>{});

        std::vector<renderer::nonindexed_primitives_buffers_bind_range> buffers_bind_range;

        for (auto it_begin = std::begin(draw_commands); it_begin != std::end(draw_commands);) {
            auto h = it_begin->vertex_buffer->device_buffer()->handle();
            auto i = it_begin->vertex_input_binding_index;

            std::vector<VkBuffer> buffer_handles;

            auto it_end = std::stable_partition(it_begin, std::end(draw_commands), [&h, &i, &buffer_handles] (auto &&b)
            {
                if (i == b.vertex_input_binding_index)
                    return h == b.vertex_buffer->device_buffer()->handle();

                else if (b.vertex_input_binding_index - i == 1) {
                    h = b.vertex_buffer->device_buffer()->handle();
                    i = b.vertex_input_binding_index;

                    buffer_handles.push_back(h);

                    return true;
                }

                return false;
            });

            buffers_bind_range.push_back(renderer::nonindexed_primitives_buffers_bind_range{
                it_begin->vertex_input_binding_index,
                buffer_handles,
                std::vector<VkDeviceSize>(std::size(buffer_handles), 0u),
                std::span{it_begin, it_end}
            });

            it_begin = it_end;
        }

        return buffers_bind_range;
    }
}
