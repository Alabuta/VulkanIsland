#define _SCL_SECURE_NO_WARNINGS


#ifdef _MSC_VER
#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib, "glfw3.lib")
#endif

#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <chrono>
#include <cmath>
#include <unordered_map>

#include "main.h"
#include "isle_math.h"
#include "instance.h"
#include "device.h"
#include "swapchain.h"
#include "program.h"
#include "buffer.h"
#include "image.h"
#include "resource.h"
#include "command_buffer.h"



#include "mesh_loader.h"
#include "TARGA_loader.h"


#if 0
auto vertices = make_array(
    Vertex{{+2, +1, 0}, {1, 0, 0}, {1, 1}},
    Vertex{{+2, -1, 0}, {0, 1, 0}, {1, 0}},
    Vertex{{-2, +1, 0}, {0, 0, 1}, {0, 1}},
    Vertex{{-2, -1, 0}, {0, 1, 1}, {0, 0}},

    Vertex{{+1, +2, -1}, {1, 0, 0}, {1, 1}},
    Vertex{{+1, -2, -1}, {0, 1, 0}, {1, 0}},
    Vertex{{-1, +2, -1}, {0, 0, 1}, {0, 1}},
    Vertex{{-1, -2, -1}, {0, 1, 1}, {0, 0}}
);

auto indices = make_array(
    0_ui16, 1_ui16, 2_ui16, 2_ui16, 1_ui16, 3_ui16,
    4_ui16, 5_ui16, 6_ui16, 6_ui16, 5_ui16, 7_ui16
);
#endif

auto mouseX = 0.f, mouseY = 0.f;

#define USE_GLM 1

struct TRANSFORMS {
#if !USE_GLM
    mat4 model;
    mat4 view;
    mat4 proj;
#else
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 modelView;
#endif
} transforms;


auto WIDTH = 800u;
auto HEIGHT = 600u;




struct app_t {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;

    std::unique_ptr<VulkanInstance> vulkanInstance;
    std::unique_ptr<VulkanDevice> vulkanDevice;

    VkSurfaceKHR surface;
    VulkanSwapchain swapchain;

    GraphicsQueue graphicsQueue;
    TransferQueue transferQueue;
    PresentationQueue presentationQueue;

    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    VkPipeline graphicsPipeline;

    VkCommandPool graphicsCommandPool, transferCommandPool;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;

    std::vector<VkCommandBuffer> commandBuffers;

    VkSemaphore imageAvailableSemaphore, renderFinishedSemaphore;

    VkBuffer vertexBuffer, indexBuffer, uboBuffer;
    std::shared_ptr<DeviceMemory> vertexMemory, indexMemory, uboMemory;

    VulkanTexture texture;
    VulkanSampler textureSampler;
};



void CleanupFrameData(app_t &app, VulkanDevice &device, VkPipeline graphicsPipeline, VkPipelineLayout pipelineLayout, VkRenderPass renderPass)
{
    if (app.graphicsCommandPool)
        vkFreeCommandBuffers(device.handle(), app.graphicsCommandPool, static_cast<std::uint32_t>(std::size(app.commandBuffers)), std::data(app.commandBuffers));

    app.commandBuffers.clear();

    if (graphicsPipeline)
        vkDestroyPipeline(device.handle(), graphicsPipeline, nullptr);

    if (pipelineLayout)
        vkDestroyPipelineLayout(device.handle(), pipelineLayout, nullptr);

    if (renderPass)
        vkDestroyRenderPass(device.handle(), renderPass, nullptr);

    CleanupSwapchain(device, app.swapchain);
}


void CreateDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout &descriptorSetLayout)
{
    std::array<VkDescriptorSetLayoutBinding, 2> constexpr layoutBindings{{
        {
            0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        },
        {
            1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1, VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        }
    }};

    VkDescriptorSetLayoutCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        nullptr, 0,
        static_cast<std::uint32_t>(std::size(layoutBindings)), std::data(layoutBindings)
    };

    if (auto result = vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout); result != VK_SUCCESS)
        throw std::runtime_error("failed to create descriptor set layout: "s + std::to_string(result));
}

