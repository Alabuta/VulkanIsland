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

                if (lhs.vertex_buffer->handle != rhs.vertex_buffer->handle)
                    return lhs.vertex_buffer->handle < rhs.vertex_buffer->handle;

                return lhs.first_vertex < rhs.first_vertex;
            }

            else return false;
        }
    };
}

namespace renderer {
    std::ostream &operator<<(std::ostream &out, renderer::nonindexed_draw_command const &c)
    {
        return out << "h" << c.vertex_buffer->handle << " i" << c.vertex_input_binding_index;// << " fv " << c.first_vertex;
    }

    std::ostream &operator<<(std::ostream &out, renderer::nonindexed_primitives_buffers_bind_range const &r)
    {
        std::ranges::copy(r.buffer_handles,  std::ostream_iterator<int>(out << "fbnd " << r.first_binding << " [", " "));
        return out << "]";
    }
}

std::vector<renderer::nonindexed_primitives_buffers_bind_range> get_nonindexed_primitives_buffers_bind_range(std::span<renderer::nonindexed_draw_command> nonindexed_draw_commands_)
{
    std::stable_sort(std::begin(nonindexed_draw_commands_), std::end(nonindexed_draw_commands_), renderer::comparator<renderer::nonindexed_draw_command>{});

    std::ranges::copy(nonindexed_draw_commands_, std::ostream_iterator<renderer::nonindexed_draw_command>(std::cout, "|"));
    std::cout << std::endl;

    std::vector<renderer::nonindexed_draw_command> draw_commands_copy{std::begin(nonindexed_draw_commands_), std::end(nonindexed_draw_commands_)};

    std::vector<renderer::nonindexed_primitives_buffers_bind_range> buffers_bind_range;

    ;

    return buffers_bind_range;
}


int main()
{
        // DC{0, 0}, DC{0, 0}, DC{1, 2}, DC{0, 4}, DC{1, 1}, DC{1, 2}, DC{1, 0},
        // DC{3, 5}, DC{4, 6}, DC{6, 8}, DC{1, 7}
    renderer::nonindexed_draw_command c{std::make_shared<resource::vertex_buffer>(0), 0};

    std::vector<renderer::nonindexed_draw_command> commands{
        {std::make_shared<resource::vertex_buffer>(0), 0},
        {std::make_shared<resource::vertex_buffer>(0), 0},
        {std::make_shared<resource::vertex_buffer>(1), 2},
        {std::make_shared<resource::vertex_buffer>(0), 4},
        {std::make_shared<resource::vertex_buffer>(1), 1},
        {std::make_shared<resource::vertex_buffer>(1), 2},
        {std::make_shared<resource::vertex_buffer>(1), 0},
        {std::make_shared<resource::vertex_buffer>(3), 5},
        {std::make_shared<resource::vertex_buffer>(4), 6},
        {std::make_shared<resource::vertex_buffer>(6), 8},
        {std::make_shared<resource::vertex_buffer>(1), 7}
    };

    std::ranges::copy(commands, std::ostream_iterator<renderer::nonindexed_draw_command>(std::cout, "|"));
    std::cout << std::endl;

    get_nonindexed_primitives_buffers_bind_range(commands);
}*/
