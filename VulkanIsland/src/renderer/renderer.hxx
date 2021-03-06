#pragma once

#include <cstdint>
#include <variant>
#include <memory>
#include <vector>
#include <span>
#include <set>

#include "utility/mpl.hxx"

#include "vulkan/device.hxx"
#include "graphics/graphics.hxx"
#include "resources/buffer.hxx"

#include "renderer/config.hxx"


namespace graphics
{
    class pipeline;
    struct material;
    class render_pass;
    class vertex_input_state_manager;
}

namespace renderer
{
    struct nonindexed_draw_command final {
        std::shared_ptr<graphics::pipeline> pipeline;
        std::shared_ptr<graphics::material> material;

        VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};
        VkDescriptorSet descriptor_set{VK_NULL_HANDLE};

        std::shared_ptr<graphics::render_pass> render_pass;

        std::shared_ptr<resource::vertex_buffer> vertex_buffer;

        std::uint32_t vertex_input_binding_index{0};

        std::uint32_t first_vertex{0};
        std::uint32_t vertex_count{0};

        std::uint32_t transform_index{0};
    };

    struct indexed_draw_command final {
        std::shared_ptr<graphics::pipeline> pipeline;
        std::shared_ptr<graphics::material> material;

        VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};
        VkDescriptorSet descriptor_set{VK_NULL_HANDLE};

        std::shared_ptr<graphics::render_pass> render_pass;

        std::shared_ptr<resource::vertex_buffer> vertex_buffer;
        std::shared_ptr<resource::index_buffer> index_buffer;

        std::uint32_t vertex_input_binding_index{0};

        std::uint32_t first_vertex{0};
        std::uint32_t vertex_count{0};

        std::uint32_t first_index{0};
        std::uint32_t index_count{0};

        std::uint32_t transform_index{0};
    };

    struct vertex_buffers_bind_range final {
        std::uint32_t first_binding;

        std::vector<VkBuffer> buffer_handles;
        std::vector<VkDeviceSize> buffer_offsets;

        std::variant<
            std::span<renderer::nonindexed_draw_command>,
            std::span<renderer::indexed_draw_command>
        > draw_commands;
    };

    struct indexed_primitives_buffers_bind_range final {
        graphics::INDEX_TYPE index_type;

        VkBuffer index_buffer_handle;
        VkDeviceSize index_buffer_offset;

        std::vector<renderer::vertex_buffers_bind_range> vertex_buffers_bind_ranges;
    };

    class draw_commands_holder final {
    public:

        void add_draw_command(renderer::nonindexed_draw_command const &draw_command);
        void add_draw_command(renderer::indexed_draw_command const &draw_command);

        [[nodiscard]]
        std::vector<renderer::vertex_buffers_bind_range> get_primitives_buffers_bind_ranges();

        [[nodiscard]]
        std::vector<renderer::indexed_primitives_buffers_bind_range> get_indexed_primitives_buffers_bind_range();

        void clear();

    private:

        template<class T>
        struct comparator final {
            template<class L, class R>
            requires mpl::are_same_v<T, std::remove_cvref_t<L>, std::remove_cvref_t<R>>
            constexpr bool operator() (L &&lhs, R &&rhs) const;
        };

        std::vector<renderer::nonindexed_draw_command> nonindexed_draw_commands_;
        std::vector<renderer::indexed_draw_command> indexed_draw_commands_;

        template<class T>
        void partion_vertex_buffers_binds(std::span<T> draw_commands, std::function<void(std::vector<VkBuffer> &&, std::span<T>)> callback);
    };

    //std::pair<renderer::nonindexed_draw_buffers_bind_range, renderer::indexed_draw_buffers_bind_range>
}

struct draw_command final {
    std::shared_ptr<graphics::material> material;
    std::shared_ptr<graphics::pipeline> pipeline;

    std::shared_ptr<resource::vertex_buffer> vertex_buffer;
    std::shared_ptr<resource::index_buffer> index_buffer;

    std::uint32_t vertex_count{0};
    std::uint32_t first_vertex{0};

    std::uint32_t index_count{0};
    std::uint32_t first_index{0};

    VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};

    std::shared_ptr<graphics::render_pass> render_pass;
    VkDescriptorSet descriptor_set{VK_NULL_HANDLE};
};

struct vertex_buffers_bind_ranges final {
    std::uint32_t first_binding;
    std::uint32_t binding_count;

    std::vector<VkBuffer> vertex_buffer_handles;
    std::vector<VkDeviceSize> vertex_buffer_offsets;

    std::span<draw_command> draw_commands;
};

struct indexed_primitives_draw final {
    graphics::INDEX_TYPE index_type;

    VkBuffer index_buffer_handle;
    VkDeviceSize index_buffer_offset;

    std::vector<vertex_buffers_bind_ranges> subranges;
};

std::pair<std::span<draw_command>, std::span<draw_command>>
separate_indexed_and_nonindexed(std::span<draw_command> draw_commands);

std::vector<vertex_buffers_bind_ranges>
separate_nonindexed_by_binds(graphics::vertex_input_state_manager &vertex_input_state_manager, std::span<draw_command> draw_commands);

std::vector<indexed_primitives_draw>
separate_indexed_by_binds(std::span<draw_command> draw_commands);