void CreateDescriptorPool(VkDevice device, VkDescriptorPool &descriptorPool)
{
    std::array<VkDescriptorPoolSize, 2> constexpr poolSizes{{
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    }};

    VkDescriptorPoolCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        nullptr, 0,
        1,
        static_cast<std::uint32_t>(std::size(poolSizes)), std::data(poolSizes)
    };

    if (auto result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool); result != VK_SUCCESS)
        throw std::runtime_error("failed to create descriptor pool: "s + std::to_string(result));
}

void CreateDescriptorSet(app_t &app, VkDevice device, VkDescriptorSet &descriptorSet)
{
    auto layouts = make_array(app.descriptorSetLayout);

    VkDescriptorSetAllocateInfo const allocateInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        app.descriptorPool,
        static_cast<std::uint32_t>(std::size(layouts)), std::data(layouts)
    };

    if (auto result = vkAllocateDescriptorSets(device, &allocateInfo, &descriptorSet); result != VK_SUCCESS)
        throw std::runtime_error("failed to allocate descriptor sets: "s + std::to_string(result));

    VkDescriptorBufferInfo const bufferInfo{
        app.uboBuffer, 0, sizeof(TRANSFORMS)
    };

    VkDescriptorImageInfo const imageInfo{
        app.textureSampler.handle, app.texture.view.handle, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    std::array<VkWriteDescriptorSet, 2> const writeDescriptorsSet{{
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptorSet,
            0, 0,
            1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            nullptr,
            &bufferInfo,
            nullptr
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptorSet,
            1, 0,
            1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            &imageInfo,
            nullptr,
            nullptr
        }
    }};

    vkUpdateDescriptorSets(device, static_cast<std::uint32_t>(std::size(writeDescriptorsSet)), std::data(writeDescriptorsSet), 0, nullptr);
}


[[nodiscard]] std::optional<VkRenderPass>
CreateRenderPass(VulkanDevice const &device, VulkanSwapchain const &swapchain) noexcept
{
    VkAttachmentDescription const colorAttachment{
        0, //VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT,
        swapchain.format,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference constexpr colorAttachmentReference{
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription const depthAttachement{
        0, //VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT,
        swapchain.depthTexture.image->format,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference constexpr depthAttachementReference{
        1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription const subpassDescription{
        0,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        0, nullptr,
        1, &colorAttachmentReference,
        nullptr,
        &depthAttachementReference,
        0, nullptr
    };

    VkSubpassDependency constexpr subpassDependency{
        VK_SUBPASS_EXTERNAL, 0,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        0
    };

    auto const attachments = make_array(colorAttachment, depthAttachement);

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

    auto const vertShaderModule = CreateShaderModule(device, vertShaderByteCode);

    auto const fragShaderByteCode = ReadShaderFile(R"(frag.spv)"sv);

    if (fragShaderByteCode.empty())
        throw std::runtime_error("failed to open fragment shader file"s);

    auto const fragShaderModule = CreateShaderModule(device, fragShaderByteCode);

    VkPipelineShaderStageCreateInfo const vertShaderCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr, 0,
        VK_SHADER_STAGE_VERTEX_BIT,
        vertShaderModule,
        "main",
        nullptr
    };

    VkPipelineShaderStageCreateInfo const fragShaderCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr, 0,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        fragShaderModule,
        "main",
        nullptr
    };

    auto const shaderStages = make_array(
        vertShaderCreateInfo, fragShaderCreateInfo
    );

    auto constexpr vertexInputBindingDescriptions = make_array(
        VkVertexInputBindingDescription{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
    );

    auto constexpr attributeDescriptions = make_array(
        VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
        VkVertexInputAttributeDescription{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
        VkVertexInputAttributeDescription{2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)}
    );

    VkPipelineVertexInputStateCreateInfo const vertexInputCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        nullptr, 0,
        static_cast<std::uint32_t>(std::size(vertexInputBindingDescriptions)), std::data(vertexInputBindingDescriptions),
        static_cast<std::uint32_t>(std::size(attributeDescriptions)), std::data(attributeDescriptions),
    };

    VkPipelineInputAssemblyStateCreateInfo constexpr vertexAssemblyStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        nullptr, 0,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VK_FALSE
    };

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

    VkPipelineRasterizationStateCreateInfo constexpr rasterizer{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        nullptr, 0,
        VK_TRUE,
        VK_FALSE,
        VK_POLYGON_MODE_FILL,
        VK_CULL_MODE_BACK_BIT,
        VK_FRONT_FACE_COUNTER_CLOCKWISE,
        VK_FALSE, 0, VK_FALSE, 0,
        1
    };

    VkPipelineMultisampleStateCreateInfo constexpr multisampleCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        nullptr, 0,
        VK_SAMPLE_COUNT_1_BIT,
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

    VkPipelineLayoutCreateInfo const layoutCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        nullptr, 0, 
        1, &app.descriptorSetLayout,
        0, nullptr
    };

    if (auto result = vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr, &app.pipelineLayout); result != VK_SUCCESS)
        throw std::runtime_error("failed to create pipeline layout: "s + std::to_string(result));

    VkGraphicsPipelineCreateInfo const graphicsPipelineCreateInfo{
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        nullptr,
        VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT,
        static_cast<std::uint32_t>(std::size(shaderStages)), std::data(shaderStages),
        &vertexInputCreateInfo, &vertexAssemblyStateCreateInfo,
        nullptr,
        &viewportStateCreateInfo,
        &rasterizer,
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

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}


