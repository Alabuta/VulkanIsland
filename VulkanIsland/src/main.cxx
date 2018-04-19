

#define _SCL_SECURE_NO_WARNINGS 

#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <chrono>
#include <cmath>
#include <unordered_map>

#include "main.h"

#include "mesh_loader.h"
#include "TARGA_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"


/*namespace std {
template<> struct hash<Vertex> {
    std::size_t operator()(Vertex const& vertex) const
    {
        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.uv) << 1);
    }
};
}*/

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

VkSwapchainKHR swapChain;
VkDescriptorSetLayout descriptorSetLayout;
VkPipelineLayout pipelineLayout;
VkRenderPass renderPass;
VkPipeline graphicsPipeline;

VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;

VkCommandPool graphicsCommandPool, transferCommandPool;

VkDescriptorPool descriptorPool;
VkDescriptorSet descriptorSet;

std::vector<VkImage> swapChainImages;
std::vector<VkImageView> swapChainImageViews;
std::vector<VkFramebuffer> swapChainFramebuffers;
std::vector<VkCommandBuffer> commandBuffers;

VkSemaphore imageAvailableSemaphore, renderFinishedSemaphore;

VkBuffer vertexBuffer, indexBuffer, uboBuffer;
VkDeviceMemory vertexBufferMemory, indexBufferMemory, uboBufferMemory;

VkImage textureImage;
VkDeviceMemory textureImageMemory;
VkImageView textureImageView;
VkSampler textureSampler;

VkImage depthImage;
VkDeviceMemory depthImageMemory;
VkImageView depthImageView;


#ifdef USE_WIN32
VKAPI_ATTR VkResult VKAPI_CALL vkCreateWin32SurfaceKHR(
    VkInstance vulkanInstance->handle(), VkWin32SurfaceCreateInfoKHR const *pCreateInfo, VkAllocationCallbacks const *pAllocator, VkSurfaceKHR *pSurface)
{
    auto traverse = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(vkGetInstanceProcAddr(vulkanInstance->handle(), "vkCreateWin32SurfaceKHR"));

    if (traverse)
        return traverse(vulkanInstance->handle(), pCreateInfo, pAllocator, pSurface);

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}
#endif


[[nodiscard]] SwapChainSupportDetails QuerySwapChainSupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;

    if (auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve device surface capabilities: "s + std::to_string(result));

    std::uint32_t formatsCount = 0;
    if (auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, nullptr); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve device surface formats count: "s + std::to_string(result));

    details.formats.resize(formatsCount);
    if (auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, std::data(details.formats)); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve device surface formats: "s + std::to_string(result));

    if (details.formats.empty())
        return {};

    std::uint32_t presentModeCount = 0;
    if (auto result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve device surface presentation modes count: "s + std::to_string(result));

    details.presentModes.resize(presentModeCount);
    if (auto result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, std::data(details.presentModes)); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve device surface presentation modes: "s + std::to_string(result));

    if (details.presentModes.empty())
        return {};

    return details;
}


[[nodiscard]] std::vector<std::byte> ReadShaderFile(std::string_view _name)
{
    auto current_path = fs::current_path();

    fs::path directory{"shaders"s};
    fs::path name{std::data(_name)};

    if (!fs::exists(current_path / directory))
        directory = current_path / fs::path{"../../VulkanIsland"s} / directory;

    std::ifstream file((directory / name).native(), std::ios::binary);

    if (!file.is_open())
        return {};

    auto const start_pos = file.tellg();
    file.ignore(std::numeric_limits<std::streamsize>::max());

    std::vector<std::byte> shaderByteCode(file.gcount());

    file.seekg(start_pos);

    if (!shaderByteCode.empty())
        file.read(reinterpret_cast<char *>(std::data(shaderByteCode)), shaderByteCode.size());

    return shaderByteCode;
}

template<class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
[[nodiscard]] VkShaderModule CreateShaderModule(VkDevice device, T &&shaderByteCode)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, std::byte>, "iterable object does not contain std::byte elements");

    if (shaderByteCode.size() % sizeof(std::uint32_t) != 0)
        throw std::runtime_error("invalid byte code buffer size");

    VkShaderModuleCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        nullptr, 0,
        shaderByteCode.size(),
        reinterpret_cast<std::uint32_t const *>(std::data(shaderByteCode))
    };

    VkShaderModule shaderModule;

    if (auto result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule); result != VK_SUCCESS)
        throw std::runtime_error("failed to create shader module: "s + std::to_string(result));

    return shaderModule;
}

