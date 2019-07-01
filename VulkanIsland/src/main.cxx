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

#include <boost/align/aligned_alloc.hpp>

#include "main.hxx"
#include "math.hxx"
#include "instance.hxx"
#include "device/device.hxx"
#include "swapchain.hxx"
#include "resources/program.hxx"
#include "resources/buffer.hxx"
#include "resources/image.hxx"
#include "resources/resource.hxx"
#include "descriptor.hxx"
#include "commandBuffer.hxx"
#include "renderer/pipelines.hxx"
#include "renderer/renderPass.hxx"
#include "semaphore.hxx"

#include "renderer/pipelineVertexInputState.hxx"
#include "renderer/material.hxx"

#include "ecs/ecs.hxx"
#include "ecs/node.hxx"
#include "ecs/mesh.hxx"
#include "ecs/transform.hxx"

#include "loaders/loaderGLTF.hxx"
#include "loaders/loaderTARGA.hxx"

#include "staging.hxx"
#include "sceneTree.hxx"

#include "input/inputManager.hxx"
#include "camera/cameraController.hxx"


auto constexpr sceneName{"unlit-test"sv};

namespace temp {
xformat model;
}

struct per_object_t final {
    glm::mat4 world{1};
    glm::mat4 normal{1};  // Transposed of the inversed of the upper left 3x3 sub-matrix of model(world)-view matrix.
};

struct renderable_t final {
    PRIMITIVE_TOPOLOGY topology;

    std::shared_ptr<Material> material;
    std::shared_ptr<VertexBuffer> vertexBuffer;

    std::uint32_t vertexCount{0};
    std::uint32_t firstVertex{0};
};

std::vector<renderable_t> renderables;


void CleanupFrameData(struct app_t &app);


struct app_t final {
    std::uint32_t width{800u};
    std::uint32_t height{600u};

    CameraSystem cameraSystem;
    std::shared_ptr<Camera> camera;

    std::unique_ptr<OrbitController> cameraController;

    std::vector<per_object_t> objects;

    staging::scene_t scene;

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

    std::vector<VkCommandBuffer> commandBuffers;

    VkSemaphore imageAvailableSemaphore{VK_NULL_HANDLE}, renderFinishedSemaphore{VK_NULL_HANDLE};

    std::shared_ptr<VulkanBuffer> perObjectBuffer, perCameraBuffer;
    void *perObjectsMappedPtr{nullptr};
    void *alignedBuffer{nullptr};

    std::size_t objectsNumber{2u};
    std::size_t alignedBufferSize{0u};

    VulkanTexture texture;

    ecs::entity_registry registry;

    ecs::NodeSystem nodeSystem{registry};
#if NOT_YET_IMPLEMENTED
    ecs::MeshSystem meshSystem{registry};
#endif

    PipelineVertexInputStatesManager pipelineVertexInputStatesManager;

    std::unique_ptr<MaterialFactory> materialFactory;
    std::unique_ptr<ShaderManager> shaderManager;
    std::unique_ptr<GraphicsPipelineManager> graphicsPipelineManager;

    ~app_t()
    {
        cleanUp();
    }