void CreateFramebuffers(VulkanDevice const &device, VkRenderPass renderPass, VulkanSwapchain &swapchain)
{
    auto &&framebuffers = swapchain.framebuffers;
    auto &&views = swapchain.views;

    framebuffers.clear();

    std::transform(std::cbegin(views), std::cend(views), std::back_inserter(framebuffers), [&device, renderPass, &swapchain] (auto &&view)
    {
        auto const attachements = make_array(view, swapchain.depthTexture.view.handle);

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


auto CreateVertexBuffer(app_t &app, VulkanDevice &device, VkBuffer &vertexBuffer)
-> std::shared_ptr<DeviceMemory>
{
    VkBuffer stagingBuffer;

    auto stagingMemory = [&] ()
    {
        auto constexpr usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        auto const bufferSize = static_cast<VkDeviceSize>(sizeof(decltype(app.vertices)::value_type) * std::size(app.vertices));

        auto memory = BufferPool::CreateBuffer(device, stagingBuffer, bufferSize, usageFlags, propertyFlags);

        if (memory) {
            void *data;

            if (auto result = vkMapMemory(device.handle(), memory->handle(), memory->offset(), memory->size(), 0, &data); result != VK_SUCCESS)
                throw std::runtime_error("failed to map vertex buffer memory: "s + std::to_string(result));

            std::uninitialized_copy(std::begin(app.vertices), std::end(app.vertices), reinterpret_cast<decltype(app.vertices)::value_type*>(data));

            vkUnmapMemory(device.handle(), memory->handle());
        }

        return memory;
    } ();

    if (stagingMemory) {
        auto constexpr usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        auto constexpr propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        auto memory = BufferPool::CreateBuffer(device, vertexBuffer, stagingMemory->size(), usageFlags, propertyFlags);

        if (memory) {
            auto copyRegions = make_array(
                VkBufferCopy{ /*stagingMemory->offset(), memory->offset()*/0, 0, stagingMemory->size()}
            );

            CopyBufferToBuffer(device, app.transferQueue, stagingBuffer, vertexBuffer, std::move(copyRegions), app.transferCommandPool);
        }

        vkDestroyBuffer(device.handle(), stagingBuffer, nullptr);

        return memory;
    }

    return { };
}

auto CreateIndexBuffer(app_t &app, VulkanDevice &device, VkBuffer &indexBuffer)
-> std::shared_ptr<DeviceMemory>
{
    VkBuffer stagingBuffer;

    auto stagingMemory = [&] ()
    {
        auto constexpr usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        auto const bufferSize = static_cast<VkDeviceSize>(sizeof(decltype(app.indices)::value_type) * std::size(app.indices));

        auto memory = BufferPool::CreateBuffer(device, stagingBuffer, bufferSize, usageFlags, propertyFlags);

        if (memory) {
            decltype(app.indices)::value_type *data;

            if (auto result = vkMapMemory(device.handle(), memory->handle(), memory->offset(), memory->size(), 0, reinterpret_cast<void**>(&data)); result != VK_SUCCESS)
                throw std::runtime_error("failed to map vertex buffer memory: "s + std::to_string(result));

            std::uninitialized_copy(std::begin(app.indices), std::end(app.indices), data);

            vkUnmapMemory(device.handle(), memory->handle());
        }

        return memory;
    } ();

    if (stagingMemory) {
        auto constexpr usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        auto constexpr propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        auto memory = BufferPool::CreateBuffer(device, indexBuffer, stagingMemory->size(), usageFlags, propertyFlags);

        if (memory) {
            auto copyRegions = make_array(
                VkBufferCopy{ /*stagingMemory->offset(), memory->offset()*/0, 0, stagingMemory->size()}
            );

            CopyBufferToBuffer(device, app.transferQueue, stagingBuffer, indexBuffer, std::move(copyRegions), app.transferCommandPool);
        }

        vkDestroyBuffer(device.handle(), stagingBuffer, nullptr);

        return memory;
    }

    return { };
}


template<class T, class U, typename std::enable_if_t<is_iterable_v<std::decay_t<T>> && is_iterable_v<std::decay_t<U>>>...>
void CreateCommandBuffers(app_t &app, VulkanDevice const &device, VkRenderPass renderPass, VkCommandPool commandPool, T &commandBuffers, U &framebuffers)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkCommandBuffer>, "iterable object does not contain VkCommandBuffer elements");
    static_assert(std::is_same_v<typename std::decay_t<U>::value_type, VkFramebuffer>, "iterable object does not contain VkFramebuffer elements");

    commandBuffers.resize(framebuffers.size());

    VkCommandBufferAllocateInfo const allocateInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        commandPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        static_cast<std::uint32_t>(std::size(commandBuffers))
    };

    if (auto result = vkAllocateCommandBuffers(device.handle(), &allocateInfo, std::data(commandBuffers)); result != VK_SUCCESS)
        throw std::runtime_error("failed to create allocate command buffers: "s + std::to_string(result));

    std::size_t i = 0;

    for (auto &commandBuffer : commandBuffers) {
        VkCommandBufferBeginInfo const beginInfo{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            nullptr,
            VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
            nullptr
        };

        if (auto result = vkBeginCommandBuffer(commandBuffer, &beginInfo); result != VK_SUCCESS)
            throw std::runtime_error("failed to record command buffer: "s + std::to_string(result));

        auto const clearColors = make_array(
            VkClearValue{{{0.64f, 0.64f, 0.64f, 1.f}}},
            VkClearValue{{{kREVERSED_DEPTH ? 0 : 1, 0}}}
        );

        VkRenderPassBeginInfo const renderPassInfo{
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            nullptr,
            renderPass,
            framebuffers.at(i++),
            {{0, 0}, app.swapchain.extent},
            static_cast<std::uint32_t>(std::size(clearColors)), std::data(clearColors)
        };

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app.graphicsPipeline);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app.pipelineLayout, 0, 1, &app.descriptorSet, 0, nullptr);

        auto const vertexBuffers = make_array(app.vertexBuffer);
        auto const offsets = make_array(VkDeviceSize{0});

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, std::data(vertexBuffers), std::data(offsets));

        auto constexpr index_type = std::is_same_v<typename decltype(app.indices)::value_type, std::uint32_t> ?
                                    VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;

        vkCmdBindIndexBuffer(commandBuffer, app.indexBuffer, 0, index_type);

        vkCmdDrawIndexed(commandBuffer, static_cast<std::uint32_t>(std::size(app.indices)), 1, 0, 0, 0);

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