template<class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
[[nodiscard]] VkSurfaceFormatKHR ChooseSwapSurfaceFormat(T &&surfaceFormats)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkSurfaceFormatKHR>, "iterable object does not contain VkSurfaceFormatKHR elements");

    if (surfaceFormats.size() == 1 && surfaceFormats.at(0).format == VK_FORMAT_UNDEFINED)
        return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

    auto supported = std::any_of(surfaceFormats.cbegin(), surfaceFormats.cend(), [] (auto &&surfaceFormat)
    {
        return surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    });

    if (supported)
        return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

    return surfaceFormats.at(0);
}

template<class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
[[nodiscard]] VkPresentModeKHR ChooseSwapPresentMode(T &&presentModes)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkPresentModeKHR>, "iterable object does not contain VkPresentModeKHR elements");

    auto mailbox = std::any_of(presentModes.cbegin(), presentModes.cend(), [] (auto &&mode)
    {
        return mode == VK_PRESENT_MODE_MAILBOX_KHR;
    });

    if (mailbox)
        return VK_PRESENT_MODE_MAILBOX_KHR;

    auto relaxed = std::any_of(presentModes.cbegin(), presentModes.cend(), [] (auto &&mode)
    {
        return mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    });

    if (relaxed)
        return VK_PRESENT_MODE_FIFO_RELAXED_KHR;

    auto fifo = std::any_of(presentModes.cbegin(), presentModes.cend(), [] (auto &&mode)
    {
        return mode == VK_PRESENT_MODE_FIFO_KHR;
    });

    if (fifo)
        return VK_PRESENT_MODE_FIFO_KHR;

    return VK_PRESENT_MODE_IMMEDIATE_KHR;
}

[[nodiscard]] VkExtent2D ChooseSwapExtent(VkSurfaceCapabilitiesKHR &surfaceCapabilities)
{
    if (surfaceCapabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
        return surfaceCapabilities.currentExtent;

    return {
        std::clamp(WIDTH, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
        std::clamp(HEIGHT, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)
    };
}

[[nodiscard]] VkImageView CreateImageView(VkDevice device, VkImage &image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr, 0,
        image,
        VK_IMAGE_VIEW_TYPE_2D,
        format,
        { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
        { aspectFlags, 0, 1, 0, 1 }
    };

    VkImageView imageView;

    if (auto result = vkCreateImageView(device, &createInfo, nullptr, &imageView); result != VK_SUCCESS)
        throw std::runtime_error("failed to create image view: "s + std::to_string(result));

    return imageView;
}

template<class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
[[nodiscard]] std::optional<VkFormat> FindSupportedFormat(VkPhysicalDevice physicalDevice, T &&candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
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

[[nodiscard]] VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice)
{
    auto const format = FindSupportedFormat(
        physicalDevice,
        make_array(VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT),
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );

    if (!format)
        throw std::runtime_error("failed to find format for depth attachement"s);

    return format.value();
}

void CreateSwapChain(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, VkSwapchainKHR &swapChain)
{
    auto swapChainSupportDetails = QuerySwapChainSupportDetails(physicalDevice, surface);

    auto surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupportDetails.formats);
    auto presentMode = ChooseSwapPresentMode(swapChainSupportDetails.presentModes);
    auto extent = ChooseSwapExtent(swapChainSupportDetails.capabilities);

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

    auto imageCount = swapChainSupportDetails.capabilities.minImageCount + 1;

    if (swapChainSupportDetails.capabilities.maxImageCount > 0)
        imageCount = std::min(imageCount, swapChainSupportDetails.capabilities.maxImageCount);

    VkSwapchainCreateInfoKHR swapchainCreateInfo{
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        nullptr, 0,
        surface,
        imageCount,
        surfaceFormat.format, surfaceFormat.colorSpace,
        extent,
        1,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0, nullptr,
        swapChainSupportDetails.capabilities.currentTransform,
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        presentMode,
        VK_FALSE,
        nullptr
    };

    auto const queueFamilyIndices = make_array(
        graphicsQueue.family(), presentationQueue.family()
    );

    if (graphicsQueue.family() != presentationQueue.family()) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = static_cast<std::uint32_t>(std::size(queueFamilyIndices));
        swapchainCreateInfo.pQueueFamilyIndices = std::data(queueFamilyIndices);
    }

    if (auto result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapChain); result != VK_SUCCESS)
        throw std::runtime_error("failed to create required swap chain: "s + std::to_string(result));

    std::uint32_t imagesCount = 0;
    if (auto result = vkGetSwapchainImagesKHR(device, swapChain, &imagesCount, nullptr); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve swap chain images count: "s + std::to_string(result));

    swapChainImages.resize(imagesCount);
    if (auto result = vkGetSwapchainImagesKHR(device, swapChain, &imagesCount, std::data(swapChainImages)); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve swap chain images: "s + std::to_string(result));
}

template<class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
void CreateSwapChainImageViews(VkDevice device, T &&swapChainImages)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkImage>, "iterable object does not contain VkImage elements");

    swapChainImageViews.clear();

    for (auto &&swapChainImage : swapChainImages) {
        auto imageView = CreateImageView(device, swapChainImage, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        swapChainImageViews.emplace_back(std::move(imageView));
    }
}

