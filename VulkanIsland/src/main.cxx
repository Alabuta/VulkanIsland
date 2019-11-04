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
#include <unordered_map>

#ifdef _MSC_VER
#define USE_EXECUTION_POLICIES
#include <execution>
#endif

#include <fmt/format.h>
#include <boost/align/aligned_alloc.hpp>

#include "main.hxx"
#include "utility/helpers.hxx"
#include "utility/mpl.hxx"
#include "math/math.hxx"

#include "vulkan/instance.hxx"
#include "vulkan/device.hxx"

#include "renderer/config.hxx"
#include "renderer/swapchain.hxx"
#include "renderer/command_buffer.hxx"

#include "resources/buffer.hxx"
#include "resources/image.hxx"
#include "resources/resource.hxx"
#include "resources/resource_manager.hxx"
#include "resources/semaphore.hxx"
#include "resources/framebuffer.hxx"

#include "descriptor.hxx"

#include "loaders/TARGA_loader.hxx"
#include "loaders/scene_loader.hxx"

#include "graphics/graphics_pipeline.hxx"
#include "graphics/pipeline_states.hxx"

#include "graphics/graphics.hxx"

#include "graphics/material.hxx"

#include "graphics/vertex.hxx"
#include "graphics/render_pass.hxx"
#include "graphics/render_flow.hxx"
#include "graphics/compatibility.hxx"

#include "platform/input/input_manager.hxx"
#include "camera/camera.hxx"
#include "camera/camera_controller.hxx"


auto constexpr kSCENE_NAME{"unlit-test"sv};

namespace temp
{
    xformat model;
}

struct per_object_t final {
    glm::mat4 world{1};
    glm::mat4 normal{1};  // Transposed of the inversed of the upper left 3x3 sub-matrix of model(world)-view matrix.
};

void cleanup_frame_data(struct app_t &app);
void create_semaphores(app_t &app);
void recreate_swap_chain(app_t &app);

template<class T, typename std::enable_if_t<mpl::is_container_v<std::remove_cvref_t<T>>>...>
[[nodiscard]] std::shared_ptr<resource::buffer> stage_data(vulkan::device &device, ResourceManager &resource_manager, T &&container);

[[nodiscard]] std::shared_ptr<resource::texture>
load_texture(app_t &app, vulkan::device &device, ResourceManager &resource_manager, std::string_view name);


std::unique_ptr<renderer::swapchain>
create_swapchain(vulkan::device const &device, renderer::platform_surface const &platform_surface, renderer::extent extent);

std::vector<graphics::attachment>
create_attachments(vulkan::device const &device, renderer::config const &renderer_config, ResourceManager &resource_manager, renderer::swapchain const &swapchain);

std::vector<graphics::attachment_description>
create_attachment_descriptions(std::vector<graphics::attachment> const &attachments);

std::shared_ptr<graphics::render_pass>
create_render_pass(graphics::render_pass_manager &render_pass_manager,
                   renderer::surface_format surface_format, std::vector<graphics::attachment_description> attachment_descriptions);

std::vector<std::shared_ptr<resource::framebuffer>>
create_framebuffers(resource::resource_manager &resource_manager, renderer::swapchain const &swapchain,
                    std::shared_ptr<graphics::render_pass> render_pass, std::vector<graphics::attachment> const &attachments);


struct draw_command final {
    std::shared_ptr<graphics::material> material;
    std::shared_ptr<graphics::pipeline> pipeline;
    std::shared_ptr<resource::vertex_buffer> vertex_buffer;

    std::uint32_t vertex_count{0};
    std::uint32_t first_vertex{0};

    VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};

    std::shared_ptr<graphics::render_pass> render_pass;
    VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
};

struct app_t final {
    std::uint32_t width{800u};
    std::uint32_t height{600u};

    camera_system cameraSystem;
    std::shared_ptr<camera> camera;

    std::unique_ptr<orbit_controller> camera_controller;

    std::vector<per_object_t> objects;

    std::unique_ptr<vulkan::instance> vulkan_instance;
    std::unique_ptr<vulkan::device> device;

    std::unique_ptr<MemoryManager> memory_manager;
    std::unique_ptr<ResourceManager> resource_manager2;

    std::unique_ptr<resource::resource_manager> resource_manager;

    VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};

    VkCommandPool graphicsCommandPool{VK_NULL_HANDLE}, transferCommandPool{VK_NULL_HANDLE};

    VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};
    VkDescriptorPool descriptorPool{VK_NULL_HANDLE};
    VkDescriptorSet descriptorSet{VK_NULL_HANDLE};

    std::vector<VkCommandBuffer> command_buffers;

    std::shared_ptr<resource::semaphore> imageAvailableSemaphore, renderFinishedSemaphore;

    std::shared_ptr<resource::buffer> perObjectBuffer, perCameraBuffer;
    void *perObjectsMappedPtr{nullptr};
    void *alignedBuffer{nullptr};

    std::size_t objectsNumber{2u};
    std::size_t alignedBufferSize{0u};

    std::shared_ptr<resource::texture> texture;

#if TEMPORARILY_DISABLED
    ecs::entity_registry registry;

    ecs::NodeSystem nodeSystem{registry};
#if NOT_YET_IMPLEMENTED
    ecs::MeshSystem meshSystem{registry};
