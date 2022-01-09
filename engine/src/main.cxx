/*#ifdef _MSC_VER
    #define _SCL_SECURE_NO_WARNINGS
#endif*/


#if defined(_DEBUG) || defined(DEBUG)
    #if defined(_MSC_VER)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-macro-identifier"
        #define _CRTDBG_MAP_ALLOC
#pragma clang diagnostic pop
        #include <crtdbg.h>
    #else
        #include <thread>
        #include <csignal>
        #include <execinfo.h>

        void posix_signal_handler(int signum)
        {
            using namespace std::string_literals;

            auto current_thread = std::this_thread::get_id();

            auto name = "unknown"s;

            switch (signum) {
                case SIGABRT: name = "SIGABRT"s;  break;
                case SIGSEGV: name = "SIGSEGV"s;  break;
                case SIGBUS:  name = "SIGBUS"s;   break;
                case SIGILL:  name = "SIGILL"s;   break;
                case SIGFPE:  name = "SIGFPE"s;   break;
                case SIGTRAP: name = "SIGTRAP"s;  break;
            }

            std::array<void *, 32> callStack;

            auto size = backtrace(std::data(callStack), std::size(callStack));

            fmt::print(stderr, "Error: signal {}\n", name);

            auto symbollist = backtrace_symbols(std::data(callStack), size);

            for (auto i = 0; i < size; ++i)
                fmt::print(stderr, "{} {} {}\n", i, current_thread, symbollist[i]);

            free(symbollist);

            exit(1);
        }
    #endif
#endif

#include <chrono>
#include <cmath>
#include <ranges>
#include <span>
#include <unordered_map>

#ifdef _MSC_VER
    #include <execution>
#endif
#include <random>
#include <ranges>
#include <functional>

#include <string>
using namespace std::string_literals;

#include <string_view>
using namespace std::string_view_literals;

#include <fmt/format.h>

#include <boost/align.hpp>
#include <boost/align/align.hpp>

#include "utility/mpl.hxx"
#include "utility/helpers.hxx"
#include "utility/exceptions.hxx"

#include "math/math.hxx"
#include "math/pack-unpack.hxx"

#include "vulkan/instance.hxx"
#include "vulkan/device.hxx"

#include "renderer/config.hxx"
#include "renderer/renderer.hxx"
#include "renderer/swapchain.hxx"
#include "renderer/command_buffer.hxx"

#include "resources/buffer.hxx"
#include "resources/image.hxx"
#include "resources/resource_manager.hxx"
#include "resources/memory_manager.hxx"
#include "resources/sync_objects.hxx"
#include "resources/framebuffer.hxx"

#include "descriptor.hxx"

#include "loaders/TARGA_loader.hxx"
#include "loaders/image_loader.hxx"
#include "loaders/scene_loader.hxx"

#include "graphics/graphics.hxx"
#include "graphics/graphics_pipeline.hxx"
#include "graphics/pipeline_states.hxx"

#include "renderer/material.hxx"

#include "graphics/vertex.hxx"
#include "graphics/render_pass.hxx"
#include "renderer/render_flow.hxx"

#include "graphics/compatibility.hxx"

#include "platform/input/input_manager.hxx"
#include "camera/camera.hxx"
#include "camera/camera_controller.hxx"

#include "primitives/primitives.hxx"

#include "main.hxx"
#include "app.hxx"




struct window_events_handler final : public platform::window::event_handler_interface {

    explicit window_events_handler(app_t &app) : app{app} { }

    app_t &app;

    void on_resize(std::int32_t const width, std::int32_t const height) override
    {
        if (app.width == static_cast<std::uint32_t>(width) && app.height == static_cast<std::uint32_t>(height))
            return;

        app.width = static_cast<std::uint32_t>(width);
        app.height = static_cast<std::uint32_t>(height);

        app.per_viewport_data.rect = glm::ivec4{0, 0, width, height};

        if (width < 1 || height < 1)
            return;

        app.resize_callback = [this]
        {
            recreate_swap_chain(app);

            update_viewport_descriptor_buffer(app);

            app.camera_->aspect = static_cast<float>(app.width) / static_cast<float>(app.height);
        };
    }
};


