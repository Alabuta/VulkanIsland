

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
#include "instance.h"
#include "device.h"
#include "swapchain.h"
#include "program.h"
#include "buffer.h"
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
#else
std::vector<Vertex> vertices;
std::vector<std::uint32_t> indices;
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


std::unique_ptr<VulkanInstance> vulkanInstance;
std::unique_ptr<VulkanDevice> vulkanDevice;

VkSurfaceKHR surface;

auto WIDTH = 800u;
auto HEIGHT = 600u;

GraphicsQueue graphicsQueue;
TransferQueue transferQueue;
PresentationQueue presentationQueue;

VkDescriptorSetLayout descriptorSetLayout;
VkPipelineLayout pipelineLayout;
VkRenderPass renderPass;
VkPipeline graphicsPipeline;

VkCommandPool graphicsCommandPool, transferCommandPool;

VkDescriptorPool descriptorPool;
VkDescriptorSet descriptorSet;

std::vector<VkFramebuffer> swapChainFramebuffers;
std::vector<VkCommandBuffer> commandBuffers;

VkSemaphore imageAvailableSemaphore, renderFinishedSemaphore;

VkBuffer vertexBuffer, indexBuffer, uboBuffer;
VkDeviceMemory vertexBufferMemory, indexBufferMemory, uboBufferMemory;

std::uint32_t mipLevels;
VkImage textureImage;
VkDeviceMemory textureImageMemory;
VkImageView textureImageView;
VkSampler textureSampler;




[[nodiscard]] VkImageView CreateImageView(VkDevice device, VkImage &image, VkFormat format, VkImageAspectFlags aspectFlags, std::uint32_t mipLevels)
{
    VkImageViewCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr, 0,
        image,
        VK_IMAGE_VIEW_TYPE_2D,
        format,
        { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
        { aspectFlags, 0, mipLevels, 0, 1 }
    };

    VkImageView imageView;

    if (auto result = vkCreateImageView(device, &createInfo, nullptr, &imageView); result != VK_SUCCESS)
        throw std::runtime_error("failed to create image view: "s + std::to_string(result));

    return imageView;
}


template<class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
[[nodiscard]] std::optional<VkFormat> FindSupportedImageFormat(VkPhysicalDevice physicalDevice, T &&candidates,
                                                               VkImageTiling tiling, VkFormatFeatureFlags features)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkFormat>, "iterable object does not contain 'VkFormat' elements");

    auto it_format = std::find_if(std::cbegin(candidates), std::cend(candidates), [physicalDevice, tiling, features] (auto candidate)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, candidate, &properties);

        switch (tiling) {
            case VK_IMAGE_TILING_LINEAR:
                return (properties.linearTilingFeatures & features) == features;

            case VK_IMAGE_TILING_OPTIMAL:
                return (properties.optimalTilingFeatures & features) == features;

            default:
                return false;
        }
    });

    return it_format != std::cend(candidates) ? *it_format : std::optional<VkFormat>();
}

[[nodiscard]] VkFormat FindDepthImageFormat(VkPhysicalDevice physicalDevice)
{
    auto const format = FindSupportedImageFormat(
        physicalDevice,
        make_array(VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT),
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );

    if (!format)
        throw std::runtime_error("failed to find format for depth attachement"s);

    return format.value();
}


