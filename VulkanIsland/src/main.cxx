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
#include "device.hxx"
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

#include "renderer/vertexLayout.hxx"
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


#define USE_DYNAMIC_PIPELINE_STATE 0

auto constexpr sceneName{"unlit-test"sv};

struct per_object_t {
    glm::mat4 world{1};
    glm::mat4 normal{1};  // Transposed of the inversed of the upper left 3x3 sub-matrix of model(world)-view matrix.
};


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

    VkSurfaceKHR surface{ VK_NULL_HANDLE };
    VulkanSwapchain swapchain;

    GraphicsQueue graphicsQueue;
    TransferQueue transferQueue;
    PresentationQueue presentationQueue;

    VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
    VkRenderPass renderPass{ VK_NULL_HANDLE };
    VkPipeline graphicsPipeline{ VK_NULL_HANDLE };

    VkCommandPool graphicsCommandPool{ VK_NULL_HANDLE }, transferCommandPool{ VK_NULL_HANDLE };

    VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
    VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
    VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };

    std::vector<VkCommandBuffer> commandBuffers;

    VkSemaphore imageAvailableSemaphore{ VK_NULL_HANDLE }, renderFinishedSemaphore{ VK_NULL_HANDLE };

    std::shared_ptr<VulkanBuffer> vertexBuffer, indexBuffer, uboBuffer;
    std::shared_ptr<VulkanBuffer> perObjectBuffer, perCameraBuffer;
    void *perObjectsMappedPtr{nullptr};
    void *alignedBuffer{nullptr};

    std::size_t objectsNumber{2u};
    std::size_t alignedBufferSize{0u};

    VulkanTexture texture;

    ecs::entity_registry registry;

    ecs::NodeSystem nodeSystem{registry};
    ecs::MeshSystem meshSystem{registry};

    std::shared_ptr<VulkanBuffer> vertexBufferA;
    std::shared_ptr<VulkanBuffer> vertexBufferB;
    VkPipeline graphicsPipelineA{VK_NULL_HANDLE};
    VkPipeline graphicsPipelineB{VK_NULL_HANDLE};
    VkPipelineLayout pipelineLayoutA{VK_NULL_HANDLE};
    VkPipelineLayout pipelineLayoutB{VK_NULL_HANDLE};

    VertexLayoutsManager vertexLayoutsManager;

    std::unique_ptr<MaterialFactory> materialFactory;
    std::unique_ptr<ShaderManager> shaderManager;

    std::shared_ptr<Material> material;
    std::shared_ptr<Material> materialA;
    std::shared_ptr<Material> materialB;


    ~app_t()
    {
        cleanUp();
    }

    void cleanUp()
    {
        if (vulkanDevice == nullptr)
            return;

        vkDeviceWaitIdle(vulkanDevice->handle());

        if (materialFactory)
            materialFactory.reset();

        if (shaderManager)
            shaderManager.reset();

        if (renderFinishedSemaphore)
            vkDestroySemaphore(vulkanDevice->handle(), renderFinishedSemaphore, nullptr);

        if (imageAvailableSemaphore)
            vkDestroySemaphore(vulkanDevice->handle(), imageAvailableSemaphore, nullptr);

        CleanupFrameData(*this);

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

        uboBuffer.reset();

        vertexBufferB.reset();
        vertexBufferA.reset();

        indexBuffer.reset();
        vertexBuffer.reset();

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


namespace temp
{
void CreateGraphicsPipeline(app_t & app, xformat::vertex_layout const & layout, std::string_view name);
void CreateGraphicsCommandBuffers(app_t &app);
}


void RecreateSwapChain(app_t &app);

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

template<class T, typename std::enable_if_t<is_container_v<std::decay_t<T>>>...>
[[nodiscard]] std::shared_ptr<VulkanBuffer> StageData(VulkanDevice &device, T &&container)
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


void CleanupFrameData(app_t &app)
{
    auto &&device = *app.vulkanDevice;

    if (app.graphicsCommandPool)
        vkFreeCommandBuffers(device.handle(), app.graphicsCommandPool, static_cast<std::uint32_t>(std::size(app.commandBuffers)), std::data(app.commandBuffers));

    app.commandBuffers.clear();

    if (app.graphicsPipeline != VK_NULL_HANDLE)
        vkDestroyPipeline(device.handle(), app.graphicsPipeline, nullptr);

    if (app.pipelineLayoutB != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device.handle(), app.pipelineLayoutB, nullptr);

    if (app.pipelineLayoutA != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device.handle(), app.pipelineLayoutA, nullptr);

    if (app.graphicsPipelineA != VK_NULL_HANDLE)
        vkDestroyPipeline(device.handle(), app.graphicsPipelineA, nullptr);

    if (app.graphicsPipelineB != VK_NULL_HANDLE)
        vkDestroyPipeline(device.handle(), app.graphicsPipelineB, nullptr);

    if (app.pipelineLayout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device.handle(), app.pipelineLayout, nullptr);

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


void CreateGraphicsPipeline(app_t &app)
{
#if OBSOLETE
    // Shader
    auto const vertShaderByteCode = app.shaderManager->ReadShaderFile(R"(vert.spv)"sv);

    if (vertShaderByteCode.empty())
        throw std::runtime_error("failed to open vertex shader file"s);

    auto const vertShaderModule = app.vulkanDevice->resourceManager().CreateShaderModule(vertShaderByteCode);

    auto const fragShaderByteCode = app.shaderManager->ReadShaderFile(R"(frag.spv)"sv);

    if (fragShaderByteCode.empty())
        throw std::runtime_error("failed to open fragment shader file"s);

    auto const fragShaderModule = app.vulkanDevice->resourceManager().CreateShaderModule(fragShaderByteCode);

    VkPipelineShaderStageCreateInfo const vertShaderCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr, 0,
        VK_SHADER_STAGE_VERTEX_BIT,
        vertShaderModule->handle(),
        "main",
        nullptr
    };

    VkPipelineShaderStageCreateInfo const fragShaderCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr, 0,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        fragShaderModule->handle(),
        "main",
        nullptr
    };

    auto shaderStages = std::array{vertShaderCreateInfo, fragShaderCreateInfo};
#endif

    // Material
    app.material = app.materialFactory->CreateMaterial<TestMaterial>();

    if (!app.material)
        throw std::runtime_error("failed to create a material"s);

    auto materialProperties = app.materialFactory->properties(app.material);

    if (!materialProperties)
        throw std::runtime_error("failed to get a material properties"s);

    auto &&shaderStages = app.materialFactory->pipelineShaderStages(app.material);


#if NOT_YET_IMPLEMENTED
    auto const vertexMapEntries = std::array{
        VkSpecializationMapEntry{ 0, 0, sizeof(std::int32_t) },
        VkSpecializationMapEntry{ 1, 4, sizeof(std::int32_t) }
    };

    std::array<std::int32_t, 2> const vertexConstants{
        0, 1//, 2, 3
    };

    VkSpecializationInfo  const vertexSpeializationInfo{
        static_cast<std::uint32_t>(std::size(vertexMapEntries)),
        std::data(vertexMapEntries),
        std::size(vertexConstants) * sizeof(decltype(vertexConstants)::value_type),
        std::data(vertexConstants),
    };
#endif

    // Vertex layout
    VertexInputStateInfo2 vertexInputStateCreateInfo{app.scene.meshes.front().submeshes.front().vertices.layout};

    VkPipelineInputAssemblyStateCreateInfo constexpr vertexAssemblyStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        nullptr, 0,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VK_FALSE
    };

    // Render pass