#endif
#endif

    std::unique_ptr<renderer::platform_surface> platform_surface;
    std::unique_ptr<renderer::swapchain> swapchain;

    renderer::config renderer_config;

    std::unique_ptr<graphics::vertex_input_state_manager> vertex_input_state_manager;
    std::unique_ptr<graphics::shader_manager> shader_manager;
    std::unique_ptr<graphics::material_factory> material_factory;
    std::unique_ptr<graphics::pipeline_factory> pipeline_factory;

    std::shared_ptr<graphics::render_pass> render_pass;
    std::unique_ptr<graphics::render_pass_manager> render_pass_manager;

    std::vector<graphics::attachment> attachments;
    std::vector<std::shared_ptr<resource::framebuffer>> framebuffers;

    std::vector<draw_command> draw_commands;

    std::function<void()> resize_callback{nullptr};

    void clean_up()
    {
        if (device == nullptr)
            return;

        vkDeviceWaitIdle(device->handle());

        draw_commands.clear();

        cleanup_frame_data(*this);

        renderFinishedSemaphore.reset();
        imageAvailableSemaphore.reset();

        pipeline_factory.reset();

        vertex_input_state_manager.reset();

        material_factory.reset();

        shader_manager.reset();

        if (pipelineLayout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(device->handle(), pipelineLayout, nullptr);

        vkDestroyDescriptorSetLayout(device->handle(), descriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device->handle(), descriptorPool, nullptr);

        render_pass.reset();
        render_pass_manager.reset();

        texture.reset();

        if (perObjectsMappedPtr)
            vkUnmapMemory(device->handle(), perObjectBuffer->memory()->handle());

        if (alignedBuffer)
            boost::alignment::aligned_free(alignedBuffer);

        perCameraBuffer.reset();
        perObjectBuffer.reset();

        if (transferCommandPool != VK_NULL_HANDLE)
            vkDestroyCommandPool(device->handle(), transferCommandPool, nullptr);

        if (graphicsCommandPool != VK_NULL_HANDLE)
            vkDestroyCommandPool(device->handle(), graphicsCommandPool, nullptr);

        platform_surface.reset();

        resource_manager.reset();
        resource_manager2.reset();
        memory_manager.reset();

        device.reset();
        vulkan_instance.reset();
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

            app.camera->aspect = static_cast<float>(app.width) / static_cast<float>(app.height);
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
        },
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

    // WARN:: remember about potential race condition with the related executing command buffer
    vkUpdateDescriptorSets(device.handle(), static_cast<std::uint32_t>(std::size(write_descriptor_sets)),
                           std::data(write_descriptor_sets), 0, nullptr);
}

void create_graphics_command_buffers(app_t &app)
{
    app.command_buffers.resize(std::size(app.swapchain->image_views()));

    VkCommandBufferAllocateInfo const allocate_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        app.graphicsCommandPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        static_cast<std::uint32_t>(std::size(app.command_buffers))
    };

    if (auto result = vkAllocateCommandBuffers(app.device->handle(), &allocate_info, std::data(app.command_buffers)); result != VK_SUCCESS)
        throw std::runtime_error(fmt::format("failed to create allocate command buffers: {0:#x}\n"s, result));

    auto &&resource_manager2 = *app.resource_manager2;
    auto &&vertex_buffers = resource_manager2.vertex_buffers();

    auto &&vertex_input_state_manager = *app.vertex_input_state_manager;

    std::uint32_t first_binding = std::numeric_limits<std::uint32_t>::max();

    std::set<std::pair<std::uint32_t, VkBuffer>> vertex_bindings_and_buffers;

    for (auto &&[vertex_layout, vertex_buffer] : vertex_buffers) {
        auto binding_index = vertex_input_state_manager.binding_index(vertex_layout);

        first_binding = std::min(binding_index, first_binding);

        vertex_bindings_and_buffers.emplace(binding_index, vertex_buffer->deviceBuffer().handle());
    }

    std::vector<VkBuffer> vertex_buffer_handles;

    std::transform(std::cbegin(vertex_bindings_and_buffers), std::cend(vertex_bindings_and_buffers),
                               std::back_inserter(vertex_buffer_handles), [] (auto &&pair) { return pair.second; });

    auto const binding_count = static_cast<std::uint32_t>(std::size(vertex_buffer_handles));

    std::vector<VkDeviceSize> vertex_buffer_offsets(binding_count, 0);

#if defined( __clang__) || defined(_MSC_VER)
    auto const clear_colors = std::array{
        VkClearValue{{{ .64f, .64f, .64f, 1.f }}},
        VkClearValue{{{ kREVERSED_DEPTH ? 0.f : 1.f, 0 }}}
    };
#else
    auto const clear_colors = std::array{
        VkClearValue{.color = {.float32 = { .64f, .64f, .64f, 1.f } } },
        VkClearValue{.depthStencil = { kREVERSED_DEPTH ? 0.f : 1.f, 0 } }
    };
#endif