    void cleanUp()
    {
        if (vulkanDevice == nullptr)
            return;

        vkDeviceWaitIdle(vulkanDevice->handle());

        renderables.clear();

        CleanupFrameData(*this);

        if (materialFactory)
            materialFactory.reset();

        if (shaderManager)
            shaderManager.reset();

        if (renderFinishedSemaphore)
            vkDestroySemaphore(vulkanDevice->handle(), renderFinishedSemaphore, nullptr);

        if (imageAvailableSemaphore)
            vkDestroySemaphore(vulkanDevice->handle(), imageAvailableSemaphore, nullptr);

        if (graphicsPipelineManager)
            graphicsPipelineManager.reset();

        if (pipelineLayout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(vulkanDevice->handle(), pipelineLayout, nullptr);

        vkDestroyDescriptorSetLayout(vulkanDevice->handle(), descriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(vulkanDevice->handle(), descriptorPool, nullptr);

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


void RecreateSwapChain(app_t &app);

template<class T, typename std::enable_if_t<is_container_v<std::decay_t<T>>>...>
[[nodiscard]] std::shared_ptr<VulkanBuffer> StageData(VulkanDevice &device, T &&container);

[[nodiscard]] std::optional<VulkanTexture>
LoadTexture(app_t &app, VulkanDevice &device, std::string_view name);


struct ResizeHandler final : public Window::IEventHandler {
    ResizeHandler(app_t &app) : app{app} { }

    app_t &app;

    void onResize(std::int32_t width, std::int32_t height) override
    {
        app.width = static_cast<std::uint32_t>(width);
        app.height = static_cast<std::uint32_t>(height);

        RecreateSwapChain(app);

        app.camera->aspect = static_cast<float>(width) / static_cast<float>(height);
    }
};


void CleanupFrameData(app_t &app)
{
    auto &&device = *app.vulkanDevice;

    if (app.graphicsCommandPool)
        vkFreeCommandBuffers(device.handle(), app.graphicsCommandPool, static_cast<std::uint32_t>(std::size(app.commandBuffers)), std::data(app.commandBuffers));

    app.commandBuffers.clear();

    if (app.renderPass != VK_NULL_HANDLE)
        vkDestroyRenderPass(device.handle(), app.renderPass, nullptr);

    CleanupSwapchain(device, app.swapchain);
}

void UpdateDescriptorSet(app_t &app, VulkanDevice const &device, VkDescriptorSet &descriptorSet)
{
    // TODO: descriptor info typed by VkDescriptorType.
    auto const cameras = std::array{
        VkDescriptorBufferInfo{app.perCameraBuffer->handle(), 0, sizeof(Camera::data_t)}
    };

    // TODO: descriptor info typed by VkDescriptorType.
    auto const objects = std::array{
        VkDescriptorBufferInfo{app.perObjectBuffer->handle(), 0, sizeof(per_object_t)}
    };

    // TODO: descriptor info typed by VkDescriptorType.
    auto const images = std::array{
        VkDescriptorImageInfo{app.texture.sampler->handle(), app.texture.view.handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
    };

    std::array<VkWriteDescriptorSet, 3> const writeDescriptorsSet{{
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
            0, static_cast<std::uint32_t>(std::size(images)),
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            std::data(images),
            nullptr,
            nullptr
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptorSet,
            2,
            0, static_cast<std::uint32_t>(std::size(objects)),
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
            nullptr,
            std::data(objects),
            nullptr
        },
    }};

    // WARN:: remember about potential race condition with the related executing command buffer
    vkUpdateDescriptorSets(device.handle(), static_cast<std::uint32_t>(std::size(writeDescriptorsSet)),
                           std::data(writeDescriptorsSet), 0, nullptr);
}

void CreateFramebuffers(VulkanDevice const &device, VkRenderPass renderPass, VulkanSwapchain &swapchain)
{
    auto &&framebuffers = swapchain.framebuffers;
    auto &&views = swapchain.views;

    framebuffers.clear();

    std::transform(std::cbegin(views), std::cend(views), std::back_inserter(framebuffers), [&device, renderPass, &swapchain] (auto &&view)
    {
        auto const attachements = std::array{swapchain.colorTexture.view.handle(), swapchain.depthTexture.view.handle(), view};

        VkFramebufferCreateInfo const createInfo{
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            nullptr, 0,
            renderPass,
            static_cast<std::uint32_t>(std::size(attachements)), std::data(attachements),
            swapchain.extent.width, swapchain.extent.height,
            1
        };

        VkFramebuffer framebuffer;

        if (auto result = vkCreateFramebuffer(device.handle(), &createInfo, nullptr, &framebuffer); result != VK_SUCCESS)
            throw std::runtime_error("failed to create a framebuffer: "s + std::to_string(result));

        return framebuffer;
    });
}

void CreateGraphicsPipelines(app_t &app)
{
    for (auto &&renderable : renderables) {
        auto pipeline = app.graphicsPipelineManager->CreateGraphicsPipeline(
            renderable.vertexBuffer->vertexLayout(), renderable.material, renderable.topology, app.pipelineLayout, app.renderPass, app.swapchain.extent
        );

        if (!pipeline)
            throw std::runtime_error("failed to get graphics pipeline"s);
    }
}

void CreateGraphicsCommandBuffers(app_t &app)
{
    app.commandBuffers.resize(std::size(app.swapchain.framebuffers));

    VkCommandBufferAllocateInfo const allocateInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        app.graphicsCommandPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        static_cast<std::uint32_t>(std::size(app.commandBuffers))
    };

    if (auto result = vkAllocateCommandBuffers(app.vulkanDevice->handle(), &allocateInfo, std::data(app.commandBuffers)); result != VK_SUCCESS)
        throw std::runtime_error("failed to create allocate command buffers: "s + std::to_string(result));

    std::size_t i = 0;

    for (auto &commandBuffer : app.commandBuffers) {
        VkCommandBufferBeginInfo const beginInfo{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            nullptr,
            VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
            nullptr
        };

        if (auto result = vkBeginCommandBuffer(commandBuffer, &beginInfo); result != VK_SUCCESS)
            throw std::runtime_error("failed to record command buffer: "s + std::to_string(result));

    #if defined( __clang__) || defined(_MSC_VER)
        auto const clearColors = std::array{
            VkClearValue{{{ .64f, .64f, .64f, 1.f }}},
            VkClearValue{{{ kREVERSED_DEPTH ? 0.f : 1.f, 0 }}}
        };
    #else
        auto const clearColors = std::array{
            VkClearValue{.color = {.float32 = { .64f, .64f, .64f, 1.f } } },
            VkClearValue{.depthStencil = { kREVERSED_DEPTH ? 0.f : 1.f, 0 } }
        };
    #endif

        VkRenderPassBeginInfo const renderPassInfo{
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            nullptr,
            app.renderPass,
            app.swapchain.framebuffers.at(i++),
            {{0, 0}, app.swapchain.extent},
            static_cast<std::uint32_t>(std::size(clearColors)), std::data(clearColors)
        };

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    #if USE_DYNAMIC_PIPELINE_STATE
        VkViewport const viewport{
            0, static_cast<float>(app.swapchain.extent.height),
            static_cast<float>(app.swapchain.extent.width), -static_cast<float>(app.swapchain.extent.height),
            0, 1
        };

        VkRect2D const scissor{
            {0, 0}, app.swapchain.extent
        };

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    #endif

        auto &&resourceManager = app.vulkanDevice->resourceManager();

        std::vector<VkBuffer> vertexBuffers;

        for (auto &&[layout, vertexBuffer] : resourceManager.vertexBuffers())
            vertexBuffers.push_back(vertexBuffer->deviceBuffer().handle());

        auto const bindingCount = static_cast<std::uint32_t>(std::size(vertexBuffers));

        std::vector<VkDeviceSize> vertexBuffersOffsets(bindingCount, 0);

        vkCmdBindVertexBuffers(commandBuffer, 0, bindingCount, std::data(vertexBuffers), std::data(vertexBuffersOffsets));

        auto &&graphicsPipelines = app.graphicsPipelineManager->graphicsPipelines();

        for (auto &&renderable : renderables) {
            auto [topology, material, vertexBuffer, vertexCount, firstVertex] = renderable;

            GraphicsPipelineManager::GraphicsPipelinePropertiesKey key{
                topology, vertexBuffer->vertexLayout(), material,
                std::array{static_cast<float>(app.swapchain.extent.width), static_cast<float>(app.swapchain.extent.height)}
            };

            if (auto it_pipeline = graphicsPipelines.find(key); it_pipeline == std::cend(graphicsPipelines))
                throw std::runtime_error("failed to find pipeline"s);

            else vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, it_pipeline->second->handle());

            std::uint32_t const dynamicOffset = 0;

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app.pipelineLayout,
                                    0, 1, &app.descriptorSet, 1, &dynamicOffset);

            vkCmdDraw(commandBuffer, vertexCount, 1, firstVertex, 0);
        }

        vkCmdEndRenderPass(commandBuffer);

        if (auto result = vkEndCommandBuffer(commandBuffer); result != VK_SUCCESS)
            throw std::runtime_error("failed to end command buffer: "s + std::to_string(result));
    }
}

void CreateSemaphores(app_t &app)
{
    if (auto semaphore = CreateSemaphore(*app.vulkanDevice); !semaphore)
        throw std::runtime_error("failed to create image semaphore"s);

    else app.imageAvailableSemaphore = *semaphore;

    if (auto semaphore = CreateSemaphore(*app.vulkanDevice); !semaphore)
        throw std::runtime_error("failed to create render semaphore"s);

    else app.renderFinishedSemaphore = *semaphore;
}

void RecreateSwapChain(app_t &app)
{
    if (app.width < 1 || app.height < 1) return;

    vkDeviceWaitIdle(app.vulkanDevice->handle());

    CleanupFrameData(app);

    auto swapchain = CreateSwapchain(*app.vulkanDevice, app.surface, app.width, app.height,
                                     app.presentationQueue, app.graphicsQueue, app.transferQueue, app.transferCommandPool);

    if (swapchain)
        app.swapchain = std::move(swapchain.value());

    else throw std::runtime_error("failed to create the swapchain"s);

    if (auto renderPass = CreateRenderPass(*app.vulkanDevice, app.swapchain); !renderPass)
        throw std::runtime_error("failed to create the render pass"s);

    else app.renderPass = std::move(renderPass.value());

#if !USE_DYNAMIC_PIPELINE_STATE
    CreateGraphicsPipelines(app);
#endif

    CreateFramebuffers(*app.vulkanDevice, app.renderPass, app.swapchain);

    CreateGraphicsCommandBuffers(app);
}


namespace temp
{
xformat populate()
{
    xformat _model;

    {
        // First triangle
        struct vertex final {
            vec<3, std::float_t> position;
            vec<2, std::float_t> texCoord;
        };

        using Position = decltype(vertex::position);
        using Texcoord = decltype(vertex::texCoord);

        auto const vertexLayoutIndex = std::size(_model.vertexLayouts);

        std::cout << "vertexLayoutIndex "s << vertexLayoutIndex << std::endl;

        {
            xformat::vertex_layout vertexLayout;
            vertexLayout.sizeInBytes = sizeof(vertex);

            vertexLayout.attributes.push_back(xformat::vertex_attribute{
                offsetof(vertex, position), semantic::position{}, Position{}, false
            });

            vertexLayout.attributes.push_back(xformat::vertex_attribute{
                offsetof(vertex, texCoord), semantic::tex_coord_0{}, Texcoord{}, false
            });

            _model.vertexLayouts.push_back(std::move(vertexLayout));
        }

        std::vector<vertex> vertices;

        vertices.push_back(vertex{
            Position{0.f, 0.f, 0.f}, Texcoord{.5f, .5f}
        });

        vertices.push_back(vertex{
            Position{-1.f, 0.f, 1.f}, Texcoord{0.f, 0.f}
        });

        vertices.push_back(vertex{
            Position{1.f, 0.f, 1.f}, Texcoord{1.f, 0.f}
        });

        xformat::non_indexed_meshlet meshlet;

        meshlet.topology = PRIMITIVE_TOPOLOGY::TRIANGLES;

        {
            auto const vertexSize = sizeof(vertex);
            auto const vertexCount = std::size(vertices);
            auto const bytesCount = vertexSize * vertexCount;

            auto &&vertexBuffer = _model.vertexBuffers[vertexLayoutIndex];

            using buffer_type_t = std::decay_t<decltype(vertexBuffer.buffer)>;

            meshlet.vertexBufferIndex = vertexLayoutIndex;
            meshlet.vertexCount = static_cast<std::uint32_t>(vertexCount);
            meshlet.firstVertex = static_cast<std::uint32_t>(vertexBuffer.count);

            std::cout << "#### @" << std::size(vertexBuffer.buffer) << std::endl;
            std::cout << "#### !" << bytesCount << '\t' << vertexBuffer.count << std::endl;

            vertexBuffer.buffer.resize(std::size(vertexBuffer.buffer) + bytesCount);

            auto writeOffset = static_cast<buffer_type_t::difference_type>(vertexBuffer.count * vertexSize);

            vertexBuffer.count += vertexCount;

            auto dstBegin = std::next(std::begin(vertexBuffer.buffer), writeOffset);

            std::cout << "!!!! " << std::distance(dstBegin, std::begin(vertexBuffer.buffer)) << std::endl;

            std::uninitialized_copy_n(reinterpret_cast<std::byte *>(std::data(vertices)), bytesCount, dstBegin);
        }

        meshlet.materialIndex = 0;
        meshlet.instanceCount = 1;
        meshlet.firstInstance = 0;

        _model.nonIndexedMeshlets.push_back(std::move(meshlet));
    }

    if constexpr (true) {
        struct vertex final {
            vec<3, std::float_t> position;
            vec<2, std::float_t> texCoord;
            vec<4, std::float_t> color;
        };

        using Position = decltype(vertex::position);
        using Texcoord = decltype(vertex::texCoord);
        using Color = decltype(vertex::color);

        auto const vertexLayoutIndex = std::size(_model.vertexLayouts);

        std::cout << "vertexLayoutIndex "s << vertexLayoutIndex << std::endl;

        {
            xformat::vertex_layout vertexLayout;
            vertexLayout.sizeInBytes = sizeof(vertex);

            vertexLayout.attributes.push_back(xformat::vertex_attribute{
                offsetof(vertex, position), semantic::position{}, Position{}, false
            });

            vertexLayout.attributes.push_back(xformat::vertex_attribute{
                offsetof(vertex, texCoord), semantic::tex_coord_0{}, Texcoord{}, false
            });

            vertexLayout.attributes.push_back(xformat::vertex_attribute{
                offsetof(vertex, color), semantic::color_0{}, Color{}, false
            });

            _model.vertexLayouts.push_back(std::move(vertexLayout));
        }

        std::vector<vertex> vertices;

        // Second triangle
        vertices.push_back(vertex{
            Position{0.f, 0.f, 0.f}, Texcoord{.5f, .5f}, Color{0.f, 0.f, 0.f, 1.f}
        });

        vertices.push_back(vertex{
            Position{1.f, 0.f, -1.f}, Texcoord{1.f, 1.f}, Color{1.f, 0.f, 1.f, 1.f}
        });

        vertices.push_back(vertex{
            Position{0.f, 0.f, -1.f}, Texcoord{.5f, 1.f}, Color{0.f, 0.f, 1.f, 1.f}
        });

        // Third triangle
        /* vertices.push_back(vertex{
            Position{0.f, 0.f, 0.f}, Texcoord{.5f, .5f}, Color{1.f, 0.f, 1.f, 1.f}
        });

        vertices.push_back(vertex{
            Position{-1.f, 0.f, -1.f}, Texcoord{0.f, 1.f}, Color{0.f, 1.f, 1.f, 1.f}
        });

        vertices.push_back(vertex{
            Position{-1.f, 0.f, 0.f}, Texcoord{0.f, .5f}, Color{1.f, 1.f, 0.f, 1.f}
        }); */

        auto constexpr vertexCountPerMeshlet = 3u;

        auto &&vertexBuffer = _model.vertexBuffers[vertexLayoutIndex];

        using buffer_type_t = std::decay_t<decltype(vertexBuffer.buffer)>;

        {
            // Second triangle
            xformat::non_indexed_meshlet meshlet;

            meshlet.topology = PRIMITIVE_TOPOLOGY::TRIANGLES;

            meshlet.vertexBufferIndex = vertexLayoutIndex;
            meshlet.vertexCount = static_cast<std::uint32_t>(vertexCountPerMeshlet);
            meshlet.firstVertex = static_cast<std::uint32_t>(vertexBuffer.count + 0u);

            meshlet.materialIndex = 0;
            meshlet.instanceCount = 1;
            meshlet.firstInstance = 0;

            _model.nonIndexedMeshlets.push_back(std::move(meshlet));
        }

        /* {
            // Third triangle
            xformat::non_indexed_meshlet meshlet;

            meshlet.topology = PRIMITIVE_TOPOLOGY::TRIANGLES;

            meshlet.vertexBufferIndex = vertexLayoutIndex;
            meshlet.vertexCount = static_cast<std::uint32_t>(vertexCountPerMeshlet);
            meshlet.firstVertex = static_cast<std::uint32_t>(vertexBuffer.count + vertexCountPerMeshlet);

            meshlet.materialIndex = 1;
            meshlet.instanceCount = 1;
            meshlet.firstInstance = 0;

            _model.nonIndexedMeshlets.push_back(std::move(meshlet));
        } */

        {
            auto const vertexSize = sizeof(vertex);
            auto const vertexCount = std::size(vertices);
            auto const bytesCount = vertexSize * vertexCount;

            std::cout << "#### %" << std::size(vertexBuffer.buffer) << std::endl;
            std::cout << "#### ^" << bytesCount << '\t' << vertexBuffer.count << std::endl;

            vertexBuffer.buffer.resize(std::size(vertexBuffer.buffer) + bytesCount);

            std::cout << "#### !" << vertexSize << '\t' << std::size(vertexBuffer.buffer) << std::endl;

            auto writeOffset = static_cast<buffer_type_t::difference_type>(vertexBuffer.count * vertexSize);

            vertexBuffer.count += vertexCount;

            auto dstBegin = std::next(std::begin(vertexBuffer.buffer), writeOffset);

            std::cout << "!!!! " << writeOffset << '\t' << std::distance(std::begin(vertexBuffer.buffer), dstBegin) << std::endl;

            std::uninitialized_copy_n(reinterpret_cast<std::byte *>(std::data(vertices)), bytesCount, dstBegin);
        }
    }

    _model.materials.push_back(xformat::material{"TexCoordsDebugMaterial"s});
    _model.materials.push_back(xformat::material{"ColorsDebugMaterial"s});
    //_model.materials.push_back(xformat::material{"NormalsDebugMaterial"s});

    return _model;
}

void stageXformat(app_t &app, xformat const &_model)
{
    auto &&resourceManager = app.vulkanDevice->resourceManager();

    for (auto &&meshlet : _model.nonIndexedMeshlets) {
        auto vertexLayoutIndex = meshlet.vertexBufferIndex;
        auto &&vertexLayout = _model.vertexLayouts[vertexLayoutIndex];

        auto &&_vertexBuffer = _model.vertexBuffers.at(vertexLayoutIndex);

        auto vertexBuffer = resourceManager.CreateVertexBuffer(vertexLayout, std::size(_vertexBuffer.buffer));

        if (vertexBuffer)
            resourceManager.StageVertexData(vertexBuffer, _vertexBuffer.buffer);

        else throw std::runtime_error("failed to get vertex buffer"s);

        auto materialIndex = meshlet.materialIndex;
        auto &&_material = _model.materials[materialIndex];

        auto material = app.materialFactory->CreateMaterial(_material.type);

        if (!material)
            throw std::runtime_error("failed to get material"s);

        renderables.push_back({
            meshlet.topology, material, vertexBuffer, meshlet.vertexCount, meshlet.firstVertex
        });
    }
}
}


void InitVulkan(Window &window, app_t &app)
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
        throw std::runtime_error("failed to create window surface: "s + std::to_string(result));
#endif