void CleanupSwapChain(VkDevice device, VkSwapchainKHR swapChain, VkPipeline graphicsPipeline, VkPipelineLayout pipelineLayout, VkRenderPass renderPass)
{
    for (auto &&swapChainFramebuffer : swapChainFramebuffers)
        vkDestroyFramebuffer(device, swapChainFramebuffer, nullptr);

    swapChainFramebuffers.clear();

    if (graphicsCommandPool)
        vkFreeCommandBuffers(device, graphicsCommandPool, static_cast<std::uint32_t>(std::size(commandBuffers)), std::data(commandBuffers));

    commandBuffers.clear();

    if (graphicsPipeline)
        vkDestroyPipeline(device, graphicsPipeline, nullptr);

    if (pipelineLayout)
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

    if (renderPass)
        vkDestroyRenderPass(device, renderPass, nullptr);

    for (auto &&swapChainImageView : swapChainImageViews)
        vkDestroyImageView(device, swapChainImageView, nullptr);

    swapChainImageViews.clear();

    if (swapChain)
        vkDestroySwapchainKHR(device, swapChain, nullptr);

    if (depthImageView)
        vkDestroyImageView(vulkanDevice->handle(), depthImageView, nullptr);

    if (depthImageMemory)
        vkFreeMemory(vulkanDevice->handle(), depthImageMemory, nullptr);

    if (depthImage)
        vkDestroyImage(vulkanDevice->handle(), depthImage, nullptr);
}


void CreateDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout &descriptorSetLayout)
{
    std::array<VkDescriptorSetLayoutBinding, 2> constexpr layoutBindings{{
        {
            0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1, VK_SHADER_STAGE_VERTEX_BIT,
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

    auto const attributeDescriptions = make_array(
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
        FindDepthFormat(physicalDevice),
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

    VkRenderPassCreateInfo const renderPassCreateInfo{
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        nullptr, 0,
        static_cast<std::uint32_t>(std::size(attachments)), std::data(attachments),
        1, &subpassDescription,
        1, &subpassDependency
    };

    if (auto result = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass); result != VK_SUCCESS)
        throw std::runtime_error("failed to create render pass: "s + std::to_string(result));
}

template<class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
void CreateFramebuffers(VkRenderPass renderPass, T &&swapChainImageViews, VkDevice device)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkImageView>, "iterable object does not contain VkImageView elements");

    swapChainFramebuffers.clear();

    for (auto &&imageView : swapChainImageViews) {
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

        if (auto result = vkCreateFramebuffer(device, &createInfo, nullptr, &framebuffer); result != VK_SUCCESS)
            throw std::runtime_error("failed to create a framebuffer: "s + std::to_string(result));

        swapChainFramebuffers.push_back(std::move(framebuffer));
    }
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

[[nodiscard]] std::optional<std::uint32_t> FindMemoryType(VkPhysicalDevice physicalDevice, std::uint32_t filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    auto const memoryTypes = to_array(memoryProperties.memoryTypes);

    auto it_type = std::find_if(memoryTypes.begin(), memoryTypes.end(), [filter, properties, i = 0](auto type) mutable
    {
        return (filter & (1 << i++)) && (type.propertyFlags & properties) == properties;
    });

    if (it_type != memoryTypes.end())
        return static_cast<std::uint32_t>(std::distance(memoryTypes.begin(), it_type));

    return {};
}

template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>...>
[[nodiscard]] VkCommandBuffer BeginSingleTimeCommand(VkDevice device, [[maybe_unused]] Q &queue)
{
    VkCommandBuffer commandBuffer;

    VkCommandBufferAllocateInfo const allocateInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        transferCommandPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1
    };

    if (auto result = vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer); result != VK_SUCCESS)
        throw std::runtime_error("failed to create allocate command buffers: "s + std::to_string(result));

    VkCommandBufferBeginInfo const beginInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        nullptr
    };

    if (auto result = vkBeginCommandBuffer(commandBuffer, &beginInfo); result != VK_SUCCESS)
        throw std::runtime_error("failed to record command buffer: "s + std::to_string(result));

    return commandBuffer;
}

template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>...>
void EndSingleTimeCommad(VkDevice device, Q &queue, VkCommandBuffer commandBuffer)
{
    if (auto result = vkEndCommandBuffer(commandBuffer); result != VK_SUCCESS)
        throw std::runtime_error("failed to end command buffer: "s + std::to_string(result));

    VkSubmitInfo const submitInfo{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        0, nullptr,
        nullptr,
        1, &commandBuffer,
        0, nullptr,
    };

    if (auto result = vkQueueSubmit(queue.handle(), 1, &submitInfo, VK_NULL_HANDLE); result != VK_SUCCESS)
        throw std::runtime_error("failed to submit command buffer: "s + std::to_string(result));

    vkQueueWaitIdle(queue.handle());

    vkFreeCommandBuffers(device, transferCommandPool, 1, &commandBuffer);
}

void CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice device,
                  VkBuffer &buffer, VkDeviceMemory &deviceMemory, VkDeviceSize size,
                  VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo const bufferCreateInfo{
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        nullptr, 0,
        size,
        usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0, nullptr
    };

    if (auto result = vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer); result != VK_SUCCESS)
        throw std::runtime_error("failed to create vertex buffer: "s + std::to_string(result));

    VkMemoryRequirements memoryReqirements;
    vkGetBufferMemoryRequirements(device, buffer, &memoryReqirements);

    std::uint32_t memTypeIndex = 0;

    if (auto index = FindMemoryType(physicalDevice, memoryReqirements.memoryTypeBits, properties); !index)
        throw std::runtime_error("failed to find suitable memory type"s);

    else memTypeIndex = index.value();

    VkMemoryAllocateInfo const memAllocInfo{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memoryReqirements.size,
        memTypeIndex
    };

    if (auto result = vkAllocateMemory(device, &memAllocInfo, nullptr, &deviceMemory); result != VK_SUCCESS)
        throw std::runtime_error("failed to allocate vertex buffer memory: "s + std::to_string(result));

    if (auto result = vkBindBufferMemory(device, buffer, deviceMemory, 0); result != VK_SUCCESS)
        throw std::runtime_error("failed to bind vertex buffer memory: "s + std::to_string(result));
}

template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>...>
void CopyBuffer(VkDevice device, Q &queue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    auto commandBuffer = BeginSingleTimeCommand(device, queue);

    VkBufferCopy const copyRegion{ 0, 0, size };

    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    EndSingleTimeCommad(device, queue, commandBuffer);
}

template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>...>
void CopyBufferToImage(VkDevice device, Q &queue, VkBuffer srcBuffer, VkImage dstImage, std::uint32_t width, std::uint32_t height)
{
    auto commandBuffer = BeginSingleTimeCommand(device, queue);

    VkBufferImageCopy const copyRegion{
        0,
        0, 0,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        { 0, 0, 0 },
        { width, height, 1 }
    };

    vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    EndSingleTimeCommad(device, queue, commandBuffer);
}

