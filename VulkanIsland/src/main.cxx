#define _SCL_SECURE_NO_WARNINGS


#if defined(_DEBUG) || defined(DEBUG)
    #if defined(_MSC_VER)
        #define _CRTDBG_MAP_ALLOC
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

            std::cerr << fmt::format("Error: signal {}\n"s, name);

            auto symbollist = backtrace_symbols(std::data(callStack), size);

            for (auto i = 0; i < size; ++i)
                std::cerr << fmt::format("{} {} {}\n"s, i, current_thread, symbollist[i]);

            free(symbollist);

            exit(1);
        }
    #endif
#endif

#include <chrono>
#include <cmath>
#include <span>
#include <unordered_map>

#ifdef _MSC_VER
    #include <execution>
#endif
#include <random>
#include <ranges>

#include <fmt/format.h>

#include <boost/align.hpp>
#include <boost/align/align.hpp>

#include "main.hxx"
#include "utility/mpl.hxx"
#include "utility/helpers.hxx"
#include "utility/exceptions.hxx"

#include "math/math.hxx"
#include "math/pack-unpack.hxx"

#include "vulkan/instance.hxx"
#include "vulkan/device.hxx"

#include "renderer/config.hxx"
#include "renderer/swapchain.hxx"
#include "renderer/command_buffer.hxx"

#include "resources/buffer.hxx"
#include "resources/image.hxx"
#include "resources/resource_manager.hxx"
#include "resources/memory_manager.hxx"
#include "resources/semaphore.hxx"
#include "resources/framebuffer.hxx"

#include "descriptor.hxx"

#include "loaders/TARGA_loader.hxx"
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


namespace temp
{
    xformat model;
}

struct per_object_t final {
    glm::mat4 world{1};
    glm::mat4 normal{1};  // Transposed and inversed upper left 3x3 sub-matrix of the model(world)-view matrix.
};


void cleanup_frame_data(struct app_t &app);
void create_semaphores(app_t &app);
void recreate_swap_chain(app_t &app);


template<class T>
[[nodiscard]] std::shared_ptr<resource::buffer>
stage_data(vulkan::device &device, resource::resource_manager &resource_manager, std::span<T> const container);

[[nodiscard]] std::shared_ptr<resource::texture>
load_texture(app_t &app, vulkan::device &device, resource::resource_manager &resource_manager, std::string_view name);


std::unique_ptr<renderer::swapchain>
create_swapchain(vulkan::device const &device, renderer::platform_surface const &platform_surface, renderer::extent extent);


struct draw_command final {
    std::shared_ptr<graphics::material> material;
    std::shared_ptr<graphics::pipeline> pipeline;

    std::shared_ptr<resource::vertex_buffer> vertex_buffer;
    std::shared_ptr<resource::index_buffer> index_buffer;

    std::uint32_t vertex_count{0};
    std::uint32_t first_vertex{0};

    std::uint32_t index_count{0};

    VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};

    std::shared_ptr<graphics::render_pass> render_pass;
    VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
};

struct app_t final {
    std::uint32_t width{800u};
    std::uint32_t height{600u};

    renderer::config renderer_config;

    std::unique_ptr<vulkan::instance> instance;
    std::unique_ptr<vulkan::device> device;

    renderer::platform_surface platform_surface;
    std::unique_ptr<renderer::swapchain> swapchain;

    std::unique_ptr<resource::resource_manager> resource_manager;
    std::unique_ptr<resource::memory_manager> memory_manager;

    std::unique_ptr<graphics::vertex_input_state_manager> vertex_input_state_manager;
    std::unique_ptr<graphics::shader_manager> shader_manager;
    std::unique_ptr<graphics::material_factory> material_factory;
    std::unique_ptr<graphics::pipeline_factory> pipeline_factory;

    std::unique_ptr<graphics::descriptor_registry> descriptor_registry;

    std::shared_ptr<graphics::render_pass> render_pass;
    std::unique_ptr<graphics::render_pass_manager> render_pass_manager;

    std::vector<graphics::attachment> attachments;
    std::vector<std::shared_ptr<resource::framebuffer>> framebuffers;

    std::shared_ptr<resource::semaphore> image_available_semaphore, render_finished_semaphore;

    camera_system cameraSystem;
    std::shared_ptr<camera> camera_;

    std::unique_ptr<orbit_controller> camera_controller;

    std::vector<per_object_t> objects;

    VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};

    VkCommandPool graphics_command_pool{VK_NULL_HANDLE}, transfer_command_pool{VK_NULL_HANDLE};

    VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};
    VkDescriptorPool descriptorPool{VK_NULL_HANDLE};
    VkDescriptorSet descriptorSet{VK_NULL_HANDLE};

    std::vector<VkCommandBuffer> command_buffers;

    std::shared_ptr<resource::buffer> perObjectBuffer, perCameraBuffer;
    void *ssbo_mapped_ptr{nullptr};

    std::size_t aligned_buffer_size{0u};

    std::shared_ptr<resource::texture> texture;

    std::vector<draw_command> draw_commands;

    std::function<void()> resize_callback{nullptr};

    void clean_up()
    {
        if (device == nullptr)
            return;

        vkDeviceWaitIdle(device->handle());

        draw_commands.clear();

        cleanup_frame_data(*this);

        render_finished_semaphore.reset();
        image_available_semaphore.reset();

        pipeline_factory.reset();

        vertex_input_state_manager.reset();

        material_factory.reset();

        shader_manager.reset();

        if (pipelineLayout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(device->handle(), pipelineLayout, nullptr);

        vkDestroyDescriptorSetLayout(device->handle(), descriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device->handle(), descriptorPool, nullptr);

        descriptor_registry.reset();

        render_pass.reset();
        render_pass_manager.reset();

        texture.reset();

        if (ssbo_mapped_ptr)
            vkUnmapMemory(device->handle(), perObjectBuffer->memory()->handle());

        perCameraBuffer.reset();
        perObjectBuffer.reset();

        if (transfer_command_pool != VK_NULL_HANDLE)
            vkDestroyCommandPool(device->handle(), transfer_command_pool, nullptr);

        if (graphics_command_pool != VK_NULL_HANDLE)
            vkDestroyCommandPool(device->handle(), graphics_command_pool, nullptr);

        temp::model.meshlets.clear();

        resource_manager.reset();
        memory_manager.reset();

        device.reset();
        instance.reset();
    }
};

