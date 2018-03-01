
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>


#include <filesystem>

#include "main.h"

#if X

VkSurfaceKHR surface;

auto WIDTH = 800u;
auto HEIGHT = 600u;

VkInstance instance;
VkDebugReportCallbackEXT debugReportCallback;
VkPhysicalDevice physicalDevice;
VkDevice device;
VkQueue graphicsQueue, transferQueue, presentationQueue;
VkSwapchainKHR swapChain;
VkPipelineLayout pipelineLayout;
VkRenderPass renderPass;
VkPipeline graphicsPipeline;

VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;

VkCommandPool commandPool;

std::vector<std::uint32_t> supportedQueuesIndices;
std::vector<VkImage> swapChainImages;
std::vector<VkImageView> swapChainImageViews;
std::vector<VkFramebuffer> swapChainFramebuffers;
std::vector<VkCommandBuffer> commandBuffers;

VkSemaphore imageAvailable, renderFinsihed;

#ifdef USE_WIN32
VKAPI_ATTR VkResult VKAPI_CALL vkCreateWin32SurfaceKHR(
    VkInstance instance, VkWin32SurfaceCreateInfoKHR const *pCreateInfo, VkAllocationCallbacks const *pAllocator, VkSurfaceKHR *pSurface)
{
    auto func = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR"));

    if (func)
        return func(instance, pCreateInfo, pAllocator, pSurface);

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}
#endif


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
        supportedQueuesIndices.at(0), supportedQueuesIndices.at(2)
    );

    if (supportedQueuesIndices.at(0) != supportedQueuesIndices.at(2)) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = std::data(queueFamilyIndices);
    }

    if (auto result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapChain); result != VK_SUCCESS)
        throw std::runtime_error("failed to create required swap chain: "s + std::to_string(result));

    std::uint32_t imagesCount = 0;
    if (auto result = vkGetSwapchainImagesKHR(device, swapChain, &imagesCount, nullptr); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve swap chain images count: "s + std::to_string(result));

    swapChainImages.resize(imagesCount);
    if (auto result = vkGetSwapchainImagesKHR(device, swapChain, &imagesCount, std::data(swapChainImages)); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve swap chain iamges: "s + std::to_string(result));
}

template<class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
void CreateSwapChainImageViews(VkDevice device, T &&swapChainImages)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkImage>, "iterable object does not contain VkImage elements");

    swapChainImageViews.clear();

    for (auto &&swapChainImage : swapChainImages) {
        VkImageViewCreateInfo const createInfo{
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            nullptr, 0,
            swapChainImage,
            VK_IMAGE_VIEW_TYPE_2D,
            swapChainImageFormat,
            {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
        };

        VkImageView imageView;

        if (auto result = vkCreateImageView(device, &createInfo, nullptr, &imageView); result != VK_SUCCESS)
            throw std::runtime_error("failed to create swap chain image view: "s + std::to_string(result));

        swapChainImageViews.push_back(std::move(imageView));
    }
}

void CleanupSwapChain(VkDevice device, VkSwapchainKHR swapChain, VkPipeline graphicsPipeline, VkPipelineLayout pipelineLayout, VkRenderPass renderPass)
{
    for (auto &&swapChainFramebuffer : swapChainFramebuffers)
        vkDestroyFramebuffer(device, swapChainFramebuffer, nullptr);

    swapChainFramebuffers.clear();

    if (commandPool)
        vkFreeCommandBuffers(device, commandPool, static_cast<std::uint32_t>(std::size(commandBuffers)), std::data(commandBuffers));

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
}