void update_descriptor_set(app_t &app, vulkan::device const &device)
{
    // TODO: descriptor info typed by VkDescriptorType.
    auto const per_camera = std::array{
        VkDescriptorBufferInfo{app.per_camera_buffer->handle(), 0, sizeof(camera::data_t)}
    };

    // TODO: descriptor info typed by VkDescriptorType.
    auto const per_object = std::array{
        VkDescriptorBufferInfo{app.per_object_buffer->handle(), 0, sizeof(per_object_t)}
    };

    // TODO: descriptor info typed by VkDescriptorType.
    auto const per_viewport = std::array{
        VkDescriptorBufferInfo{app.per_viewport_buffer->handle(), 0, sizeof(per_viewport_t)}
    };

//#if TEMPORARILY_DISABLED
    // TODO: descriptor info typed by VkDescriptorType.
    auto const per_image = std::array{
        VkDescriptorImageInfo{app.texture->sampler->handle(), app.texture->view->handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
    };
//#endif

    std::array<VkWriteDescriptorSet, 4> const write_descriptor_sets{{
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            app.view_resources_descriptor_set,
            0,
            0, static_cast<std::uint32_t>(std::size(per_camera)),
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            nullptr,
            std::data(per_camera),
            nullptr
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            app.object_resources_descriptor_set,
            0, // it is 1 when there was only one descriptor set
            0, static_cast<std::uint32_t>(std::size(per_object)),
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
            nullptr,
            std::data(per_object),
            nullptr
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            app.view_resources_descriptor_set,
            1,
            0, static_cast<std::uint32_t>(std::size(per_viewport)),
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            nullptr,
            std::data(per_viewport),
            nullptr
        },
//#if TEMPORARILY_DISABLED
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            app.image_resources_descriptor_set,
            0,
            0, static_cast<std::uint32_t>(std::size(per_image)),
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            std::data(per_image),
            nullptr,
            nullptr
        }
//#endif
    }};

    // :WARN: remember about potential race condition with the related executing command buffer.
    vkUpdateDescriptorSets(device.handle(), static_cast<std::uint32_t>(std::size(write_descriptor_sets)),
                           std::data(write_descriptor_sets), 0, nullptr);
}

void create_graphics_command_buffers(app_t &app)
{
    app.command_buffers.resize(std::size(app.swapchain->image_views()));

    VkCommandBufferAllocateInfo const allocate_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        app.graphics_command_pool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        static_cast<std::uint32_t>(std::size(app.command_buffers))
    };

    if (auto result = vkAllocateCommandBuffers(app.device->handle(), &allocate_info, std::data(app.command_buffers)); result != VkResult::VK_SUCCESS)
        throw vulkan::exception{fmt::format("failed to create allocate command buffers: {0:#x}", result)};

    /*auto const clear_colors = std::array{
        VkClearValue{ .color = { .float32 = { .64f, .64f, .64f, 1.f } } },
        VkClearValue{ .depthStencil = { app.renderer_config.reversed_depth ? 0.f : 1.f, 0 } }
    };*/
#if defined(__clang__)/* || defined(_MSC_VER)*/
    auto const clear_colors = std::array{
        VkClearValue{{{ .64f, .64f, .64f, 1.f }}},
        VkClearValue{{{ app.renderer_config.reversed_depth ? 0.f : 1.f, 0 }}}
    };
#else
    auto const clear_colors = std::array{
        VkClearValue{ .color = { .float32 = { .64f, .64f, .64f, 1.f } } },
        VkClearValue{ .depthStencil = { app.renderer_config.reversed_depth ? 0.f : 1.f, 0 } }
    };