#if USE_DYNAMIC_PIPELINE_STATE
    auto const dynamicStates = std::array{
        VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT,
        VkDynamicState::VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo const dynamicStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        nullptr, 0,
        static_cast<std::uint32_t>(std::size(dynamicStates)),
        std::data(dynamicStates)
    };

    auto constexpr rasterizerDiscardEnable = VK_TRUE;
#else
    VkViewport const viewport{
        0, static_cast<float>(app.swapchain.extent.height),
        static_cast<float>(app.swapchain.extent.width), -static_cast<float>(app.swapchain.extent.height),
        0, 1
    };

    VkRect2D const scissor{
        {0, 0}, app.swapchain.extent
    };

    VkPipelineViewportStateCreateInfo const viewportStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        nullptr, 0,
        1, &viewport,
        1, &scissor
    };

    // auto constexpr rasterizerDiscardEnable = VK_FALSE;
#endif

    VkPipelineMultisampleStateCreateInfo const multisampleCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        nullptr, 0,
        app.vulkanDevice->samplesCount(),//VK_SAMPLE_COUNT_1_BIT
        VK_FALSE, 1,
        nullptr,
        VK_FALSE,
        VK_FALSE
    };

#if OBSOLETE
    // Material
    VkPipelineRasterizationStateCreateInfo constexpr rasterizerState{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        nullptr, 0,
        VK_TRUE,
        rasterizerDiscardEnable,
        VK_POLYGON_MODE_FILL,
        VK_CULL_MODE_BACK_BIT,
        VK_FRONT_FACE_COUNTER_CLOCKWISE,
        VK_FALSE,
        0.f, 0.f, 0.f,
        1.f
    };

    VkPipelineDepthStencilStateCreateInfo constexpr depthStencilStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        nullptr, 0,
        VK_TRUE, VK_TRUE,
        kREVERSED_DEPTH ? VK_COMPARE_OP_GREATER : VK_COMPARE_OP_LESS,
        VK_FALSE,
        VK_FALSE, VkStencilOpState{}, VkStencilOpState{},
        0, 1
    };

    VkPipelineColorBlendAttachmentState constexpr colorBlendAttachment{
        VK_FALSE,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_OP_ADD,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_OP_ADD,
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo const colorBlendStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        nullptr, 0,
        VK_FALSE,
        VK_LOGIC_OP_COPY,
        1,
        &colorBlendAttachment,
        { 0, 0, 0, 0 }
    };