    QueuePool<
        instances_number<GraphicsQueue>,
        instances_number<TransferQueue>,
        instances_number<PresentationQueue>
    > qpool;

    app.vulkanDevice = std::make_unique<VulkanDevice>(*app.vulkanInstance, app.surface, config::deviceExtensions, std::move(qpool));

    app.shaderManager = std::make_unique<ShaderManager>(*app.vulkanDevice);
    app.materialFactory = std::make_unique<MaterialFactory>(*app.shaderManager);
    app.graphicsPipelineManager = std::make_unique<GraphicsPipelineManager>(*app.vulkanDevice, *app.materialFactory, app.pipelineVertexInputStatesManager);

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

    if (auto result = glTF::load(sceneName, app.scene, app.nodeSystem); !result)
        throw std::runtime_error("failed to load a mesh"s);


    if (auto pipelineLayout = CreatePipelineLayout(*app.vulkanDevice, std::array{app.descriptorSetLayout}); !pipelineLayout)
        throw std::runtime_error("failed to create the pipeline layout"s);

    else app.pipelineLayout = std::move(pipelineLayout.value());

    temp::model = temp::populate();
    temp::stageXformat(app, temp::model);

    app.vulkanDevice->resourceManager().TransferStagedVertexData(app.transferCommandPool, app.transferQueue);
    CreateGraphicsPipelines(app);

