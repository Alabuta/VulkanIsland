#include <algorithm>
#include <ranges>
#include <tuple>

#include <boost/align.hpp>
#include <boost/align/align.hpp>

#include "resources/sync_objects.hxx"
#include "renderer/renderer.hxx"

#include "graphics/graphics_pipeline.hxx"

#include "app.hxx"


namespace render
{
    template<class T>
    template<class L, class R> requires mpl::are_same_v<T, std::remove_cvref_t<L>, std::remove_cvref_t<R>>
    constexpr bool draw_commands_holder::comparator<T>::operator() (L &&lhs, R &&rhs) const
    {
        if constexpr (std::is_same_v<T, render::indexed_draw_command>) {
            if (lhs.index_buffer->index_type() != rhs.index_buffer->index_type())
                return lhs.index_buffer->index_type() < rhs.index_buffer->index_type();

            if (lhs.index_buffer->device_buffer()->handle() != rhs.index_buffer->device_buffer()->handle())
                return lhs.index_buffer->device_buffer()->handle() < rhs.index_buffer->device_buffer()->handle();
        }

        if (lhs.vertex_input_binding_index != rhs.vertex_input_binding_index)
            return lhs.vertex_input_binding_index < rhs.vertex_input_binding_index;

        if (lhs.vertex_buffer->device_buffer()->handle() != rhs.vertex_buffer->device_buffer()->handle())
            return lhs.vertex_buffer->device_buffer()->handle() < rhs.vertex_buffer->device_buffer()->handle();

        return lhs.first_vertex < rhs.first_vertex;
    }

    void draw_commands_holder::add_draw_command(render::nonindexed_draw_command const &draw_command)
    {
        nonindexed_draw_commands_.push_back(draw_command);
    }

    void draw_commands_holder::add_draw_command(render::indexed_draw_command const &draw_command)
    {
        indexed_draw_commands_.push_back(draw_command);
    }

    std::vector<render::vertex_buffers_bind_range>
    draw_commands_holder::get_primitives_buffers_bind_ranges()
    {
        auto &draw_commands = nonindexed_draw_commands_;

        std::stable_sort(std::begin(draw_commands), std::end(draw_commands), comparator<render::nonindexed_draw_command>{});

        std::vector<render::vertex_buffers_bind_range> buffers_bind_range;

        partition_vertex_buffers_binds<render::nonindexed_draw_command>(draw_commands, [&buffers_bind_range] (auto &&buffer_handles, auto range)
        {
            buffers_bind_range.push_back({
                range.front().vertex_input_binding_index,
                buffer_handles,
                std::vector<VkDeviceSize>(std::size(buffer_handles), 0u),
                range
            });
        });

        return buffers_bind_range;
    }

    std::vector<render::indexed_primitives_buffers_bind_range>
    draw_commands_holder::get_indexed_primitives_buffers_bind_range()
    {
        auto &draw_commands = indexed_draw_commands_;

        std::stable_sort(std::begin(draw_commands), std::end(draw_commands), comparator<render::indexed_draw_command>{});

        std::vector<render::indexed_primitives_buffers_bind_range> indexed_buffers_bind_range;

        for (auto it_begin = std::begin(draw_commands); it_begin != std::end(draw_commands);) {
            auto it = std::adjacent_find(it_begin, std::end(draw_commands), [] (auto &&lhs, auto &&rhs)
            {
                if (lhs.index_buffer->device_buffer()->handle() != rhs.index_buffer->device_buffer()->handle())
                    return true;

                return lhs.index_buffer->index_type() != rhs.index_buffer->index_type();
            });

            if (it != std::end(draw_commands))
                it = std::next(it);

            std::vector<render::vertex_buffers_bind_range> vertex_buffers_bind_ranges;

            partition_vertex_buffers_binds<render::indexed_draw_command>(std::span{ it_begin, it }, [&vertex_buffers_bind_ranges](auto &&buffer_handles, auto range)
            {
                vertex_buffers_bind_ranges.push_back({
                    range.front().vertex_input_binding_index,
                    buffer_handles,
                    std::vector<VkDeviceSize>(std::size(buffer_handles), 0u),
                    range
                });
            });

            indexed_buffers_bind_range.push_back({
                it_begin->index_buffer->index_type(),
                it_begin->index_buffer->device_buffer()->handle(),
                0u,
                vertex_buffers_bind_ranges
            });

            it_begin = it;
        }

        return indexed_buffers_bind_range;
    }