void CleanupSwapChain(VulkanDevice *device, VkSwapchainKHR swapChain, VkPipeline graphicsPipeline, VkPipelineLayout pipelineLayout, VkRenderPass renderPass)
{
    if (graphicsCommandPool)
        vkFreeCommandBuffers(device->handle(), graphicsCommandPool, static_cast<std::uint32_t>(std::size(commandBuffers)), std::data(commandBuffers));

    commandBuffers.clear();

    if (graphicsPipeline)
        vkDestroyPipeline(device->handle(), graphicsPipeline, nullptr);

    if (pipelineLayout)
        vkDestroyPipelineLayout(device->handle(), pipelineLayout, nullptr);

    if (renderPass)
        vkDestroyRenderPass(device->handle(), renderPass, nullptr);

    CleanupSwapChain(device, swapChain);
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



void CreateGraphicsPipeline(VkDevice device)
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
        0, 0,
        static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height),
        1, 0
    };

    VkRect2D const scissor{
        {0, 0}, swapChainExtent
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
        VK_CULL_MODE_NONE,
        VK_FRONT_FACE_CLOCKWISE,
        VK_FALSE, 0, VK_FALSE, 0,
        1
    };

    VkPipelineMultisampleStateCreateInfo constexpr multisampleCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        nullptr, 0,
        VK_SAMPLE_COUNT_1_BIT,
        VK_FALSE,
        1,
        nullptr,
        VK_FALSE,
        VK_FALSE
    };

    VkPipelineDepthStencilStateCreateInfo constexpr depthStencilStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        nullptr, 0,
        VK_TRUE, VK_TRUE,
        VK_COMPARE_OP_GREATER,
        VK_FALSE,
        VK_FALSE, VkStencilOpState{}, VkStencilOpState{},
        1, 0
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
        {0, 0, 0, 0}
    };

    VkPipelineLayoutCreateInfo constexpr layoutCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        nullptr, 0, 
        1, &descriptorSetLayout,
        0, nullptr
    };

    if (auto result = vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr, &pipelineLayout); result != VK_SUCCESS)
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
        pipelineLayout,
        renderPass,
        0,
        VK_NULL_HANDLE, -1
    };

    if (auto result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &graphicsPipeline); result != VK_SUCCESS)
        throw std::runtime_error("failed to create graphics pipeline: "s + std::to_string(result));

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void CreateRenderPass(VkPhysicalDevice physicalDevice, VkDevice device)
{
    VkAttachmentDescription const colorAttachment{
        VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT,
        swapChainImageFormat,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference constexpr colorAttachmentReference{
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription const depthAttachement{
        VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT,
        FindDepthImageFormat(physicalDevice),
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

    if (auto result = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass); result != VK_SUCCESS)
        throw std::runtime_error("failed to create render pass: "s + std::to_string(result));
}



template<class T, class U, typename std::enable_if_t<is_iterable_v<std::decay_t<T>> && is_iterable_v<std::decay_t<U>>>...>
void CreateFramebuffers(VulkanDevice *vulkanDevice, VkRenderPass renderPass, T &&imageViews, U &framebuffers)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkImageView>, "iterable object does not contain VkImageView elements");
    static_assert(std::is_same_v<typename std::decay_t<U>::value_type, VkFramebuffer>, "iterable object does not contain VkFramebuffer elements");

    framebuffers.clear();

    std::transform(std::cbegin(imageViews), std::cend(imageViews), std::back_inserter(framebuffers), [vulkanDevice, renderPass] (auto &&imageView)
    {
        auto const attachements = make_array(imageView, depthImageView);

        VkFramebufferCreateInfo const createInfo{
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            nullptr, 0,
            renderPass,
            static_cast<std::uint32_t>(std::size(attachements)), std::data(attachements),
            swapChainExtent.width, swapChainExtent.height,
            1
        };

        VkFramebuffer framebuffer;

        if (auto result = vkCreateFramebuffer(vulkanDevice->handle(), &createInfo, nullptr, &framebuffer); result != VK_SUCCESS)
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






template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>...>
void TransitionImageLayout(VulkanDevice *vulkanDevice, Q &queue, VkImage image, VkFormat format, VkImageLayout srcLayout, VkImageLayout dstLayout, std::uint32_t mipLevels, VkCommandPool commandPool)
{
    auto commandBuffer = BeginSingleTimeCommand(vulkanDevice, queue, commandPool);

    VkImageMemoryBarrier barrier{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        0, 0,
        srcLayout, dstLayout,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        image,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, 1 }
    };

    if (dstLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    VkPipelineStageFlags srcStageFlags, dstStageFlags;

    if (srcLayout == VK_IMAGE_LAYOUT_UNDEFINED && dstLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }

    else if (srcLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && dstLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        srcStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    else if (srcLayout == VK_IMAGE_LAYOUT_UNDEFINED && dstLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }

    else throw std::logic_error("unsupported layout transition"s);

    vkCmdPipelineBarrier(commandBuffer, srcStageFlags, dstStageFlags, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    EndSingleTimeCommand(vulkanDevice, queue, commandBuffer, commandPool);
}



void CreateVertexBuffer(VulkanDevice *vulkanDevice, VkBuffer &vertexBuffer, VkDeviceMemory &vertexBufferMemory)
{
    VkDeviceSize bufferSize = sizeof(decltype(vertices)::value_type) * std::size(vertices);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    CreateBuffer(vulkanDevice, stagingBuffer, stagingBufferMemory, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    decltype(vertices)::value_type *data;
    if (auto result = vkMapMemory(vulkanDevice->handle(), stagingBufferMemory, 0, bufferSize, 0, reinterpret_cast<void**>(&data)); result != VK_SUCCESS)
        throw std::runtime_error("failed to map vertex buffer memory: "s + std::to_string(result));

    std::uninitialized_copy(std::begin(vertices), std::end(vertices), data);

    vkUnmapMemory(vulkanDevice->handle(), stagingBufferMemory);

    CreateBuffer(vulkanDevice, vertexBuffer, vertexBufferMemory, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    CopyBufferToBuffer(vulkanDevice, transferQueue, stagingBuffer, vertexBuffer, bufferSize, transferCommandPool);

    vkFreeMemory(vulkanDevice->handle(), stagingBufferMemory, nullptr);
    vkDestroyBuffer(vulkanDevice->handle(), stagingBuffer, nullptr);
}

void CreateIndexBuffer(VulkanDevice *vulkanDevice, VkBuffer &indexBuffer, VkDeviceMemory &indexBufferMemory)
{
    VkDeviceSize bufferSize = sizeof(decltype(indices)::value_type) * std::size(indices);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    CreateBuffer(vulkanDevice, stagingBuffer, stagingBufferMemory, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    decltype(indices)::value_type *data;
    if (auto result = vkMapMemory(vulkanDevice->handle(), stagingBufferMemory, 0, bufferSize, 0, reinterpret_cast<void**>(&data)); result != VK_SUCCESS)
        throw std::runtime_error("failed to map vertex buffer memory: "s + std::to_string(result));

    std::uninitialized_copy(std::begin(indices), std::end(indices), data);

    vkUnmapMemory(vulkanDevice->handle(), stagingBufferMemory);

    CreateBuffer(vulkanDevice, indexBuffer, indexBufferMemory, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    CopyBufferToBuffer(vulkanDevice, transferQueue, stagingBuffer, indexBuffer, bufferSize, transferCommandPool);

    vkFreeMemory(vulkanDevice->handle(), stagingBufferMemory, nullptr);
    vkDestroyBuffer(vulkanDevice->handle(), stagingBuffer, nullptr);
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

void CreateDescriptorSet(VkDevice device, VkDescriptorSet &descriptorSet)
{
    auto layouts = make_array(descriptorSetLayout);

    VkDescriptorSetAllocateInfo const allocateInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        descriptorPool,
        static_cast<std::uint32_t>(std::size(layouts)), std::data(layouts)
    };

    if (auto result = vkAllocateDescriptorSets(device, &allocateInfo, &descriptorSet); result != VK_SUCCESS)
        throw std::runtime_error("failed to allocate descriptor sets: "s + std::to_string(result));

    VkDescriptorBufferInfo const bufferInfo{
        uboBuffer, 0, sizeof(TRANSFORMS)
    };

    VkDescriptorImageInfo const imageInfo{
        textureSampler, textureImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    std::cout << &textureImageView << '\n';
    std::cout << &textureSampler << '\n';
    std::cout << &imageInfo << '\n';

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

    std::cout << &writeDescriptorsSet << '\n';

    vkUpdateDescriptorSets(device, static_cast<std::uint32_t>(std::size(writeDescriptorsSet)), std::data(writeDescriptorsSet), 0, nullptr);
}



template<class T, class U, typename std::enable_if_t<is_iterable_v<std::decay_t<T>> && is_iterable_v<std::decay_t<U>>>...>
void CreateCommandBuffers(VulkanDevice *vulkanDevice, VkRenderPass renderPass, VkCommandPool commandPool, T &commandBuffers, U &framebuffers)
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

    if (auto result = vkAllocateCommandBuffers(vulkanDevice->handle(), &allocateInfo, std::data(commandBuffers)); result != VK_SUCCESS)
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
            VkClearValue{{{0.f, 0}}}
        );

        VkRenderPassBeginInfo const renderPassInfo{
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            nullptr,
            renderPass,
            framebuffers.at(i++),
            {{0, 0}, swapChainExtent},
            static_cast<std::uint32_t>(std::size(clearColors)), std::data(clearColors)
        };

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

        auto const vertexBuffers = make_array(vertexBuffer);
        auto const offsets = make_array(VkDeviceSize{0});

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, std::data(vertexBuffers), std::data(offsets));

        auto constexpr index_type = std::is_same_v<decltype(indices)::value_type, std::uint32_t> ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, index_type);

        vkCmdDrawIndexed(commandBuffer, static_cast<std::uint32_t>(std::size(indices)), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (auto result = vkEndCommandBuffer(commandBuffer); result != VK_SUCCESS)
            throw std::runtime_error("failed to end command buffer: "s + std::to_string(result));
    }
}


void CreateSemaphores(VkDevice device)
{
    VkSemaphoreCreateInfo constexpr createInfo{
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        nullptr, 0
    };

    if (auto result = vkCreateSemaphore(device, &createInfo, nullptr, &imageAvailableSemaphore); result != VK_SUCCESS)
        throw std::runtime_error("failed to create image semaphore: "s + std::to_string(result));

    if (auto result = vkCreateSemaphore(device, &createInfo, nullptr, &renderFinishedSemaphore); result != VK_SUCCESS)
        throw std::runtime_error("failed to create render semaphore: "s + std::to_string(result));
}


template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>...>
void GenerateMipMaps(VulkanDevice *vulkanDevice, Q &queue, VkImage image, std::int32_t width, std::int32_t height, std::uint32_t mipLevels, VkCommandPool commandPool)
{
    auto commandBuffer = BeginSingleTimeCommand(vulkanDevice, queue, commandPool);

    VkImageMemoryBarrier barrier{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        0, 0,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        image,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
    };

    for (auto i = 1u; i < mipLevels; ++i) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit const imageBlit{
            { VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1 },
            {{ 0, 0, 0 }, { width, height, 1 }},
            { VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1 },
            {{ 0, 0, 0 }, { width / 2, height / 2, 1 }}
        };

        vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (width > 1) width /= 2;
        if (height > 1) height /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    EndSingleTimeCommand(vulkanDevice, queue, commandBuffer, commandPool);
}


void CreateTextureImage(VulkanDevice *vulkanDevice, VkImage &imageHandle, VkDeviceMemory &imageMemory)
{
    Image image;

    //if (auto result = LoadTARGA("Hebe/textures/HebehebemissinSG1_normal.tga"sv); !result)
    if (auto result = LoadTARGA("sponza/textures/sponza_curtain_blue_diff.tga"sv); !result)
    //if (auto result = LoadTARGA("chalet/textures/chalet.tga"sv); !result)
        throw std::runtime_error("failed to load an image"s);

    else image = result.value();

    mipLevels = static_cast<std::uint32_t>(std::floor(std::log2(std::max(image.width, image.height)))) + 1;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;

    std::visit([&] (auto &&texels)
    {
        using texel_t = typename std::decay_t<decltype(texels)>::value_type;

        auto const bufferSize = static_cast<VkDeviceSize>(std::size(texels) * sizeof(texel_t));

        CreateBuffer(vulkanDevice, stagingBuffer, stagingBufferMemory, bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        texel_t *data;

        if (auto result = vkMapMemory(vulkanDevice->handle(), stagingBufferMemory, 0, bufferSize, 0, reinterpret_cast<void**>(&data)); result != VK_SUCCESS)
            throw std::runtime_error("failed to map image buffer memory: "s + std::to_string(result));

        std::uninitialized_copy(std::begin(texels), std::end(texels), data);

        vkUnmapMemory(vulkanDevice->handle(), stagingBufferMemory);

    }, std::move(image.data));

    CreateImage(vulkanDevice, imageHandle, imageMemory, static_cast<std::uint32_t>(image.width), static_cast<std::uint32_t>(image.height), mipLevels, format,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    TransitionImageLayout(vulkanDevice, transferQueue,
                          imageHandle, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels,
                          transferCommandPool);

    CopyBufferToImage(vulkanDevice, transferQueue, stagingBuffer, imageHandle, static_cast<std::uint32_t>(image.width), static_cast<std::uint32_t>(image.height), transferCommandPool);

    //TransitionImageLayout(device, transferQueue, imageHandle, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);

    GenerateMipMaps(vulkanDevice, transferQueue, imageHandle, image.width, image.height, mipLevels, transferCommandPool);

    vkFreeMemory(vulkanDevice->handle(), stagingBufferMemory, nullptr);
    vkDestroyBuffer(vulkanDevice->handle(), stagingBuffer, nullptr);
}

void CreateTextureImageView(VkDevice device, VkImageView &imageView, VkImage &image, std::uint32_t mipLevels)
{
    imageView = CreateImageView(device, image, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
}

void CreateTextureSampler(VkDevice device, VkSampler &sampler, std::uint32_t mipLevels)
{
    VkSamplerCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        nullptr, 0,
        VK_FILTER_LINEAR, VK_FILTER_LINEAR,
        VK_SAMPLER_MIPMAP_MODE_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        0.f,
        VK_TRUE, 16,
        VK_FALSE, VK_COMPARE_OP_ALWAYS,
        0.f, static_cast<float>(mipLevels),
        VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        VK_FALSE
    };

    if (auto result = vkCreateSampler(device, &createInfo, nullptr, &sampler); result != VK_SUCCESS)
        throw std::runtime_error("failed to create sampler: "s + std::to_string(result));
}


void CreateDepthResources(VulkanDevice *vulkanDevice, VkImage &image, VkDeviceMemory &imageMemory, VkImageView &imageView)
{
    auto const format = FindDepthImageFormat(vulkanDevice->physical_handle());

    CreateImage(vulkanDevice, image, imageMemory, swapChainExtent.width, swapChainExtent.height, 1, format,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    imageView = CreateImageView(vulkanDevice->handle(), image, format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

    TransitionImageLayout(vulkanDevice, transferQueue,
                          image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1,
                          transferCommandPool);
}


void RecreateSwapChain()
{
    if (WIDTH < 1 || HEIGHT < 1) return;

    vkDeviceWaitIdle(vulkanDevice->handle());

    CleanupSwapChain(vulkanDevice.get(), swapChain, graphicsPipeline, pipelineLayout, renderPass);

    CreateSwapChain(vulkanDevice.get(), surface, swapChain, WIDTH, HEIGHT, presentationQueue, graphicsQueue);
    CreateSwapChainImageAndViews(vulkanDevice.get(), swapChainImages, swapChainImageViews);

    CreateDepthResources(vulkanDevice.get(), depthImage, depthImageMemory, depthImageView);

    CreateRenderPass(vulkanDevice->physical_handle(), vulkanDevice->handle());
    CreateGraphicsPipeline(vulkanDevice->handle());

    CreateFramebuffers(vulkanDevice.get(), renderPass, swapChainImageViews, swapChainFramebuffers);

    CreateCommandBuffers(vulkanDevice.get(), renderPass, graphicsCommandPool, commandBuffers, swapChainFramebuffers);
}

void OnWindowResize([[maybe_unused]] GLFWwindow *window, int width, int height)
{
    WIDTH = width;
    HEIGHT = height;

    RecreateSwapChain();
}

void DrawFrame(VkDevice device, VkSwapchainKHR swapChain)
{
    vkQueueWaitIdle(presentationQueue.handle());

    std::uint32_t imageIndex;

    switch (auto result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<std::uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex); result) {
        case VK_ERROR_OUT_OF_DATE_KHR:
            RecreateSwapChain();
            return;

        case VK_SUBOPTIMAL_KHR:
        case VK_SUCCESS:
            break;

        default:
            throw std::runtime_error("failed to acquire next image index: "s + std::to_string(result));
    }

    auto const waitSemaphores = make_array(imageAvailableSemaphore);
    auto const signalSemaphores = make_array(renderFinishedSemaphore);

    std::array<VkPipelineStageFlags, 1> constexpr waitStages{
        { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }
    };

    VkSubmitInfo const submitInfo{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        static_cast<std::uint32_t>(std::size(waitSemaphores)), std::data(waitSemaphores),
        std::data(waitStages),
        1, &commandBuffers.at(imageIndex),
        static_cast<std::uint32_t>(std::size(signalSemaphores)), std::data(signalSemaphores),
    };

    if (auto result = vkQueueSubmit(graphicsQueue.handle(), 1, &submitInfo, VK_NULL_HANDLE); result != VK_SUCCESS)
        throw std::runtime_error("failed to submit draw command buffer: "s + std::to_string(result));

    auto const swapchains = make_array(swapChain);

    VkPresentInfoKHR const presentInfo{
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        nullptr,
        static_cast<std::uint32_t>(std::size(signalSemaphores)), std::data(signalSemaphores),
        static_cast<std::uint32_t>(std::size(swapchains)), std::data(swapchains),
        &imageIndex, nullptr
    };

    switch (auto result = vkQueuePresentKHR(presentationQueue.handle(), &presentInfo); result) {
        case VK_ERROR_OUT_OF_DATE_KHR:
        case VK_SUBOPTIMAL_KHR:
            RecreateSwapChain();
            return;

        case VK_SUCCESS:
            break;

        default:
            throw std::runtime_error("failed to submit request to present framebuffer: "s + std::to_string(result));
    }
}

void InitVulkan(GLFWwindow *window)
{
    vulkanInstance = std::make_unique<VulkanInstance>(extensions, layers);

#if USE_WIN32
    VkWin32SurfaceCreateInfoKHR const win32CreateInfo = {
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        nullptr, 0,
        GetModuleHandle(nullptr),
        glfwGetWin32Window(window)
    };

    vkCreateWin32SurfaceKHR(vkInstance, &win32CreateInfo, nullptr, &vkSurface);
#else
    if (auto result = glfwCreateWindowSurface(vulkanInstance->handle(), window, nullptr, &surface); result != VK_SUCCESS)
        throw std::runtime_error("failed to create window surface: "s + std::to_string(result));
#endif

    QueuePool<
        type_instances_number<GraphicsQueue>,
        type_instances_number<TransferQueue>,
        type_instances_number<PresentationQueue>
    > qpool;

    vulkanDevice = std::make_unique<VulkanDevice>(*vulkanInstance, surface, deviceExtensions, std::move(qpool));

    graphicsQueue = vulkanDevice->Get<GraphicsQueue>();
    transferQueue = vulkanDevice->Get<TransferQueue>();
    presentationQueue = vulkanDevice->Get<PresentationQueue>();

    CreateSwapChain(vulkanDevice.get(), surface, swapChain, WIDTH, HEIGHT, presentationQueue, graphicsQueue);
    CreateSwapChainImageAndViews(vulkanDevice.get(), swapChainImages, swapChainImageViews);

    CreateDescriptorSetLayout(vulkanDevice->handle(), descriptorSetLayout);

    CreateRenderPass(vulkanDevice->physical_handle(), vulkanDevice->handle());
    CreateGraphicsPipeline(vulkanDevice->handle());

    CreateCommandPool(vulkanDevice->handle(), transferQueue, transferCommandPool, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    CreateCommandPool(vulkanDevice->handle(), graphicsQueue, graphicsCommandPool, 0);

    CreateDepthResources(vulkanDevice.get(), depthImage, depthImageMemory, depthImageView);

    CreateFramebuffers(vulkanDevice.get(), renderPass, swapChainImageViews, swapChainFramebuffers);

    CreateTextureImage(vulkanDevice.get(), textureImage, textureImageMemory);
    CreateTextureImageView(vulkanDevice->handle(), textureImageView, textureImage, mipLevels);
    CreateTextureSampler(vulkanDevice->handle(), textureSampler, mipLevels);

    if (auto result = LoadModel("chalet.obj"sv, vertices, indices); !result)
        throw std::runtime_error("failed to load mesh"s);

    CreateVertexBuffer(vulkanDevice.get(), vertexBuffer, vertexBufferMemory);
    CreateIndexBuffer(vulkanDevice.get(), indexBuffer, indexBufferMemory);

    CreateUniformBuffer(vulkanDevice.get(), uboBuffer, uboBufferMemory, sizeof(TRANSFORMS));

    CreateDescriptorPool(vulkanDevice->handle(), descriptorPool);
    CreateDescriptorSet(vulkanDevice->handle(), descriptorSet);

    CreateCommandBuffers(vulkanDevice.get(), renderPass, graphicsCommandPool, commandBuffers, swapChainFramebuffers);

    CreateSemaphores(vulkanDevice->handle());
}

void CleanUp()
{
    vkDeviceWaitIdle(vulkanDevice->handle());

    if (renderFinishedSemaphore)
        vkDestroySemaphore(vulkanDevice->handle(), renderFinishedSemaphore, nullptr);

    if (imageAvailableSemaphore)
        vkDestroySemaphore(vulkanDevice->handle(), imageAvailableSemaphore, nullptr);

    CleanupSwapChain(vulkanDevice.get(), swapChain, graphicsPipeline, pipelineLayout, renderPass);

    vkDestroyDescriptorSetLayout(vulkanDevice->handle(), descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(vulkanDevice->handle(), descriptorPool, nullptr);

    if (textureSampler)
        vkDestroySampler(vulkanDevice->handle(), textureSampler, nullptr);

    if (textureImageView)
        vkDestroyImageView(vulkanDevice->handle(), textureImageView, nullptr);

    if (textureImageMemory)
        vkFreeMemory(vulkanDevice->handle(), textureImageMemory, nullptr);

    if (textureImage)
        vkDestroyImage(vulkanDevice->handle(), textureImage, nullptr);

    if (uboBufferMemory)
        vkFreeMemory(vulkanDevice->handle(), uboBufferMemory, nullptr);

    if (uboBuffer)
        vkDestroyBuffer(vulkanDevice->handle(), uboBuffer, nullptr);

    if (indexBufferMemory)
        vkFreeMemory(vulkanDevice->handle(), indexBufferMemory, nullptr);

    if (indexBuffer)
        vkDestroyBuffer(vulkanDevice->handle(), indexBuffer, nullptr);

    if (vertexBufferMemory)
        vkFreeMemory(vulkanDevice->handle(), vertexBufferMemory, nullptr);

    if (vertexBuffer)
        vkDestroyBuffer(vulkanDevice->handle(), vertexBuffer, nullptr);

    if (transferCommandPool)
        vkDestroyCommandPool(vulkanDevice->handle(), transferCommandPool, nullptr);

    if (graphicsCommandPool)
        vkDestroyCommandPool(vulkanDevice->handle(), graphicsCommandPool, nullptr);

    if (surface)
        vkDestroySurfaceKHR(vulkanInstance->handle(), surface, nullptr);

    vulkanDevice.reset(nullptr);
    vulkanInstance.reset(nullptr);
}

void UpdateUniformBuffer(VkDevice device, std::uint32_t width, std::uint32_t height)
{
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

    // Default OpenGL perspective projection matrix.
    auto constexpr kFOV = 72.f, zNear = .1f, zFar = 1000.f;

#if !USE_GLM
    auto const f = 1.f / std::tan(kFOV * kPI_DIV_180 * 0.5f);

    // Default OpenGL perspective projection matrix.
    auto kA = -(zFar + zNear) / (zFar - zNear);
    auto kB = -2.f * zFar * zNear / (zFar - zNear);

    kA = 0;
    kB = zNear;

    transforms.proj = mat4(
        f / aspect, 0, 0, 0,
        0, -f, 0, 0,
        0, 0, kA, -1,
        0, 0, kB, 0
    );
#else
    transforms.proj = glm::perspective(glm::radians(kFOV), aspect, zNear, zFar);
    transforms.proj[1][1] *= -1;
#endif

    VkDeviceSize bufferSize = sizeof(transforms);

    decltype(transforms) *data;
    if (auto result = vkMapMemory(device, uboBufferMemory, 0, bufferSize, 0, reinterpret_cast<void**>(&data)); result != VK_SUCCESS)
        throw std::runtime_error("failed to map vertex buffer memory: "s + std::to_string(result));

    auto const array = make_array(transforms);
    std::uninitialized_copy(std::begin(array), std::end(array), data);

    vkUnmapMemory(device, uboBufferMemory);
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

    glfwSetWindowSizeCallback(window, OnWindowResize);

    // glfwSetCursorPosCallback(window, CursorCallback);

    std::cout << measure<>::execution(InitVulkan, window) << '\n';
    //InitVulkan(window);

    while (!glfwWindowShouldClose(window) && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
        glfwPollEvents();
        UpdateUniformBuffer(vulkanDevice->handle(), WIDTH, HEIGHT);
        DrawFrame(vulkanDevice->handle(), swapChain);
    }

    CleanUp();

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;

} catch (std::exception const &ex) {
    std::cout << ex.what() << std::endl;
}
