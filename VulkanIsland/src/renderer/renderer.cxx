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
        std::stable_sort(std::begin(nonindexed_draw_commands_), std::end(nonindexed_draw_commands_), comparator<renderer::nonindexed_draw_command>{});

        std::vector<renderer::nonindexed_draw_command> draw_commands_copy{std::begin(nonindexed_draw_commands_), std::end(nonindexed_draw_commands_)};

        std::vector<renderer::nonindexed_primitives_buffers_bind_range> buffers_bind_range;

        ;

        return buffers_bind_range;
    }
}

/*#include <algorithm>
#include <iostream>
#include <variant>
#include <memory>
#include <string>
#include <vector>
#include <ranges>
#include <span>
#include <map>
#include <set>

#include <boost/range/algorithm.hpp>

namespace resource {
    struct vertex_buffer final {
        vertex_buffer(std::uint32_t h = 0) : handle{h} { }

        std::uint32_t handle;
    };
}

namespace renderer {
    struct nonindexed_draw_command final {
        std::shared_ptr<resource::vertex_buffer> vertex_buffer;
        std::uint32_t vertex_input_binding_index{0};
        std::uint32_t first_vertex{0};
    };

    struct nonindexed_primitives_buffers_bind_range final {
        std::uint32_t first_binding;
        std::uint32_t binding_count;

        std::vector<std::uint32_t> buffer_handles;
        std::vector<std::uint32_t> buffer_offsets;

        std::span<renderer::nonindexed_draw_command> draw_commands;
    };

    template<class T>
    struct comparator final {
        template<class L, class R>
        constexpr bool operator() (L &&lhs, R &&rhs) const
        {
            if constexpr (std::is_same_v<T, renderer::nonindexed_draw_command>) {
                if (lhs.vertex_input_binding_index != rhs.vertex_input_binding_index)
                    return lhs.vertex_input_binding_index < rhs.vertex_input_binding_index;

                return lhs.vertex_buffer->handle < rhs.vertex_buffer->handle;

#if 0
                if (lhs.vertex_buffer->handle != rhs.vertex_buffer->handle)
                    return lhs.vertex_buffer->handle < rhs.vertex_buffer->handle;

                return lhs.first_vertex < rhs.first_vertex;
#endif
            }

            else return false;
        }
    };
}

namespace renderer
{
    std::ostream &operator<<(std::ostream &out, renderer::nonindexed_draw_command const &c)
    {
        return out << "h" << c.vertex_buffer->handle << " i" << c.vertex_input_binding_index;// << " v" << c.first_vertex;
    }

    std::ostream &operator<<(std::ostream &out, renderer::nonindexed_primitives_buffers_bind_range const &r)
    {
        std::ranges::copy(r.buffer_handles, std::ostream_iterator<int>(out << "fbnd " << r.first_binding << " [", " "));
        return out << "]";
    }
}

std::vector<renderer::nonindexed_primitives_buffers_bind_range> get_nonindexed_primitives_buffers_bind_range(std::span<renderer::nonindexed_draw_command> nonindexed_draw_commands_)
{
    std::stable_sort(std::begin(nonindexed_draw_commands_), std::end(nonindexed_draw_commands_), renderer::comparator<renderer::nonindexed_draw_command>{});

    std::ranges::copy(nonindexed_draw_commands_, std::ostream_iterator<renderer::nonindexed_draw_command>(std::cout, "|"));
    std::cout << std::endl;

    auto it_begin = std::begin(nonindexed_draw_commands_);

    while (it_begin != std::end(nonindexed_draw_commands_)) {
        auto &&a = *it_begin;

        auto p = [a] (auto &&b)
        {
            if (a.vertex_input_binding_index == b.vertex_input_binding_index)
                return a.vertex_buffer->handle == b.vertex_buffer->handle;

            else
                return b.vertex_input_binding_index - a.vertex_input_binding_index > 1;

            return false;
        };

        auto it_end = std::partition_point(it_begin, std::end(nonindexed_draw_commands_), p);

        std::copy(it_begin, it_end, std::ostream_iterator<renderer::nonindexed_draw_command>(std::cout, "|"));
        std::cout << std::endl;

        it_begin = it_end;
    }

    std::vector<renderer::nonindexed_primitives_buffers_bind_range> buffers_bind_range;

    ;

    return buffers_bind_range;
}


int main()
{
    
    h0 i0|h0 i0|h0 i1|
    h4 i0|h1 i1|h2 i1|h2 i1|
    h7 i1|
    h5 i3|h6 i4|
    h8 i6|
    
    std::vector<renderer::nonindexed_draw_command> commands{
        {std::make_shared<resource::vertex_buffer>(0), 0},
        {std::make_shared<resource::vertex_buffer>(0), 0},
        {std::make_shared<resource::vertex_buffer>(2), 1},
        {std::make_shared<resource::vertex_buffer>(4), 0},
        {std::make_shared<resource::vertex_buffer>(1), 1},
        {std::make_shared<resource::vertex_buffer>(2), 1},
        {std::make_shared<resource::vertex_buffer>(0), 1},
        {std::make_shared<resource::vertex_buffer>(5), 3},
        {std::make_shared<resource::vertex_buffer>(6), 4},
        {std::make_shared<resource::vertex_buffer>(8), 6},
        {std::make_shared<resource::vertex_buffer>(7), 1}
    };

    std::ranges::copy(commands, std::ostream_iterator<renderer::nonindexed_draw_command>(std::cout, "|"));
    std::cout << std::endl;

    auto buffers_bind_range = get_nonindexed_primitives_buffers_bind_range(commands);
}*/
