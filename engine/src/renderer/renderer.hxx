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
#include "swapchain.hxx"
#include "command_buffer.hxx"


namespace graphics
{
    class pipeline;
    struct material;
    class render_pass;
    class vertex_input_state_manager;
}

namespace render
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
            std::span<render::nonindexed_draw_command>,
            std::span<render::indexed_draw_command>
        > draw_commands;
    };

    struct indexed_primitives_buffers_bind_range final {
        graphics::INDEX_TYPE index_type;

        VkBuffer index_buffer_handle;
        VkDeviceSize index_buffer_offset;

        std::vector<render::vertex_buffers_bind_range> vertex_buffers_bind_ranges;
    };

    class draw_commands_holder final {
    public:

        void add_draw_command(render::nonindexed_draw_command const &draw_command);
        void add_draw_command(render::indexed_draw_command const &draw_command);

        [[nodiscard]]
        std::vector<render::vertex_buffers_bind_range> get_primitives_buffers_bind_ranges();

        [[nodiscard]]
        std::vector<render::indexed_primitives_buffers_bind_range> get_indexed_primitives_buffers_bind_range();

        void clear();

    private:

        template<class T>
        struct comparator final {
            template<class L, class R>
            requires mpl::are_same_v<T, std::remove_cvref_t<L>, std::remove_cvref_t<R>>
            constexpr bool operator() (L &&lhs, R &&rhs) const;
        };

        std::vector<render::nonindexed_draw_command> nonindexed_draw_commands_;
        std::vector<render::indexed_draw_command> indexed_draw_commands_;

        template<class T>
        void partion_vertex_buffers_binds(std::span<T> draw_commands, std::function<void(std::vector<VkBuffer> &&, std::span<T>)> callback) const;
    };

    //std::pair<render::nonindexed_draw_buffers_bind_range, render::indexed_draw_buffers_bind_range>

    struct render_pass final {


    };

    class renderer final {
    public:

        renderer(render::config const &renderer_config, vulkan::device &device);

        void render_frame(std::span<VkCommandBuffer const> command_buffers, std::function<void(void)> const &recreate_swap_chain_callback);

        void fill_draw_command_buffers(std::span<VkCommandBuffer> command_buffers, render::draw_commands_holder &draw_commands_holder, struct app_t const &app);

//        std::shared_ptr<vulkan::command_buffer> create_command_buffer();

    private:

        std::size_t current_frame_index_{0};

        render::config renderer_config_;

        std::unique_ptr<vulkan::instance> instance_;
        std::unique_ptr<vulkan::device> device_;

//        std::unique_ptr<render::swapchain> swapchain_;

        std::array<std::shared_ptr<resource::semaphore>, render::kCONCURRENTLY_PROCESSED_FRAMES> image_available_semaphores_;
        std::array<std::shared_ptr<resource::semaphore>, render::kCONCURRENTLY_PROCESSED_FRAMES> render_finished_semaphores_;

        std::array<std::shared_ptr<resource::fence>, render::kCONCURRENTLY_PROCESSED_FRAMES> concurrent_frames_fences_;

        std::vector<std::shared_ptr<resource::fence>> busy_frames_fences_;

        /*std::shared_ptr<graphics::render_pass> render_pass_;
        std::unique_ptr<graphics::render_pass_manager> render_pass_manager_;*/

        /*std::vector<graphics::attachment> attachments_;
        std::vector<std::shared_ptr<resource::framebuffer>> framebuffers_;*/
    };
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