template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, std::decay_t<Q>>>...>
void TransitionImageLayout(VkDevice device, Q &queue, VkImage image, VkFormat format, VkImageLayout srcLayout, VkImageLayout dstLayout)
{
    auto commandBuffer = BeginSingleTimeCommand(device, queue);

    VkImageMemoryBarrier barrier{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        0, 0,
        srcLayout, dstLayout,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        image,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
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

    EndSingleTimeCommad(device, queue, commandBuffer);
}

void LoadModel(std::string_view name)
{
    if (auto result = LoadModel(name, vertices, indices); !result)
        throw std::runtime_error("failed to load mesh"s);
}

void CreateVertexBuffer(VkPhysicalDevice physicalDevice, VkDevice device)
{
    VkDeviceSize bufferSize = sizeof(decltype(vertices)::value_type) * std::size(vertices);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    CreateBuffer(physicalDevice, device, stagingBuffer, stagingBufferMemory, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    decltype(vertices)::value_type *data;
    if (auto result = vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, reinterpret_cast<void**>(&data)); result != VK_SUCCESS)
        throw std::runtime_error("failed to map vertex buffer memory: "s + std::to_string(result));

    std::uninitialized_copy(std::begin(vertices), std::end(vertices), data);

    vkUnmapMemory(device, stagingBufferMemory);

    CreateBuffer(physicalDevice, device, vertexBuffer, vertexBufferMemory, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    CopyBuffer(device, transferQueue, stagingBuffer, vertexBuffer, bufferSize);

    vkFreeMemory(device, stagingBufferMemory, nullptr);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
}

void CreateIndexBuffer(VkPhysicalDevice physicalDevice, VkDevice device)
{
    VkDeviceSize bufferSize = sizeof(decltype(indices)::value_type) * std::size(indices);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    CreateBuffer(physicalDevice, device, stagingBuffer, stagingBufferMemory, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    decltype(indices)::value_type *data;
    if (auto result = vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, reinterpret_cast<void**>(&data)); result != VK_SUCCESS)
        throw std::runtime_error("failed to map vertex buffer memory: "s + std::to_string(result));

    std::uninitialized_copy(std::begin(indices), std::end(indices), data);

    vkUnmapMemory(device, stagingBufferMemory);

    CreateBuffer(physicalDevice, device, indexBuffer, indexBufferMemory, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    CopyBuffer(device, transferQueue, stagingBuffer, indexBuffer, bufferSize);

    vkFreeMemory(device, stagingBufferMemory, nullptr);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
}

void CreateUniformBuffer(VkPhysicalDevice physicalDevice, VkDevice device)
{
    CreateBuffer(physicalDevice, device, uboBuffer, uboBufferMemory, sizeof(TRANSFORMS), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
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

void CreateCommandBuffers(VkDevice device, VkRenderPass renderPass, VkCommandPool commandPool)
{
    commandBuffers.resize(swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo const allocateInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        commandPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        static_cast<std::uint32_t>(std::size(commandBuffers))
    };

    if (auto result = vkAllocateCommandBuffers(device, &allocateInfo, std::data(commandBuffers)); result != VK_SUCCESS)
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

        auto constexpr clearColors = make_array(
            VkClearValue{{{0.64f, 0.64f, 0.64f, 1.f}}},
            VkClearValue{{{0.f, 0}}}
        );

        VkRenderPassBeginInfo const renderPassInfo{
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            nullptr,
            renderPass,
            swapChainFramebuffers.at(i++),
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


void CreateImage(VkPhysicalDevice physicalDevice, VkDevice device,
                 VkImage &image, VkDeviceMemory &deviceMemory, std::uint32_t width, std::uint32_t height,
                 VkFormat format, VkImageTiling tiling, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    VkImageCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr, 0,
        VK_IMAGE_TYPE_2D,
        format,
        {width, height, 1},
        1, 1,
        VK_SAMPLE_COUNT_1_BIT,
        tiling,
        usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0, nullptr,
        VK_IMAGE_LAYOUT_UNDEFINED
    };

    if (auto result = vkCreateImage(device, &createInfo, nullptr, &image); result != VK_SUCCESS)
        throw std::runtime_error("failed to create image: "s + std::to_string(result));

    VkMemoryRequirements memoryReqirements;
    vkGetImageMemoryRequirements(device, image, &memoryReqirements);

    std::uint32_t memTypeIndex = 0;

    if (auto index = FindMemoryType(physicalDevice, memoryReqirements.memoryTypeBits, properties); !index)
        throw std::runtime_error("failed to find suitable memory type"s);

    else memTypeIndex = index.value();

    VkMemoryAllocateInfo const memAllocInfo{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memoryReqirements.size,
        memTypeIndex
    };

    if (auto result = vkAllocateMemory(device, &memAllocInfo, nullptr, &deviceMemory); result != VK_SUCCESS)
        throw std::runtime_error("failed to allocate image buffer memory: "s + std::to_string(result));

    if (auto result = vkBindImageMemory(device, image, deviceMemory, 0); result != VK_SUCCESS)
        throw std::runtime_error("failed to bind image buffer memory: "s + std::to_string(result));
}

void CreateTextureImage(VkPhysicalDevice physicalDevice, VkDevice device, VkImage &image, VkDeviceMemory &imageMemory)
{
    std::vector<std::byte> pixels1;

    if (auto result = LoadTARGA("chalet.tga"sv, pixels1); !result)
        throw std::runtime_error("failed to load an image"s);

    auto current_path = fs::current_path();

    fs::path directory{"textures"s};
    fs::path name{"chalet.jpg"s};

    if (!fs::exists(current_path / directory))
        directory = current_path / fs::path{"../../VulkanIsland"s} / directory;

    auto path = (directory / name).string();

    int width, height, channels;
    auto pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels)
        throw std::runtime_error("failed to load an image"s);

    VkDeviceSize bufferSize = width * height * 4;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    CreateBuffer(physicalDevice, device, stagingBuffer, stagingBufferMemory, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    stbi_uc *data;
    if (auto result = vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, reinterpret_cast<void**>(&data)); result != VK_SUCCESS)
        throw std::runtime_error("failed to map image buffer memory: "s + std::to_string(result));

    std::uninitialized_copy_n(pixels, static_cast<std::size_t>(bufferSize), data);

    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    CreateImage(physicalDevice, device, image, imageMemory, static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    TransitionImageLayout(device, transferQueue, image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    CopyBufferToImage(device, transferQueue, stagingBuffer, image, static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));

    TransitionImageLayout(device, transferQueue, image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkFreeMemory(device, stagingBufferMemory, nullptr);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
}

void CreateTextureImageView(VkDevice device, VkImageView &imageView, VkImage &image)
{
    imageView = CreateImageView(device, image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
}

void CreateTextureSampler(VkDevice device, VkSampler &sampler)
{
    VkSamplerCreateInfo constexpr createInfo{
        VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        nullptr, 0,
        VK_FILTER_LINEAR, VK_FILTER_LINEAR,
        VK_SAMPLER_MIPMAP_MODE_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        0,
        VK_TRUE, 16,
        VK_FALSE, VK_COMPARE_OP_ALWAYS,
        0, 0,
        VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        VK_FALSE
    };

    if (auto result = vkCreateSampler(device, &createInfo, nullptr, &sampler); result != VK_SUCCESS)
        throw std::runtime_error("failed to create sampler: "s + std::to_string(result));
}


void CreateDepthResources(VkPhysicalDevice physicalDevice, VkDevice device, VkImage &image, VkDeviceMemory &imageMemory, VkImageView &imageView)
{
    auto const format = FindDepthFormat(physicalDevice);

    CreateImage(physicalDevice, device, image, imageMemory, swapChainExtent.width, swapChainExtent.height, format,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    imageView = CreateImageView(device, image, format, VK_IMAGE_ASPECT_DEPTH_BIT);

    TransitionImageLayout(device, transferQueue, image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}


void RecreateSwapChain()
{
    if (WIDTH < 1 || HEIGHT < 1) return;

    vkDeviceWaitIdle(vulkanDevice->handle());

    CleanupSwapChain(vulkanDevice->handle(), swapChain, graphicsPipeline, pipelineLayout, renderPass);

    CreateSwapChain(vulkanDevice->physical_handle(), vulkanDevice->handle(), surface, swapChain);
    CreateSwapChainImageViews(vulkanDevice->handle(), swapChainImages);

    CreateRenderPass(vulkanDevice->physical_handle(), vulkanDevice->handle());
    CreateGraphicsPipeline(vulkanDevice->handle());

    CreateDepthResources(vulkanDevice->physical_handle(), vulkanDevice->handle(), depthImage, depthImageMemory, depthImageView);

    CreateFramebuffers(renderPass, swapChainImageViews, vulkanDevice->handle());

    CreateCommandBuffers(vulkanDevice->handle(), renderPass, graphicsCommandPool);
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
        {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}
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

    CreateSwapChain(vulkanDevice->physical_handle(), vulkanDevice->handle(), surface, swapChain);
    CreateSwapChainImageViews(vulkanDevice->handle(), swapChainImages);

    CreateDescriptorSetLayout(vulkanDevice->handle(), descriptorSetLayout);

    CreateRenderPass(vulkanDevice->physical_handle(), vulkanDevice->handle());
    CreateGraphicsPipeline(vulkanDevice->handle());

    CreateCommandPool(vulkanDevice->handle(), transferQueue, transferCommandPool, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    CreateCommandPool(vulkanDevice->handle(), graphicsQueue, graphicsCommandPool, 0);

    CreateDepthResources(vulkanDevice->physical_handle(), vulkanDevice->handle(), depthImage, depthImageMemory, depthImageView);

    CreateFramebuffers(renderPass, swapChainImageViews, vulkanDevice->handle());

    CreateTextureImage(vulkanDevice->physical_handle(), vulkanDevice->handle(), textureImage, textureImageMemory);
    CreateTextureImageView(vulkanDevice->handle(), textureImageView, textureImage);
    CreateTextureSampler(vulkanDevice->handle(), textureSampler);

    LoadModel("chalet.obj"sv);
    CreateVertexBuffer(vulkanDevice->physical_handle(), vulkanDevice->handle());
    CreateIndexBuffer(vulkanDevice->physical_handle(), vulkanDevice->handle());

    CreateUniformBuffer(vulkanDevice->physical_handle(), vulkanDevice->handle());

    CreateDescriptorPool(vulkanDevice->handle(), descriptorPool);
    CreateDescriptorSet(vulkanDevice->handle(), descriptorSet);

    CreateCommandBuffers(vulkanDevice->handle(), renderPass, graphicsCommandPool);

    CreateSemaphores(vulkanDevice->handle());
}

void CleanUp()
{
    vkDeviceWaitIdle(vulkanDevice->handle());

    if (renderFinishedSemaphore)
        vkDestroySemaphore(vulkanDevice->handle(), renderFinishedSemaphore, nullptr);

    if (imageAvailableSemaphore)
        vkDestroySemaphore(vulkanDevice->handle(), imageAvailableSemaphore, nullptr);

    CleanupSwapChain(vulkanDevice->handle(), swapChain, graphicsPipeline, pipelineLayout, renderPass);

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

    transforms.model = glm::rotate(glm::mat4(1.f), .24f * time * glm::radians(90.f), glm::vec3{0, 1, 0});
    transforms.view = glm::lookAt(glm::vec3{1.2f, 0.8f, 1.2f}, glm::vec3{0, .4f, 0}, glm::vec3{0, 1, 0});
#endif
    auto const aspect = static_cast<float>(width) / static_cast<float>(height);

    [[maybe_unused]] auto constexpr kPI = 3.14159265358979323846f;
    [[maybe_unused]] auto constexpr kPI_DIV_180 = 0.01745329251994329576f;
    [[maybe_unused]] auto constexpr kPI_DIV_180_INV = 57.2957795130823208767f;

    // Default OpenGL perspective projection matrix.
    auto constexpr kFOV = 72.f, zNear = .01f, zFar = 1000.f;

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

    InitVulkan(window);

    while (!glfwWindowShouldClose(window)) {
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