    template<class T>
    void draw_commands_holder::partition_vertex_buffers_binds(
            std::span<T> draw_commands,
            std::function<void(std::vector<VkBuffer> &&, std::span<T>)> callback) const
    {
        for (auto it_begin = std::begin(draw_commands); it_begin != std::end(draw_commands);) {
            auto h = it_begin->vertex_buffer->device_buffer()->handle();
            auto i = it_begin->vertex_input_binding_index;

            std::vector<VkBuffer> buffer_handles(1, h);

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

            callback(std::move(buffer_handles), std::span{it_begin, it_end});

            it_begin = it_end;
        }
    }

    void draw_commands_holder::clear()
    {
        nonindexed_draw_commands_.clear();
        indexed_draw_commands_.clear();
    }
}

namespace render
{
    renderer::renderer(render::config const &renderer_config, vulkan::device &) : renderer_config_{ renderer_config }
    {
    }

    void renderer::render_frame(std::span<VkCommandBuffer const> const command_buffers, std::function<void(void)> const &recreate_swap_chain_callback)
    {
        auto &&device = *device_;
        auto &&swapchain = *swapchain_;

        auto &&image_available_semaphore = image_available_semaphores_[current_frame_index_];
        auto &&render_finished_semaphore = render_finished_semaphores_[current_frame_index_];

#define USE_FENCES 1

#if USE_FENCES
        auto &&frame_fence = concurrent_frames_fences_[current_frame_index_];

        if (auto result = vkWaitForFences(device.handle(), 1, frame_fence->handle_ptr(), VK_TRUE, std::numeric_limits<std::uint64_t>::max()); result != VK_SUCCESS)
            throw vulkan::exception(fmt::format("failed to wait current frame fence: {0:#x}", result));
#else
        vkQueueWaitIdle(device.presentation_queue.handle());
#endif

        /*VkAcquireNextImageInfoKHR next_image_info{
            VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
            nullptr,
            swapchain.handle(),
            std::numeric_limits<std::uint64_t>::max(),
            app.image_available_semaphore->handle(),
            VK_NULL_HANDLE
        };*/

        std::uint32_t image_index;

        switch (auto result = vkAcquireNextImageKHR(device.handle(), swapchain.handle(), std::numeric_limits<std::uint64_t>::max(),
                                                    image_available_semaphore->handle(), VK_NULL_HANDLE, &image_index); result) {
            case VK_ERROR_OUT_OF_DATE_KHR:
                recreate_swap_chain_callback();
                return;

            case VK_SUBOPTIMAL_KHR:
            case VK_SUCCESS:
                break;

            default:
                throw vulkan::exception(fmt::format("failed to acquire next image index: {0:#x}", result));
        }

#if USE_FENCES
        if (auto &&fence = busy_frames_fences_.at(image_index); fence && fence->handle() != VK_NULL_HANDLE)
            if (auto result = vkWaitForFences(device.handle(), 1, fence->handle_ptr(), VK_TRUE, std::numeric_limits<std::uint64_t>::max()); result != VK_SUCCESS)
                throw vulkan::exception(fmt::format("failed to wait busy frame fence: {0:#x}", result));

        busy_frames_fences_.at(image_index) = frame_fence;
#endif

        auto const wait_semaphores = std::array{ image_available_semaphore->handle() };
        auto const signal_semaphores = std::array{ render_finished_semaphore->handle() };

        auto const wait_stages = std::array{
            VkPipelineStageFlags{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }
        };

        VkSubmitInfo const submit_info{
            VK_STRUCTURE_TYPE_SUBMIT_INFO,
            nullptr,
            static_cast<std::uint32_t>(std::size(wait_semaphores)), std::data(wait_semaphores),
            std::data(wait_stages),
            1, &command_buffers[image_index],
            static_cast<std::uint32_t>(std::size(signal_semaphores)), std::data(signal_semaphores),
        };

#if USE_FENCES
        if (auto result = vkResetFences(device.handle(), 1, frame_fence->handle_ptr()); result != VK_SUCCESS)
            throw vulkan::exception(fmt::format("failed to reset previous frame fence: {0:#x}", result));

        if (auto result = vkQueueSubmit(device.graphics_queue.handle(), 1, &submit_info, frame_fence->handle()); result != VK_SUCCESS)
            throw vulkan::exception(fmt::format("failed to submit draw command buffer: {0:#x}", result));
#else
        if (auto result = vkQueueSubmit(device.graphics_queue.handle(), 1, &submit_info, VK_NULL_HANDLE); result != VK_SUCCESS)
        throw vulkan::exception(fmt::format("failed to submit draw command buffer: {0:#x}", result));
#endif

        auto swapchain_handle = swapchain.handle();

        VkPresentInfoKHR const present_info{
                VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                nullptr,
                static_cast<std::uint32_t>(std::size(signal_semaphores)), std::data(signal_semaphores),
                1, &swapchain_handle,
                &image_index, nullptr
        };

        switch (auto result = vkQueuePresentKHR(device.presentation_queue.handle(), &present_info); result) {
            case VK_ERROR_OUT_OF_DATE_KHR:
            case VK_SUBOPTIMAL_KHR:
                recreate_swap_chain_callback();
                return;

            case VK_SUCCESS:
                break;

            default:
                throw vulkan::exception(fmt::format("failed to submit request to present framebuffer: {0:#x}", result));
        }

#if USE_FENCES
        current_frame_index_ = (current_frame_index_ + 1) % render::kCONCURRENTLY_PROCESSED_FRAMES;
#endif
    }