struct window_events_handler final : public platform::window::event_handler_interface {

    window_events_handler(app_t &app) : app{app} { }

    app_t &app;

    void on_resize(std::int32_t width, std::int32_t height) override
    {
        if (app.width == static_cast<std::uint32_t>(width) && app.height == static_cast<std::uint32_t>(height))
            return;

        app.width = static_cast<std::uint32_t>(width);
        app.height = static_cast<std::uint32_t>(height);

        if (width < 1 || height < 1)
            return;

        app.resize_callback = [this]
        {
            recreate_swap_chain(app);

            app.camera_->aspect = static_cast<float>(app.width) / static_cast<float>(app.height);
        };
    }
};


void update_descriptor_set(app_t &app, vulkan::device const &device, VkDescriptorSet &descriptorSet)
{
    // TODO: descriptor info typed by VkDescriptorType.
    auto const per_camera = std::array{
        VkDescriptorBufferInfo{app.perCameraBuffer->handle(), 0, sizeof(camera::data_t)}
    };

    // TODO: descriptor info typed by VkDescriptorType.
    auto const per_object = std::array{
        VkDescriptorBufferInfo{app.perObjectBuffer->handle(), 0, sizeof(per_object_t)}
    };

#if TEMPORARILY_DISABLED
    // TODO: descriptor info typed by VkDescriptorType.
    auto const per_image = std::array{
        VkDescriptorImageInfo{app.texture.sampler->handle(), app.texture.view.handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
    };
#endif

    std::array<VkWriteDescriptorSet, 2> const write_descriptor_sets{{
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptorSet,
            0,
            0, static_cast<std::uint32_t>(std::size(per_camera)),
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            nullptr,
            std::data(per_camera),
            nullptr
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptorSet,
            1,
            0, static_cast<std::uint32_t>(std::size(per_object)),
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
            nullptr,
            std::data(per_object),
            nullptr
        }
#if TEMPORARILY_DISABLED
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptorSet,
            2,
            0, static_cast<std::uint32_t>(std::size(per_image)),
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            std::data(per_image),
            nullptr,
            nullptr
        }
#endif
    }};

    // WARN:: remember about potential race condition with the related executing command buffer.
    vkUpdateDescriptorSets(device.handle(), static_cast<std::uint32_t>(std::size(write_descriptor_sets)),
                           std::data(write_descriptor_sets), 0, nullptr);
}