std::optional<VulkanTexture> LoadTexture(app_t &app, VulkanDevice &device, std::string_view name)
{
    std::optional<VulkanTexture> texture;

    auto constexpr generateMipMaps = true;

    if (auto rawImage = LoadTARGA(name); rawImage) {
        auto [stagingBuffer, stagingMemory] = StageImage(device, *rawImage);

        if (stagingMemory) {
            auto const width = static_cast<std::uint32_t>(rawImage->width);
            auto const height = static_cast<std::uint32_t>(rawImage->height);

            auto constexpr usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            auto constexpr propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            auto constexpr tiling = VK_IMAGE_TILING_OPTIMAL;

            texture = CreateTexture(device, rawImage->format, width, height, rawImage->mipLevels, tiling, VK_IMAGE_ASPECT_COLOR_BIT, usageFlags, propertyFlags);

            if (texture) {
                TransitionImageLayout(device, app.transferQueue, *texture->image, VK_IMAGE_LAYOUT_UNDEFINED,
                                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, app.transferCommandPool);

                CopyBufferToImage(device, app.transferQueue, stagingBuffer, texture->image->handle, width, height, app.transferCommandPool);

                if (generateMipMaps)
                    GenerateMipMaps(device, app.transferQueue, *texture->image, app.transferCommandPool);

                else TransitionImageLayout(device, app.transferQueue, *texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, app.transferCommandPool);
            }
        }

        vkDestroyBuffer(device.handle(), stagingBuffer, nullptr);
    }

    else std::cerr << "failed to load an image\n"s;

    /*auto sampler = CreateImageSampler(app.vulkanDevice->handle(), image->mipLevels);

    if (!sampler)
        return { };*/

    return texture;
}