#ifdef _MSC_VER
    std::size_t i = 0;

    for (auto &command_buffer : app.command_buffers) {
#else
    for (std::size_t i = 0; auto &commandBuffer : app.commandBuffers) {
#endif
        VkCommandBufferBeginInfo const begin_info{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            nullptr,
            VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
            nullptr
        };

        if (auto result = vkBeginCommandBuffer(command_buffer, &begin_info); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to record command buffer: {0:#x}\n"s, result));

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

        for (auto &&draw_command : app.draw_commands) {
            auto [
                material, pipeline, vertex_buffer,
                vertex_count, first_vertex,
                pipelineLayout, render_pass, descriptorSet
            ] = draw_command;

            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->handle());

            std::uint32_t const dynamic_offset = 0;

            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                                    0, 1, &descriptorSet, 1, &dynamic_offset);

            vkCmdDraw(command_buffer, vertex_count, 1, first_vertex, 0);
        }

        vkCmdEndRenderPass(command_buffer);

        if (auto result = vkEndCommandBuffer(command_buffer); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to end command buffer: {0:#x}\n"s, result));
    }
}


void cleanup_frame_data(app_t &app)
{
    auto &&device = *app.device;

    if (app.graphicsCommandPool)
        vkFreeCommandBuffers(device.handle(), app.graphicsCommandPool, static_cast<std::uint32_t>(std::size(app.command_buffers)), std::data(app.command_buffers));

    app.command_buffers.clear();

    app.framebuffers.clear();
    app.attachments.clear();

    app.swapchain.reset();
}

void recreate_swap_chain(app_t &app)
{
    if (app.width < 1 || app.height < 1)
        return;

    auto &&device = *app.device;

    vkDeviceWaitIdle(device.handle());

    cleanup_frame_data(app);

    app.swapchain = create_swapchain(*app.device, *app.platform_surface, renderer::extent{app.width, app.height});

    if (app.swapchain == nullptr)
        throw std::runtime_error("failed to create the swapchain"s);

    app.attachments = create_attachments(*app.device, app.renderer_config, *app.resource_manager2, *app.swapchain);

    if (app.attachments.empty())
        throw std::runtime_error("failed to create the attachments"s);

    auto attachment_descriptions = create_attachment_descriptions(app.attachments);

    app.render_pass = create_render_pass(*app.render_pass_manager, app.swapchain->surface_format(), attachment_descriptions);

    if (app.render_pass == nullptr)
        throw std::runtime_error("failed to create the render pass"s);

    app.framebuffers = create_framebuffers(*app.resource_manager, *app.swapchain, app.render_pass, app.attachments);

    if (app.framebuffers.empty())
        throw std::runtime_error("failed to create the framebuffers"s);

#if !USE_DYNAMIC_PIPELINE_STATE
    CreateGraphicsPipelines(app);
#endif

    create_graphics_command_buffers(app);
}

void build_render_pipelines(app_t &app, xformat const &_model)
{
    std::vector<graphics::render_pipeline> render_pipelines;

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

    auto &&resource_manager2 = *app.resource_manager2;

    for (auto &&meshlet : _model.non_indexed_meshlets) {
        auto material_index = meshlet.material_index;
        auto [technique_index, name] = _model.materials[material_index];

        std::cout << fmt::format("{}#{}\n"s, name, technique_index);

        auto material = material_factory.material(name, technique_index);

        auto vertex_layout_index = meshlet.vertex_buffer_index;
        auto &&vertex_layout = _model.vertex_layouts[vertex_layout_index];

        auto &&vertex_data_buffer = _model.vertex_buffers.at(vertex_layout_index);

        auto vertex_buffer = resource_manager2.CreateVertexBuffer(vertex_layout, std::size(vertex_data_buffer.buffer));

        if (vertex_buffer)
            resource_manager2.StageVertexData(vertex_buffer, vertex_data_buffer.buffer);

        else throw std::runtime_error("failed to get vertex buffer"s);

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
                material, pipeline, vertex_buffer,
                meshlet.vertex_count, meshlet.first_vertex,
                app.pipelineLayout, app.render_pass, app.descriptorSet
            }
        );
    }
}