[[nodiscard]] std::vector<std::byte> ReadShaderFile(std::string_view _name)
{
    auto current_path = std::experimental::filesystem::current_path();

    std::experimental::filesystem::path directory{"shaders"s};
    std::experimental::filesystem::path name{std::data(_name)};

    if (!std::experimental::filesystem::exists(current_path / directory))
        directory = current_path / "../../VulkanIsland"s / directory;

    std::ifstream file(directory / name, std::ios::binary);

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

    VkPipelineVertexInputStateCreateInfo constexpr vertexInputCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        nullptr, 0,
        0, nullptr,
        0, nullptr
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
        VK_FALSE,
        VK_FALSE,
        VK_POLYGON_MODE_FILL,
        VK_CULL_MODE_BACK_BIT,
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
        0, nullptr,
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
        nullptr,
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

void CreateRenderPass(VkDevice device)
{
    VkAttachmentDescription const colorAttachment{
        VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT,
        swapChainImageFormat,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference constexpr attachementReference{
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription const subpassDescription{
        0,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        0, nullptr,
        1, &attachementReference,
        nullptr, nullptr,
        0, nullptr
    };

    VkSubpassDependency constexpr subpassDependency{
        VK_SUBPASS_EXTERNAL, 0,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        0
    };

    VkRenderPassCreateInfo const renderPassCreateInfo{
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        nullptr, 0,
        1, &colorAttachment,
        1, &subpassDescription,
        1, &subpassDependency
    };

    if (auto result = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass); result != VK_SUCCESS)
        throw std::runtime_error("failed to create render pass: "s + std::to_string(result));
}

template<class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
void CreateFramebuffers(VkRenderPass renderPass, T &&swapChainImageViews)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkImageView>, "iterable object does not contain VkImageView elements");

    swapChainFramebuffers.clear();

    for (auto &&imageView : swapChainImageViews) {
        auto const attachements = make_array(imageView);

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

void CreateCommandPool(VkDevice device)
{
    VkCommandPoolCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        0,
        supportedQueuesIndices.at(0)
    };

    if (auto result = vkCreateCommandPool(device, &createInfo, nullptr, &commandPool); result != VK_SUCCESS)
        throw std::runtime_error("failed to create a command buffer: "s + std::to_string(result));
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

        VkClearValue constexpr clearColor{0.f, 0.f, 0.f, 1.f};

        VkRenderPassBeginInfo const renderPassInfo{
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            nullptr,
            renderPass,
            swapChainFramebuffers.at(i++),
            {{0, 0}, swapChainExtent},
            1, &clearColor
        };

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

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

    if (auto result = vkCreateSemaphore(device, &createInfo, nullptr, &imageAvailable); result != VK_SUCCESS)
        throw std::runtime_error("failed to create image semaphore: "s + std::to_string(result));

    if (auto result = vkCreateSemaphore(device, &createInfo, nullptr, &renderFinsihed); result != VK_SUCCESS)
        throw std::runtime_error("failed to create render semaphore: "s + std::to_string(result));
}


void RecreateSwapChain(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, VkSwapchainKHR swapChain, VkPipeline graphicsPipeline, VkPipelineLayout pipelineLayout, VkRenderPass renderPass)
{
    vkDeviceWaitIdle(device);

    CleanupSwapChain(device, swapChain, graphicsPipeline, pipelineLayout, renderPass);

    CreateSwapChain(physicalDevice, device, surface, swapChain);
    CreateSwapChainImageViews(device, swapChainImages);

    CreateRenderPass(device);
    CreateGraphicsPipeline(device);

    CreateFramebuffers(renderPass, swapChainImageViews);

    CreateCommandBuffers(device, renderPass, commandPool);
}

void OnWindowResize(GLFWwindow *window, int width, int height)
{
    WIDTH = width;
    HEIGHT = height;

    RecreateSwapChain(physicalDevice, device, surface, swapChain, graphicsPipeline, pipelineLayout, renderPass);
}

void DrawFrame(VkDevice device, VkSwapchainKHR swapChain)
{
    std::uint32_t imageIndex;

    if (auto result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<std::uint64_t>::max(), imageAvailable, VK_NULL_HANDLE, &imageIndex); result != VK_SUCCESS)
        throw std::runtime_error("failed to acquire next image index: "s + std::to_string(result));

    std::array<VkPipelineStageFlags, 1> constexpr waitStages{
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    auto const waitSemaphores = make_array(imageAvailable);
    auto const signalSemaphores = make_array(renderFinsihed);

    VkSubmitInfo const info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        static_cast<std::uint32_t>(std::size(waitSemaphores)), std::data(waitSemaphores),
        std::data(waitStages),
        1, &commandBuffers.at(imageIndex),
        static_cast<std::uint32_t>(std::size(signalSemaphores)), std::data(signalSemaphores),
    };

    if (auto result = vkQueueSubmit(graphicsQueue, 1, &info, VK_NULL_HANDLE); result != VK_SUCCESS)
        throw std::runtime_error("failed to submit draw command buffer: "s + std::to_string(result));

    auto const swapchains = make_array(swapChain);

    VkPresentInfoKHR const presentInfo{
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        nullptr,
        static_cast<std::uint32_t>(std::size(signalSemaphores)), std::data(signalSemaphores),
        static_cast<std::uint32_t>(std::size(swapchains)), std::data(swapchains),
        &imageIndex, nullptr
    };

    auto result = VK_SUCCESS;
    if (result = vkQueuePresentKHR(presentationQueue, &presentInfo); result != VK_SUCCESS)
        throw std::runtime_error("failed to submit request to present framebuffer: "s + std::to_string(result));

    vkQueueWaitIdle(presentationQueue);
}