    void renderer::fill_draw_command_buffers(std::span<VkCommandBuffer> command_buffers, render::draw_commands_holder &draw_commands_holder, app_t const &app)
    {
#if defined(__clang__)/* || defined(_MSC_VER)*/
        auto const clear_colors = std::array{
            VkClearValue{{{ .64f, .64f, .64f, 1.f }}},
            VkClearValue{{{ renderer_config_.reversed_depth ? 0.f : 1.f, 0 }}}
        };
#else
        auto const clear_colors = std::array{
            VkClearValue{ .color = { .float32 = { .64f, .64f, .64f, 1.f }}},
            VkClearValue{ .depthStencil = { renderer_config_.reversed_depth ? 0.f : 1.f, 0 }}
        };
#endif

        auto non_indexed = draw_commands_holder.get_primitives_buffers_bind_ranges();
        auto indexed = draw_commands_holder.get_indexed_primitives_buffers_bind_range();

        for (auto i = 0u; auto &command_buffer : command_buffers) {
            VkCommandBufferBeginInfo const begin_info{
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                nullptr,
                0,
                nullptr
            };

            if (auto result = vkBeginCommandBuffer(command_buffer, &begin_info); result != VK_SUCCESS)
                throw vulkan::exception(fmt::format("failed to record command buffer: {0:#x}", result));

            // :TODO: get render area from render pass abstraction
            auto [width, height] = app.swapchain->extent();

            // :TODO: use render pass abstraction as the method parameter
            VkRenderPassBeginInfo const render_pass_info{
                VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                nullptr,
                app.render_pass->handle(),
                app.framebuffers.at(i++)->handle(),
                {{0, 0}, VkExtent2D{width, height}},
                static_cast<std::uint32_t>(std::size(clear_colors)), std::data(clear_colors)
            };

            vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

#if USE_DYNAMIC_PIPELINE_STATE
            VkViewport const viewport{
                0, static_cast<float>(height),
                static_cast<float>(width), -static_cast<float>(height),
                0, 1
            };

            VkRect2D const scissor{
                {0, 0}, VkExtent2D{width, height}
            };

            vkCmdSetViewport(command_buffer, 0, 1, &viewport);
            vkCmdSetScissor(command_buffer, 0, 1, &scissor);
#endif

            const auto min_offset_alignment = static_cast<std::size_t>(device_->device_limits().min_storage_buffer_offset_alignment);
            auto aligned_offset = boost::alignment::align_up(sizeof(per_object_t), min_offset_alignment);

            for (auto &&range : indexed) {
                vkCmdBindIndexBuffer(command_buffer, range.index_buffer_handle, range.index_buffer_offset, convert_to::vulkan(range.index_type));

                for (auto &&subrange : range.vertex_buffers_bind_ranges) {
                    vkCmdBindVertexBuffers(command_buffer, subrange.first_binding, static_cast<std::uint32_t>(std::size(subrange.buffer_handles)),
                                           std::data(subrange.buffer_handles), std::data(subrange.buffer_offsets));

                    std::visit([&] (auto span)
                   {
                       if constexpr (std::is_same_v<typename decltype(span)::value_type, render::indexed_draw_command>) {
                           for (auto &&dc : span) {
                               vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, dc.pipeline->handle());

                               std::array<VkDescriptorSet, 3> descriptor_sets{
                                   app.view_resources_descriptor_set, dc.descriptor_set, app.image_resources_descriptor_set
                               };

                               std::array<std::uint32_t, 1> dynamic_offsets{
                                   dc.transform_index * static_cast<std::uint32_t>(aligned_offset)
                               };

                               vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, dc.pipeline_layout,
                                                       0,
                                                       static_cast<std::uint32_t>(std::size(descriptor_sets)), std::data(descriptor_sets),
                                                       static_cast<std::uint32_t>(std::size(dynamic_offsets)), std::data(dynamic_offsets));

                               vkCmdDrawIndexed(command_buffer, dc.index_count, 1, dc.first_index, static_cast<std::int32_t>(dc.first_vertex), 0);
                           }
                       }
                   }, subrange.draw_commands);
                }
            }