void RecreateSwapChain(app_t &app)
{
    if (WIDTH < 1 || HEIGHT < 1) return;

    vkDeviceWaitIdle(app.vulkanDevice->handle());

    CleanupFrameData(app, *app.vulkanDevice, app.graphicsPipeline, app.pipelineLayout, app.renderPass);

    auto swapchain = CreateSwapchain(*app.vulkanDevice, app.surface, WIDTH, HEIGHT,
                                     app.presentationQueue, app.graphicsQueue, app.transferQueue, app.transferCommandPool);

    if (swapchain)
        app.swapchain = std::move(swapchain.value());

    else throw std::runtime_error("failed to create the swapchain"s);

    if (auto renderPass = CreateRenderPass(*app.vulkanDevice, app.swapchain); !renderPass)
        throw std::runtime_error("failed to create the render pass"s);

    else app.renderPass = std::move(renderPass.value());

    CreateGraphicsPipeline(app, app.vulkanDevice->handle());

    CreateFramebuffers(*app.vulkanDevice, app.renderPass, app.swapchain);

    CreateCommandBuffers(app, *app.vulkanDevice, app.renderPass, app.graphicsCommandPool, app.commandBuffers, app.swapchain.framebuffers);
}

void OnWindowResize(GLFWwindow *window, int width, int height)
{
    WIDTH = width;
    HEIGHT = height;

    auto app = reinterpret_cast<app_t *>(glfwGetWindowUserPointer(window));

    RecreateSwapChain(*app);
}