int main()
try {
    //GraphicsQueue q(6);

    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(84);

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(WIDTH, HEIGHT, "VulkanIsland", nullptr, nullptr);

    glfwSetWindowSizeCallback(window, OnWindowResize);

    VulkanInstance vulkanInstance(extensions, layers);
    instance = vulkanInstance.handle();

#if USE_WIN32
    VkWin32SurfaceCreateInfoKHR const win32CreateInfo = {
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        nullptr, 0,
        GetModuleHandle(nullptr),
        glfwGetWin32Window(window)
    };

    vkCreateWin32SurfaceKHR(vkInstance, &win32CreateInfo, nullptr, &vkSurface);
#else
    glfwCreateWindowSurface(instance, window, nullptr, &surface);
#endif

    VulkanDevice vulkanDevice(vulkanInstance, surface, deviceExtensions);
    physicalDevice = vulkanDevice.physical_handle();
    device = vulkanDevice.handle();

    supportedQueuesIndices = vulkanDevice.supported_queues_indices();

    vkGetDeviceQueue(device, vulkanDevice.supported_queues_indices().at(0), 0, &graphicsQueue);
    vkGetDeviceQueue(device, vulkanDevice.supported_queues_indices().at(1), 0, &transferQueue);
    vkGetDeviceQueue(device, vulkanDevice.supported_queues_indices().at(2), 0, &presentationQueue);
    //presentationQueue = graphicsQueue;

    CreateSwapChain(physicalDevice, device, surface, swapChain);
    CreateSwapChainImageViews(device, swapChainImages);

    CreateRenderPass(device);
    CreateGraphicsPipeline(device);

    CreateFramebuffers(renderPass, swapChainImageViews);

    CreateCommandPool(device);
    CreateCommandBuffers(device, renderPass, commandPool);

    CreateSemaphores(device);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        DrawFrame(device, swapChain);
    }

    vkDeviceWaitIdle(device);

    if (renderFinsihed)
        vkDestroySemaphore(device, renderFinsihed, nullptr);

    if (imageAvailable)
        vkDestroySemaphore(device, imageAvailable, nullptr);

    CleanupSwapChain(device, swapChain, graphicsPipeline, pipelineLayout, renderPass);

    if (commandPool)
        vkDestroyCommandPool(device, commandPool, nullptr);

    if (surface)
        vkDestroySurfaceKHR(instance, surface, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;

} catch (std::exception const &ex) {
    std::cout << ex.what() << "\n";
}
#endif