namespace temp
{
xformat populate()
{
    xformat _model;

    auto constexpr vertexCountPerMeshlet = 3u;

    {
        // First triangle
        struct vertex_struct final {
            vertex::static_array<3, boost::float32_t> position;
            //vertex::static_array<3, boost::float32_t> normal;
            vertex::static_array<2, boost::float32_t> texCoord;
            vertex::static_array<3, boost::float32_t> color;
        };

        auto const vertex_layout_index = std::size(_model.vertex_layouts);

        _model.vertex_layouts.push_back(
            vertex::create_vertex_layout(
                vertex::position{}, decltype(vertex_struct::position){}, false,
                //vertex::normal{}, decltype(vertex_struct::normal){}, false,
                vertex::tex_coord_0{}, decltype(vertex_struct::texCoord){}, false,
                vertex::color_0{}, decltype(vertex_struct::color){}, false
            )
        );

        std::vector<vertex_struct> vertices;

        vertices.push_back(vertex_struct{
            {{0.f, 0.f, 0.f}}/*, {{ 0.f, 1.f, 0.f }}*/, {{.5f, .5f}}, {{ .8f, 1.f, .2f }}
        });

        vertices.push_back(vertex_struct{
            {{-1.f, 0.f, 1.f}}/*, {{ 0.f, 1.f, 0.f }}*/, {{0.f, 0.f}}, {{ 1.f, .8f, .2f }}
        });

        vertices.push_back(vertex_struct{
            {{0.f, 0.f, 1.f}}/*, {{ 0.f, 1.f, 0.f }}*/, {{1.f, 0.f}}, {{ .2f, 0.8f, 1.f }}
        });

        xformat::non_indexed_meshlet meshlet;

        meshlet.topology = graphics::PRIMITIVE_TOPOLOGY::TRIANGLES;

        {
            auto const vertexSize = sizeof(vertex_struct);
            auto const vertex_count = std::size(vertices);
            auto const bytesCount = vertexSize * vertex_count;

            auto &&vertexBuffer = _model.vertex_buffers[vertex_layout_index];

            using buffer_type_t = std::remove_cvref_t<decltype(vertexBuffer.buffer)>;

            meshlet.vertex_buffer_index = vertex_layout_index;
            meshlet.vertex_count = static_cast<std::uint32_t>(vertexCountPerMeshlet);
            meshlet.first_vertex = static_cast<std::uint32_t>(vertexBuffer.count);

            vertexBuffer.buffer.resize(std::size(vertexBuffer.buffer) + bytesCount);

            auto writeOffset = static_cast<buffer_type_t::difference_type>(vertexBuffer.count * vertexSize);

            vertexBuffer.count += vertex_count;

            auto dstBegin = std::next(std::begin(vertexBuffer.buffer), writeOffset);

            std::uninitialized_copy_n(reinterpret_cast<std::byte *>(std::data(vertices)), bytesCount, dstBegin);
        }

        meshlet.material_index = 2;
        meshlet.instance_count = 1;
        meshlet.first_instance = 0;

        _model.non_indexed_meshlets.push_back(std::move(meshlet));
    }

    {
        struct vertex_struct final {
            vertex::static_array<3, boost::float32_t> position;
            vertex::static_array<2, boost::float32_t> texCoord;
            vertex::static_array<4, boost::float32_t> color;
        };

        auto const vertex_layout_index = std::size(_model.vertex_layouts);

        _model.vertex_layouts.push_back(
            vertex::create_vertex_layout(
                vertex::position{}, decltype(vertex_struct::position){}, false,
                vertex::tex_coord_0{}, decltype(vertex_struct::texCoord){}, false,
                vertex::color_0{}, decltype(vertex_struct::color){}, false
            )
        );

        std::vector<vertex_struct> vertices;

        // Second triangle
        vertices.push_back(vertex_struct{
            {{0.f, 0.f, 0.f}}, {{.5f, .5f}}, {{0.f, 0.f, 0.f, 1.f}}
        });

        vertices.push_back(vertex_struct{
            {{1.f, 0.f, -1.f}}, {{1.f, 1.f}}, {{1.f, 0.f, 1.f, 1.f}}
        });

        vertices.push_back(vertex_struct{
            {{0.f, 0.f, -1.f}}, {{.5f, 1.f}}, {{0.f, 0.f, 1.f, 1.f}}
        });

        // Third triangle
        vertices.push_back(vertex_struct{
            {{0.f, 0.f, 0.f}}, {{.5f, .5f}}, {{1.f, 0.f, 1.f, 1.f}}
        });

        vertices.push_back(vertex_struct{
            {{-1.f, 0.f, -1.f}}, {{0.f, 1.f}}, {{0.f, 1.f, 1.f, 1.f}}
        });

        vertices.push_back(vertex_struct{
            {{-1.f, 0.f, 0.f}}, {{0.f, .5f}}, {{1.f, 1.f, 0.f, 1.f}}
        });

        auto &&vertexBuffer = _model.vertex_buffers[vertex_layout_index];

        using buffer_type_t = std::remove_cvref_t<decltype(vertexBuffer.buffer)>;

        {
            // Second triangle
            xformat::non_indexed_meshlet meshlet;

            meshlet.topology = graphics::PRIMITIVE_TOPOLOGY::TRIANGLES;

            meshlet.vertex_buffer_index = vertex_layout_index;
            meshlet.vertex_count = static_cast<std::uint32_t>(vertexCountPerMeshlet);
            meshlet.first_vertex = static_cast<std::uint32_t>(vertexBuffer.count + 0u);

            meshlet.material_index = 1;
            meshlet.instance_count = 1;
            meshlet.first_instance = 0;

            _model.non_indexed_meshlets.push_back(std::move(meshlet));
        }

        {
            // Third triangle
            xformat::non_indexed_meshlet meshlet;

            meshlet.topology = graphics::PRIMITIVE_TOPOLOGY::TRIANGLES;

            meshlet.vertex_buffer_index = vertex_layout_index;
            meshlet.vertex_count = static_cast<std::uint32_t>(vertexCountPerMeshlet);
            meshlet.first_vertex = static_cast<std::uint32_t>(vertexBuffer.count + vertexCountPerMeshlet);

            meshlet.material_index = 0;
            meshlet.instance_count = 1;
            meshlet.first_instance = 0;

            _model.non_indexed_meshlets.push_back(std::move(meshlet));
        }

        {
            auto const vertexSize = sizeof(vertex_struct);
            auto const vertex_count = std::size(vertices);
            auto const bytesCount = vertexSize * vertex_count;

            vertexBuffer.buffer.resize(std::size(vertexBuffer.buffer) + bytesCount);

            auto writeOffset = static_cast<buffer_type_t::difference_type>(vertexBuffer.count * vertexSize);

            vertexBuffer.count += vertex_count;

            auto dstBegin = std::next(std::begin(vertexBuffer.buffer), writeOffset);

            std::uninitialized_copy_n(reinterpret_cast<std::byte *>(std::data(vertices)), bytesCount, dstBegin);
        }
    }
    
    {
        struct vertex_struct final {
            vertex::static_array<3, boost::float32_t> position;
            vertex::static_array<2, boost::float32_t> texCoord;
            vertex::static_array<3, boost::float32_t> color;
        };

        auto const vertex_layout_index = std::size(_model.vertex_layouts);

        _model.vertex_layouts.push_back(
            vertex::create_vertex_layout(
                vertex::position{}, decltype(vertex_struct::position){}, false,
                vertex::tex_coord_0{}, decltype(vertex_struct::texCoord){}, false,
                vertex::color_0{}, decltype(vertex_struct::color){}, false
            )
        );

        std::vector<vertex_struct> vertices;

        // Fourth triangle
        vertices.push_back(vertex_struct{
            {{0.f, 0.f, 0.f}}, {{.5f, .5f}}, {{1.f, 0.f, 0.f}}
        });

        vertices.push_back(vertex_struct{
            {{1.f, 0.f, 1.f}}, {{1.f, 1.f}}, {{0.f, 0.5f, 0.5f}}
        });

        vertices.push_back(vertex_struct{
            {{1.f, 0.f, 0.f}}, {{.5f, 1.f}}, {{.8f, .5f, 0.f}}
        });

        auto &&vertexBuffer = _model.vertex_buffers[vertex_layout_index];

        using buffer_type_t = std::remove_cvref_t<decltype(vertexBuffer.buffer)>;

        {
            // Fourth triangle
            xformat::non_indexed_meshlet meshlet;

            meshlet.topology = graphics::PRIMITIVE_TOPOLOGY::TRIANGLES;

            meshlet.vertex_buffer_index = vertex_layout_index;
            meshlet.vertex_count = static_cast<std::uint32_t>(vertexCountPerMeshlet);
            meshlet.first_vertex = static_cast<std::uint32_t>(vertexBuffer.count + vertexCountPerMeshlet);

            meshlet.material_index = 2;
            meshlet.instance_count = 1;
            meshlet.first_instance = 0;

            _model.non_indexed_meshlets.push_back(std::move(meshlet));
        }

        {
            auto const vertexSize = sizeof(vertex_struct);
            auto const vertex_count = std::size(vertices);
            auto const bytesCount = vertexSize * vertex_count;

            vertexBuffer.buffer.resize(std::size(vertexBuffer.buffer) + bytesCount);

            auto writeOffset = static_cast<buffer_type_t::difference_type>(vertexBuffer.count * vertexSize);

            vertexBuffer.count += vertex_count;

            auto dstBegin = std::next(std::begin(vertexBuffer.buffer), writeOffset);

            std::uninitialized_copy_n(reinterpret_cast<std::byte *>(std::data(vertices)), bytesCount, dstBegin);
        }
    }

    _model.materials.push_back(xformat::material{0, "debug/texture-coordinate-debug"s});
    _model.materials.push_back(xformat::material{0, "debug/color-debug-material"s});
    _model.materials.push_back(xformat::material{1, "debug/color-debug-material"s});
    //_model.materials.push_back(xformat::material{0, "debug/normal-debug"s});

    return _model;
}
}