#endif

    if (auto pipelineLayout = CreatePipelineLayout(*app.vulkanDevice, std::array{app.descriptorSetLayout}); pipelineLayout)
        app.pipelineLayout = pipelineLayout.value();


    VkGraphicsPipelineCreateInfo const graphicsPipelineCreateInfo{
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        nullptr,
        VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT,
        static_cast<std::uint32_t>(std::size(shaderStages)), std::data(shaderStages),
        &vertexInputStateCreateInfo.info(),
        &vertexAssemblyStateCreateInfo,
        nullptr,
#if USE_DYNAMIC_PIPELINE_STATE
        nullptr,
#else
        &viewportStateCreateInfo,
#endif
        &materialProperties->rasterizationState,
        &multisampleCreateInfo,
        &materialProperties->depthStencilState,
        &materialProperties->colorBlendState,
        nullptr,
        app.pipelineLayout,
        app.renderPass,
        0,
        VK_NULL_HANDLE, -1
    };

    if (auto result = vkCreateGraphicsPipelines(app.vulkanDevice->handle(), VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &app.graphicsPipeline); result != VK_SUCCESS)
        throw std::runtime_error("failed to create graphics pipeline: "s + std::to_string(result));
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


template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>...>
void CreateCommandPool(VkDevice device, Q &queue, VkCommandPool &commandPool, VkCommandPoolCreateFlags flags)
{
    VkCommandPoolCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        flags,
        queue.family()
    };

    if (auto result = vkCreateCommandPool(device, &createInfo, nullptr, &commandPool); result != VK_SUCCESS)
        throw std::runtime_error("failed to create a command buffer: "s + std::to_string(result));
}


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
            VkClearValue{ .color = { .float32 = { .64f, .64f, .64f, 1.f } } },
            VkClearValue{ .depthStencil = { kREVERSED_DEPTH ? 0.f : 1.f, 0 } }
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

        auto const vertexBuffers = std::array{app.vertexBuffer->handle()};
        auto const offsets = std::array{VkDeviceSize{0}};

        vkCmdBindVertexBuffers(commandBuffer, 5, 1, std::data(vertexBuffers), std::data(offsets));

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

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app.graphicsPipeline);

        std::size_t const stride = app.alignedBufferSize / app.objectsNumber;
        std::size_t instanceIndex = 0;

        for (auto &&mesh : app.scene.meshes) {
            for (auto &&submesh : mesh.submeshes) {
                auto const dynamicOffset = static_cast<uint32_t>(instanceIndex * stride);

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app.pipelineLayout,
                                        0, 1, &app.descriptorSet, 1, &dynamicOffset);

                if (submesh.indices.count == 0)
                    vkCmdDraw(commandBuffer, submesh.vertices.count, 1, 0, 0);

                else {
                    auto index_type = std::visit([] (auto type)
                    {
                        if constexpr (std::is_same_v<typename std::decay_t<decltype(type)>, std::uint32_t>)
                            return VK_INDEX_TYPE_UINT32;

                        else return VK_INDEX_TYPE_UINT16;

                    }, submesh.indices.type);

                    vkCmdBindIndexBuffer(commandBuffer, app.indexBuffer->handle(), submesh.indices.begin, index_type);

                    vkCmdDrawIndexed(commandBuffer, submesh.indices.count, 1, 0/*submesh.indices.begin / (2 + 2 * index_type)*/, 0, 0);
                }

                ++instanceIndex;
            }
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
    CreateGraphicsPipeline(app);