    CreateFramebuffers(*app.vulkanDevice, app.renderPass, app.swapchain);

    // "chalet/textures/chalet.tga"sv
    // "Hebe/textures/HebehebemissinSG1_metallicRoughness.tga"sv
    if (auto result = LoadTexture(app, *app.vulkanDevice, "sponza/textures/sponza_curtain_blue_diff.tga"sv); !result)
        throw std::runtime_error("failed to load a texture"s);

    else app.texture = std::move(result.value());

    if (auto result = app.vulkanDevice->resourceManager().CreateImageSampler(app.texture.image->mipLevels()); !result)
        throw std::runtime_error("failed to create a texture sampler"s);

    else app.texture.sampler = result;

    auto alignment = static_cast<std::size_t>(app.vulkanDevice->properties().limits.minStorageBufferOffsetAlignment);

    app.alignedBufferSize = aligned_size(sizeof(per_object_t), alignment) * app.objectsNumber;

    app.alignedBuffer = boost::alignment::aligned_alloc(alignment, app.alignedBufferSize);

    app.objects.resize(app.objectsNumber);

    if (app.perObjectBuffer = CreateStorageBuffer(*app.vulkanDevice, app.alignedBufferSize); !app.perObjectBuffer)
        throw std::runtime_error("failed to init per object uniform buffer"s);