void create_graphics_command_buffers(app_t &app, std::span<draw_command> draw_commands)
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
        throw vulkan::exception{fmt::format("failed to create allocate command buffers: {0:#x}"s, result)};

    //auto &&resource_manager = *app.resource_manager;
    //auto &&vertex_buffers = resource_manager.vertex_buffers();

    auto &&vertex_input_state_manager = *app.vertex_input_state_manager;

    std::uint32_t first_binding = std::numeric_limits<std::uint32_t>::max();

    std::set<std::pair<std::uint32_t, VkBuffer>> vertex_bindings_and_buffers;

    /*for (auto &&[vertex_layout, vertex_buffer] : vertex_buffers) {
        auto binding_index = vertex_input_state_manager.binding_index(vertex_layout);

        first_binding = std::min(binding_index, first_binding);

        vertex_bindings_and_buffers.emplace(binding_index, vertex_buffer->device_buffer().handle());
    }*/
    auto binding_index = vertex_input_state_manager.binding_index(draw_commands.front().vertex_buffer->vertex_layout());
    first_binding = std::min(binding_index, first_binding);
    vertex_bindings_and_buffers.emplace(binding_index, draw_commands.front().vertex_buffer->device_buffer()->handle());

    std::vector<VkBuffer> vertex_buffer_handles;

    std::transform(std::cbegin(vertex_bindings_and_buffers), std::cend(vertex_bindings_and_buffers),
                               std::back_inserter(vertex_buffer_handles), [] (auto pair) { return pair.second; });

    auto const binding_count = static_cast<std::uint32_t>(std::size(vertex_buffer_handles));

    std::vector<VkDeviceSize> vertex_buffer_offsets(binding_count, 0);

    auto const clear_colors = std::array{
        VkClearValue{ .color = { .float32 = { .64f, .64f, .64f, 1.f } } },
        VkClearValue{ .depthStencil = { app.renderer_config.reversed_depth ? 0.f : 1.f, 0 } }
    };

    std::sort(std::begin(draw_commands), std::end(draw_commands), [&vertex_input_state_manager] (auto &&lhs, auto &&rhs)
    {
        auto lhs_binding_index = vertex_input_state_manager.binding_index(lhs.vertex_buffer->vertex_layout());
        auto rhs_binding_index = vertex_input_state_manager.binding_index(rhs.vertex_buffer->vertex_layout());

        if (lhs_binding_index == rhs_binding_index)
            return lhs.vertex_buffer->device_buffer()->handle() < rhs.vertex_buffer->device_buffer()->handle();

        return lhs_binding_index < rhs_binding_index;
    });

    auto it_last = std::unique(std::begin(draw_commands), std::end(draw_commands), [&vertex_input_state_manager] (auto &&lhs, auto &&rhs)
    {
        auto lhs_binding_index = vertex_input_state_manager.binding_index(lhs.vertex_buffer->vertex_layout());
        auto rhs_binding_index = vertex_input_state_manager.binding_index(rhs.vertex_buffer->vertex_layout());

        if (lhs_binding_index == rhs_binding_index)
            return lhs.vertex_buffer->device_buffer()->handle() == rhs.vertex_buffer->device_buffer()->handle();

        return false;
    });

    auto unique_pairs = std::span{std::begin(draw_commands), it_last};

    auto it = std::begin(draw_commands);

    while (it != it_last) {
        it = std::adjacent_find(it, it_last, [&vertex_input_state_manager] (auto &&lhs, auto &&rhs)
        {
            auto lhs_binding_index = vertex_input_state_manager.binding_index(lhs.vertex_buffer->vertex_layout());
            auto rhs_binding_index = vertex_input_state_manager.binding_index(rhs.vertex_buffer->vertex_layout());

            return (rhs_binding_index - lhs_binding_index) != 1;
        });
    }

    std::vector<std::span<draw_command>> subranges;

    for (std::size_t i = 0; auto &command_buffer : app.command_buffers) {
        VkCommandBufferBeginInfo const begin_info{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            nullptr,
            VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
            nullptr
        };

        if (auto result = vkBeginCommandBuffer(command_buffer, &begin_info); result != VK_SUCCESS)
            throw vulkan::exception(fmt::format("failed to record command buffer: {0:#x}"s, result));

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

        vkCmdBindVertexBuffers(command_buffer, first_binding, binding_count, std::data(vertex_buffer_handles), std::data(vertex_buffer_offsets));

        std::uint32_t dynamic_offset = 0;

        auto min_offset_alignment = static_cast<std::size_t>(app.device->device_limits().min_storage_buffer_offset_alignment);
        auto aligned_offset = boost::alignment::align_up(sizeof(per_object_t), min_offset_alignment);

        for (auto &&draw_command : draw_commands) {
            auto [
                material, pipeline, vertex_buffer, index_buffer,
                vertex_count, first_vertex, index_count,
                pipeline_layout, render_pass, descriptor_set
            ] = draw_command;

            if (index_buffer && index_count) {
                auto index_type = index_buffer->format() == graphics::FORMAT::R16_UINT ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;

                vkCmdBindIndexBuffer(command_buffer, index_buffer->device_buffer().handle(), 0, index_type);
            }

            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->handle());

            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout,
                                    0, 1, &descriptor_set, 1, &dynamic_offset);

            if (index_buffer && index_count)
                vkCmdDrawIndexed(command_buffer, index_count, 1, first_vertex, 0, 0);

            else vkCmdDraw(command_buffer, vertex_count, 1, first_vertex, 0);

            dynamic_offset += static_cast<std::uint32_t>(aligned_offset);
        }

        vkCmdEndRenderPass(command_buffer);

        if (auto result = vkEndCommandBuffer(command_buffer); result != VK_SUCCESS)
            throw vulkan::exception(fmt::format("failed to end command buffer: {0:#x}"s, result));
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

    auto attachment_descriptions = create_attachment_descriptions(device, app.renderer_config, *swapchain);

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

    create_graphics_command_buffers(app, app.draw_commands);
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

    //auto &&resource_manager = *app.resource_manager;

    for (auto &&scene_node : model_.scene_nodes) {
        auto [transform_index, mesh_index] = scene_node;

        [[maybe_unused]] auto &&transform = model_.transforms.at(transform_index);

        auto &&mesh = model_.meshes.at(mesh_index);
        auto &&[meshlets] = mesh;

        for (auto meshlet_index : meshlets) {
            auto &&meshlet = model_.meshlets.at(meshlet_index);

            auto material_index = meshlet.material_index;
            auto [technique_index, name] = model_.materials[material_index];

            auto vertex_layout_index = meshlet.vertex_buffer_index;
            if (vertex_layout_index < 0)
                throw graphics::exception("invalid vertex buffer index"s);

            auto &&vertex_layout = model_.vertex_layouts[vertex_layout_index];

            auto vertex_layout_name = graphics::to_string(vertex_layout);

            fmt::print("{}.{}.{}\n"s, name, technique_index, vertex_layout_name);

            auto material = material_factory.material(name, technique_index, vertex_layout);

            if (material == nullptr)
                throw graphics::exception("failed to create material"s);

            auto vertex_buffer = meshlet.vertex_buffer;

            /*auto &&vertex_data_buffer = model_.vertex_buffers.at(vertex_layout_index);

            auto vertex_buffer = resource_manager.create_vertex_buffer(vertex_layout, std::size(vertex_data_buffer.buffer));

            if (vertex_buffer)
                resource_manager.stage_buffer_data(vertex_buffer, vertex_data_buffer.buffer);

            else throw graphics::exception("failed to get vertex buffer"s);*/

            std::shared_ptr<resource::index_buffer> index_buffer;
            
            /*if (auto index_buffer_index = meshlet.index_buffer_index; index_buffer_index > -1) {
                auto &&index_data_buffer = model_.index_buffers.at(index_buffer_index);

                index_buffer = resource_manager.create_index_buffer(index_data_buffer.format, std::size(vertex_data_buffer.buffer));

                if (index_buffer)
                    resource_manager.stage_buffer_data(index_buffer, index_data_buffer.buffer);

                else throw graphics::exception("failed to get index buffer"s);
            }*/

            [[maybe_unused]] auto &&vertex_input_state = vertex_input_state_manager.vertex_input_state(vertex_layout);

            auto adjusted_vertex_input_state = vertex_input_state_manager.get_adjusted_vertex_input_state(vertex_layout, material->vertex_layout);

            auto primitive_topology = meshlet.topology;

            graphics::pipeline_states pipeline_states{
                primitive_topology,
                adjusted_vertex_input_state,
                rasterization_state,
                depth_stencil_state,
                color_blend_state
            };

            auto pipeline = pipeline_factory.create_pipeline(material, pipeline_states, app.pipelineLayout, app.render_pass, 0u);

            app.draw_commands.push_back(
                draw_command{
                    material, pipeline, vertex_buffer, index_buffer,
                    meshlet.vertex_count, meshlet.first_vertex, 0u,
                    app.pipelineLayout, app.render_pass, app.descriptorSet
                }
            );
        }
    }
}