renderer::config adjust_renderer_config(vulkan::device const &device)
{
    auto &&device_limits = device.device_limits();

    auto sample_counts = std::min(device_limits.framebuffer_color_sample_counts, device_limits.framebuffer_depth_sample_counts);

    renderer::config renderer_config;

    renderer_config.framebuffer_sample_counts = std::min(sample_counts, renderer_config.framebuffer_sample_counts);

    return renderer_config;
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

        } catch (std::runtime_error const &ex) {
            std::cout << ex.what() << std::endl;
        }

        if (swapchain)
            break;
    }

    return swapchain;
}

std::vector<graphics::attachment>
create_attachments(vulkan::device const &device, renderer::config const &renderer_config, ResourceManager &resource_manager, renderer::swapchain const &swapchain)
{
    auto constexpr mip_levels = 1u;
    auto constexpr view_type = graphics::IMAGE_VIEW_TYPE::TYPE_2D;

    auto [width, height] = swapchain.extent();

    std::vector<graphics::attachment> attachments;

    auto samples_count = renderer_config.framebuffer_sample_counts;

    {
        /* | graphics::IMAGE_USAGE::TRANSFER_DESTINATION*/
        auto constexpr usage_flags = graphics::IMAGE_USAGE::TRANSIENT_ATTACHMENT | graphics::IMAGE_USAGE::COLOR_ATTACHMENT;
        auto constexpr property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT /*| VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT*/;
        auto constexpr tiling = graphics::IMAGE_TILING::OPTIMAL;
        auto constexpr aspect_flags = graphics::IMAGE_ASPECT::COLOR_BIT;

        auto format = swapchain.surface_format().format;

        auto image = resource_manager.CreateImage(format, width, height, mip_levels, samples_count, tiling, usage_flags, property_flags);

        if (image == nullptr)
            throw std::runtime_error("failed to create image for the color attachment"s);

        auto image_view = resource_manager.CreateImageView(image, view_type, convert_to::vulkan(aspect_flags));

        if (image_view == nullptr)
            throw std::runtime_error("failed to create image view for the color attachment"s);

        attachments.push_back(graphics::color_attachment{format, tiling, mip_levels, samples_count, image, image_view});
    }

    {
        auto constexpr usage_flags = graphics::IMAGE_USAGE::TRANSIENT_ATTACHMENT | graphics::IMAGE_USAGE::DEPTH_STENCIL_ATTACHMENT;
        auto constexpr property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT /*| VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT*/;
        auto constexpr tiling = graphics::IMAGE_TILING::OPTIMAL;
        auto constexpr aspect_flags = graphics::IMAGE_ASPECT::DEPTH_BIT;

        auto format = find_supported_image_format(
            device,
            {graphics::FORMAT::D32_SFLOAT, graphics::FORMAT::D32_SFLOAT_S8_UINT, graphics::FORMAT::D24_UNORM_S8_UINT},
            tiling,
            graphics::FORMAT_FEATURE::DEPTH_STENCIL_ATTACHMENT
        );

        if (!format)
            throw std::runtime_error("failed to find supported depth format"s);

        auto image = resource_manager.CreateImage(*format, width, height, mip_levels, samples_count, tiling, usage_flags, property_flags);

        if (image == nullptr)
            throw std::runtime_error("failed to create image for the depth attachment"s);

        auto image_view = resource_manager.CreateImageView(image, view_type, convert_to::vulkan(aspect_flags));

        if (image_view == nullptr)
            throw std::runtime_error("failed to create image view for the depth attachment"s);

        attachments.push_back(graphics::depth_attachment{*format, tiling, mip_levels, samples_count, image, image_view});
    }

    return attachments;
}

std::vector<graphics::attachment_description>
create_attachment_descriptions(std::vector<graphics::attachment> const &attachments)
{
    auto [color_attachment_format, color_attachment_samples_count] = std::visit([] (auto &&attachment)
    {
        return std::pair{attachment.format, attachment.samples_count};

    }, attachments.at(0));

    auto [depth_attachment_format, depth_attachment_samples_count] = std::visit([] (auto &&attachment)
    {
        return std::pair{attachment.format, attachment.samples_count};

    }, attachments.at(1));

    return std::vector{
        graphics::attachment_description{
            color_attachment_format,
            color_attachment_samples_count,
            graphics::ATTACHMENT_LOAD_TREATMENT::CLEAR,
            graphics::ATTACHMENT_STORE_TREATMENT::DONT_CARE,
            graphics::IMAGE_LAYOUT::UNDEFINED,
            graphics::IMAGE_LAYOUT::COLOR_ATTACHMENT
        },
        graphics::attachment_description{
            depth_attachment_format,
            depth_attachment_samples_count,
            graphics::ATTACHMENT_LOAD_TREATMENT::CLEAR,
            graphics::ATTACHMENT_STORE_TREATMENT::DONT_CARE,
            graphics::IMAGE_LAYOUT::UNDEFINED,
            graphics::IMAGE_LAYOUT::DEPTH_STENCIL_ATTACHMENT
        }
    };
}

