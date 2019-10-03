#define _SCL_SECURE_NO_WARNINGS


#if defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
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
#include "utility/mpl.hxx"
#include "math.hxx"
#include "instance.hxx"
#include "device/device.hxx"
#include "swapchain.hxx"
#include "resources/program.hxx"
#include "resources/buffer.hxx"
#include "resources/image.hxx"
#include "resources/resource.hxx"
#include "resources/semaphore.hxx"
#include "descriptor.hxx"
#include "command_buffer.hxx"
#include "renderer/pipelines.hxx"
#include "renderer/graphics_pipeline.hxx"
#include "renderer/pipeline_states.hxx"
#include "renderer/renderPass.hxx"

#include "renderer/graphics.hxx"
#include "renderer/pipelineVertexInputState.hxx"
#include "renderer/material.hxx"

#include "renderer/vertex.hxx"
#include "renderer/render_flow.hxx"
#include "renderer/compatibility.hxx"

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

struct draw_command final {
    std::shared_ptr<graphics::material> material;
    std::shared_ptr<graphics::pipeline> pipeline;
    std::shared_ptr<VertexBuffer> vertex_buffer;

    std::uint32_t vertex_count{0};
    std::uint32_t first_vertex{0};

    VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
    VkRenderPass renderPass{VK_NULL_HANDLE};
    VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
};

struct app_t final {
    std::uint32_t width{800u};
    std::uint32_t height{600u};

    camera_system cameraSystem;
    std::shared_ptr<camera> camera;

    std::unique_ptr<orbit_controller> camera_controller;

    std::vector<per_object_t> objects;

    std::unique_ptr<VulkanInstance> vulkanInstance;
    std::unique_ptr<VulkanDevice> vulkanDevice;

    VkSurfaceKHR surface{VK_NULL_HANDLE};
    VulkanSwapchain swapchain;

    GraphicsQueue graphicsQueue;
    TransferQueue transferQueue;
    PresentationQueue presentationQueue;

    VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
    VkRenderPass renderPass{VK_NULL_HANDLE};

    VkCommandPool graphicsCommandPool{VK_NULL_HANDLE}, transferCommandPool{VK_NULL_HANDLE};

    VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};
    VkDescriptorPool descriptorPool{VK_NULL_HANDLE};
    VkDescriptorSet descriptorSet{VK_NULL_HANDLE};

    std::vector<VkCommandBuffer> command_buffers;

    std::shared_ptr<resource::semaphore> imageAvailableSemaphore, renderFinishedSemaphore;

    std::shared_ptr<VulkanBuffer> perObjectBuffer, perCameraBuffer;
    void *perObjectsMappedPtr{nullptr};
    void *alignedBuffer{nullptr};

    std::size_t objectsNumber{2u};
    std::size_t alignedBufferSize{0u};

    VulkanTexture texture;

#if TEMPORARILY_DISABLED
    ecs::entity_registry registry;

    ecs::NodeSystem nodeSystem{registry};
#if NOT_YET_IMPLEMENTED
    ecs::MeshSystem meshSystem{registry};