namespace temp
{
    void add_plane(app_t &app, xformat &model_, std::size_t vertex_layout_index, std::size_t material_index)
    {
        auto const &vertex_layout = model_.vertex_layouts.at(vertex_layout_index);

        glm::vec4 color;

        {
            std::random_device random_device;
            std::mt19937 generator{random_device()};

            std::uniform_real_distribution<float> uniform_real_distribution{0.f, 1.f};

            color = glm::vec4{
                uniform_real_distribution(generator),
                uniform_real_distribution(generator),
                uniform_real_distribution(generator),
                1.f
            };
        }

        auto constexpr topology = graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP;
        auto constexpr indices_format = graphics::FORMAT::R16_UINT;

        primitives::plane_create_info const create_info{
            vertex_layout, topology, graphics::FORMAT::UNDEFINED,
            1.f, 1.f, 1u, 1u
        };

        auto const vertex_count = primitives::calculate_plane_vertices_number(create_info);
        auto const vertex_size = vertex_layout.size_bytes;

        auto const vertex_buffer_allocation_size = vertex_count * vertex_size;
        std::cout << "vertex_buffer_allocation_size "s << vertex_buffer_allocation_size << std::endl;

        auto staging_buffer = app.resource_manager->create_staging_buffer(vertex_buffer_allocation_size);

        primitives::generate_plane(create_info, staging_buffer->mapped_ptr(), color);

        /*{
            struct vertex final {
                std::array<float, 3> p;
                std::array<float, 3> n;
                std::array<std::uint16_t, 2> t;
                std::array<std::uint8_t, 4> c;
            };

            auto it_vertex = reinterpret_cast<vertex *>(std::data(staging_buffer->mapped_ptr()));

            for (auto i = 0u; i < vertex_count; ++i, ++it_vertex) {
                std::cout << "p " << it_vertex->p[0] << ' ' << it_vertex->p[1] << ' ' << it_vertex->p[2] << std::endl;
                std::cout << "n " << it_vertex->n[0] << ' ' << it_vertex->n[1] << ' ' << it_vertex->n[2] << std::endl;
                std::cout << "t " << it_vertex->t[0] << ' ' << it_vertex->t[1] << std::endl;
                std::cout << "c " << it_vertex->c[0] << ' ' << it_vertex->c[1] << ' ' << it_vertex->c[2] << std::endl;
            }
        }*/

        auto vertex_buffer = app.resource_manager->stage_vertex_data(vertex_layout, staging_buffer, app.transfer_command_pool);

        //auto &&vertex_buffer = model_.vertex_buffers[vertex_layout_index];
        //auto &&index_buffer = model_.index_buffers[indices_format];

        {
            xformat::meshlet meshlet;

            meshlet.topology = topology;

            auto first_vertex = (vertex_buffer->offset_bytes() - vertex_buffer_allocation_size) / vertex_size;

            meshlet.vertex_buffer_index = vertex_layout_index;
            meshlet.vertex_buffer = vertex_buffer;
            meshlet.vertex_count = static_cast<std::uint32_t>(vertex_count);
            meshlet.first_vertex = static_cast<std::uint32_t>(first_vertex);

            meshlet.material_index = material_index;
            meshlet.instance_count = 1;
            meshlet.first_instance = 0;

            std::vector<std::size_t> meshlets{std::size(model_.meshlets)};
            model_.meshes.push_back(xformat::mesh{meshlets});

            model_.meshlets.push_back(std::move(meshlet));
        }

        /*{
            auto write_offset = static_cast<std::ptrdiff_t>(vertex_buffer.count * vertex_size);

            vertex_buffer.buffer.resize(std::size(vertex_buffer.buffer) + vertex_buffer_allocation_size);
            vertex_buffer.count += vertex_count;

            auto it_vertex_buffer = std::next(std::begin(vertex_buffer.buffer), write_offset);

            primitives::generate_plane(create_info, it_vertex_buffer, color);
        }*/

    #if 0
        {
            auto const vl = vertex::create_vertex_layout(
                vertex::SEMANTIC::POSITION, graphics::FORMAT::RGB32_SFLOAT,
                vertex::SEMANTIC::TEXCOORD_0, graphics::FORMAT::RG16_UNORM,
                vertex::SEMANTIC::COLOR_0, graphics::FORMAT::RGBA8_UNORM
            );

            struct vertex final {
                std::array<float, 3> p;
                std::array<std::uint16_t, 2> n;
                std::array<std::uint8_t, 4> c;
            };

            auto constexpr tpl = graphics::PRIMITIVE_TOPOLOGY::TRIANGLE_STRIP;

            primitives::plane_create_info const ci{
                vl, tpl, indices_format,
                1.f, 1.f, 1u, 2u
            };

            auto const vc = primitives::calculate_plane_vertices_number(ci);
            auto const ic = primitives::calculate_plane_indices_number(ci);

            auto format_inst = graphics::instantiate_format(indices_format);

            if (!format_inst.has_value())
                throw resource::exception("failed to instantiate indices format"s);

            auto index_size_in_bytes = std::visit([] (auto type)
            {
                return sizeof(decltype(type));

            }, format_inst.value());

            std::vector<std::byte> vbuffer(vc * vl.size_bytes);
            std::vector<std::byte> ibuffer(ic *index_size_in_bytes);

            primitives::generate_plane_indexed(ci, std::begin(vbuffer), std::begin(ibuffer), color);

            auto it_vertex = reinterpret_cast<vertex *>(std::data(vbuffer));
            auto it_index = reinterpret_cast<std::uint16_t *>(std::data(ibuffer));

            for (auto i = 0u; i < vc; ++i, ++it_vertex) {
                std::cout << "p " << it_vertex->p[0] << ' ' << it_vertex->p[1] << ' ' << it_vertex->p[2] << std::endl;
                std::cout << "n " << it_vertex->n[0] << ' ' << it_vertex->n[1] << std::endl;
                std::cout << "c " << it_vertex->c[0] << ' ' << it_vertex->c[1] << ' ' << it_vertex->c[2] << std::endl;
            }

            for (auto i = 0u; i < ic; i += 3, ++it_index) {
                std::cout << "i " << *it_index << ' ' << *++it_index << ' ' << *++it_index << std::endl;
            }
        }
    #endif
    }

