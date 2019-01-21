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
#include "program.hxx"
#include "buffer.hxx"
#include "image.hxx"
#include "resource.hxx"
#include "descriptor.hxx"
#include "commandBuffer.hxx"
#include "pipeline.hxx"

#include "loaders/loaderGLTF.hxx"
#include "loaderTARGA.hxx"

#include "mesh.hxx"
#include "sceneTree.hxx"

#include "input/inputManager.hxx"
#include "camera/cameraController.hxx"


#define USE_GLM 1
#define USE_ALIGNMENT 1
#define USE_DYNAMIC_PIPELINE_STATE 0


struct per_object_t {
    glm::mat4 world{1};
    glm::mat4 normal{1};  // Transposed of the inversed of the upper left 3x3 sub-matrix of model(world)-view matrix.
};




template<class T>
class aligned_iterator {
public:

    //aligned_iterator(aligned_iterator const &);
    //~aligned_iterator();

    //aligned_iterator& operator= (const aligned_iterator &);
    aligned_iterator &operator++ ()
    {
        ++current;
        return;
    }

    T &operator* () noexcept { return *current; }

private:

    T *current;
};

template<class T>
class aligned_forward_iterator {
public:

    ;

private:
};


namespace std
{
    template<typename T>
    struct iterator_traits<aligned_forward_iterator<T>> {
        using difference_type = std::ptrdiff_t;
        using value_type = std::remove_cv_t<T>;
        using pointer = T * ;
        using reference = T & ;
        using iterator_category = forward_iterator_tag;
    };
}

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
    VulkanSwapchain swapchain{ VK_NULL_HANDLE };

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


    ~app_t()
    {
        cleanUp();
    }

    void cleanUp()
    {
        if (vulkanDevice == nullptr)
            return;

        vkDeviceWaitIdle(vulkanDevice->handle());

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

#if USE_ALIGNMENT
        if (alignedBuffer)
            boost::alignment::aligned_free(alignedBuffer);
#endif

        perCameraBuffer.reset();
        perObjectBuffer.reset();

        uboBuffer.reset();

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


[[nodiscard]] std::optional<VkRenderPass>
CreateRenderPass(VulkanDevice const &device, VulkanSwapchain const &swapchain) noexcept
{
    VkAttachmentDescription const colorAttachment{
        0, //VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT,
        swapchain.format,
        device.samplesCount(),
        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference constexpr colorAttachmentReference{
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription const depthAttachement{
        0, //VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT,
        swapchain.depthTexture.image->format(),
        device.samplesCount(),
        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference constexpr depthAttachementReference{
        1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription const colorAttachmentResolve{
        0,
        swapchain.format,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference constexpr colorAttachmentResolveReference{
        2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription const subpassDescription{
        0,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        0, nullptr,
        1, &colorAttachmentReference,
        &colorAttachmentResolveReference,
        &depthAttachementReference,
        0, nullptr
    };

    VkSubpassDependency constexpr subpassDependency{
        VK_SUBPASS_EXTERNAL, 0,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        0
    };

    auto const attachments = std::array{colorAttachment, depthAttachement, colorAttachmentResolve};

#if NOT_YET_IMPLEMENTED
    VkInputAttachmentAspectReference const depthAttachmentAspectReference{
        0, 0, VK_IMAGE_ASPECT_DEPTH_BIT
    };

    VkRenderPassInputAttachmentAspectCreateInfo const depthAttachmentAspect{
        VK_STRUCTURE_TYPE_RENDER_PASS_INPUT_ATTACHMENT_ASPECT_CREATE_INFO,
        nullptr,
        1, &depthAttachmentAspectReference
    };
#endif

    VkRenderPassCreateInfo const renderPassCreateInfo{
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        nullptr,
        0,
        static_cast<std::uint32_t>(std::size(attachments)), std::data(attachments),
        1, &subpassDescription,
        1, &subpassDependency
    };

    VkRenderPass handle;

    if (auto result = vkCreateRenderPass(device.handle(), &renderPassCreateInfo, nullptr, &handle); result != VK_SUCCESS) {
        std::cerr << "failed to create render pass: "s << result << '\n';
        return { };
    }

    return handle;
}


void CreateGraphicsPipeline(app_t &app, VkDevice device)
{
    auto const vertShaderByteCode = ReadShaderFile(R"(vert.spv)"sv);

    if (vertShaderByteCode.empty())
        throw std::runtime_error("failed to open vertex shader file"s);

    auto const vertShaderModule = app.vulkanDevice->resourceManager().CreateShaderModule(vertShaderByteCode);

    auto const fragShaderByteCode = ReadShaderFile(R"(frag.spv)"sv);

    if (fragShaderByteCode.empty())
        throw std::runtime_error("failed to open fragment shader file"s);

    auto const fragShaderModule = app.vulkanDevice->resourceManager().CreateShaderModule(fragShaderByteCode);

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

    auto shaderStages = std::array{ vertShaderCreateInfo, fragShaderCreateInfo };

    VertexInputStateInfo vertexInputStateCreateInfo{app.scene.meshes.front().submeshes.front().vertices.layout};

    VkPipelineInputAssemblyStateCreateInfo constexpr vertexAssemblyStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        nullptr, 0,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VK_FALSE
    };

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

    auto constexpr rasterizerDiscardEnable = VK_FALSE;
#endif

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

    VkPipelineMultisampleStateCreateInfo const multisampleCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        nullptr, 0,
        app.vulkanDevice->samplesCount(),//VK_SAMPLE_COUNT_1_BIT
        VK_FALSE, 1,
        nullptr,
        VK_FALSE,
        VK_FALSE
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
        &rasterizerState,
        &multisampleCreateInfo,
        &depthStencilStateCreateInfo,
        &colorBlendStateCreateInfo,
        nullptr,
        app.pipelineLayout,
        app.renderPass,
        0,
        VK_NULL_HANDLE, -1
    };

    if (auto result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &app.graphicsPipeline); result != VK_SUCCESS)
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


[[nodiscard]] std::shared_ptr<VulkanBuffer>
CreateUniformBuffer(VulkanDevice &device, std::size_t size)
{
    auto constexpr usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    return device.resourceManager().CreateBuffer(size, usageFlags, propertyFlags);
}

[[nodiscard]] std::shared_ptr<VulkanBuffer>
CreateCoherentStorageBuffer(VulkanDevice &device, std::size_t size)
{
    auto constexpr usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    return device.resourceManager().CreateBuffer(size, usageFlags, propertyFlags);
}

[[nodiscard]] std::shared_ptr<VulkanBuffer>
CreateStorageBuffer(VulkanDevice &device, std::size_t size)
{
    auto constexpr usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    return device.resourceManager().CreateBuffer(size, usageFlags, propertyFlags);
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

        //auto const descriptorSets = std::array{app.descriptorSet};

        /*vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app.pipelineLayout,
                                0, static_cast<std::uint32_t>(std::size(descriptorSets)), std::data(descriptorSets), 0, nullptr);*/

        auto const vertexBuffers = std::array{app.vertexBuffer->handle()};
        auto const offsets = std::array{VkDeviceSize{0}};

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, std::data(vertexBuffers), std::data(offsets));

        /*if (app.indexBuffer) {
            auto index_type = std::visit([] (auto type)
            {
                return std::is_same_v<typename decltype(type), std::uint32_t> ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;

            }, app.scene.meshes.front().submeshes.front().indices.type);

            vkCmdBindIndexBuffer(commandBuffer, app.indexBuffer->handle(), 0, index_type);
        }*/

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

void CreateSemaphores(app_t &app, VkDevice device)
{
    VkSemaphoreCreateInfo constexpr createInfo{
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        nullptr, 0
    };

    if (auto result = vkCreateSemaphore(device, &createInfo, nullptr, &app.imageAvailableSemaphore); result != VK_SUCCESS)
        throw std::runtime_error("failed to create image semaphore: "s + std::to_string(result));

    if (auto result = vkCreateSemaphore(device, &createInfo, nullptr, &app.renderFinishedSemaphore); result != VK_SUCCESS)
        throw std::runtime_error("failed to create render semaphore: "s + std::to_string(result));
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
    CreateGraphicsPipeline(app, app.vulkanDevice->handle());
#endif

    CreateFramebuffers(*app.vulkanDevice, app.renderPass, app.swapchain);

    CreateGraphicsCommandBuffers(app);
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

    if (auto descriptorSetLayout = CreateDescriptorSetLayout(*app.vulkanDevice); !descriptorSetLayout)
        throw std::runtime_error("failed to create the descriptor set layout"s);

    else app.descriptorSetLayout = std::move(descriptorSetLayout.value());

    if (auto renderPass = CreateRenderPass(*app.vulkanDevice, app.swapchain); !renderPass)
        throw std::runtime_error("failed to create the render pass"s);

    else app.renderPass = std::move(renderPass.value());

    if (auto result = glTF::load("unlit-test"sv, app.scene); !result)
        throw std::runtime_error("failed to load a mesh"s);

    if (app.vertexBuffer = InitVertexBuffer(app); !app.vertexBuffer)
        throw std::runtime_error("failed to init vertex buffer"s);

    if (!std::empty(app.scene.indexBuffer)) {
        if (app.indexBuffer = InitIndexBuffer(app); !app.indexBuffer)
            throw std::runtime_error("failed to init index buffer"s);
    }

    CreateGraphicsPipeline(app, app.vulkanDevice->handle());

    CreateFramebuffers(*app.vulkanDevice, app.renderPass, app.swapchain);

    // "chalet/textures/chalet.tga"sv
    // "Hebe/textures/HebehebemissinSG1_metallicRoughness.tga"sv
    if (auto result = LoadTexture(app, *app.vulkanDevice, "sponza/textures/sponza_curtain_blue_diff.tga"sv); !result)
        throw std::runtime_error("failed to load a texture"s);

    else app.texture = std::move(result.value());

    if (auto result = app.vulkanDevice->resourceManager().CreateImageSampler(app.texture.image->mipLevels()); !result)
        throw std::runtime_error("failed to create a texture sampler"s);

    else app.texture.sampler = result;

#if USE_ALIGNMENT
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

#else
    if (app.perObjectBuffer = CreateStorageBuffer(*app.vulkanDevice, sizeof(per_object_t)); !app.perObjectBuffer)
        throw std::runtime_error("failed to init per object uniform buffer"s);
#endif

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

    CreateSemaphores(app, app.vulkanDevice->handle());
}



template<class T>
struct Allocator final {
    using value_type = T;

    using pointer = T *;
    using const_pointer = T const *;

    using reference = T &;
    using const_reference = T const &;

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    Allocator() noexcept = default;

    template <class U>
    constexpr Allocator(Allocator<U> const &) noexcept { }

    [[nodiscard]] T *allocate(std::size_t n)
    {
        return nullptr;
    }
};


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
        object.world = glm::translate(glm::mat4{ 1.f }, glm::vec3{ 2 * instanceIndex, 0, 0 });
        object.world = glm::rotate(object.world, glm::radians(-90.f), glm::vec3{ 1, 0, 0 });
        //object.world = glm::scale(object.world, glm::vec3{.01f});

        object.normal = glm::inverseTranspose(object.world);

        auto dstPtr = reinterpret_cast<std::byte *>(app.alignedBuffer) + stride * instanceIndex;

        //std::uninitialized_copy_n(&object, 1, reinterpret_cast<per_object_t *>(app.alignedBuffer));
        memcpy(dstPtr, &object, sizeof(std::decay_t<decltype(object)>));

    /*#if USE_ALIGNMENT
        memcpy(app.perObjectsMappedPtr, dstPtr, stride);
    #else
        std::uninitialized_copy_n(&app.object, 1, reinterpret_cast<per_object_t *>(app.alignedBufferSize));
    #endif*/

        ++instanceIndex;
    }

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




int main()
try {
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(84);
#endif

#if 0
    SceneTree sceneTree;

    if (auto node = sceneTree.AttachNode(sceneTree.root()); node)
        sceneTree.AttachNode(*node);

    if (auto node = sceneTree.AddChild(sceneTree.root()); node) {
        sceneTree.AddChild(*node);
		auto middle = sceneTree.AddChild(*node);
		sceneTree.AddChild(*node);

		if (middle)
			sceneTree.DestroyNode(*middle);

        //if (auto child = sceneTree.AddChild(*node); child)
        //    sceneTree.DestroyNode(*child);

        sceneTree.AddChild(*node);
    }
#endif

    /*
    AttachNode(parent node handle)
        Calculate the child depth index
        Get children layer at the child depth index (create if there isn't)

        1. Parent does have children
            Find a chunk range adjacent to the children range end

            1. 1. If there is the requsted chunk
                Emplace a new child node at chunk position
                Create a node handle to newly emplaced the child node

                Extent the children range end by one index

                Update the chunk range
                
            1. 2. If there is not
                Find a range of continuous chunks for all children plus one

                1. 2. 1. If there is such a range
                    Move all children nodes plus one to available chunks
                    Create a node handle to newly emplaced the child node
                    Update children node handles

                    Recalculate the parent children range

                    Update available chunks' ranges

                1. 2. 2. If there isn't
                    Append to the children layer end all children nodes plus one
                    Create a node handle to newly emplaced the child node
                    Update children node handles

                    Recalculate the parent children range

                    Put the empty nodes to a chunk range


        2. Parent doesn't have children
            Find a chunk range inside the children layer for child node emplacement

            2. 1. If there is chunk range
                Emplace a new child node at chunk position

                Update the chunk range

            2. 2. If there is not
                Append to the children layer end a new child node

            Create a node handle to newly emplaced the child node

            Update the parent children range

        return node handle

        */

    /*

    DestroyNode(node handle)
        Validate node handle

        Get and validate node
        Get and validate node info

        Get node chidren range

        Get and validate parent handle
        Get and validate parent node
        Get and validate parent node info

        Get parent children range

        1. If parent has children more than one
            1. 1. If node placed at the begining of parent's children range
                Increment parent's children range begin index

            1. 2. If node placed at the end of parent's children range
                Decrement parent's children range end index

            1. 3. Else
                Move following nodes by one index back
                Update moved nodes handles

                Decrement parent's children range end index

        2. If it hasn't
            Reset parent's children range

            Add new empty node to available chunks

        3. Node does have children
            Call DestroyChildren(node handle).

        Invalidate node handle

        return nothing
        */

    /*DestroyChildren(node handle)
        Validate node handle

        Get and validate node
        Get and validate node info

        Get node chidren range

        
        ...

        Collect child's node hadle indices

        Traverse vector of child nodes handle indexes and invalidate them

        Reset parent's children range

        return nothing
    */

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
    app.cameraController->lookAt(glm::vec3{.8, 2.4, 2.4}, {0, .8, 0});

    std::cout << measure<>::execution(InitVulkan, window, std::ref(app)) << " ms\n"s;

    window.update([&app]
    {
        glfwPollEvents();

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
