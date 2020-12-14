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
        if constexpr (std::is_same_v<T, renderer::indexed_draw_command>) {
            if (lhs.index_buffer->index_type != rhs.index_buffer->index_type)
                return lhs.index_buffer->index_type < rhs.index_buffer->index_type;

            if (lhs.index_buffer->device_buffer()->handle() != rhs.index_buffer->device_buffer()->handle())
                return lhs.index_buffer->device_buffer()->handle() < rhs.index_buffer->device_buffer()->handle();
        }

        if (lhs.vertex_input_binding_index != rhs.vertex_input_binding_index)
            return lhs.vertex_input_binding_index < rhs.vertex_input_binding_index;

        if (lhs.vertex_buffer->device_buffer()->handle() != rhs.vertex_buffer->device_buffer()->handle())
            return lhs.vertex_buffer->device_buffer()->handle() < rhs.vertex_buffer->device_buffer()->handle();

        return lhs.first_vertex < rhs.first_vertex;
    }

    void draw_commands_holder::add_draw_command(renderer::nonindexed_draw_command const &draw_command)
    {
        nonindexed_draw_commands_.push_back(draw_command);
    }

    void draw_commands_holder::add_draw_command(renderer::indexed_draw_command const &draw_command)
    {
        indexed_draw_commands_.push_back(draw_command);
    }

    template<class T>
    void foo(std::span<T> draw_commands, std::function<void(std::vector<VkBuffer> &&)>)
    {
        ;
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

#if 0

#include <algorithm>
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

enum INDEX_TYPE {
    UNDEFINED = 0,
    UINT_16,
    UINT_32
};

namespace resource
{
    struct vertex_buffer final {
        vertex_buffer(std::uint32_t h = 0) : handle{h} { }
        std::uint32_t handle;
    };

    struct index_buffer final {
        index_buffer(std::uint32_t h, INDEX_TYPE i) : handle{h}, index_type{i} { }
        INDEX_TYPE index_type;
        std::uint32_t handle;
    };
}

namespace renderer
{
    struct nonindexed_draw_command final {
        std::shared_ptr<resource::vertex_buffer> vertex_buffer;
        std::uint32_t vertex_input_binding_index{0};
        std::uint32_t first_vertex{0};
    };

    struct indexed_draw_command final {
        std::shared_ptr<resource::vertex_buffer> vertex_buffer;
        std::shared_ptr<resource::index_buffer> index_buffer;
        std::uint32_t vertex_input_binding_index{0};
        std::uint32_t first_vertex{0};
        std::uint32_t first_index{0};
    };

    struct nonindexed_primitives_buffers_bind_range final {
        std::uint32_t first_binding;
        std::vector<std::uint32_t> buffer_handles;
        std::vector<std::uint32_t> buffer_offsets;
        std::span<renderer::nonindexed_draw_command> draw_commands;
    };

    struct indexed_primitives_buffers_bind_range final {
        INDEX_TYPE index_type;
        std::uint32_t index_buffer_handle;
        std::uint32_t index_buffer_offset;
        std::span<renderer::nonindexed_draw_command> draw_commands;
    };

    template<class T>
    struct comparator final {
        template<class L, class R>
        constexpr bool operator() (L &&lhs, R &&rhs) const
        {
            if constexpr (std::is_same_v<T, renderer::indexed_draw_command>) {
                if (lhs.index_buffer->index_type != rhs.index_buffer->index_type)
                    return lhs.index_buffer->index_type < rhs.index_buffer->index_type;

                if (lhs.index_buffer->handle != rhs.index_buffer->handle)
                    return lhs.index_buffer->handle < rhs.index_buffer->handle;
            }

            if (lhs.vertex_input_binding_index != rhs.vertex_input_binding_index)
                return lhs.vertex_input_binding_index < rhs.vertex_input_binding_index;

            if (lhs.vertex_buffer->handle != rhs.vertex_buffer->handle)
                return lhs.vertex_buffer->handle < rhs.vertex_buffer->handle;

            return lhs.first_vertex < rhs.first_vertex;
        }
    };
}

namespace renderer
{
    std::ostream &operator<<(std::ostream &out, renderer::nonindexed_draw_command const &c)
    {
        return out << "h" << c.vertex_buffer->handle << " l" << c.vertex_input_binding_index;// << " v" << c.first_vertex;
    }
    std::ostream &operator<<(std::ostream &out, renderer::indexed_draw_command const &c)
    {
        return out << "h" << c.index_buffer->handle << "idx" << c.index_buffer->index_type << " h" << c.vertex_buffer->handle << "L" << c.vertex_input_binding_index;// << " v" << c.first_vertex;
    }
    std::ostream &operator<<(std::ostream &out, renderer::nonindexed_primitives_buffers_bind_range const &r)
    {
        std::ranges::copy(r.buffer_handles, std::ostream_iterator<int>(out << "fbnd " << r.first_binding << " [", " "));
        return out << "]";
    }
    // std::ostream &operator<<(std::ostream &out, renderer::indexed_primitives_buffers_bind_range const &r)
    // {
    //     out << "idx " << r.index_type << 
    //     std::ranges::copy(r.buffer_handles, std::ostream_iterator<int>(out << "fbnd " << r.first_binding << " [", " "));
    //     return out << "]";
    // }
}

template<class T>
auto get_primitives_buffers_bind_range(std::span<T> draw_commands)
{
    ;
}

std::vector<renderer::nonindexed_primitives_buffers_bind_range> get_nonindexed_primitives_buffers_bind_range(std::span<renderer::nonindexed_draw_command> nonindexed_draw_commands_)
{
    std::stable_sort(std::begin(nonindexed_draw_commands_), std::end(nonindexed_draw_commands_), renderer::comparator<renderer::nonindexed_draw_command>{});

    std::ranges::copy(nonindexed_draw_commands_, std::ostream_iterator<renderer::nonindexed_draw_command>(std::cout, "|"));
    std::cout << std::endl;

    std::vector<renderer::nonindexed_primitives_buffers_bind_range> buffers_bind_range;

    for (auto it_begin = std::begin(nonindexed_draw_commands_); it_begin != std::end(nonindexed_draw_commands_);) {
        auto h = it_begin->vertex_buffer->handle;
        auto i = it_begin->vertex_input_binding_index;
        // std::cout << "a " << h << ' ' << i << std::endl;

        std::vector<std::uint32_t> buffer_handles(1, h);

        auto it_end = std::stable_partition(it_begin, std::end(nonindexed_draw_commands_), [&h, &i, &buffer_handles] (auto &&b)
        {
            if (i == b.vertex_input_binding_index) {
                // std::cout << "bf " << h << ' ' << i << '|' << b.vertex_buffer->handle << ' ' << b.vertex_input_binding_index << std::boolalpha << (h == b.vertex_buffer->handle) << std::endl;
                return h == b.vertex_buffer->handle;
            }

            else if (b.vertex_input_binding_index - i == 1) {
                // std::cout << "bs " << h << ' ' << i << '|' << b.vertex_buffer->handle << ' ' << b.vertex_input_binding_index << std::endl;
                h = b.vertex_buffer->handle;
                i = b.vertex_input_binding_index;

                buffer_handles.push_back(h);
                return true;
            }

            return false;
        });

        std::copy(it_begin, it_end, std::ostream_iterator<renderer::nonindexed_draw_command>(std::cout, "|"));
        std::cout << std::endl;

        std::ranges::copy(buffer_handles, std::ostream_iterator<std::uint32_t>(std::cout, " "));
        std::cout << std::endl;

        buffers_bind_range.push_back(renderer::nonindexed_primitives_buffers_bind_range{
            it_begin->vertex_input_binding_index,
            buffer_handles,
            std::vector<std::uint32_t>(std::size(buffer_handles), 0u),
            std::span{it_begin, it_end}
                                     });

        it_begin = it_end;
    }

    return buffers_bind_range;
}

std::vector<renderer::indexed_primitives_buffers_bind_range> get_indexed_primitives_buffers_bind_range(std::span<renderer::indexed_draw_command> draw_commands_)
{
    std::stable_sort(std::begin(draw_commands_), std::end(draw_commands_), renderer::comparator<renderer::indexed_draw_command>{});

    std::ranges::copy(draw_commands_, std::ostream_iterator<renderer::indexed_draw_command>(std::cout, "|"));
    std::cout << std::endl;

    auto it_begin = std::begin(draw_commands_);

    while (it_begin != std::end(draw_commands_)) {
        auto it = std::adjacent_find(it_begin, std::end(draw_commands_), [] (auto &&lhs, auto &&rhs)
        {
            return lhs.index_buffer->index_type != rhs.index_buffer->index_type || lhs.index_buffer->handle != rhs.index_buffer->handle;
        });

        if (it != std::end(draw_commands_))
            it = std::next(it);

        auto dcs = std::span{it_begin, it};

        // auto buffers_bind_range = get_nonindexed_primitives_buffers_bind_range(dcs);

        // std::ranges::copy(dcs, std::ostream_iterator<renderer::indexed_draw_command>(std::cout, "|"));
        // std::cout << std::endl;

        for (auto it_begin2 = std::begin(dcs); it_begin2 != std::end(dcs);) {
            auto h = it_begin2->vertex_buffer->handle;
            auto i = it_begin2->vertex_input_binding_index;
            // std::cout << "a " << h << ' ' << i << std::endl;

            std::vector<std::uint32_t> buffer_handles(1, h);

            auto it_end2 = std::stable_partition(it_begin2, std::end(dcs), [&h, &i, &buffer_handles] (auto &&b)
            {
                if (i == b.vertex_input_binding_index) {
                    // std::cout << "bf " << h << ' ' << i << '|' << b.vertex_buffer->handle << ' ' << b.vertex_input_binding_index << std::boolalpha << (h == b.vertex_buffer->handle) << std::endl;
                    return h == b.vertex_buffer->handle;
                }

                else if (b.vertex_input_binding_index - i == 1) {
                    // std::cout << "bs " << h << ' ' << i << '|' << b.vertex_buffer->handle << ' ' << b.vertex_input_binding_index << std::endl;
                    h = b.vertex_buffer->handle;
                    i = b.vertex_input_binding_index;

                    buffer_handles.push_back(h);
                    return true;
    }

                return false;
});

            std::copy(it_begin2, it_end2, std::ostream_iterator<renderer::indexed_draw_command>(std::cout, "|"));
            std::cout << std::endl;

            // std::ranges::copy(buffer_handles, std::ostream_iterator<std::uint32_t>(std::cout, " "));
            // std::cout << std::endl;

            // buffers_bind_range.push_back(renderer::nonindexed_primitives_buffers_bind_range{
            //     it_begin2->vertex_input_binding_index,
            //     buffer_handles,
            //     std::vector<std::uint32_t>(std::size(buffer_handles), 0u),
            //     std::span{it_begin2, it_end2}
            // });

            it_begin2 = it_end2;
        }

        // std::ranges::copy(dcs, std::ostream_iterator<renderer::indexed_draw_command>(std::cout, "|"));
        std::cout << std::endl;

        it_begin = it;
    }

    return {};
}

int main()
{
    // h0 i0|h0 i0|h0 i1|
    // h4 i0|h1 i1|h2 i1|h2 i1|
    // h7 i1|
    // h5 i3|h6 i4|
    // h8 i6|

    // std::vector<renderer::nonindexed_draw_command> commands{
    //     {std::make_shared<resource::vertex_buffer>(0), 0},
    //     {std::make_shared<resource::vertex_buffer>(0), 0},
    //     {std::make_shared<resource::vertex_buffer>(2), 1},
    //     {std::make_shared<resource::vertex_buffer>(4), 0},
    //     {std::make_shared<resource::vertex_buffer>(1), 1},
    //     // {std::make_shared<resource::vertex_buffer>(1), 2},
    //     {std::make_shared<resource::vertex_buffer>(2), 1},
    //     {std::make_shared<resource::vertex_buffer>(0), 1},
    //     {std::make_shared<resource::vertex_buffer>(5), 3},
    //     {std::make_shared<resource::vertex_buffer>(6), 4},
    //     {std::make_shared<resource::vertex_buffer>(8), 6},
    //     {std::make_shared<resource::vertex_buffer>(0), 1},
    //     {std::make_shared<resource::vertex_buffer>(7), 1}
    // };

    // auto buffers_bind_range = get_nonindexed_primitives_buffers_bind_range(commands);

    // std::ranges::copy(commands, std::ostream_iterator<renderer::nonindexed_draw_command>(std::cout, "|"));
    // std::cout << std::endl;

    // std::ranges::copy(buffers_bind_range, std::ostream_iterator<renderer::nonindexed_primitives_buffers_bind_range>(std::cout, "|"));
    // std::cout << std::endl;

    std::vector<renderer::indexed_draw_command> idxcommands{
        {std::make_shared<resource::vertex_buffer>(0), std::make_shared<resource::index_buffer>(0, INDEX_TYPE::UINT_16), 0, 0},
        {std::make_shared<resource::vertex_buffer>(0), std::make_shared<resource::index_buffer>(1, INDEX_TYPE::UINT_16), 0, 0},
        {std::make_shared<resource::vertex_buffer>(2), std::make_shared<resource::index_buffer>(0, INDEX_TYPE::UINT_16), 1, 0},
        {std::make_shared<resource::vertex_buffer>(4), std::make_shared<resource::index_buffer>(1, INDEX_TYPE::UINT_16), 0, 0},
        {std::make_shared<resource::vertex_buffer>(1), std::make_shared<resource::index_buffer>(3, INDEX_TYPE::UINT_16), 1, 0},
        {std::make_shared<resource::vertex_buffer>(1), std::make_shared<resource::index_buffer>(2, INDEX_TYPE::UINT_16), 2, 0},
        {std::make_shared<resource::vertex_buffer>(2), std::make_shared<resource::index_buffer>(4, INDEX_TYPE::UINT_16), 1, 0},
        {std::make_shared<resource::vertex_buffer>(0), std::make_shared<resource::index_buffer>(1, INDEX_TYPE::UINT_16), 1, 0},
        {std::make_shared<resource::vertex_buffer>(5), std::make_shared<resource::index_buffer>(1, INDEX_TYPE::UINT_16), 3, 0},
        {std::make_shared<resource::vertex_buffer>(6), std::make_shared<resource::index_buffer>(1, INDEX_TYPE::UINT_16), 4, 0},
        {std::make_shared<resource::vertex_buffer>(8), std::make_shared<resource::index_buffer>(2, INDEX_TYPE::UINT_16), 6, 0},
        {std::make_shared<resource::vertex_buffer>(0), std::make_shared<resource::index_buffer>(0, INDEX_TYPE::UINT_16), 1, 0},
        {std::make_shared<resource::vertex_buffer>(7), std::make_shared<resource::index_buffer>(1, INDEX_TYPE::UINT_16), 1, 0}
    };

    get_indexed_primitives_buffers_bind_range(idxcommands);

    // std::ranges::copy(commands, std::ostream_iterator<renderer::nonindexed_draw_command>(std::cout, "|"));
    // std::cout << std::endl;
}
#endif