    xformat populate(app_t &app)
    {
        xformat model_;
    
        model_.materials.push_back(xformat::material{0, "debug/color-debug-material"s});
        model_.materials.push_back(xformat::material{1, "debug/color-debug-material"s});
        model_.materials.push_back(xformat::material{0, "debug/normal-debug"s});
        model_.materials.push_back(xformat::material{0, "debug/texture-coordinate-debug"s});

        model_.transforms.push_back(
            glm::rotate(glm::translate(glm::mat4{1.f}, glm::vec3{-.5, 0, +.5}), glm::radians(-90.f), glm::vec3{1, 0, 0}));
        model_.transforms.push_back(
            glm::rotate(glm::translate(glm::mat4{1.f}, glm::vec3{+1, 1, -1}*0.f), glm::radians(-90.f * 0), glm::vec3{1, 0, 0}));
        model_.transforms.push_back(
            glm::rotate(glm::translate(glm::mat4{1.f}, glm::vec3{-1, 1, -1}*0.f), glm::radians(-90.f * 0), glm::vec3{1, 0, 0}));
        model_.transforms.push_back(
            glm::rotate(glm::translate(glm::mat4{1.f}, glm::vec3{+.5, 0, +.5}), glm::radians(-90.f), glm::vec3{1, 0, 0}));
        model_.transforms.push_back(
            glm::rotate(glm::translate(glm::mat4{1.f}, glm::vec3{0}), glm::radians(0.f), glm::vec3{1, 0, 0}));

        model_.vertex_layouts.push_back(vertex::create_vertex_layout(
            vertex::SEMANTIC::POSITION, graphics::FORMAT::RGB32_SFLOAT,
            vertex::SEMANTIC::NORMAL, graphics::FORMAT::RGB32_SFLOAT,
            vertex::SEMANTIC::TEXCOORD_0, graphics::FORMAT::RG16_UNORM,
            vertex::SEMANTIC::COLOR_0, graphics::FORMAT::RGBA8_UNORM
        ));

        model_.vertex_layouts.push_back(vertex::create_vertex_layout(
            vertex::SEMANTIC::POSITION, graphics::FORMAT::RGB32_SFLOAT,
            vertex::SEMANTIC::TEXCOORD_0, graphics::FORMAT::RG16_UNORM,
            vertex::SEMANTIC::COLOR_0, graphics::FORMAT::RGBA8_UNORM
        ));

        model_.vertex_layouts.push_back(vertex::create_vertex_layout(
            vertex::SEMANTIC::POSITION, graphics::FORMAT::RGB32_SFLOAT,
            vertex::SEMANTIC::NORMAL, graphics::FORMAT::RG16_SNORM,
            vertex::SEMANTIC::TEXCOORD_0, graphics::FORMAT::RG16_UNORM,
            vertex::SEMANTIC::COLOR_0, graphics::FORMAT::RGBA8_UNORM
        ));

        model_.scene_nodes.push_back(xformat::scene_node{0u, std::size(model_.meshes)});
        add_plane(app, model_, 0, 2);
    #if 0
        {
            auto const vertex_layout_index = 1u;
            auto &&vertex_layout = model_.vertex_layouts.at(vertex_layout_index);

            struct vertex_struct final {
                std::array<boost::float32_t, 3> position;
                std::array<std::uint16_t, 2> texCoord;
                std::array<std::uint8_t, 4> color;
            };

            std::vector<vertex_struct> vertices;

            auto constexpr max_8ui = std::numeric_limits<std::uint8_t>::max();
            auto constexpr max_16ui = std::numeric_limits<std::uint16_t>::max();

            // Second triangle
            vertices.push_back(vertex_struct{
                {0.f, 0.f, 0.f}, {max_16ui / 2, max_16ui / 2}, {0, 0, 0, max_8ui}
            });

            vertices.push_back(vertex_struct{
                {1.f, 0.f, -1.f}, {max_16ui, max_16ui}, {max_8ui, 0, max_8ui, max_8ui}
            });

            vertices.push_back(vertex_struct{
                {0.f, 0.f, -1.f}, {max_16ui / 2, max_16ui}, {0, 0, max_8ui, max_8ui}
            });

            // Third triangle
            vertices.push_back(vertex_struct{
                {0.f, 0.f, 0.f}, {max_16ui / 2, max_16ui / 2}, {max_8ui, 0, max_8ui, max_8ui}
            });

            vertices.push_back(vertex_struct{
                {-1.f, 0.f, -1.f}, {0, max_16ui}, {0, max_8ui, max_8ui, max_8ui}
            });

            vertices.push_back(vertex_struct{
                {-1.f, 0.f, 0.f}, {0, max_16ui / 2}, {max_8ui, max_8ui, 0, max_8ui}
            });

            auto &&vertex_buffer = model_.vertex_buffers[vertex_layout_index];

            {
                model_.scene_nodes.push_back(xformat::scene_node{1u, std::size(model_.meshes)});
                std::vector<std::size_t> meshlets{std::size(model_.meshlets)};
                model_.meshes.push_back(xformat::mesh{meshlets});

                // Second triangle
                xformat::meshlet meshlet;

                meshlet.topology = graphics::PRIMITIVE_TOPOLOGY::TRIANGLES;

                meshlet.vertex_buffer_index = vertex_layout_index;
                meshlet.vertex_count = static_cast<std::uint32_t>(3);
                meshlet.first_vertex = static_cast<std::uint32_t>(vertex_buffer.count + 0u);

                meshlet.material_index = 0;
                meshlet.instance_count = 1;
                meshlet.first_instance = 0;

                model_.meshlets.push_back(std::move(meshlet));
            }

            {
                model_.scene_nodes.push_back(xformat::scene_node{2u, std::size(model_.meshes)});
                std::vector<std::size_t> meshlets{std::size(model_.meshlets)};
                model_.meshes.push_back(xformat::mesh{meshlets});

                // Third triangle
                xformat::meshlet meshlet;

                meshlet.topology = graphics::PRIMITIVE_TOPOLOGY::TRIANGLES;

                meshlet.vertex_buffer_index = vertex_layout_index;
                meshlet.vertex_count = static_cast<std::uint32_t>(3);
                meshlet.first_vertex = static_cast<std::uint32_t>(vertex_buffer.count + 3u);

                meshlet.material_index = 3;
                meshlet.instance_count = 1;
                meshlet.first_instance = 0;

                model_.meshlets.push_back(std::move(meshlet));
            }

            {
                auto const vertex_count = std::size(vertices);
                auto const vertex_size = vertex_layout.size_bytes;

                auto const bytes_count = vertex_count * vertex_size;
                auto const write_offset = static_cast<std::ptrdiff_t>(vertex_buffer.count * vertex_size);

                vertex_buffer.buffer.resize(vertex_buffer.count * vertex_size + bytes_count);
                vertex_buffer.count += vertex_count;

                auto it_dst = std::next(std::begin(vertex_buffer.buffer), write_offset);

                auto vertices_bytes_view = std::as_bytes(std::span{vertices});
                std::copy_n(std::begin(vertices_bytes_view), vertices_bytes_view.size_bytes(), it_dst);
            }
        }
        
        model_.scene_nodes.push_back(xformat::scene_node{3u, std::size(model_.meshes)});
        add_plane(model_, 1, 1);

        model_.scene_nodes.push_back(xformat::scene_node{4u, std::size(model_.meshes)});
        add_plane(model_, 2, 2);
    #endif
        return model_;
    }
}


