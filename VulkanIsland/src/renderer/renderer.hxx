#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <span>
#include <set>

#include "utility/mpl.hxx"

#include "vulkan/device.hxx"
#include "graphics/graphics.hxx"
#include "resources/buffer.hxx"

#include "renderer/config.hxx"


namespace renderer
{
    struct nonindexed_draw_command final {
        std::shared_ptr<resource::vertex_buffer> vertex_buffer;

        std::uint32_t vertex_input_binding_index{0};

        std::uint32_t first_vertex{0};
        std::uint32_t vertex_count{0};
    };

    struct indexed_draw_command final {
        std::shared_ptr<resource::vertex_buffer> vertex_buffer;
        std::shared_ptr<resource::index_buffer> index_buffer;

        std::uint32_t vertex_input_binding_index{0};

        std::uint32_t first_vertex{0};
        std::uint32_t vertex_count{0};

        std::uint32_t first_index{0};
        std::uint32_t index_count{0};
    };

    struct nonindexed_primitives_buffers_bind_range final {
        std::uint32_t first_binding;
        std::uint32_t binding_count;

        std::vector<VkBuffer> buffer_handles;
        std::vector<VkDeviceSize> buffer_offsets;

        std::span<renderer::nonindexed_draw_command> draw_commands;
    };

    struct indexed_primitives_buffers_bind_range final {
        graphics::INDEX_TYPE index_type;

        VkBuffer index_buffer_handle;
        VkDeviceSize index_buffer_offset;

        std::span<renderer::indexed_draw_command> draw_commands;
    };

    class draw_commands_holder final {
    public:

        void add_draw_command(renderer::nonindexed_draw_command const &draw_command);
        void add_draw_command(renderer::indexed_draw_command const &draw_command);

        [[nodiscard]]
        std::vector<renderer::nonindexed_primitives_buffers_bind_range> get_nonindexed_primitives_buffers_bind_range();

    private:

        template<class T>
        struct comparator final {
            template<class L, class R>
            requires mpl::are_same_v<T, std::remove_cvref_t<L>, std::remove_cvref_t<R>>
            constexpr bool operator() (L &&lhs, R &&rhs) const;
        };

        std::vector<renderer::nonindexed_draw_command> nonindexed_draw_commands_;
        std::vector<renderer::indexed_draw_command> indexed_draw_commands_;
    };

    //std::pair<renderer::nonindexed_draw_buffers_bind_range, renderer::indexed_draw_buffers_bind_range>
}