#endif

    CreateFramebuffers(*app.vulkanDevice, app.renderPass, app.swapchain);

    temp::CreateGraphicsCommandBuffers(app);
    //CreateGraphicsCommandBuffers(app);
}


namespace temp
{
xformat::vertex_layout layoutA;
xformat::vertex_layout layoutB;

xformat populate()
{
    struct vertex final {
        vec<3, std::float_t> position;
        vec<2, std::float_t> texCoord;
    };

    xformat model;

    {
        xformat::vertex_layout vertexLayout;
        vertexLayout.sizeInBytes = sizeof(vertex);

        vertexLayout.attributes.push_back(xformat::vertex_attribute{
            0, semantic::position{}, vec<3, std::float_t>{}, false
        });

        vertexLayout.attributes.push_back(xformat::vertex_attribute{
            sizeof(vec<3, std::float_t>), semantic::tex_coord_0{}, vec<2, std::float_t>{}, false
        });

        model.vertexLayouts.push_back(std::move(vertexLayout));
    }

    using vertices_t = std::vector<vertex>;
    std::vector<vertices_t> vertexBuffers;

    {
        vertices_t vertices;

        // First triangle
        vertices.push_back(vertex{
            vec<3, std::float_t>{0.f, 0.f, 0.f}, vec<2, std::float_t>{.5f, .5f}
        });

        vertices.push_back(vertex{
            vec<3, std::float_t>{-1.f, 0.f, 1.f}, vec<2, std::float_t>{0.f, 0.f}
        });

        vertices.push_back(vertex{
            vec<3, std::float_t>{1.f, 0.f, 1.f}, vec<2, std::float_t>{1.f, 0.f}
        });

        // Second triangle
        vertices.push_back(vertex{
            vec<3, std::float_t>{0.f, 0.f, 0.f}, vec<2, std::float_t>{.5f, .5f}
        });

        vertices.push_back(vertex{
            vec<3, std::float_t>{1.f, 0.f, -1.f}, vec<2, std::float_t>{1.f, 1.f}
        });

        vertices.push_back(vertex{
            vec<3, std::float_t>{0.f, 0.f, 1.f}, vec<2, std::float_t>{.5f, 1.f}
        });

        vertexBuffers.push_back(std::move(vertices));
    }

    std::transform(std::begin(vertexBuffers), std::end(vertexBuffers), std::back_inserter(model.vertexBuffers), [] (auto &&srcBuffer)
    {
        xformat::vertex_buffer dstBuffer;

        dstBuffer.vertexLayoutIndex = 0;
        dstBuffer.count = std::size(srcBuffer);

        dstBuffer.buffer.resize(sizeof(vertex) * std::size(srcBuffer));

        std::uninitialized_copy(std::begin(srcBuffer), std::end(srcBuffer), reinterpret_cast<vertex *>(std::data(dstBuffer.buffer)));

        return dstBuffer;
    });

    model.materials.push_back(xformat::material{ PRIMITIVE_TOPOLOGY::TRIANGLES });

    {
        xformat::non_indexed_meshlet meshlet;

        meshlet.vertexBufferIndex = 0;
        meshlet.materialIndex = 0;
        meshlet.vertexCount = 3;
        meshlet.instanceCount = 1;
        meshlet.firstVertex = 0;
        meshlet.firstInstance = 0;

        model.nonIndexedMeshlets.push_back(std::move(meshlet));
    }

    {
        xformat::non_indexed_meshlet meshlet;

        meshlet.vertexBufferIndex = 0;
        meshlet.materialIndex = 0;
        meshlet.vertexCount = 3;
        meshlet.instanceCount = 1;
        meshlet.firstVertex = 3;
        meshlet.firstInstance = 0;

        model.nonIndexedMeshlets.push_back(std::move(meshlet));
    }

    return model;
}

void populate(app_t &app)
{
    {
        struct vertex final {
            vec<3, std::float_t> position;
            vec<2, std::float_t> texCoord;
        };

        layoutA.sizeInBytes = sizeof(vertex);

        layoutA.attributes.push_back(xformat::vertex_attribute{0, semantic::position{}, vec<3, std::float_t>{}, false});
        layoutA.attributes.push_back(xformat::vertex_attribute{sizeof(vec<3, std::float_t>), semantic::tex_coord_0{}, vec<2, std::float_t>{}, false});

        std::vector<vertex> vertices;

        // First triangle
        vertices.push_back(vertex{
            vec<3, std::float_t>{0.f, 0.f, 0.f}, vec<2, std::float_t>{.5f, .5f}
        });

        vertices.push_back(vertex{
            vec<3, std::float_t>{-1.f, 0.f, 1.f}, vec<2, std::float_t>{0.f, 0.f}
        });

        vertices.push_back(vertex{
            vec<3, std::float_t>{1.f, 0.f, 1.f}, vec<2, std::float_t>{1.f, 0.f}
        });

        auto &&buffer = app.vertexBufferA;

        if (auto stagingBuffer = StageData(*app.vulkanDevice, vertices); stagingBuffer) {
            auto constexpr usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            auto constexpr propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            buffer = app.vulkanDevice->resourceManager().CreateBuffer(stagingBuffer->memory()->size() * 10, usageFlags, propertyFlags);

            if (buffer) {
                auto copyRegions = std::array{VkBufferCopy{ 0, 0, stagingBuffer->memory()->size() }};

                CopyBufferToBuffer(*app.vulkanDevice, app.transferQueue, stagingBuffer->handle(),
                                   buffer->handle(), std::move(copyRegions), app.transferCommandPool);
            }
        }
    }

    {
        struct vertex final {
            vec<3, std::float_t> position;
            vec<4, std::float_t> color;
        };

        layoutB.sizeInBytes = sizeof(vertex);

        layoutB.attributes.push_back(xformat::vertex_attribute{0, semantic::position{}, vec<3, std::float_t>{}, false});
        layoutB.attributes.push_back(xformat::vertex_attribute{sizeof(vec<3, std::float_t>), semantic::color_0{}, vec<4, std::float_t>{}, false});

        std::vector<vertex> vertices;

        // Second triangle
        vertices.push_back(vertex{
            vec<3, std::float_t>{0.f, 0.f, 0.f}, vec<4, std::float_t>{1.f, 0.f, 0.f, 1.f}
        });

        vertices.push_back(vertex{
            vec<3, std::float_t>{1.f, 0.f, -1.f}, vec<4, std::float_t>{0.f, 1.f, 0.f, 1.f}
        });

        vertices.push_back(vertex{
            vec<3, std::float_t>{0.f, 0.f, -1.f}, vec<4, std::float_t>{0.f, 0.f, 1.f, 1.f}
        });

        auto &&buffer = app.vertexBufferB;

        if (auto stagingBuffer = StageData(*app.vulkanDevice, vertices); stagingBuffer) {
            auto constexpr usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            auto constexpr propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            buffer = app.vulkanDevice->resourceManager().CreateBuffer(stagingBuffer->memory()->size(), usageFlags, propertyFlags);

            if (buffer) {
                auto copyRegions = std::array{VkBufferCopy{ 0, 0, stagingBuffer->memory()->size() }};

                CopyBufferToBuffer(*app.vulkanDevice, app.transferQueue, stagingBuffer->handle(),
                                   buffer->handle(), std::move(copyRegions), app.transferCommandPool);
            }
        }
    }
}

void CreateGraphicsPipeline(app_t &app, xformat::vertex_layout const &layout, std::string_view name)
{
    // Material
    if (name == "A"sv)
        app.materialA = app.materialFactory->CreateMaterial<TexCoordsDebugMaterial>();

    else app.materialB = app.materialFactory->CreateMaterial<NormalsDebugMaterial>();

    auto material = name == "A"sv ? app.materialA : app.materialB;

    if (!material)
        throw std::runtime_error("failed to create a material"s);

    auto materialProperties = app.materialFactory->properties(material);

    if (!materialProperties)
        throw std::runtime_error("failed to get a material properties"s);

    auto &&shaderStages = app.materialFactory->pipelineShaderStages(material);

    // Vertex layout
    auto const vertexInputInfo = app.vertexLayoutsManager.info(layout);

    VkPipelineInputAssemblyStateCreateInfo constexpr vertexAssemblyStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        nullptr, 0,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VK_FALSE
    };