            for (auto &&range : non_indexed) {
                vkCmdBindVertexBuffers(command_buffer, range.first_binding, static_cast<std::uint32_t>(std::size(range.buffer_handles)),
                                       std::data(range.buffer_handles), std::data(range.buffer_offsets));

                std::visit([&] (auto span)
               {
                   for (auto &&dc : span) {
                       vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, dc.pipeline->handle());

                       std::array<VkDescriptorSet, 3> descriptor_sets{
                           app.view_resources_descriptor_set, dc.descriptor_set, app.image_resources_descriptor_set
                       };

                       std::array<std::uint32_t, 1> dynamic_offsets{
                           dc.transform_index *static_cast<std::uint32_t>(aligned_offset)
                       };

                       vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, dc.pipeline_layout,
                                               0,
                                               static_cast<std::uint32_t>(std::size(descriptor_sets)), std::data(descriptor_sets),
                                               static_cast<std::uint32_t>(std::size(dynamic_offsets)), std::data(dynamic_offsets));

                       vkCmdDraw(command_buffer, dc.vertex_count, 1, dc.first_vertex, 0);
                   }
               }, range.draw_commands);
            }

            vkCmdEndRenderPass(command_buffer);

            if (auto result = vkEndCommandBuffer(command_buffer); result != VK_SUCCESS)
                throw vulkan::exception(fmt::format("failed to end command buffer: {0:#x}", result));
        }
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