std::unique_ptr<renderer::swapchain>
create_swapchain(vulkan::device const &device, renderer::platform_surface const &platform_surface, renderer::extent extent)
{
    auto surface_formats = std::vector<renderer::surface_format>{
        { graphics::FORMAT::BGRA8_SRGB, graphics::COLOR_SPACE::SRGB_NONLINEAR },
        { graphics::FORMAT::RGBA8_SRGB, graphics::COLOR_SPACE::SRGB_NONLINEAR }
    };

    std::unique_ptr<renderer::swapchain> swapchain;

    for (auto surface_format : surface_formats) {
        try {
            swapchain = std::make_unique<renderer::swapchain>(device, platform_surface, surface_format, extent);

        } catch (vulkan::swapchain_exception const &ex) {
            std::cout << ex.what() << std::endl;
        }

        if (swapchain)
            break;
    }

    return swapchain;
}

void create_semaphores(app_t &app)
{
    auto &&resource_manager = *app.resource_manager;

    if (auto semaphore = resource_manager.create_semaphore(); !semaphore)
        throw resource::exception("failed to create image semaphore"s);

    else app.image_available_semaphore = semaphore;

    if (auto semaphore = resource_manager.create_semaphore(); !semaphore)
        throw resource::exception("failed to create render semaphore"s);

    else app.render_finished_semaphore = semaphore;
}