    else {
        auto &&buffer = *app.perObjectBuffer;

        auto offset = buffer.memory()->offset();
        auto size = buffer.memory()->size();

        if (auto result = vkMapMemory(app.vulkanDevice->handle(), buffer.memory()->handle(), offset, size, 0, &app.perObjectsMappedPtr); result != VK_SUCCESS)
            throw std::runtime_error("failed to map per object uniform buffer memory: "s + std::to_string(result));
    }

    if (app.perCameraBuffer = CreateCoherentStorageBuffer(*app.vulkanDevice, sizeof(Camera::data_t)); !app.perCameraBuffer)
        throw std::runtime_error("failed to init per camera uniform buffer"s);

    if (auto descriptorPool = CreateDescriptorPool(*app.vulkanDevice); !descriptorPool)
        throw std::runtime_error("failed to create the descriptor pool"s);

    else app.descriptorPool = std::move(descriptorPool.value());

    if (auto descriptorSet = CreateDescriptorSet(*app.vulkanDevice, app.descriptorPool, std::array{app.descriptorSetLayout}); !descriptorSet)
        throw std::runtime_error("failed to create the descriptor pool"s);

    else app.descriptorSet = std::move(descriptorSet.value());

    UpdateDescriptorSet(app, *app.vulkanDevice, app.descriptorSet);