void DrawFrame(VulkanDevice const &vulkanDevice, app_t &app)
{
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

    auto const waitSemaphores = make_array(app.imageAvailableSemaphore);
    auto const signalSemaphores = make_array(app.renderFinishedSemaphore);

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

void InitVulkan(GLFWwindow *window, app_t &app)
{
    app.vulkanInstance = std::make_unique<VulkanInstance>(extensions, layers);

#if USE_WIN32
    VkWin32SurfaceCreateInfoKHR const win32CreateInfo = {
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        nullptr, 0,
        GetModuleHandle(nullptr),
        glfwGetWin32Window(window)
    };

    vkCreateWin32SurfaceKHR(vkInstance, &win32CreateInfo, nullptr, &vkSurface);
#else
    if (auto result = glfwCreateWindowSurface(app.vulkanInstance->handle(), window, nullptr, &app.surface); result != VK_SUCCESS)
        throw std::runtime_error("failed to create window surface: "s + std::to_string(result));
#endif

    QueuePool<
        type_instances_number<GraphicsQueue>,
        type_instances_number<TransferQueue>,
        type_instances_number<PresentationQueue>
    > qpool;

    app.vulkanDevice = std::make_unique<VulkanDevice>(*app.vulkanInstance, app.surface, deviceExtensions, std::move(qpool));

    app.graphicsQueue = app.vulkanDevice->queue<GraphicsQueue>();
    app.transferQueue = app.vulkanDevice->queue<TransferQueue>();
    app.presentationQueue = app.vulkanDevice->queue<PresentationQueue>();

    CreateCommandPool(app.vulkanDevice->handle(), app.transferQueue, app.transferCommandPool, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    CreateCommandPool(app.vulkanDevice->handle(), app.graphicsQueue, app.graphicsCommandPool, 0);

    auto swapchain = CreateSwapchain(*app.vulkanDevice, app.surface, WIDTH, HEIGHT,
                                     app.presentationQueue, app.graphicsQueue, app.transferQueue, app.transferCommandPool);

    if (swapchain)
        app.swapchain = std::move(swapchain.value());

    else throw std::runtime_error("failed to create the swapchain"s);

    CreateDescriptorSetLayout(app.vulkanDevice->handle(), app.descriptorSetLayout);

    if (auto renderPass = CreateRenderPass(*app.vulkanDevice, app.swapchain); !renderPass)
        throw std::runtime_error("failed to create the render pass"s);

    else app.renderPass = std::move(renderPass.value());

    CreateGraphicsPipeline(app, app.vulkanDevice->handle());

    CreateFramebuffers(*app.vulkanDevice, app.renderPass, app.swapchain);

    // "chalet/textures/chalet.tga"sv
    // "Hebe/textures/HebehebemissinSG1_metallicRoughness.tga"sv
    if (auto result = LoadTexture(app, *app.vulkanDevice, "sponza/textures/sponza_curtain_blue_diff.tga"sv); !result)
        throw std::runtime_error("failed to load a texture"s);

    else app.texture = std::move(result.value());

    if (auto result = app.vulkanDevice->resourceManager().CreateImageSampler(app.texture.image->mipLevels); !result)
        throw std::runtime_error("failed to create a texture sampler"s);

    else app.textureSampler = std::move(result.value());

    if (auto result = LoadGLTF("sponza"sv, app.vertices, app.indices); !result)
        throw std::runtime_error("failed to load a mesh"s);

    app.vertexMemory = CreateVertexBuffer(app, *app.vulkanDevice, app.vertexBuffer);
    app.indexMemory = CreateIndexBuffer(app, *app.vulkanDevice, app.indexBuffer);

    app.uboMemory = BufferPool::CreateUniformBuffer(*app.vulkanDevice, app.uboBuffer, sizeof(TRANSFORMS));

    CreateDescriptorPool(app.vulkanDevice->handle(), app.descriptorPool);
    CreateDescriptorSet(app, app.vulkanDevice->handle(), app.descriptorSet);

    CreateCommandBuffers(app, *app.vulkanDevice, app.renderPass, app.graphicsCommandPool, app.commandBuffers, app.swapchain.framebuffers);

    CreateSemaphores(app, app.vulkanDevice->handle());
}

void CleanUp(app_t &app)
{
    vkDeviceWaitIdle(app.vulkanDevice->handle());

    if (app.renderFinishedSemaphore)
        vkDestroySemaphore(app.vulkanDevice->handle(), app.renderFinishedSemaphore, nullptr);

    if (app.imageAvailableSemaphore)
        vkDestroySemaphore(app.vulkanDevice->handle(), app.imageAvailableSemaphore, nullptr);

    CleanupFrameData(app, *app.vulkanDevice, app.graphicsPipeline, app.pipelineLayout, app.renderPass);

    vkDestroyDescriptorSetLayout(app.vulkanDevice->handle(), app.descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(app.vulkanDevice->handle(), app.descriptorPool, nullptr);

    vkDestroySampler(app.vulkanDevice->handle(), app.textureSampler.handle, nullptr);
    vkDestroyImageView(app.vulkanDevice->handle(), app.texture.view.handle, nullptr);

    app.texture.image->memory.reset();
    app.texture.image.reset();

    if (app.uboBuffer)
        vkDestroyBuffer(app.vulkanDevice->handle(), app.uboBuffer, nullptr);

    if (app.indexBuffer)
        vkDestroyBuffer(app.vulkanDevice->handle(), app.indexBuffer, nullptr);

    if (app.vertexBuffer)
        vkDestroyBuffer(app.vulkanDevice->handle(), app.vertexBuffer, nullptr);

    app.vertexMemory.reset();
    app.indexMemory.reset();
    app.uboMemory.reset();

    if (app.transferCommandPool)
        vkDestroyCommandPool(app.vulkanDevice->handle(), app.transferCommandPool, nullptr);

    if (app.graphicsCommandPool)
        vkDestroyCommandPool(app.vulkanDevice->handle(), app.graphicsCommandPool, nullptr);

    if (app.surface)
        vkDestroySurfaceKHR(app.vulkanInstance->handle(), app.surface, nullptr);

    app.vulkanDevice.reset(nullptr);
    app.vulkanInstance.reset(nullptr);
}

void UpdateUniformBuffer(VulkanDevice const &device, DeviceMemory const &memory, std::uint32_t width, std::uint32_t height)
{
    if (width * height < 1) return;

#if !USE_GLM
    transforms.model = mat4{
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    transforms.view = mat4(
        1, 0, 0, 0,
        0, 0.707106709, 0.707106709, 0,
        0, -0.707106709, 0.707106709, 0,
        0, 0, -1.41421354, 1
    );

    transforms.view = lookAt(vec3{0, 1, 1}, vec3{0, 0, 0}, vec3{0, 1, 0});

    auto view = glm::lookAt(glm::vec3{0, 1, 1}, glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0});
#else
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    transforms.model = glm::mat4(1.f);
    //transforms.model = glm::rotate(transforms.model, .24f * time * glm::radians(90.f), glm::vec3{0, 1, 0});
    transforms.model = glm::rotate(transforms.model, glm::radians(90.f), glm::vec3{1, 0, 0});
    transforms.model = glm::rotate(transforms.model, glm::radians(90.f), glm::vec3{0, 0, 1});
    //transforms.model = glm::scale(transforms.model, glm::vec3{.1f, .1f, .1f});
    //transforms.model = glm::translate(transforms.model, {0, 0, -250});
    //transforms.model = glm::rotate(glm::mat4(1.f), .24f * time * glm::radians(90.f), glm::vec3{0, 1, 0});// *glm::scale(glm::mat4(1.f), {.0f, .0f, .0f});

    /*auto translate = glm::vec3{0.f, 4.f, 0.f + 0*std::sin(time) * 40.f};

    transforms.view = glm::mat4(1.f);
    transforms.view = glm::translate(transforms.view, translate);*/
    transforms.view = glm::lookAt(glm::vec3{10.f, 20.f, 0.f + std::sin(time * .4f) * 64.f}, glm::vec3{0, 10.f, 0}, glm::vec3{0, 1, 0});


    transforms.modelView = transforms.view * transforms.model;
#endif
    auto const aspect = static_cast<float>(width) / static_cast<float>(height);

    [[maybe_unused]] auto constexpr kPI = 3.14159265358979323846f;
    [[maybe_unused]] auto constexpr kPI_DIV_180 = 0.01745329251994329576f;
    [[maybe_unused]] auto constexpr kPI_DIV_180_INV = 57.2957795130823208767f;

    auto constexpr kFOV = 72.f, zNear = .1f, zFar = 1000.f;
    auto const f = 1.f / std::tan(kFOV * kPI_DIV_180 * .5f);

    auto kA = -zFar / (zFar - zNear);
    auto kB = -zFar * zNear / (zFar - zNear);

    if constexpr (kREVERSED_DEPTH) {
        kA = -kA - 1;
        kB *= -1;
    }

    auto proj = mat4(
        f / aspect, 0, 0, 0,
        0, f, 0, 0,
        0, 0, kA, -1,
        0, 0, kB, 0
    );

    transforms.proj = glm::make_mat4(std::data(proj.m));
    //transforms.proj = glm::perspective(glm::radians(kFOV), aspect, zNear, zFar);

    void *data;
    if (auto result = vkMapMemory(device.handle(), memory.handle(), memory.offset(), memory.size(), 0, &data); result != VK_SUCCESS)
        throw std::runtime_error("failed to map vertex buffer memory: "s + std::to_string(result));

    auto const array = make_array(transforms);
    std::uninitialized_copy(std::begin(array), std::end(array), reinterpret_cast<decltype(transforms) *>(data));

    vkUnmapMemory(device.handle(), memory.handle());
}

/* void CursorCallback(GLFWwindow *window, double x, double y)
{
    mouseX = WIDTH * .5f - static_cast<float>(x);
    mouseY = HEIGHT * .5f - static_cast<float>(y);
}*/


int main()
try {
#ifdef _MSC_VER
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(84);
#endif

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(WIDTH, HEIGHT, "VulkanIsland", nullptr, nullptr);

    app_t app;
    glfwSetWindowUserPointer(window, &app);

    glfwSetWindowSizeCallback(window, OnWindowResize);

    // glfwSetCursorPosCallback(window, CursorCallback);

    std::cout << measure<>::execution(InitVulkan, window, std::ref(app)) << '\n';
    //InitVulkan(window);

    while (!glfwWindowShouldClose(window) && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
        glfwPollEvents();
        UpdateUniformBuffer(*app.vulkanDevice.get(), *app.uboMemory, WIDTH, HEIGHT);
        DrawFrame(*app.vulkanDevice, app);
    }

    CleanUp(app);

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;

} catch (std::exception const &ex) {
    std::cout << ex.what() << std::endl;
    std::cin.get();
}