    // Render pass
    VkViewport const viewport{
        0, static_cast<float>(app.swapchain.extent.height),
        static_cast<float>(app.swapchain.extent.width), -static_cast<float>(app.swapchain.extent.height),
        0, 1
    };

    VkRect2D const scissor{
        {0, 0}, app.swapchain.extent
    };

    VkPipelineViewportStateCreateInfo const viewportStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        nullptr, 0,
        1, &viewport,
        1, &scissor
    };

    VkPipelineMultisampleStateCreateInfo const multisampleCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        nullptr, 0,
        app.vulkanDevice->samplesCount(),//VK_SAMPLE_COUNT_1_BIT
        VK_FALSE, 1,
        nullptr,
        VK_FALSE,
        VK_FALSE
    };

    auto &&pipelineLayout = name == "A"sv ? app.pipelineLayoutA : app.pipelineLayoutB;

    if (auto result = CreatePipelineLayout(*app.vulkanDevice, std::array{app.descriptorSetLayout}); result)
        pipelineLayout = result.value();


    VkGraphicsPipelineCreateInfo const graphicsPipelineCreateInfo{
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        nullptr,
        VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT,
        static_cast<std::uint32_t>(std::size(shaderStages)), std::data(shaderStages),
        &vertexInputInfo,
        &vertexAssemblyStateCreateInfo,
        nullptr,
#if USE_DYNAMIC_PIPELINE_STATE
        nullptr,
#else
        &viewportStateCreateInfo,
#endif
        &materialProperties->rasterizationState,
        &multisampleCreateInfo,
        &materialProperties->depthStencilState,
        &materialProperties->colorBlendState,
        nullptr,
        pipelineLayout,
        app.renderPass,
        0,
        VK_NULL_HANDLE, -1
    };

    auto pipeline = &app.graphicsPipelineA;

    if (name == "B"sv) pipeline = &app.graphicsPipelineB;

    if (auto result = vkCreateGraphicsPipelines(app.vulkanDevice->handle(), VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, pipeline); result != VK_SUCCESS)
        throw std::runtime_error("failed to create graphics pipeline: "s + std::to_string(result));
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
            VkClearValue{ .color = { .float32 = { .64f, .64f, .64f, 1.f } } },
            VkClearValue{ .depthStencil = { kREVERSED_DEPTH ? 0.f : 1.f, 0 } }
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

        auto const vertexBuffers = std::array{app.vertexBufferA->handle(), app.vertexBufferB->handle()};
        auto const offsets = std::array{VkDeviceSize{0}, VkDeviceSize{0}};

        auto const bindingCount = static_cast<std::uint32_t>(std::size(offsets));

        vkCmdBindVertexBuffers(commandBuffer, 0, bindingCount, std::data(vertexBuffers), std::data(offsets));

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

        auto const verticesCount = 3u;

        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app.graphicsPipelineA);

            std::size_t const stride = app.alignedBufferSize / app.objectsNumber;
            auto const dynamicOffset = static_cast<std::uint32_t>(0 * stride);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app.pipelineLayoutA,
                                    0, 1, &app.descriptorSet, 1, &dynamicOffset);

            vkCmdDraw(commandBuffer, verticesCount, 1, 0, 0);
        }

        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app.graphicsPipelineB);

            std::size_t const stride = app.alignedBufferSize / app.objectsNumber;
            auto const dynamicOffset = static_cast<std::uint32_t>(1 * stride);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app.pipelineLayoutB,
                                    0, 1, &app.descriptorSet, 1, &dynamicOffset);

            vkCmdDraw(commandBuffer, verticesCount, 1, 0, 0);
        }

        vkCmdEndRenderPass(commandBuffer);

        if (auto result = vkEndCommandBuffer(commandBuffer); result != VK_SUCCESS)
            throw std::runtime_error("failed to end command buffer: "s + std::to_string(result));
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

    app.graphicsQueue = app.vulkanDevice->queue<GraphicsQueue>();
    app.transferQueue = app.vulkanDevice->queue<TransferQueue>();
    app.presentationQueue = app.vulkanDevice->queue<PresentationQueue>();

    CreateCommandPool(app.vulkanDevice->handle(), app.transferQueue, app.transferCommandPool, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    CreateCommandPool(app.vulkanDevice->handle(), app.graphicsQueue, app.graphicsCommandPool, 0);

    auto swapchain = CreateSwapchain(*app.vulkanDevice, app.surface, app.width, app.height,
                                     app.presentationQueue, app.graphicsQueue, app.transferQueue, app.transferCommandPool);

    if (swapchain)
        app.swapchain = std::move(swapchain.value());

    else throw std::runtime_error("failed to create the swapchain"s);

    auto model = temp::populate();
    temp::populate(app);

    if (auto descriptorSetLayout = CreateDescriptorSetLayout(*app.vulkanDevice); !descriptorSetLayout)
        throw std::runtime_error("failed to create the descriptor set layout"s);

    else app.descriptorSetLayout = std::move(descriptorSetLayout.value());

    if (auto renderPass = CreateRenderPass(*app.vulkanDevice, app.swapchain); !renderPass)
        throw std::runtime_error("failed to create the render pass"s);

    else app.renderPass = std::move(renderPass.value());

    if (auto result = glTF::load(sceneName, app.scene, app.nodeSystem); !result)
        throw std::runtime_error("failed to load a mesh"s);

    if (app.vertexBuffer = InitVertexBuffer(app); !app.vertexBuffer)
        throw std::runtime_error("failed to init vertex buffer"s);

    if (!std::empty(app.scene.indexBuffer)) {
        if (app.indexBuffer = InitIndexBuffer(app); !app.indexBuffer)
            throw std::runtime_error("failed to init index buffer"s);
    }


    temp::CreateGraphicsPipeline(app, temp::layoutA, "A"sv);
    temp::CreateGraphicsPipeline(app, temp::layoutB, "B"sv);

    CreateGraphicsPipeline(app);

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

    temp::CreateGraphicsCommandBuffers(app);
    //CreateGraphicsCommandBuffers(app);

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
        app.registry.sort<ecs::mesh>(ecs::mesh());

        app.nodeSystem.update();
        app.meshSystem.update();

        Update(app);

        DrawFrame(app);
    });

    app.cleanUp();

    glfwTerminate();

    return 0;

} catch (std::exception const &ex) {
    std::cout << ex.what() << std::endl;
    std::cin.get();
}