    CreateGraphicsCommandBuffers(app);

    CreateSemaphores(app);
}

void Update(app_t &app)
{
    app.cameraController->update();
    app.cameraSystem.update();

    auto &&device = *app.vulkanDevice;

    {
        auto &&buffer = *app.perCameraBuffer;

        auto offset = buffer.memory()->offset();
        auto size = buffer.memory()->size();

        void *data;

        if (auto result = vkMapMemory(device.handle(), buffer.memory()->handle(), offset, size, 0, &data); result != VK_SUCCESS)
            throw std::runtime_error("failed to map per camera uniform buffer memory: "s + std::to_string(result));

        std::uninitialized_copy_n(&app.camera->data, 1, reinterpret_cast<Camera::data_t *>(data));

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

void DrawFrame(app_t &app)
{
    auto &&vulkanDevice = *app.vulkanDevice;

    vkQueueWaitIdle(app.presentationQueue.handle());

    std::uint32_t imageIndex;

    switch (auto result = vkAcquireNextImageKHR(vulkanDevice.handle(), app.swapchain.handle,
            std::numeric_limits<std::uint64_t>::max(),app.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex); result) {
        case VK_ERROR_OUT_OF_DATE_KHR:
            RecreateSwapChain(app);
            return;

        case VK_SUBOPTIMAL_KHR:
        case VK_SUCCESS:
            break;

        default:
            throw std::runtime_error("failed to acquire next image index: "s + std::to_string(result));
    }

    auto const waitSemaphores = std::array{app.imageAvailableSemaphore};
    auto const signalSemaphores = std::array{app.renderFinishedSemaphore};

    std::array<VkPipelineStageFlags, 1> constexpr waitStages{
        { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }
    };

    VkSubmitInfo const submitInfo{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        static_cast<std::uint32_t>(std::size(waitSemaphores)), std::data(waitSemaphores),
        std::data(waitStages),
        1, &app.commandBuffers.at(imageIndex),
        static_cast<std::uint32_t>(std::size(signalSemaphores)), std::data(signalSemaphores),
    };

    if (auto result = vkQueueSubmit(app.graphicsQueue.handle(), 1, &submitInfo, VK_NULL_HANDLE); result != VK_SUCCESS)
        throw std::runtime_error("failed to submit draw command buffer: "s + std::to_string(result));

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
            RecreateSwapChain(app);
            return;

        case VK_SUCCESS:
            break;

        default:
            throw std::runtime_error("failed to submit request to present framebuffer: "s + std::to_string(result));
    }
}





int main()
try {
#if defined(_MSC_VER)
    _CrtSetDbgFlag(_CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#else
	std::signal(SIGSEGV, PosixSignalHandler);
	std::signal(SIGTRAP, PosixSignalHandler);
#endif

    glfwInit();

    app_t app;

    Window window{"VulkanIsland"sv, static_cast<std::int32_t>(app.width), static_cast<std::int32_t>(app.height)};

    auto resizeHandler = std::make_shared<ResizeHandler>(app);
    window.connectEventHandler(resizeHandler);

    auto inputManager = std::make_shared<InputManager>();
    window.connectInputHandler(inputManager);

    app.camera = app.cameraSystem.createCamera();
    app.camera->aspect = static_cast<float>(app.width) / static_cast<float>(app.height);

    app.cameraController = std::make_unique<OrbitController>(app.camera, *inputManager);
    app.cameraController->lookAt(glm::vec3{0, 2, 1}, {0, 0, 0});

    std::cout << measure<>::execution(InitVulkan, window, std::ref(app)) << " ms\n"s;

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

        app.registry.sort<ecs::node>(ecs::node());
    #if NOT_YET_IMPLEMENTED
        app.registry.sort<ecs::mesh>(ecs::mesh());
    #endif

        app.nodeSystem.update();
    #if NOT_YET_IMPLEMENTED
        app.meshSystem.update();
    #endif
        Update(app);

        DrawFrame(app);
    });

    app.cleanUp();

    glfwTerminate();
} catch (std::exception const &ex) {
    std::cout << ex.what() << std::endl;
    std::cin.get();
}