#endif
#endif

    PipelineVertexInputStatesManager pipelineVertexInputStatesManager;

    std::unique_ptr<MaterialFactory> materialFactory;
    std::unique_ptr<ShaderManager> shaderManager;
    std::unique_ptr<GraphicsPipelineManager> graphicsPipelineManager;

    std::unique_ptr<graphics::shader_manager> shader_manager;
    std::unique_ptr<graphics::material_factory> material_factory;
    std::unique_ptr<graphics::vertex_input_state_manager> vertex_input_state_manager;
    std::unique_ptr<graphics::pipeline_factory> pipeline_factory;

    std::vector<draw_command> draw_commands;

    ~app_t()
    {
        clean_up();
    }

    void clean_up()
    {
        if (vulkanDevice == nullptr)
            return;

        vkDeviceWaitIdle(vulkanDevice->handle());

        draw_commands.clear();

        cleanup_frame_data(*this);

        if (materialFactory)
            materialFactory.reset();

        if (shaderManager)
            shaderManager.reset();

        if (renderFinishedSemaphore)
            renderFinishedSemaphore.reset();

        if (imageAvailableSemaphore)
            imageAvailableSemaphore.reset();

        if (pipeline_factory)
            pipeline_factory.reset();

        if (vertex_input_state_manager)
            vertex_input_state_manager.reset();

        if (material_factory)
            material_factory.reset();

        if (shader_manager)
            shader_manager.reset();

        if (graphicsPipelineManager)
            graphicsPipelineManager.reset();

        if (pipelineLayout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(vulkanDevice->handle(), pipelineLayout, nullptr);

        vkDestroyDescriptorSetLayout(vulkanDevice->handle(), descriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(vulkanDevice->handle(), descriptorPool, nullptr);

    #if USE_DYNAMIC_PIPELINE_STATE
        if (renderPass != VK_NULL_HANDLE)
            vkDestroyRenderPass(vulkanDevice->handle(), renderPass, nullptr);
    #endif

        texture.sampler.reset();
        if (texture.view.handle() != VK_NULL_HANDLE)
            vkDestroyImageView(vulkanDevice->handle(), texture.view.handle(), nullptr);
        texture.image.reset();

        if (perObjectsMappedPtr)
            vkUnmapMemory(vulkanDevice->handle(), perObjectBuffer->memory()->handle());

        if (alignedBuffer)
            boost::alignment::aligned_free(alignedBuffer);

        perCameraBuffer.reset();
        perObjectBuffer.reset();

        if (transferCommandPool != VK_NULL_HANDLE)
            vkDestroyCommandPool(vulkanDevice->handle(), transferCommandPool, nullptr);

        if (graphicsCommandPool != VK_NULL_HANDLE)
            vkDestroyCommandPool(vulkanDevice->handle(), graphicsCommandPool, nullptr);

        if (surface != VK_NULL_HANDLE)
            vkDestroySurfaceKHR(vulkanInstance->handle(), surface, nullptr);

        vulkanDevice.reset();
        vulkanInstance.reset();
    }
};


void recreate_swap_chain(app_t &app);

template<class T> requires mpl::container<std::remove_cvref_t<T>>
[[nodiscard]] std::shared_ptr<VulkanBuffer> stage_data(VulkanDevice &device, T &&container);

[[nodiscard]] std::optional<VulkanTexture>
load_texture(app_t &app, VulkanDevice &device, std::string_view name);


struct window_events_handler final : public platform::window::event_handler_interface {

    window_events_handler(app_t &app) : app{app} { }

    app_t &app;

    void on_resize(std::int32_t width, std::int32_t height) override
    {
        app.width = static_cast<std::uint32_t>(width);
        app.height = static_cast<std::uint32_t>(height);

        recreate_swap_chain(app);

        app.camera->aspect = static_cast<float>(width) / static_cast<float>(height);
    }
};


void update_descriptor_set(app_t &app, VulkanDevice const &device, VkDescriptorSet &descriptorSet)
{
    // TODO: descriptor info typed by VkDescriptorType.
    auto const cameras = std::array{
        VkDescriptorBufferInfo{app.perCameraBuffer->handle(), 0, sizeof(camera::data_t)}
    };

    // TODO: descriptor info typed by VkDescriptorType.
    auto const objects = std::array{
        VkDescriptorBufferInfo{app.perObjectBuffer->handle(), 0, sizeof(per_object_t)}
    };

#if TEMPORARILY_DISABLED
    // TODO: descriptor info typed by VkDescriptorType.
    auto const images = std::array{
        VkDescriptorImageInfo{app.texture.sampler->handle(), app.texture.view.handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
    };
#endif

    std::array<VkWriteDescriptorSet, 2> const writeDescriptorsSet{{
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptorSet,
            0,
            0, static_cast<std::uint32_t>(std::size(cameras)),
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            nullptr,
            std::data(cameras),
            nullptr
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptorSet,
            1,
            0, static_cast<std::uint32_t>(std::size(objects)),
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
            nullptr,
            std::data(objects),
            nullptr
        },
#if TEMPORARILY_DISABLED
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptorSet,
            2,
            0, static_cast<std::uint32_t>(std::size(images)),
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            std::data(images),
            nullptr,
            nullptr
        }
#endif
    }};

    // WARN:: remember about potential race condition with the related executing command buffer
    vkUpdateDescriptorSets(device.handle(), static_cast<std::uint32_t>(std::size(writeDescriptorsSet)),
                           std::data(writeDescriptorsSet), 0, nullptr);
}