std::shared_ptr<graphics::render_pass>
create_render_pass(graphics::render_pass_manager &render_pass_manager,
                   renderer::surface_format surface_format, std::vector<graphics::attachment_description> attachment_descriptions)
{
    attachment_descriptions.push_back(
        graphics::attachment_description{
            surface_format.format,
            1,
            graphics::ATTACHMENT_LOAD_TREATMENT::DONT_CARE,
            graphics::ATTACHMENT_STORE_TREATMENT::STORE,
            graphics::IMAGE_LAYOUT::UNDEFINED,
            graphics::IMAGE_LAYOUT::PRESENT_SOURCE
        }
    );
    
    return render_pass_manager.create_render_pass(
        attachment_descriptions,
        std::vector{
            graphics::subpass_description{
                { },
                {
                    graphics::attachment_reference{
                        0, 0, graphics::IMAGE_LAYOUT::COLOR_ATTACHMENT
                    }
                },
                {
                    graphics::attachment_reference{
                        0, 2, graphics::IMAGE_LAYOUT::COLOR_ATTACHMENT
                    }
                },
                graphics::attachment_reference{
                    0, 1, graphics::IMAGE_LAYOUT::DEPTH_STENCIL_ATTACHMENT
                },
                { }
            }
        },
        std::vector{
            graphics::subpass_dependency{
                std::nullopt, 0,
                graphics::PIPELINE_STAGE::COLOR_ATTACHMENT_OUTPUT,
                graphics::PIPELINE_STAGE::COLOR_ATTACHMENT_OUTPUT,
                std::nullopt,
                graphics::MEMORY_ACCESS_TYPE::COLOR_ATTACHMENT_READ | graphics::MEMORY_ACCESS_TYPE::COLOR_ATTACHMENT_WRITE
            }
        }
    );
}

std::vector<std::shared_ptr<resource::framebuffer>>
create_framebuffers(resource::resource_manager &resource_manager, renderer::swapchain const &swapchain,
                    std::shared_ptr<graphics::render_pass> render_pass, std::vector<graphics::attachment> const &attachments)
{
    std::vector<std::shared_ptr<resource::framebuffer>> framebuffers;

    auto &&swapchain_views = swapchain.image_views();
    auto extent = swapchain.extent();

    std::vector<std::shared_ptr<resource::image_view>> image_views;

    for (auto attachment : attachments) {
        std::visit([&image_views] (auto &&attachment)
        {
            image_views.push_back(attachment.image_view);

        }, std::move(attachment));
    }

    std::transform(std::cbegin(swapchain_views), std::cend(swapchain_views), std::back_inserter(framebuffers), [&] (auto &&swapchain_view)
    {
        auto _image_views = image_views;
        _image_views.push_back(swapchain_view);

        return resource_manager.create_framebuffer(extent, render_pass, _image_views);
    });

    return framebuffers;
}