template<class T, typename std::enable_if_t<is_container_v<std::decay_t<T>>>...>
[[nodiscard]] std::shared_ptr<VulkanBuffer>
StageData(VulkanDevice &device, T &&container)
{
    auto constexpr usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    using type = typename std::decay_t<T>::value_type;

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
LoadTexture(app_t &app, VulkanDevice &device, std::string_view name)
{
    std::optional<VulkanTexture> texture;

    auto constexpr generateMipMaps = true;

    if (auto rawImage = LoadTARGA(name); rawImage) {
        auto stagingBuffer = std::visit([&device] (auto &&data)
        {
            return StageData(device, std::forward<decltype(data)>(data));
        }, std::move(rawImage->data));

        if (stagingBuffer) {
            auto const width = static_cast<std::uint16_t>(rawImage->width);
            auto const height = static_cast<std::uint16_t>(rawImage->height);

            auto constexpr usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            auto constexpr propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            auto constexpr tiling = VK_IMAGE_TILING_OPTIMAL;

            texture = CreateTexture(device, rawImage->format, rawImage->type, width, height, rawImage->mipLevels,
                                    VK_SAMPLE_COUNT_1_BIT, tiling, VK_IMAGE_ASPECT_COLOR_BIT, usageFlags, propertyFlags);

            if (texture) {
                TransitionImageLayout(device, app.transferQueue, *texture->image, VK_IMAGE_LAYOUT_UNDEFINED,
                                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, app.transferCommandPool);

                CopyBufferToImage(device, app.transferQueue, stagingBuffer->handle(), texture->image->handle(), width, height, app.transferCommandPool);

                if (generateMipMaps)
                    GenerateMipMaps(device, app.transferQueue, *texture->image, app.transferCommandPool);

                else TransitionImageLayout(device, app.transferQueue, *texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, app.transferCommandPool);
            }
        }
    }

    else std::cerr << "failed to load an image\n"s;

    return texture;
}


#if 0
[[nodiscard]] std::shared_ptr<VulkanBuffer> InitVertexBuffer(app_t &app)
{
    if (std::empty(app.scene.vertexBuffer))
        return { };

    std::shared_ptr<VulkanBuffer> buffer;

    auto &&vertices = app.scene.vertexBuffer;

    if (auto stagingBuffer = StageData(*app.vulkanDevice, vertices); stagingBuffer) {
        auto constexpr usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        auto constexpr propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        buffer = app.vulkanDevice->resourceManager().CreateBuffer(stagingBuffer->memory()->size(), usageFlags, propertyFlags);

        if (buffer) {
            auto copyRegions = std::array{
                VkBufferCopy{
                /*stagingBuffer->memory()->offset(), stagingBuffer->memory()->offset()*/
                0, 0, stagingBuffer->memory()->size()
            }
            };

            CopyBufferToBuffer(*app.vulkanDevice, app.transferQueue, stagingBuffer->handle(),
                               buffer->handle(), std::move(copyRegions), app.transferCommandPool);
        }
    }

    return buffer;
}

[[nodiscard]] std::shared_ptr<VulkanBuffer> InitIndexBuffer(app_t &app)
{
    auto &&indices = app.scene.indexBuffer;

    if (std::empty(indices))
        return { };

    std::shared_ptr<VulkanBuffer> buffer;

    if (auto stagingBuffer = StageData(*app.vulkanDevice, indices); stagingBuffer) {
        auto constexpr usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        auto constexpr propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        buffer = app.vulkanDevice->resourceManager().CreateBuffer(stagingBuffer->memory()->size(), usageFlags, propertyFlags);

        if (buffer) {
            auto copyRegions = std::array{
                VkBufferCopy{
                /*stagingBuffer->memory()->offset(), stagingBuffer->memory()->offset()*/
                0, 0, stagingBuffer->memory()->size()
            }
            };

            CopyBufferToBuffer(*app.vulkanDevice, app.transferQueue, stagingBuffer->handle(),
                               buffer->handle(), std::move(copyRegions), app.transferCommandPool);
        }
    }

    return buffer;
}
#endif