void init(platform::window &window, app_t &app)
{
    auto instance = std::make_unique<vulkan::instance>();

    app.platform_surface = instance->get_platform_surface(window);

    app.device = std::make_unique<vulkan::device>(*instance, app.platform_surface);

    app.renderer_config = renderer::adjust_renderer_config(app.device->device_limits());

    app.memory_manager = std::make_unique<resource::memory_manager>(*app.device);
    app.resource_manager = std::make_unique<resource::resource_manager>(*app.device, app.renderer_config, *app.memory_manager);

    app.shader_manager = std::make_unique<graphics::shader_manager>(*app.device);
    app.material_factory = std::make_unique<graphics::material_factory>();
    app.vertex_input_state_manager = std::make_unique<graphics::vertex_input_state_manager>();
    app.pipeline_factory = std::make_unique<graphics::pipeline_factory>(*app.device, app.renderer_config, *app.shader_manager);

    app.render_pass_manager = std::make_unique<graphics::render_pass_manager>(*app.device);

    app.descriptor_registry = std::make_unique<graphics::descriptor_registry>(*app.device);

    if (auto command_pool = create_command_pool(*app.device, app.device->transfer_queue, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT); command_pool)
        app.transfer_command_pool = *command_pool;

    else throw graphics::exception("failed to transfer command pool"s);

    if (auto command_pool = create_command_pool(*app.device, app.device->graphics_queue, 0); command_pool)
        app.graphics_command_pool = *command_pool;

    else throw graphics::exception("failed to graphics command pool"s);

    create_frame_data(app);

    if (auto descriptorSetLayout = CreateDescriptorSetLayout(*app.device); !descriptorSetLayout)
        throw graphics::exception("failed to create the descriptor set layout"s);

    else app.descriptorSetLayout = std::move(descriptorSetLayout.value());

#if TEMPORARILY_DISABLED
    if (auto result = glTF::load(sceneName, app.scene, app.nodeSystem); !result)
        throw resource::exception("failed to load a mesh"s);
#endif

    if (auto pipelineLayout = create_pipeline_layout(*app.device, std::array{app.descriptorSetLayout}); !pipelineLayout)
        throw graphics::exception("failed to create the pipeline layout"s);

    else app.pipelineLayout = std::move(pipelineLayout.value());

#if TEMPORARILY_DISABLED
    // "chalet/textures/chalet.tga"sv
    // "Hebe/textures/HebehebemissinSG1_metallicRoughness.tga"sv
    if (auto result = load_texture(app, *app.device, "sponza/textures/sponza_curtain_blue_diff.tga"sv); !result)
        throw resource::exception("failed to load a texture"s);

    else app.texture = std::move(result.value());

    if (auto result = app.device->resource_manager.create_image_sampler(app.texture.image->mip_levels()); !result)
        throw resource::exception("failed to create a texture sampler"s);

    else app.texture.sampler = result;
#endif

    temp::model = temp::populate(app);

    auto min_offset_alignment = static_cast<std::size_t>(app.device->device_limits().min_storage_buffer_offset_alignment);
    auto aligned_offset = boost::alignment::align_up(sizeof(per_object_t), min_offset_alignment);

    app.objects.resize(std::size(temp::model.scene_nodes));

    app.aligned_buffer_size = aligned_offset * std::size(app.objects);

    if (app.perObjectBuffer = CreateStorageBuffer(*app.resource_manager, app.aligned_buffer_size); app.perObjectBuffer) {
        auto &&buffer = *app.perObjectBuffer;

        auto offset = buffer.memory()->offset();
        auto size = buffer.memory()->size();

        if (auto result = vkMapMemory(app.device->handle(), buffer.memory()->handle(), offset, size, 0, &app.ssbo_mapped_ptr); result != VK_SUCCESS)
            throw vulkan::exception(fmt::format("failed to map per object uniform buffer memory: {0:#x}"s, result));
    }

    else throw graphics::exception("failed to init per object uniform buffer"s);

    if (app.perCameraBuffer = create_coherent_storage_buffer(*app.resource_manager, sizeof(camera::data_t)); !app.perCameraBuffer)
        throw graphics::exception("failed to init per camera uniform buffer"s);

    if (auto descriptorPool = create_descriptor_pool(*app.device); !descriptorPool)
        throw graphics::exception("failed to create the descriptor pool"s);

    else app.descriptorPool = std::move(descriptorPool.value());

    if (auto descriptorSet = create_descriptor_sets(*app.device, app.descriptorPool, std::array{app.descriptorSetLayout}); !descriptorSet)
        throw graphics::exception("failed to create the descriptor pool"s);

    else app.descriptorSet = std::move(descriptorSet.value());

    update_descriptor_set(app, *app.device, app.descriptorSet);

    build_render_pipelines(app, temp::model);

    //app.resource_manager->transfer_vertex_buffers_data(app.transfer_command_pool, app.device->transfer_queue);
    //app.resource_manager->transfer_index_buffers_data(app.transfer_command_pool, app.device->transfer_queue);

    create_graphics_command_buffers(app, app.draw_commands);

    create_semaphores(app);

    app.instance = std::move(instance);
}

void update(app_t &app)
{
    if (app.resize_callback) {
        app.resize_callback();
        app.resize_callback = nullptr;
    }

    app.camera_controller->update();
    app.cameraSystem.update();

    auto &&device = *app.device;

    {
        auto &&buffer = *app.perCameraBuffer;

        auto offset = buffer.memory()->offset();
        auto size = buffer.memory()->size();

        void *data;

        if (auto result = vkMapMemory(device.handle(), buffer.memory()->handle(), offset, size, 0, &data); result != VK_SUCCESS)
            throw vulkan::exception(fmt::format("failed to map per camera uniform buffer memory: {0:#x}"s, result));

        std::copy_n(&app.camera_->data, 1, reinterpret_cast<camera::data_t *>(data));

        vkUnmapMemory(device.handle(), buffer.memory()->handle());
    }

    std::transform(std::cbegin(temp::model.scene_nodes), std::cend(temp::model.scene_nodes),
                   std::begin(app.objects), [] (auto &&scene_node)
    {
        auto &&transform = temp::model.transforms.at(scene_node.transform_index);

        auto normal = glm::inverseTranspose(transform);

        return per_object_t{transform, normal};
    });

    using objects_type = typename decltype(app.objects)::value_type;
    auto it_begin = reinterpret_cast<objects_type *>(app.ssbo_mapped_ptr);

    std::size_t const stride = app.aligned_buffer_size / std::size(app.objects);

#ifdef _MSC_VER
    std::copy(std::execution::par, std::cbegin(app.objects), std::cend(app.objects), strided_bidirectional_iterator{it_begin, stride});
#else
    std::copy(std::cbegin(app.objects), std::cend(app.objects), strided_bidirectional_iterator<objects_type>{it_begin, stride});
#endif

    auto const mappedRanges = std::array{
        VkMappedMemoryRange{
            VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            nullptr,
            app.perObjectBuffer->memory()->handle(),
            app.perObjectBuffer->memory()->offset(),
            app.aligned_buffer_size
        }
    };

    vkFlushMappedMemoryRanges(app.device->handle(), static_cast<std::uint32_t>(std::size(mappedRanges)), std::data(mappedRanges));
}