void init(platform::window &window, app_t &app)
{
    app.vulkan_instance = std::make_unique<vulkan::instance>();

    app.platform_surface = std::make_unique<renderer::platform_surface>(*app.vulkan_instance, window);

    app.device = std::make_unique<vulkan::device>(*app.vulkan_instance, app.platform_surface.get());

    app.renderer_config = adjust_renderer_config(*app.device);

    app.memory_manager = std::make_unique<MemoryManager>(*app.device);
    app.resource_manager2 = std::make_unique<ResourceManager>(*app.device, *app.memory_manager);

    app.resource_manager = std::make_unique<resource::resource_manager>(*app.device);

    app.shader_manager = std::make_unique<graphics::shader_manager>(*app.device);
    app.material_factory = std::make_unique<graphics::material_factory>();
    app.vertex_input_state_manager = std::make_unique<graphics::vertex_input_state_manager>();
    app.pipeline_factory = std::make_unique<graphics::pipeline_factory>(*app.device, app.renderer_config, *app.shader_manager);

    app.render_pass_manager = std::make_unique<graphics::render_pass_manager>(*app.device);

    if (auto commandPool = CreateCommandPool(*app.device, app.device->transfer_queue, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT); commandPool)
        app.transferCommandPool = *commandPool;

    else throw std::runtime_error("failed to transfer command pool"s);

    if (auto commandPool = CreateCommandPool(*app.device, app.device->graphics_queue, 0); commandPool)
        app.graphicsCommandPool = *commandPool;

    else throw std::runtime_error("failed to graphics command pool"s);

    {
        app.swapchain = create_swapchain(*app.device, *app.platform_surface, renderer::extent{app.width, app.height});

        if (app.swapchain == nullptr)
            throw std::runtime_error("failed to create the swapchain"s);

        auto attachments = create_attachments(*app.device, app.renderer_config, *app.resource_manager2, *app.swapchain);

        if (attachments.empty())
            throw std::runtime_error("failed to create the attachments"s);

        auto attachment_descriptions = create_attachment_descriptions(attachments);

        app.render_pass = create_render_pass(*app.render_pass_manager, app.swapchain->surface_format(), attachment_descriptions);

        if (app.render_pass == nullptr)
            throw std::runtime_error("failed to create the render pass"s);

        auto framebuffers = create_framebuffers(*app.resource_manager, *app.swapchain, app.render_pass, attachments);

        if (framebuffers.empty())
            throw std::runtime_error("failed to create the framebuffers"s);

        app.attachments = std::move(attachments);
        app.framebuffers = std::move(framebuffers);
    }

    if (auto descriptorSetLayout = CreateDescriptorSetLayout(*app.device); !descriptorSetLayout)
        throw std::runtime_error("failed to create the descriptor set layout"s);

    else app.descriptorSetLayout = std::move(descriptorSetLayout.value());

#if TEMPORARILY_DISABLED
    if (auto result = glTF::load(sceneName, app.scene, app.nodeSystem); !result)
        throw std::runtime_error("failed to load a mesh"s);
#endif

    if (auto pipelineLayout = create_pipeline_layout(*app.device, std::array{app.descriptorSetLayout}); !pipelineLayout)
        throw std::runtime_error("failed to create the pipeline layout"s);

    else app.pipelineLayout = std::move(pipelineLayout.value());

#if TEMPORARILY_DISABLED
    // "chalet/textures/chalet.tga"sv
    // "Hebe/textures/HebehebemissinSG1_metallicRoughness.tga"sv
    if (auto result = load_texture(app, *app.device, "sponza/textures/sponza_curtain_blue_diff.tga"sv); !result)
        throw std::runtime_error("failed to load a texture"s);

    else app.texture = std::move(result.value());

    if (auto result = app.device->resource_manager2().CreateImageSampler(app.texture.image->mip_levels()); !result)
        throw std::runtime_error("failed to create a texture sampler"s);

    else app.texture.sampler = result;
#endif

    auto alignment = static_cast<std::size_t>(app.device->device_limits().min_storage_buffer_offset_alignment);

    app.alignedBufferSize = aligned_size(sizeof(per_object_t), alignment) * app.objectsNumber;
    app.alignedBuffer = boost::alignment::aligned_alloc(alignment, app.alignedBufferSize);

    app.objects.resize(app.objectsNumber);

    if (app.perObjectBuffer = CreateStorageBuffer(*app.resource_manager2, app.alignedBufferSize); app.perObjectBuffer) {
        auto &&buffer = *app.perObjectBuffer;

        auto offset = buffer.memory()->offset();
        auto size = buffer.memory()->size();

        if (auto result = vkMapMemory(app.device->handle(), buffer.memory()->handle(), offset, size, 0, &app.perObjectsMappedPtr); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to map per object uniform buffer memory: {0:#x}\n"s, result));
    }

    else throw std::runtime_error("failed to init per object uniform buffer"s);

    if (app.perCameraBuffer = CreateCoherentStorageBuffer(*app.resource_manager2, sizeof(camera::data_t)); !app.perCameraBuffer)
        throw std::runtime_error("failed to init per camera uniform buffer"s);

    if (auto descriptorPool = CreateDescriptorPool(*app.device); !descriptorPool)
        throw std::runtime_error("failed to create the descriptor pool"s);

    else app.descriptorPool = std::move(descriptorPool.value());

    if (auto descriptorSet = CreateDescriptorSet(*app.device, app.descriptorPool, std::array{app.descriptorSetLayout}); !descriptorSet)
        throw std::runtime_error("failed to create the descriptor pool"s);

    else app.descriptorSet = std::move(descriptorSet.value());

    update_descriptor_set(app, *app.device, app.descriptorSet);

    temp::model = temp::populate();

    build_render_pipelines(app, temp::model);

    app.resource_manager2->TransferStagedVertexData(app.transferCommandPool, app.device->transfer_queue);

    create_graphics_command_buffers(app);

    create_semaphores(app);
}

void update(app_t &app)
{
    app.camera_controller->update();
    app.cameraSystem.update();

    auto &&device = *app.device;

    {
        auto &&buffer = *app.perCameraBuffer;

        auto offset = buffer.memory()->offset();
        auto size = buffer.memory()->size();

        void *data;

        if (auto result = vkMapMemory(device.handle(), buffer.memory()->handle(), offset, size, 0, &data); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to map per camera uniform buffer memory: {0:#x}\n"s, result));

        std::uninitialized_copy_n(&app.camera->data, 1, reinterpret_cast<camera::data_t *>(data));

        vkUnmapMemory(device.handle(), buffer.memory()->handle());
    }

    std::size_t const stride = app.alignedBufferSize / app.objectsNumber;
    std::size_t instanceIndex = 0;

    for (auto &&object : app.objects) {
        object.world = glm::translate(glm::mat4{1.f}, glm::vec3{0, 0, -0.125f * static_cast<float>(instanceIndex)});
        //object.world = glm::rotate(object.world, glm::radians(-90.f), glm::vec3{1, 0, 0});
        //object.world = glm::scale(object.world, glm::vec3{.01f});

        object.normal = glm::inverseTranspose(object.world);

        ++instanceIndex;
    }

    auto it_begin = reinterpret_cast<decltype(app.objects)::value_type *>(app.alignedBuffer);

#ifdef _MSC_VER
    std::copy(std::execution::par_unseq, std::cbegin(app.objects), std::cend(app.objects), strided_forward_iterator{it_begin, stride});
#else
    std::copy(std::cbegin(app.objects), std::cend(app.objects), strided_forward_iterator{it_begin, stride});
#endif

    memcpy(app.perObjectsMappedPtr, app.alignedBuffer, app.alignedBufferSize);

    auto const mappedRanges = std::array{
        VkMappedMemoryRange{
            VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            nullptr,
            app.perObjectBuffer->memory()->handle(),
            app.perObjectBuffer->memory()->offset(),
            app.alignedBufferSize
        }
    };

    vkFlushMappedMemoryRanges(app.device->handle(), static_cast<std::uint32_t>(std::size(mappedRanges)), std::data(mappedRanges));
}

void render_frame(app_t &app)
{
    if (app.width < 1 || app.height < 1)
        return;

    auto &&device = *app.device;

    //vkQueueWaitIdle(device.presentation_queue.handle());

    std::uint32_t image_index;

    /*VkAcquireNextImageInfoKHR next_image_info{
        VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
        nullptr,
        app.swapchain->handle(),
        std::numeric_limits<std::uint64_t>::max(),
        app.imageAvailableSemaphore->handle(),
        VK_NULL_HANDLE
    };*/

    switch (auto result = vkAcquireNextImageKHR(device.handle(), app.swapchain->handle(), std::numeric_limits<std::uint64_t>::max(),
            app.imageAvailableSemaphore->handle(), VK_NULL_HANDLE, &image_index); result) {
        case VK_ERROR_OUT_OF_DATE_KHR:
            recreate_swap_chain(app);
            return;

        case VK_SUBOPTIMAL_KHR:
        case VK_SUCCESS:
            break;

        default:
            throw std::runtime_error(fmt::format("failed to acquire next image index: {0:#x}\n"s, result));
    }

    auto const wait_semaphores = std::array{ app.imageAvailableSemaphore->handle() };
    auto const signal_semaphores = std::array{ app.renderFinishedSemaphore->handle() };

    std::array<VkPipelineStageFlags, 1> constexpr wait_stages{
        { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }
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
        throw std::runtime_error(fmt::format("failed to submit draw command buffer: {0:#x}\n"s, result));

    auto swapchain_handle = app.swapchain->handle();

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
            throw std::runtime_error(fmt::format("failed to submit request to present framebuffer: {0:#x}\n"s, result));
    }
}

int main()
/*try */{
#if defined(_MSC_VER)
    _CrtSetDbgFlag(_CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#else
	std::signal(SIGSEGV, posix_signal_handler);
	std::signal(SIGTRAP, posix_signal_handler);
#endif

    if (auto result = glfwInit(); result != GLFW_TRUE)
        throw std::runtime_error(fmt::format("failed to init GLFW: {0:#x}\n"s, result));

    app_t app;

    platform::window window{"VulkanIsland"sv, static_cast<std::int32_t>(app.width), static_cast<std::int32_t>(app.height)};

    auto app_window_events_handler = std::make_shared<window_events_handler>(app);
    window.connect_event_handler(app_window_events_handler);

    auto input_manager = std::make_shared<platform::input_manager>();
    window.connect_input_handler(input_manager);

    app.camera = app.cameraSystem.create_camera();
    app.camera->aspect = static_cast<float>(app.width) / static_cast<float>(app.height);

    app.camera_controller = std::make_unique<orbit_controller>(app.camera, *input_manager);
    app.camera_controller->look_at(glm::vec3{0, 2, 1}, {0, 0, 0});

    std::cout << measure<>::execution(init, window, std::ref(app)) << " ms\n"s;

    window.update([&app]
    {
        glfwPollEvents();

        if (app.resize_callback) {
            app.resize_callback();
            app.resize_callback = nullptr;
        }

    #if TEMPORARILY_DISABLED
        app.registry.sort<ecs::node>(ecs::node());
        #if NOT_YET_IMPLEMENTED
            app.registry.sort<ecs::mesh>(ecs::mesh());
        #endif

            app.nodeSystem.update();
        #if NOT_YET_IMPLEMENTED
            app.meshSystem.update();
        #endif
    #endif
        update(app);

        render_frame(app);
    });

    app.clean_up();

    glfwTerminate();
}/* catch (std::exception const &ex) {
    std::cout << ex.what() << std::endl;
    std::cin.get();
}*/


void create_semaphores(app_t &app)
{
    auto &&resource_manager2 = *app.resource_manager2;

    if (auto semaphore = resource_manager2.create_semaphore(); !semaphore)
        throw std::runtime_error("failed to create image semaphore"s);

    else app.imageAvailableSemaphore = semaphore;

    if (auto semaphore = resource_manager2.create_semaphore(); !semaphore)
        throw std::runtime_error("failed to create render semaphore"s);

    else app.renderFinishedSemaphore = semaphore;
}

template<class T, typename std::enable_if_t<mpl::is_container_v<std::remove_cvref_t<T>>>...>
[[nodiscard]] std::shared_ptr<resource::buffer>
stage_data(vulkan::device &device, ResourceManager &resource_manager, T &&container)
{
    auto constexpr usageFlags = graphics::BUFFER_USAGE::TRANSFER_SOURCE;
    auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    using type = typename std::remove_cvref_t<T>::value_type;

    auto const bufferSize = static_cast<VkDeviceSize>(sizeof(type) * std::size(container));

    auto buffer = resource_manager.CreateBuffer(bufferSize, usageFlags, propertyFlags);

    if (buffer) {
        void *data;

        auto &&memory = buffer->memory();

        if (auto result = vkMapMemory(device.handle(), memory->handle(), memory->offset(), memory->size(), 0, &data); result != VK_SUCCESS)
            std::cerr << "failed to map staging buffer memory: "s << result << '\n';

        else {
            std::uninitialized_copy(std::begin(container), std::end(container), reinterpret_cast<type *>(data));

            vkUnmapMemory(device.handle(), buffer->memory()->handle());
        }
    }

    return buffer;
}

[[nodiscard]] std::shared_ptr<resource::texture>
load_texture(app_t &app, vulkan::device &device, ResourceManager &resource_manager, std::string_view name)
{
    std::shared_ptr<resource::texture> texture;

    auto constexpr generateMipMaps = true;

    if (auto rawImage = LoadTARGA(name); rawImage) {
        auto stagingBuffer = std::visit([&device, &resource_manager] (auto &&data)
        {
            return stage_data(device, resource_manager, std::forward<decltype(data)>(data));
        }, std::move(rawImage->data));

        if (stagingBuffer) {
            auto const width = static_cast<std::uint16_t>(rawImage->width);
            auto const height = static_cast<std::uint16_t>(rawImage->height);

            auto constexpr usageFlags = graphics::IMAGE_USAGE::TRANSFER_SOURCE | graphics::IMAGE_USAGE::TRANSFER_DESTINATION | graphics::IMAGE_USAGE::SAMPLED;
            auto constexpr propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            auto constexpr tiling = graphics::IMAGE_TILING::OPTIMAL;

            texture = CreateTexture(resource_manager, rawImage->format, rawImage->view_type, width, height, rawImage->mip_levels,
                                    1u, tiling, VK_IMAGE_ASPECT_COLOR_BIT, usageFlags, propertyFlags);

            if (texture) {
                TransitionImageLayout(device, device.transfer_queue, *texture->image, graphics::IMAGE_LAYOUT::UNDEFINED,
                                      graphics::IMAGE_LAYOUT::TRANSFER_DESTINATION, app.transferCommandPool);

                CopyBufferToImage(device, device.transfer_queue, stagingBuffer->handle(), texture->image->handle(), width, height, app.transferCommandPool);

                if (generateMipMaps)
                    GenerateMipMaps(device, device.transfer_queue, *texture->image, app.transferCommandPool);

                else TransitionImageLayout(device, device.transfer_queue, *texture->image, graphics::IMAGE_LAYOUT::TRANSFER_DESTINATION,
                                           graphics::IMAGE_LAYOUT::SHADER_READ_ONLY, app.transferCommandPool);
            }
        }
    }

    else std::cerr << "failed to load an image\n"s;

    return texture;
}