#endif

    auto non_indexed = app.draw_commands_holder.get_primitives_buffers_bind_ranges();
    auto indexed = app.draw_commands_holder.get_indexed_primitives_buffers_bind_range();

    for (std::size_t i = 0; auto &command_buffer : app.command_buffers) {
        VkCommandBufferBeginInfo const begin_info{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            nullptr,
            0,
            nullptr
        };

        if (auto result = vkBeginCommandBuffer(command_buffer, &begin_info); result != VK_SUCCESS)
            throw vulkan::exception(fmt::format("failed to record command buffer: {0:#x}", result));

        auto [width, height] = app.swapchain->extent();

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

        const auto min_offset_alignment = static_cast<std::size_t>(app.device->device_limits().min_storage_buffer_offset_alignment);
        auto aligned_offset = boost::alignment::align_up(sizeof(per_object_t), min_offset_alignment);

        for (auto &&range : indexed) {
            vkCmdBindIndexBuffer(command_buffer, range.index_buffer_handle, range.index_buffer_offset, convert_to::vulkan(range.index_type));

            for (auto &&subrange : range.vertex_buffers_bind_ranges) {
                vkCmdBindVertexBuffers(command_buffer, subrange.first_binding, static_cast<std::uint32_t>(std::size(subrange.buffer_handles)),
                                       std::data(subrange.buffer_handles), std::data(subrange.buffer_offsets));

                std::visit([&] (auto span)
                {
                    if constexpr (std::is_same_v<typename decltype(span)::value_type, renderer::indexed_draw_command>) {
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

void create_frame_data(app_t &app)
{
    auto &&device = *app.device;
    auto &&platform_surface = app.platform_surface;
    auto &&resource_manager = *app.resource_manager;
    
    auto swapchain = create_swapchain(device, platform_surface, renderer::extent{app.width, app.height});

    if (swapchain == nullptr)
        throw graphics::exception("failed to create the swapchain"s);

    const auto attachment_descriptions = create_attachment_descriptions(device, app.renderer_config, *swapchain);

    if (attachment_descriptions.empty())
        throw graphics::exception("failed to create the attachment descriptions"s);

    auto attachments = create_attachments(resource_manager, attachment_descriptions, swapchain->extent());

    if (attachments.empty())
        throw graphics::exception("failed to create the attachments"s);

    auto render_pass = create_render_pass(*app.render_pass_manager, swapchain->surface_format(), attachment_descriptions);

    if (render_pass == nullptr)
        throw graphics::exception("failed to create the render pass"s);

    auto framebuffers = create_framebuffers(resource_manager, *swapchain, render_pass, attachments);

    if (framebuffers.empty())
        throw graphics::exception("failed to create the framebuffers"s);

    app.swapchain = std::move(swapchain);
    app.attachments = std::move(attachments);
    app.render_pass = std::move(render_pass);
    app.framebuffers = std::move(framebuffers);
}

void cleanup_frame_data(app_t &app)
{
    auto &&device = *app.device;

    if (app.graphics_command_pool)
        vkFreeCommandBuffers(device.handle(), app.graphics_command_pool, static_cast<std::uint32_t>(std::size(app.command_buffers)), std::data(app.command_buffers));

    app.command_buffers.clear();

    app.framebuffers.clear();
    app.attachments.clear();

    app.swapchain.reset();
}

void recreate_swap_chain(app_t &app)
{
    if (app.width < 1 || app.height < 1)
        return;

    vkDeviceWaitIdle(app.device->handle());

    cleanup_frame_data(app);
    create_frame_data(app);

#if !USE_DYNAMIC_PIPELINE_STATE
    CreateGraphicsPipelines(app);
#endif

    create_graphics_command_buffers(app);
}

void build_render_pipelines(app_t &app, xformat const &model_)
{
    std::vector<graphics::render_graph> render_pipelines;

    graphics::rasterization_state rasterization_state{
        graphics::CULL_MODE::BACK,
        graphics::POLYGON_FRONT_FACE::COUNTER_CLOCKWISE,
        graphics::POLYGON_MODE::FILL,
        1.f
    };

    graphics::depth_stencil_state depth_stencil_state{
        true, true, graphics::COMPARE_OPERATION::GREATER,
        false, { 0.f, 1.f },
        false, graphics::stencil_state{ }, graphics::stencil_state{ }
    };

    graphics::color_blend_state color_blend_state{
        false, graphics::LOGIC_OPERATION::COPY,
        { 0.f, 0.f, 0.f, 0.f },
        { graphics::color_blend_attachment_state{ } }
    };

    auto &&material_factory = *app.material_factory;
    auto &&vertex_input_state_manager = *app.vertex_input_state_manager;
    auto &&pipeline_factory = *app.pipeline_factory;

    for (auto &&scene_node : model_.scene_nodes) {
        auto [transform_index, mesh_index] = scene_node;

        //[[maybe_unused]] auto &&transform = model_.transforms.at(transform_index);

        auto &&mesh = model_.meshes.at(mesh_index);
        auto &&[meshlets] = mesh;

        for (auto meshlet_index : meshlets) {
            auto &&meshlet = model_.meshlets.at(meshlet_index);

            auto primitive_topology = meshlet.topology;

            auto material_index = meshlet.material_index;
            auto [technique_index, name] = model_.materials[material_index];

            std::shared_ptr<resource::vertex_buffer> vertex_buffer = meshlet.vertex_buffer;
            std::shared_ptr<resource::index_buffer> index_buffer = meshlet.index_buffer;

            auto &&vertex_layout = vertex_buffer->vertex_layout();
            auto vertex_layout_name = graphics::to_string(vertex_layout);

            fmt::print("{}.{}.{}\n", name, technique_index, vertex_layout_name);

            auto material = material_factory.material(name, technique_index, vertex_layout, primitive_topology);

            if (material == nullptr)
                throw graphics::exception("failed to create material"s);

            [[maybe_unused]] auto &&vertex_input_state = vertex_input_state_manager.vertex_input_state(vertex_layout);

            auto adjusted_vertex_input_state = vertex_input_state_manager.get_adjusted_vertex_input_state(vertex_layout, material->vertex_layout);

            graphics::pipeline_states pipeline_states{
                primitive_topology,
                adjusted_vertex_input_state,
                rasterization_state,
                depth_stencil_state,
                color_blend_state
            };

            auto pipeline = pipeline_factory.create_pipeline(material, pipeline_states, app.pipeline_layout, app.render_pass, 0u);

            auto vertex_input_binding_index = app.vertex_input_state_manager->binding_index(vertex_buffer->vertex_layout());

            if (index_buffer) {
                app.draw_commands_holder.add_draw_command(
                    renderer::indexed_draw_command{
                        pipeline, material, app.pipeline_layout, app.object_resources_descriptor_set, app.render_pass,
                        vertex_buffer, index_buffer, vertex_input_binding_index,
                        meshlet.first_vertex, meshlet.vertex_count, meshlet.first_index, meshlet.index_count,
                        static_cast<std::uint32_t>(transform_index)
                    }
                );
            }

            else {
                app.draw_commands_holder.add_draw_command(
                    renderer::nonindexed_draw_command{
                        pipeline, material, app.pipeline_layout, app.object_resources_descriptor_set, app.render_pass,
                        vertex_buffer, vertex_input_binding_index, meshlet.first_vertex, meshlet.vertex_count,
                        static_cast<std::uint32_t>(transform_index)
                    }
                );
            }
        }
    }
}

void create_sync_objects(app_t &app)
{
    auto &&resource_manager = *app.resource_manager;

    std::ranges::generate(app.image_available_semaphores, [&resource_manager] ()
    {
        if (auto semaphore = resource_manager.create_semaphore(); semaphore)
            return semaphore;

        throw resource::exception("failed to create image semaphore"s);
    });

    std::ranges::generate(app.render_finished_semaphores, [&resource_manager] ()
    {
        if (auto semaphore = resource_manager.create_semaphore(); semaphore)
            return semaphore;

        throw resource::exception("failed to create render semaphore"s);
    });

    std::ranges::generate(app.concurrent_frames_fences, [&resource_manager] ()
    {
        if (auto fence = resource_manager.create_fence(true); fence)
            return fence;

        throw resource::exception("failed to create frame fence"s);
    });

    app.busy_frames_fences.resize(std::size(app.swapchain->image_views()), nullptr);
}

void update_viewport_descriptor_buffer(app_t const &app)
{
    auto &&device = *app.device;
    auto &&buffer = *app.per_viewport_buffer;

    auto const offset = buffer.memory()->offset();
    auto const size = buffer.memory()->size();

    void *data = nullptr;

    if (auto result = vkMapMemory(device.handle(), buffer.memory()->handle(), offset, size, 0, &data); result != VK_SUCCESS)
        throw vulkan::exception(fmt::format("failed to map per viewpor uniform buffer memory: {0:#x}", result));

    std::copy_n(&app.per_viewport_data, 1, static_cast<per_viewport_t *>(data));

    vkUnmapMemory(device.handle(), buffer.memory()->handle());
}

static void update(app_t &app)
{
    if (app.resize_callback) {
        app.resize_callback();
        app.resize_callback = nullptr;
    }

    app.camera_controller->update();
    app.cameraSystem.update();

    {
        auto &&device = *app.device;
        auto &&buffer = *app.per_camera_buffer;

        auto const offset = buffer.memory()->offset();
        auto const size = buffer.memory()->size();

        void *data = nullptr;

        if (auto result = vkMapMemory(device.handle(), buffer.memory()->handle(), offset, size, 0, &data); result != VK_SUCCESS)
            throw vulkan::exception(fmt::format("failed to map per camera uniform buffer memory: {0:#x}", result));

        std::copy_n(&app.camera_->data, 1, static_cast<camera::data_t *>(data));

        vkUnmapMemory(device.handle(), buffer.memory()->handle());
    }

    std::ranges::transform(app.xmodel.scene_nodes, std::begin(app.objects), [&xmodel = app.xmodel, &camera = app.camera_] (auto &&scene_node)
    {
        auto &&transform = xmodel.transforms.at(scene_node.transform_index);

        auto normal = glm::inverseTranspose(camera->data.view * transform);

        return per_object_t{transform, normal};
    });

    using objects_type = typename decltype(app.objects)::value_type;
    auto it_begin = static_cast<objects_type *>(app.ssbo_mapped_ptr);

    std::size_t const stride = app.aligned_buffer_size / std::size(app.objects);

#if defined(_MSC_VER) && !defined(__clang__)
    std::copy(std::execution::par, std::cbegin(app.objects), std::cend(app.objects), strided_bidirectional_iterator{it_begin, stride});
#elif defined(__clang__)
    std::copy(std::cbegin(app.objects), std::cend(app.objects), strided_bidirectional_iterator<objects_type>{it_begin, stride});
#else
    std::ranges::copy(app.objects, strided_bidirectional_iterator<objects_type>{it_begin, stride});
#endif

    auto const mapped_ranges = std::array{
        VkMappedMemoryRange{
            VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            nullptr,
            app.per_object_buffer->memory()->handle(),
            app.per_object_buffer->memory()->offset(),
            app.aligned_buffer_size
        }
    };

    vkFlushMappedMemoryRanges(app.device->handle(), static_cast<std::uint32_t>(std::size(mapped_ranges)), std::data(mapped_ranges));
}

static void render_frame(app_t &app)
{
    if (app.width < 1 || app.height < 1)
        return;

    auto &&device = *app.device;
    auto &&swapchain = *app.swapchain;

    auto &&image_available_semaphore = app.image_available_semaphores[app.current_frame_index];
    auto &&render_finished_semaphore = app.render_finished_semaphores[app.current_frame_index];

#define USE_FENCES 1

#if USE_FENCES
    auto &&frame_fence = app.concurrent_frames_fences[app.current_frame_index];

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
            recreate_swap_chain(app);
            return;

        case VK_SUBOPTIMAL_KHR:
        case VK_SUCCESS:
            break;

        default:
            throw vulkan::exception(fmt::format("failed to acquire next image index: {0:#x}", result));
    }

#if USE_FENCES
    if (auto &&fence = app.busy_frames_fences.at(image_index); fence && fence->handle() != VK_NULL_HANDLE)
        if (auto result = vkWaitForFences(device.handle(), 1, fence->handle_ptr(), VK_TRUE, std::numeric_limits<std::uint64_t>::max()); result != VK_SUCCESS)
            throw vulkan::exception(fmt::format("failed to wait busy frame fence: {0:#x}", result));

    app.busy_frames_fences.at(image_index) = frame_fence;
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
        1, &app.command_buffers.at(image_index),
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
            recreate_swap_chain(app);
            return;

        case VK_SUCCESS:
            break;

        default:
            throw vulkan::exception(fmt::format("failed to submit request to present framebuffer: {0:#x}", result));
    }

#if USE_FENCES
    app.current_frame_index = (app.current_frame_index + 1) % renderer::kCONCURRENTLY_PROCESSED_FRAMES;
#endif
}

int main()
{
#if defined(_MSC_VER)
    #if defined(_DEBUG) || defined(DEBUG)
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    #endif
#else
    static_assert(__cpp_concepts >= 201500); // check compiled with -fconcepts
    static_assert(__cplusplus >= 201703L);

    #if defined(_DEBUG) || defined(DEBUG)
        std::signal(SIGSEGV, posix_signal_handler);
        std::signal(SIGTRAP, posix_signal_handler);
    #endif
#endif

    if (auto result = glfwInit(); result != GLFW_TRUE)
        throw std::runtime_error(fmt::format("failed to init GLFW: {0:#x}", result));

    app_t app;

    platform::window window{"engine"sv, static_cast<std::int32_t>(app.width), static_cast<std::int32_t>(app.height)};

    const auto app_window_events_handler = std::make_shared<window_events_handler>(app);
    window.connect_event_handler(app_window_events_handler);

    const auto input_manager = std::make_shared<platform::input_manager>();
    window.connect_input_handler(input_manager);

    app.camera_ = app.cameraSystem.create_camera();
    app.camera_->aspect = static_cast<float>(app.width) / static_cast<float>(app.height);

    app.camera_controller = std::make_unique<orbit_controller>(app.camera_, *input_manager);
    app.camera_controller->look_at(glm::vec3{2, 2, 2}, {0, 0, 0});

    std::cout << measure<>::execution([&app, &window] ()
    {
        app.init(window);
    }) << " ms\n"s;

    window.update([&app]
    {
        glfwPollEvents();

        update(app);

        render_frame(app);
    });

    app.clean_up();

    glfwTerminate();
}