namespace render
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

    struct vertex_buffers_bind_range final {
        std::uint32_t first_binding;
        std::vector<std::uint32_t> buffer_handles;
        std::vector<std::uint32_t> buffer_offsets;
        std::span<render::nonindexed_draw_command> draw_commands;
    };

    struct indexed_primitives_buffers_bind_range final {
        INDEX_TYPE index_type;
        std::uint32_t index_buffer_handle;
        std::uint32_t index_buffer_offset;
        std::span<render::nonindexed_draw_command> draw_commands;
    };

    template<class T>
    struct comparator final {
        template<class L, class R>
        constexpr bool operator() (L &&lhs, R &&rhs) const
        {
            if constexpr (std::is_same_v<T, render::indexed_draw_command>) {
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

namespace render
{
    std::ostream &operator<<(std::ostream &out, render::nonindexed_draw_command const &c)
    {
        return out << "h" << c.vertex_buffer->handle << " l" << c.vertex_input_binding_index;// << " v" << c.first_vertex;
    }
    std::ostream &operator<<(std::ostream &out, render::indexed_draw_command const &c)
    {
        return out << "h" << c.index_buffer->handle << "idx" << c.index_buffer->index_type << " h" << c.vertex_buffer->handle << "L" << c.vertex_input_binding_index;// << " v" << c.first_vertex;
    }
    std::ostream &operator<<(std::ostream &out, render::vertex_buffers_bind_range const &r)
    {
        std::ranges::copy(r.buffer_handles, std::ostream_iterator<int>(out << "fbnd " << r.first_binding << " [", " "));
        return out << "]";
    }
    // std::ostream &operator<<(std::ostream &out, render::indexed_primitives_buffers_bind_range const &r)
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

std::vector<render::vertex_buffers_bind_range> get_primitives_buffers_bind_ranges(std::span<render::nonindexed_draw_command> nonindexed_draw_commands_)
{
    std::stable_sort(std::begin(nonindexed_draw_commands_), std::end(nonindexed_draw_commands_), render::comparator<render::nonindexed_draw_command>{});

    std::ranges::copy(nonindexed_draw_commands_, std::ostream_iterator<render::nonindexed_draw_command>(std::cout, "|"));
    std::cout << std::endl;

    std::vector<render::vertex_buffers_bind_range> buffers_bind_range;

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

        std::copy(it_begin, it_end, std::ostream_iterator<render::nonindexed_draw_command>(std::cout, "|"));
        std::cout << std::endl;

        std::ranges::copy(buffer_handles, std::ostream_iterator<std::uint32_t>(std::cout, " "));
        std::cout << std::endl;

        buffers_bind_range.push_back(render::vertex_buffers_bind_range{
            it_begin->vertex_input_binding_index,
            buffer_handles,
            std::vector<std::uint32_t>(std::size(buffer_handles), 0u),
            std::span{it_begin, it_end}
                                     });

        it_begin = it_end;
    }

    return buffers_bind_range;
}

std::vector<render::indexed_primitives_buffers_bind_range> get_indexed_primitives_buffers_bind_range(std::span<render::indexed_draw_command> draw_commands_)
{
    std::stable_sort(std::begin(draw_commands_), std::end(draw_commands_), render::comparator<render::indexed_draw_command>{});

    std::ranges::copy(draw_commands_, std::ostream_iterator<render::indexed_draw_command>(std::cout, "|"));
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

        // auto buffers_bind_range = get_primitives_buffers_bind_ranges(dcs);

        // std::ranges::copy(dcs, std::ostream_iterator<render::indexed_draw_command>(std::cout, "|"));
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

            std::copy(it_begin2, it_end2, std::ostream_iterator<render::indexed_draw_command>(std::cout, "|"));
            std::cout << std::endl;

            // std::ranges::copy(buffer_handles, std::ostream_iterator<std::uint32_t>(std::cout, " "));
            // std::cout << std::endl;

            // buffers_bind_range.push_back(render::vertex_buffers_bind_range{
            //     it_begin2->vertex_input_binding_index,
            //     buffer_handles,
            //     std::vector<std::uint32_t>(std::size(buffer_handles), 0u),
            //     std::span{it_begin2, it_end2}
            // });

            it_begin2 = it_end2;
        }

        // std::ranges::copy(dcs, std::ostream_iterator<render::indexed_draw_command>(std::cout, "|"));
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

    // std::vector<render::nonindexed_draw_command> commands{
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

    // auto buffers_bind_range = get_primitives_buffers_bind_ranges(commands);

    // std::ranges::copy(commands, std::ostream_iterator<render::nonindexed_draw_command>(std::cout, "|"));
    // std::cout << std::endl;

    // std::ranges::copy(buffers_bind_range, std::ostream_iterator<render::vertex_buffers_bind_range>(std::cout, "|"));
    // std::cout << std::endl;

    std::vector<render::indexed_draw_command> idxcommands{
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

    // std::ranges::copy(commands, std::ostream_iterator<render::nonindexed_draw_command>(std::cout, "|"));
    // std::cout << std::endl;
}
#endif

std::pair<std::span<draw_command>, std::span<draw_command>> separate_indexed_and_nonindexed(std::span<draw_command> draw_commands)
{
    auto it = std::stable_partition(std::begin(draw_commands), std::end(draw_commands), [] (auto &&draw_command)
    {
        return draw_command.index_buffer != nullptr;
    });

    return std::pair{std::span{std::begin(draw_commands), it}, std::span{it, std::end(draw_commands)}};
}

std::vector<vertex_buffers_bind_ranges>
separate_nonindexed_by_binds(graphics::vertex_input_state_manager &vertex_input_state_manager, std::span<draw_command> draw_commands)
{
    std::stable_sort(std::begin(draw_commands), std::end(draw_commands), [&] (auto &&lhs, auto &&rhs)
    {
        auto lhs_binding_index = vertex_input_state_manager.binding_index(lhs.vertex_buffer->vertex_layout());
        auto rhs_binding_index = vertex_input_state_manager.binding_index(rhs.vertex_buffer->vertex_layout());

        if (lhs_binding_index == rhs_binding_index)
            return lhs.vertex_buffer->device_buffer()->handle() < rhs.vertex_buffer->device_buffer()->handle();

        return lhs_binding_index < rhs_binding_index;
    });

    std::vector<draw_command> rc(std::begin(draw_commands), std::end(draw_commands));
    std::span<draw_command> newrc{rc};
    std::vector<std::span<draw_command>> subranges;

    auto it_out = std::begin(draw_commands);

    for (auto it_begin = std::begin(newrc); it_begin != std::end(newrc); it_begin = std::begin(newrc)) {
        auto it_temp = it_out;

        it_out = std::unique_copy(it_begin, std::end(newrc), it_out, [&] (auto &&lhs, auto &&rhs)
        {
            auto lhs_binding_index = vertex_input_state_manager.binding_index(lhs.vertex_buffer->vertex_layout());
            auto rhs_binding_index = vertex_input_state_manager.binding_index(rhs.vertex_buffer->vertex_layout());

            if (lhs_binding_index == rhs_binding_index)
                return lhs.vertex_buffer->device_buffer()->handle() != rhs.vertex_buffer->device_buffer()->handle();

            return false;
        });

        auto subrange = std::span{it_temp, it_out};

        auto it_end = std::set_difference(std::begin(newrc), std::end(newrc),
                                          std::begin(subrange), std::end(subrange),
                                          std::begin(newrc), [&] (auto &&lhs, auto &&rhs)
        {
            auto lhs_binding_index = vertex_input_state_manager.binding_index(lhs.vertex_buffer->vertex_layout());
            auto rhs_binding_index = vertex_input_state_manager.binding_index(rhs.vertex_buffer->vertex_layout());

            return lhs_binding_index < rhs_binding_index;
        });

        subranges.push_back(subrange);

        newrc = std::span{std::begin(newrc), it_end};
    }

    std::vector<vertex_buffers_bind_ranges> bind_ranges;

    for (auto subrange : subranges) {
        for (auto it_begin = std::begin(subrange); it_begin != std::end(subrange);) {
            auto it = std::adjacent_find(it_begin, std::end(subrange), [&] (auto &&lhs, auto &&rhs)
            {
                auto lhs_binding_index = vertex_input_state_manager.binding_index(lhs.vertex_buffer->vertex_layout());
                auto rhs_binding_index = vertex_input_state_manager.binding_index(rhs.vertex_buffer->vertex_layout());

                return (rhs_binding_index - lhs_binding_index) > 1;
            });

            if (it != std::end(subrange))
                it = std::next(it);

            std::span<draw_command> consecutive_binds{it_begin, it};

            std::vector<std::pair<std::uint32_t, VkBuffer>> layout_index_and_buffer_pairs;

            std::transform(it_begin, it, std::back_inserter(layout_index_and_buffer_pairs), [&] (auto &&lhs)
            {
                auto binding_index = vertex_input_state_manager.binding_index(lhs.vertex_buffer->vertex_layout());

                return std::pair{binding_index, lhs.vertex_buffer->device_buffer()->handle()};
            });

            auto itx = std::unique(std::begin(layout_index_and_buffer_pairs), std::end(layout_index_and_buffer_pairs), [] (auto lhs, auto rhs)
            {
                return lhs == rhs;
            });

            /*auto handles = consecutive_binds | std::views::transform([] (auto &&lhs)
            {
                return lhs.vertex_buffer->device_buffer()->handle();
            });*/

            std::vector<VkBuffer> vertex_buffer_handles;

            std::transform(std::begin(layout_index_and_buffer_pairs), itx, std::back_inserter(vertex_buffer_handles), [] (auto &&lhs)
            {
                return lhs.second;
            });

            //std::unique_copy(std::begin(handles), std::end(handles), std::back_inserter(vertex_buffer_handles));

            auto first_binding_index = vertex_input_state_manager.binding_index(consecutive_binds.front().vertex_buffer->vertex_layout());
            auto last_binding_index = vertex_input_state_manager.binding_index(consecutive_binds.back().vertex_buffer->vertex_layout());

            bind_ranges.push_back(vertex_buffers_bind_ranges{
                first_binding_index,
                last_binding_index - first_binding_index + 1,
                vertex_buffer_handles,
                std::vector<VkDeviceSize>(std::size(vertex_buffer_handles), 0),
                consecutive_binds
            });

            it_begin = it;
        }
    }

    return bind_ranges;
}

std::vector<indexed_primitives_draw> separate_indexed_by_binds(std::span<draw_command> draw_commands)
{
    std::stable_sort(std::begin(draw_commands), std::end(draw_commands), [] (auto &&lhs, auto &&rhs)
    {
        if (lhs.index_buffer->index_type() == rhs.index_buffer->index_type())
            return lhs.index_buffer->device_buffer()->handle() < rhs.index_buffer->device_buffer()->handle();

        return lhs.index_buffer->index_type() < rhs.index_buffer->index_type();
    });

#if 0
    std::vector<draw_command> draw_commands_copy(std::begin(draw_commands), std::end(draw_commands));
    std::span<draw_command> copy_range{draw_commands_copy};

    std::vector<std::span<draw_command>> unique_subranges;

    auto it_out = std::begin(draw_commands);

    for (auto it_begin = std::begin(copy_range); it_begin != std::end(copy_range); it_begin = std::begin(copy_range)) {
        auto it = it_out;

        it_out = std::unique_copy(it_begin, std::begin(copy_range), it_out, [] (auto &&lhs, auto &&rhs)
        {
            if (lhs.index_buffer->index_type() == rhs.index_buffer->index_type())
                return lhs.index_buffer->device_buffer()->handle() != rhs.index_buffer->device_buffer()->handle();

            return false;
        });

        auto subrange = std::span{it, it_out};

        auto it_end = std::set_difference(std::begin(copy_range), std::end(copy_range),
                                          std::begin(subrange), std::end(subrange),
                                          std::begin(copy_range), [] (auto &&lhs, auto &&rhs)
        {
            return lhs.index_buffer->index_type() < rhs.index_buffer->index_type();
        });

        subranges.push_back(subrange);

        copy_range = std::span{std::begin(copy_range), it_end};
    }

    std::vector<indexed_primitives_draw> bind_ranges;

    for (auto subrange : subranges) {
        for (auto it_begin = std::begin(subrange); it_begin != std::end(subrange);) {
            auto it = std::adjacent_find(it_begin, std::end(subrange), [&] (auto &&lhs, auto &&rhs)
            {
                return (rhs.index_buffer->index_type() - lhs.index_buffer->index_type()) > 1;
            });

            if (it != std::end(subrange))
                it = std::next(it);

            std::span<draw_command> consecutive_binds{it_begin, it};

            auto handles = consecutive_binds | std::views::transform([] (auto &&lhs)
            {
                return lhs.index_buffer->device_buffer()->handle();
            });

            std::vector<VkBuffer> index_buffer_handles;

            std::unique_copy(std::begin(handles), std::end(handles), std::back_inserter(index_buffer_handles), [] (auto lhs, auto rhs)
            {
                return lhs == rhs;
            });

            bind_ranges.push_back(indexed_primitives_draw{
                first_binding_index,
                last_binding_index - first_binding_index + 1,
                vertex_buffer_handles,
                std::vector<VkDeviceSize>(std::size(vertex_buffer_handles), 0),
                consecutive_binds
            });

            /*auto first_binding_index = vertex_input_state_manager.binding_index(consecutive_binds.front().vertex_buffer->vertex_layout());
            auto last_binding_index = vertex_input_state_manager.binding_index(consecutive_binds.back().vertex_buffer->vertex_layout());

            bind_ranges.push_back(indexed_primitives_draw{
                first_binding_index,
                last_binding_index - first_binding_index + 1,
                vertex_buffer_handles,
                std::vector<VkDeviceSize>(std::size(vertex_buffer_handles), 0),
                consecutive_binds
            });*/

            it_begin = it;
        }
    }
#endif

    /*auto it = std::adjacent_find(std::begin(draw_commands), std::end(draw_commands), [] (auto &&lhs, auto &&rhs)
    {
        return lhs.index_buffer->index_type() != rhs.index_buffer->index_type();
    });

    if (it != std::end(subrange))
        it = std::next(it);*/

    return {};
}