void render_frame(app_t &app)
{
    if (app.width < 1 || app.height < 1)
        return;

    auto &&device = *app.device;
    auto &&swapchain = *app.swapchain;

    vkQueueWaitIdle(device.presentation_queue.handle());

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
            app.image_available_semaphore->handle(), VK_NULL_HANDLE, &image_index); result) {
        case VK_ERROR_OUT_OF_DATE_KHR:
            recreate_swap_chain(app);
            return;

        case VK_SUBOPTIMAL_KHR:
        case VK_SUCCESS:
            break;

        default:
            throw vulkan::exception(fmt::format("failed to acquire next image index: {0:#x}"s, result));
    }

    auto const wait_semaphores = std::array{ app.image_available_semaphore->handle() };
    auto const signal_semaphores = std::array{ app.render_finished_semaphore->handle() };

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

    if (auto result = vkQueueSubmit(device.graphics_queue.handle(), 1, &submit_info, VK_NULL_HANDLE); result != VK_SUCCESS)
        throw vulkan::exception(fmt::format("failed to submit draw command buffer: {0:#x}"s, result));

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
            throw vulkan::exception(fmt::format("failed to submit request to present framebuffer: {0:#x}"s, result));
    }
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
        throw std::runtime_error(fmt::format("failed to init GLFW: {0:#x}"s, result));

    app_t app;

    platform::window window{"VulkanIsland"sv, static_cast<std::int32_t>(app.width), static_cast<std::int32_t>(app.height)};

    auto app_window_events_handler = std::make_shared<window_events_handler>(app);
    window.connect_event_handler(app_window_events_handler);

    auto input_manager = std::make_shared<platform::input_manager>();
    window.connect_input_handler(input_manager);

    app.camera_ = app.cameraSystem.create_camera();
    app.camera_->aspect = static_cast<float>(app.width) / static_cast<float>(app.height);

    app.camera_controller = std::make_unique<orbit_controller>(app.camera_, *input_manager);
    app.camera_controller->look_at(glm::vec3{0, 2, 1}, {0, 0, 0});

    std::cout << measure<>::execution(init, window, std::ref(app)) << " ms\n"s;

    window.update([&app]
    {
        glfwPollEvents();

        update(app);

        render_frame(app);
    });

    app.clean_up();

    glfwTerminate();
}

[[nodiscard]] std::shared_ptr<resource::texture>
load_texture(app_t &app, vulkan::device &device, resource::resource_manager &resource_manager, std::string_view name)
{
    std::shared_ptr<resource::texture> texture;

    auto constexpr generateMipMaps = true;

    if (auto rawImage = LoadTARGA(name); rawImage) {
        auto staging_buffer = std::visit([&device, &resource_manager] (auto &&data)
        {
            return stage_data(device, resource_manager, std::span{data});
        }, std::move(rawImage->data));

        if (staging_buffer) {
            auto const width = static_cast<std::uint16_t>(rawImage->width);
            auto const height = static_cast<std::uint16_t>(rawImage->height);

            auto constexpr usage_flags = graphics::IMAGE_USAGE::TRANSFER_SOURCE | graphics::IMAGE_USAGE::TRANSFER_DESTINATION | graphics::IMAGE_USAGE::SAMPLED;
            auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::DEVICE_LOCAL;

            auto constexpr tiling = graphics::IMAGE_TILING::OPTIMAL;

            auto type = graphics::IMAGE_TYPE::TYPE_2D;
            auto format = rawImage->format;
            auto view_type = rawImage->view_type;
            auto mip_levels = rawImage->mip_levels;
            auto aspectFlags = graphics::IMAGE_ASPECT::COLOR_BIT;
            auto samples_count = 1u;

            auto extent = renderer::extent{width, height};

            if (auto image = resource_manager.create_image(type, format, extent, mip_levels, samples_count, tiling, usage_flags, property_flags); image) {
                if (auto view = resource_manager.create_image_view(image, view_type, aspectFlags); view)
            #if NOT_YET_IMPLEMENTED
                    if (auto sampler = resource_manager.create_image_sampler(mip_levels()); sampler)
                        texture.emplace(image, *view, sampler);
            #else
                    texture = std::make_shared<resource::texture>(image, view, nullptr);
            #endif
            }

            if (texture) {
                image_layout_transition(device, device.transfer_queue, *texture->image, graphics::IMAGE_LAYOUT::UNDEFINED,
                                      graphics::IMAGE_LAYOUT::TRANSFER_DESTINATION, app.transfer_command_pool);

                copy_buffer_to_image(device, device.transfer_queue, staging_buffer->handle(), texture->image->handle(), extent, app.transfer_command_pool);

                if (generateMipMaps)
                    generate_mip_maps(device, device.transfer_queue, *texture->image, app.transfer_command_pool);

                else image_layout_transition(device, device.transfer_queue, *texture->image, graphics::IMAGE_LAYOUT::TRANSFER_DESTINATION,
                                           graphics::IMAGE_LAYOUT::SHADER_READ_ONLY, app.transfer_command_pool);
            }
        }
    }

    else throw resource::exception(fmt::format("failed to load an image: {0:#x}"s, name));


    return texture;
}


template<class T>
[[nodiscard]] std::shared_ptr<resource::buffer>
stage_data(vulkan::device &device, resource::resource_manager &resource_manager, std::span<T> const container)
{
    auto constexpr usage_flags = graphics::BUFFER_USAGE::TRANSFER_SOURCE;
    auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::HOST_VISIBLE | graphics::MEMORY_PROPERTY_TYPE::HOST_COHERENT;

    auto buffer = resource_manager.create_buffer(container.size_bytes(), usage_flags, property_flags);

    if (buffer) {
        void *data;

        auto &&memory = buffer->memory();

        if (auto result = vkMapMemory(device.handle(), memory->handle(), memory->offset(), memory->size(), 0, &data); result != VK_SUCCESS)
            throw vulkan::exception(fmt::format("failed to map staging buffer memory: {0:#x}"s, result));

        else {
            std::copy(std::begin(container), std::end(container), reinterpret_cast<T *>(data));

            vkUnmapMemory(device.handle(), buffer->memory()->handle());
        }
    }

    return buffer;
}