void create_graphics_command_buffers(app_t &app)
{
    app.command_buffers.resize(std::size(app.swapchain.framebuffers));

    VkCommandBufferAllocateInfo const allocate_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        app.graphicsCommandPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        static_cast<std::uint32_t>(std::size(app.command_buffers))
    };

    if (auto result = vkAllocateCommandBuffers(app.vulkanDevice->handle(), &allocate_info, std::data(app.command_buffers)); result != VK_SUCCESS)
        throw std::runtime_error(fmt::format("failed to create allocate command buffers: {0:#x}\n"s, result));

    auto &&resource_manager = app.vulkanDevice->resourceManager();
    auto &&vertex_buffers = resource_manager.vertex_buffers();

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
                               std::back_inserter(vertex_buffer_handles), [] (auto &&pair)
    {
        return pair.second;
    });

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

        VkRenderPassBeginInfo const renderPassInfo{
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            nullptr,
            app.renderPass,
            app.swapchain.framebuffers.at(i++),
            {{0, 0}, app.swapchain.extent},
            static_cast<std::uint32_t>(std::size(clear_colors)), std::data(clear_colors)
        };

        vkCmdBeginRenderPass(command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    #if USE_DYNAMIC_PIPELINE_STATE
        VkViewport const viewport{
            0, static_cast<float>(app.swapchain.extent.height),
            static_cast<float>(app.swapchain.extent.width), -static_cast<float>(app.swapchain.extent.height),
            0, 1
        };

        VkRect2D const scissor{
            {0, 0}, app.swapchain.extent
        };

        vkCmdSetViewport(command_buffer, 0, 1, &viewport);
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);
    #endif

        vkCmdBindVertexBuffers(command_buffer, first_binding, binding_count, std::data(vertex_buffer_handles), std::data(vertex_buffer_offsets));

        for (auto &&draw_command : app.draw_commands) {
            auto [
                material, pipeline, vertex_buffer,
                vertex_count, first_vertex,
                pipelineLayout, renderPass, descriptorSet
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

void create_semaphores(app_t &app)
{
    auto &&resourceManager = app.vulkanDevice->resourceManager();

    if (auto semaphore = resourceManager.create_semaphore(); !semaphore)
        throw std::runtime_error("failed to create image semaphore"s);

    else app.imageAvailableSemaphore = semaphore;

    if (auto semaphore = resourceManager.create_semaphore(); !semaphore)
        throw std::runtime_error("failed to create render semaphore"s);

    else app.renderFinishedSemaphore = semaphore;
}

void cleanup_frame_data(app_t &app)
{
    auto &&device = *app.vulkanDevice;

    if (app.graphicsCommandPool)
        vkFreeCommandBuffers(device.handle(), app.graphicsCommandPool, static_cast<std::uint32_t>(std::size(app.command_buffers)), std::data(app.command_buffers));

    app.command_buffers.clear();

#if !USE_DYNAMIC_PIPELINE_STATE
    if (app.renderPass != VK_NULL_HANDLE)
        vkDestroyRenderPass(device.handle(), app.renderPass, nullptr);
#endif

    CleanupSwapchain(device, app.swapchain);
}

void recreate_swap_chain(app_t &app)
{
    if (app.width < 1 || app.height < 1) return;

    vkDeviceWaitIdle(app.vulkanDevice->handle());

    cleanup_frame_data(app);

    auto swapchain = CreateSwapchain(*app.vulkanDevice, app.surface, app.width, app.height,
                                     app.presentationQueue, app.graphicsQueue, app.transferQueue, app.transferCommandPool);

    if (swapchain)
        app.swapchain = std::move(swapchain.value());

    else throw std::runtime_error("failed to create the swapchain"s);

#if !USE_DYNAMIC_PIPELINE_STATE
    if (auto renderPass = CreateRenderPass(*app.vulkanDevice, app.swapchain); !renderPass)
        throw std::runtime_error("failed to create the render pass"s);

    else app.renderPass = std::move(renderPass.value());

    CreateGraphicsPipelines(app);
#endif

    CreateFramebuffers(*app.vulkanDevice, app.renderPass, app.swapchain);

    create_graphics_command_buffers(app);
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
            vertex::static_array<2, boost::float32_t> texCoord;
            //vertex::static_array<3, boost::float32_t> normal;
        };

        auto const vertex_layout_index = std::size(_model.vertex_layouts);

        _model.vertex_layouts.push_back(
            create_vertex_layout(
                vertex::position{}, decltype(vertex_struct::position){}, false,
                vertex::tex_coord_0{}, decltype(vertex_struct::texCoord){}, false
                //vertex::normal{}, decltype(vertex_struct::normal){}, false
            )
        );

        std::vector<vertex_struct> vertices;

        vertices.push_back(vertex_struct{
            {{0.f, 0.f, 0.f}}, {{.5f, .5f}}//, {{ 0.f, 1.f, 0.f }}
        });

        vertices.push_back(vertex_struct{
            {{-1.f, 0.f, 1.f}}, {{0.f, 0.f}}//, {{ 0.f, 1.f, 0.f }}
        });

        vertices.push_back(vertex_struct{
            {{0.f, 0.f, 1.f}}, {{1.f, 0.f}}//, {{ 0.f, 1.f, 0.f }}
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

        meshlet.material_index = 0;
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
            create_vertex_layout(
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
            create_vertex_layout(
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
            meshlet.first_vertex = static_cast<std::uint32_t>(vertexBuffer.count);

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

    auto &&resource_manager = app.vulkanDevice->resourceManager();

    for (auto &&meshlet : _model.non_indexed_meshlets) {
        auto material_index = meshlet.material_index;
        auto [technique_index, name] = _model.materials[material_index];

        std::cout << fmt::format("{}#{}\n"s, name, technique_index);

        auto material = material_factory.material(name, technique_index);

        auto vertex_layout_index = meshlet.vertex_buffer_index;
        auto &&vertex_layout = _model.vertex_layouts[vertex_layout_index];

        auto &&vertex_data_buffer = _model.vertex_buffers.at(vertex_layout_index);

        auto vertex_buffer = resource_manager.CreateVertexBuffer(vertex_layout, std::size(vertex_data_buffer.buffer));

        if (vertex_buffer)
            resource_manager.StageVertexData(vertex_buffer, vertex_data_buffer.buffer);

        else throw std::runtime_error("failed to get vertex buffer"s);

        auto &&vertex_input_state = vertex_input_state_manager.vertex_input_state(vertex_layout);

        auto adjusted_vertex_input_state = vertex_input_state_manager.get_adjusted_vertex_input_state(vertex_layout, material->vertex_layout);

        auto primitive_topology = meshlet.topology;

        graphics::pipeline_states pipeline_states{
            primitive_topology,
            adjusted_vertex_input_state,
            rasterization_state,
            depth_stencil_state,
            color_blend_state
        };

        auto pipeline = pipeline_factory.create_pipeline(material, pipeline_states, app.pipelineLayout, app.renderPass, 0u);

        app.draw_commands.push_back(
            draw_command{
                material, pipeline, vertex_buffer,
                meshlet.vertex_count, meshlet.first_vertex,
                app.pipelineLayout, app.renderPass, app.descriptorSet
            }
        );
    }
}
}


void init_vulkan(platform::window &window, app_t &app)
{
    app.vulkanInstance = std::make_unique<VulkanInstance>(config::extensions, config::layers);

#if USE_WIN32
    VkWin32SurfaceCreateInfoKHR const win32CreateInfo{
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        nullptr, 0,
        GetModuleHandle(nullptr),
        glfwGetWin32Window(window.handle())
    };

    vkCreateWin32SurfaceKHR(app.vulkanInstance->handle(), &win32CreateInfo, nullptr, &app.surface);
#else
    if (auto result = glfwCreateWindowSurface(app.vulkanInstance->handle(), window.handle(), nullptr, &app.surface); result != VK_SUCCESS)
        throw std::runtime_error(fmt::format("failed to create window surface: {0:#x}\n"s, result));
#endif

    QueuePool<
        mpl::instances_number<GraphicsQueue>,
        mpl::instances_number<TransferQueue>,
        mpl::instances_number<PresentationQueue>
    > qpool;

    app.vulkanDevice = std::make_unique<VulkanDevice>(*app.vulkanInstance, app.surface, config::deviceExtensions, std::move(qpool));

    app.shaderManager = std::make_unique<ShaderManager>(*app.vulkanDevice);
    app.materialFactory = std::make_unique<MaterialFactory>(*app.shaderManager);
    app.graphicsPipelineManager = std::make_unique<GraphicsPipelineManager>(*app.vulkanDevice, *app.materialFactory, app.pipelineVertexInputStatesManager);

    app.shader_manager = std::make_unique<graphics::shader_manager>(*app.vulkanDevice);
    app.material_factory = std::make_unique<graphics::material_factory>();
    app.vertex_input_state_manager = std::make_unique<graphics::vertex_input_state_manager>();
    app.pipeline_factory = std::make_unique<graphics::pipeline_factory>(*app.vulkanDevice, *app.shader_manager);

    app.graphicsQueue = app.vulkanDevice->queue<GraphicsQueue>();
    app.transferQueue = app.vulkanDevice->queue<TransferQueue>();
    app.presentationQueue = app.vulkanDevice->queue<PresentationQueue>();

    if (auto commandPool = CreateCommandPool(app.vulkanDevice->handle(), app.transferQueue, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT); commandPool)
        app.transferCommandPool = *commandPool;

    else throw std::runtime_error("failed to transfer command pool"s);

    if (auto commandPool = CreateCommandPool(app.vulkanDevice->handle(), app.graphicsQueue, 0); commandPool)
        app.graphicsCommandPool = *commandPool;

    else throw std::runtime_error("failed to graphics command pool"s);

    auto swapchain = CreateSwapchain(*app.vulkanDevice, app.surface, app.width, app.height,
                                     app.presentationQueue, app.graphicsQueue, app.transferQueue, app.transferCommandPool);

    if (swapchain)
        app.swapchain = std::move(swapchain.value());

    else throw std::runtime_error("failed to create the swapchain"s);

    if (auto descriptorSetLayout = CreateDescriptorSetLayout(*app.vulkanDevice); !descriptorSetLayout)
        throw std::runtime_error("failed to create the descriptor set layout"s);

    else app.descriptorSetLayout = std::move(descriptorSetLayout.value());

    if (auto renderPass = CreateRenderPass(*app.vulkanDevice, app.swapchain); !renderPass)
        throw std::runtime_error("failed to create the render pass"s);

    else app.renderPass = std::move(renderPass.value());

#if TEMPORARILY_DISABLED
    if (auto result = glTF::load(sceneName, app.scene, app.nodeSystem); !result)
        throw std::runtime_error("failed to load a mesh"s);
#endif

    if (auto pipelineLayout = CreatePipelineLayout(*app.vulkanDevice, std::array{app.descriptorSetLayout}); !pipelineLayout)
        throw std::runtime_error("failed to create the pipeline layout"s);

    else app.pipelineLayout = std::move(pipelineLayout.value());

#if TEMPORARILY_DISABLED
    // "chalet/textures/chalet.tga"sv
    // "Hebe/textures/HebehebemissinSG1_metallicRoughness.tga"sv
    if (auto result = load_texture(app, *app.vulkanDevice, "sponza/textures/sponza_curtain_blue_diff.tga"sv); !result)
        throw std::runtime_error("failed to load a texture"s);

    else app.texture = std::move(result.value());

    if (auto result = app.vulkanDevice->resourceManager().CreateImageSampler(app.texture.image->mipLevels()); !result)
        throw std::runtime_error("failed to create a texture sampler"s);

    else app.texture.sampler = result;
#endif

    auto alignment = static_cast<std::size_t>(app.vulkanDevice->properties().limits.minStorageBufferOffsetAlignment);

    app.alignedBufferSize = aligned_size(sizeof(per_object_t), alignment) * app.objectsNumber;

    app.alignedBuffer = boost::alignment::aligned_alloc(alignment, app.alignedBufferSize);

    app.objects.resize(app.objectsNumber);

    if (app.perObjectBuffer = CreateStorageBuffer(*app.vulkanDevice, app.alignedBufferSize); app.perObjectBuffer) {
        auto &&buffer = *app.perObjectBuffer;

        auto offset = buffer.memory()->offset();
        auto size = buffer.memory()->size();

        if (auto result = vkMapMemory(app.vulkanDevice->handle(), buffer.memory()->handle(), offset, size, 0, &app.perObjectsMappedPtr); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to map per object uniform buffer memory: {0:#x}\n"s, result));
    }

    else throw std::runtime_error("failed to init per object uniform buffer"s);

    if (app.perCameraBuffer = CreateCoherentStorageBuffer(*app.vulkanDevice, sizeof(camera::data_t)); !app.perCameraBuffer)
        throw std::runtime_error("failed to init per camera uniform buffer"s);

    if (auto descriptorPool = CreateDescriptorPool(*app.vulkanDevice); !descriptorPool)
        throw std::runtime_error("failed to create the descriptor pool"s);

    else app.descriptorPool = std::move(descriptorPool.value());

    if (auto descriptorSet = CreateDescriptorSet(*app.vulkanDevice, app.descriptorPool, std::array{app.descriptorSetLayout}); !descriptorSet)
        throw std::runtime_error("failed to create the descriptor pool"s);

    else app.descriptorSet = std::move(descriptorSet.value());

    update_descriptor_set(app, *app.vulkanDevice, app.descriptorSet);

    temp::model = temp::populate();
    temp::build_render_pipelines(app, temp::model);

    app.vulkanDevice->resourceManager().TransferStagedVertexData(app.transferCommandPool, app.transferQueue);

    CreateFramebuffers(*app.vulkanDevice, app.renderPass, app.swapchain);

    create_graphics_command_buffers(app);

    create_semaphores(app);
}

void update(app_t &app)
{
    app.camera_controller->update();
    app.cameraSystem.update();

    auto &&device = *app.vulkanDevice;

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

    vkFlushMappedMemoryRanges(app.vulkanDevice->handle(), static_cast<std::uint32_t>(std::size(mappedRanges)), std::data(mappedRanges));
}

void render_frame(app_t &app)
{
    auto &&vulkanDevice = *app.vulkanDevice;

    vkQueueWaitIdle(app.presentationQueue.handle());

    std::uint32_t imageIndex;

    switch (auto result = vkAcquireNextImageKHR(vulkanDevice.handle(), app.swapchain.handle, std::numeric_limits<std::uint64_t>::max(),
            app.imageAvailableSemaphore->handle(), VK_NULL_HANDLE, &imageIndex); result) {
        case VK_ERROR_OUT_OF_DATE_KHR:
            recreate_swap_chain(app);
            return;

        case VK_SUBOPTIMAL_KHR:
        case VK_SUCCESS:
            break;

        default:
            throw std::runtime_error(fmt::format("failed to acquire next image index: {0:#x}\n"s, result));
    }

    auto const waitSemaphores = std::array{ app.imageAvailableSemaphore->handle() };
    auto const signalSemaphores = std::array{ app.renderFinishedSemaphore->handle() };

    std::array<VkPipelineStageFlags, 1> constexpr waitStages{
        { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }
    };

    VkSubmitInfo const submitInfo{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        static_cast<std::uint32_t>(std::size(waitSemaphores)), std::data(waitSemaphores),
        std::data(waitStages),
        1, &app.command_buffers.at(imageIndex),
        static_cast<std::uint32_t>(std::size(signalSemaphores)), std::data(signalSemaphores),
    };

    if (auto result = vkQueueSubmit(app.graphicsQueue.handle(), 1, &submitInfo, VK_NULL_HANDLE); result != VK_SUCCESS)
        throw std::runtime_error(fmt::format("failed to submit draw command buffer: {0:#x}\n"s, result));

    VkPresentInfoKHR const presentInfo{
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        nullptr,
        static_cast<std::uint32_t>(std::size(signalSemaphores)), std::data(signalSemaphores),
        1, &app.swapchain.handle,
        &imageIndex, nullptr
    };

    switch (auto result = vkQueuePresentKHR(app.presentationQueue.handle(), &presentInfo); result) {
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

    glfwInit();

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

    std::cout << measure<>::execution(init_vulkan, window, std::ref(app)) << " ms\n"s;

    /*auto root = app.registry.create();

    app.registry.assign<Transform>(root, glm::mat4{1}, glm::mat4{1});
    app.nodeSystem.attachNode(root, root, "root"sv);

    auto entityA = app.registry.create();

    app.registry.assign<Transform>(entityA, glm::mat4{1}, glm::mat4{1});
    app.nodeSystem.attachNode(root, entityA, "entityA"sv);

    auto entityB = app.registry.create();

    app.registry.assign<Transform>(entityB, glm::mat4{1}, glm::mat4{1});
    app.nodeSystem.attachNode(root, entityB, "entityB"sv);

    auto entityC = app.registry.create();

    app.registry.assign<Transform>(entityC, glm::mat4{1}, glm::mat4{1});
    app.nodeSystem.attachNode(entityA, entityC, "entityC"sv);

    auto entityD = app.registry.create();

    app.registry.assign<Transform>(entityD, glm::mat4{1}, glm::mat4{1});
    app.nodeSystem.attachNode(entityB, entityD, "entityD"sv);*/

    window.update([&app]
    {
        glfwPollEvents();

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


template<class T> requires mpl::container<std::remove_cvref_t<T>>
[[nodiscard]] std::shared_ptr<VulkanBuffer>
stage_data(VulkanDevice &device, T &&container)
{
    auto constexpr usageFlags = graphics::BUFFER_USAGE::TRANSFER_SOURCE;
    auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    using type = typename std::remove_cvref_t<T>::value_type;

    auto const bufferSize = static_cast<VkDeviceSize>(sizeof(type) * std::size(container));

    auto buffer = device.resourceManager().CreateBuffer(bufferSize, usageFlags, propertyFlags);

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

[[nodiscard]] std::optional<VulkanTexture>
load_texture(app_t &app, VulkanDevice &device, std::string_view name)
{
    std::optional<VulkanTexture> texture;

    auto constexpr generateMipMaps = true;

    if (auto rawImage = LoadTARGA(name); rawImage) {
        auto stagingBuffer = std::visit([&device] (auto &&data)
        {
            return stage_data(device, std::forward<decltype(data)>(data));
        }, std::move(rawImage->data));

        if (stagingBuffer) {
            auto const width = static_cast<std::uint16_t>(rawImage->width);
            auto const height = static_cast<std::uint16_t>(rawImage->height);

            auto constexpr usageFlags = graphics::IMAGE_USAGE::TRANSFER_SOURCE | graphics::IMAGE_USAGE::TRANSFER_DESTINATION | graphics::IMAGE_USAGE::SAMPLED;
            auto constexpr propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            auto constexpr tiling = graphics::IMAGE_TILING::OPTIMAL;

            texture = CreateTexture(device, rawImage->format, rawImage->view_type, width, height, rawImage->mipLevels,
                                    VK_SAMPLE_COUNT_1_BIT, tiling, VK_IMAGE_ASPECT_COLOR_BIT, usageFlags, propertyFlags);

            if (texture) {
                TransitionImageLayout(device, app.transferQueue, *texture->image, graphics::IMAGE_LAYOUT::UNDEFINED,
                                      graphics::IMAGE_LAYOUT::TRANSFER_DESTINATION, app.transferCommandPool);

                CopyBufferToImage(device, app.transferQueue, stagingBuffer->handle(), texture->image->handle(), width, height, app.transferCommandPool);

                if (generateMipMaps)
                    GenerateMipMaps(device, app.transferQueue, *texture->image, app.transferCommandPool);

                else TransitionImageLayout(device, app.transferQueue, *texture->image, graphics::IMAGE_LAYOUT::TRANSFER_DESTINATION,
                                           graphics::IMAGE_LAYOUT::SHADER_READ_ONLY, app.transferCommandPool);
            }
        }
    }

    else std::cerr << "failed to load an image\n"s;

    return texture;
}
