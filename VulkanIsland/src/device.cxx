
#include "main.h"
#include "device.h"
#include "device_defaults.h"

using namespace std::string_literals;
using namespace std::string_view_literals;



namespace {


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

auto constexpr requiredQueues = make_array(
    VkQueueFamilyProperties{VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT},
    VkQueueFamilyProperties{VK_QUEUE_TRANSFER_BIT}
);

template<bool check_on_duplicates = false>
[[nodiscard]] bool CheckRequiredDeviceExtensions(VkPhysicalDevice physicalDevice, std::vector<std::string_view> &&extensions)
{
    std::vector<VkExtensionProperties> requiredExtensions;

    auto constexpr extensionsComp = [] (auto &&lhs, auto &&rhs)
    {
        return std::lexicographical_compare(std::cbegin(lhs.extensionName), std::cend(lhs.extensionName), std::cbegin(rhs.extensionName), std::cend(rhs.extensionName));
    };

    std::transform(extensions.begin(), extensions.end(), std::back_inserter(requiredExtensions), [] (auto &&name)
    {
        VkExtensionProperties prop{};
        std::uninitialized_copy_n(std::begin(name), std::size(name), prop.extensionName);

        return prop;
    });

    std::sort(requiredExtensions.begin(), requiredExtensions.end(), extensionsComp);

    if constexpr (check_on_duplicates)
    {
        auto it = std::unique(requiredExtensions.begin(), requiredExtensions.end(), [] (auto &&lhs, auto &&rhs)
        {
            return std::equal(std::cbegin(lhs.extensionName), std::cend(lhs.extensionName), std::cbegin(rhs.extensionName), std::cend(rhs.extensionName));
        });

        requiredExtensions.erase(it, requiredExtensions.end());
    }

    std::uint32_t extensionsCount = 0;
    if (auto result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionsCount, nullptr); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve device extensions count: "s + std::to_string(result));

    std::vector<VkExtensionProperties> supportedExtensions(extensionsCount);
    if (auto result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionsCount, std::data(supportedExtensions)); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve device extensions: "s + std::to_string(result));

    std::sort(supportedExtensions.begin(), supportedExtensions.end(), extensionsComp);

    return std::includes(supportedExtensions.begin(), supportedExtensions.end(), requiredExtensions.begin(), requiredExtensions.end(), extensionsComp);
}

template<class T, class U, typename std::enable_if_t<is_iterable_v<std::decay_t<T>> && std::is_same_v<std::decay_t<U>, VkQueueFamilyProperties>>...>
[[nodiscard]] std::optional<std::uint32_t> GetRequiredQueueFamilyIndex(T &&queueFamilies, U &&requiredQueue)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkQueueFamilyProperties>, "iterable object does not contain VkQueueFamilyProperties elements");
    
    // Strict matching.
    auto it_family = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [&requiredQueue] (auto &&queueFamily)
    {
        return queueFamily.queueCount > 0 && queueFamily.queueFlags == requiredQueue.queueFlags;
    });

    if (it_family != queueFamilies.cend())
        return static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_family));

    // Tolerant matching.
    it_family = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [&requiredQueue] (auto &&queueFamily)
    {
        return queueFamily.queueCount > 0 && (queueFamily.queueFlags & requiredQueue.queueFlags) == requiredQueue.queueFlags;
    });

    if (it_family != queueFamilies.cend())
        return static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_family));

    return {};
}

template<class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
[[nodiscard]] std::optional<std::uint32_t> GetPresentationQueueFamilyIndex(T &&queueFamilies, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkQueueFamilyProperties>, "iterable object does not contain VkQueueFamilyProperties elements");

    auto it_presentationQueue = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [physicalDevice, surface, size = queueFamilies.size()] (auto queueFamily)
    {
        std::vector<std::uint32_t> queueFamiliesIndices(size);
        std::iota(queueFamiliesIndices.begin(), queueFamiliesIndices.end(), 0);

        return std::find_if(queueFamiliesIndices.crbegin(), queueFamiliesIndices.crend(), [physicalDevice, surface] (auto queueIndex)
        {
            VkBool32 surfaceSupported = 0;
            if (auto result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueIndex, surface, &surfaceSupported); result != VK_SUCCESS)
                throw std::runtime_error("failed to retrieve surface support: "s + std::to_string(result));

            return surfaceSupported != VK_TRUE;

        }) != queueFamiliesIndices.crend();
    });

    if (it_presentationQueue != queueFamilies.cend())
        return static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_presentationQueue));

    return {};
}


template<class Q, std::enable_if_t<std::is_base_of_v<VulkanQueue<std::decay_t<Q>>, std::decay_t<Q>>>...>
std::optional<Q> GetQueue()
{
    using T = std::decay_t<Q>;

    if constexpr (std::is_same_v<T, GraphicsQueue>)
    {
        ;
    }

    return {};
}

}

VulkanDevice::~VulkanDevice()
{
    if (device_ != VK_NULL_HANDLE)
        vkDeviceWaitIdle(device_);

    if (device_ != VK_NULL_HANDLE)
        vkDestroyDevice(device_, nullptr);

    device_ = VK_NULL_HANDLE;
    physicalDevice_ = VK_NULL_HANDLE;
}

void VulkanDevice::PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, std::vector<std::string_view> &&extensions)
{
    std::uint32_t devicesCount = 0;

    if (auto result = vkEnumeratePhysicalDevices(instance, &devicesCount, nullptr); result != VK_SUCCESS || devicesCount == 0)
        throw std::runtime_error("failed to find physical device with Vulkan API support: "s + std::to_string(result));

    std::vector<VkPhysicalDevice> devices(devicesCount);
    if (auto result = vkEnumeratePhysicalDevices(instance, &devicesCount, std::data(devices)); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve physical devices: "s + std::to_string(result));

    // Matching by supported features and extensions.
    auto it_end = std::remove_if(devices.begin(), devices.end(), [&extensions] (auto &&device)
    {
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);

        if (!ComparePhysicalDeviceFeatures(features))
            return true;

        return !CheckRequiredDeviceExtensions(device, std::move(extensions));
    });

    devices.erase(it_end, devices.end());

    // Matching by required graphics, transfer and presentation queues.
    it_end = std::remove_if(devices.begin(), devices.end(), [surface] (auto &&device)
    {
        if (!GraphicsQueue::supported_by_device(device))
            return true;

        std::uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyPropertyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyPropertyCount, std::data(queueFamilies));

        if (queueFamilies.empty())
            return true;

        std::vector<std::uint32_t> supportedQueuesFamilyIndices;

        for (auto &&queueProp : requiredQueues)
            if (auto const queueIndex = GetRequiredQueueFamilyIndex(queueFamilies, queueProp); queueIndex)
                supportedQueuesFamilyIndices.emplace_back(queueIndex.value());

        if (supportedQueuesFamilyIndices.empty())
            return true;

        return !GetPresentationQueueFamilyIndex(queueFamilies, device, surface).has_value();
    });

    devices.erase(it_end, devices.end());

    // Matchin by the swap chain properties support.
    it_end = std::remove_if(devices.begin(), devices.end(), [surface] (auto &&device)
    {
        auto const details = QuerySwapChainSupportDetails(device, surface);
        return details.formats.empty() || details.presentModes.empty();
    });

    devices.erase(it_end, devices.end());

    if (devices.empty())
        throw std::runtime_error("failed to pick physical device"s);

    auto constexpr deviceTypesPriority = make_array(
        VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, VK_PHYSICAL_DEVICE_TYPE_CPU
    );

    // Sorting by device type.
    for (auto deviceType : deviceTypesPriority) {
        auto id_next_type = std::stable_partition(devices.begin(), devices.end(), [deviceType] (auto &&device)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);

            return properties.deviceType == deviceType;
        });

        if (id_next_type != devices.begin())
            break;
    }

    physicalDevice_ = devices.front();
}

void VulkanDevice::CreateDevice(VkInstance instance, VkSurfaceKHR surface, std::vector<char const *> &&extensions)
{
    std::uint32_t queueFamilyPropertyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyPropertyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyPropertyCount, std::data(queueFamilies));

    if (queueFamilies.empty())
        throw std::runtime_error("there's no queue families on device"s);

    supportedQueuesIndices_.clear();

    for (auto &&queueProp : requiredQueues)
        if (auto const queueIndex = GetRequiredQueueFamilyIndex(queueFamilies, queueProp); queueIndex)
            supportedQueuesIndices_.emplace_back(queueIndex.value());

    if (supportedQueuesIndices_.empty())
        throw std::runtime_error("device does not support required queues"s);

    if (auto const presentationFamilyQueueIndex = GetPresentationQueueFamilyIndex(queueFamilies, physicalDevice_, surface); presentationFamilyQueueIndex)
        supportedQueuesIndices_.emplace_back(presentationFamilyQueueIndex.value());

    else throw std::runtime_error("picked queue does not support presentation surface"s);

    std::set<std::uint32_t> const uniqueFamilyQueueIndices{supportedQueuesIndices_.cbegin(), supportedQueuesIndices_.cend()};
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    for (auto &&queueFamilyIndex : uniqueFamilyQueueIndices) {
        auto constexpr queuePriority = 1.f;

        VkDeviceQueueCreateInfo queueCreateInfo{
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            nullptr, 0,
            queueFamilyIndex, 1,
            &queuePriority
        };

        queueCreateInfos.push_back(std::move(queueCreateInfo));
    }

    VkPhysicalDeviceFeatures constexpr deviceFeatures{};

    VkDeviceCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        nullptr, 0,
        static_cast<std::uint32_t>(std::size(queueCreateInfos)), std::data(queueCreateInfos),
        0, nullptr,
        static_cast<std::uint32_t>(std::size(extensions)), std::data(extensions),
        &deviceFeatures
    };

    if (auto result = vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_); result != VK_SUCCESS)
        throw std::runtime_error("failed to create logical device: "s + std::to_string(result));
}

#if !X

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <set>
#include <cmath>

const auto WIDTH = 800u;
const auto HEIGHT = 600u;

/*const std::vector<const char*> layers = {
    //"VK_LAYER_LUNARG_api_dump",
    "VK_LAYER_LUNARG_core_validation",
    "VK_LAYER_LUNARG_object_tracker",
    "VK_LAYER_LUNARG_parameter_validation",
    "VK_LAYER_GOOGLE_threading",
    "VK_LAYER_GOOGLE_unique_objects",

    "VK_LAYER_NV_nsight"
};*/

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
{
    auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}

struct QueueFamilyIndices {
    int graphicsFamily = -1;
    int presentFamily = -1;

    bool isComplete()
    {
        return graphicsFamily >= 0 && presentFamily >= 0;
    }
};



class HelloTriangleApplication {
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow * window;

    VkInstance instance;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentationQueue;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;

    std::unique_ptr<VulkanInstance> vulkanInstance;
    std::unique_ptr<VulkanDevice> vulkanDevice;


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

    void initWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

        glfwSetWindowUserPointer(window, this);
        glfwSetWindowSizeCallback(window, HelloTriangleApplication::onWindowResized);
    }

    void initVulkan()
    {
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        createSemaphores();
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(device);
    }

    void cleanupSwapChain()
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

    void cleanup()
    {
        cleanupSwapChain();

        vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);

        vkDestroyCommandPool(device, commandPool, nullptr);

        //vkDestroyDevice(device, nullptr);
        //DestroyDebugReportCallbackEXT(instance, callback, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        //vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

    static void onWindowResized(GLFWwindow* window, int, int)
    {
        HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->recreateSwapChain();
    }

    void recreateSwapChain()
    {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        if (width == 0 || height == 0) return;

        vkDeviceWaitIdle(device);

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandBuffers();
    }


    void createInstance()
    {
        vulkanInstance = std::move(std::make_unique<VulkanInstance>(extensions, layers));
        instance = vulkanInstance->handle();
    }

    void createSurface()
    {
        glfwCreateWindowSurface(instance, window, nullptr, &surface);
    }

    void pickPhysicalDevice()
    {
        vulkanDevice = std::move(std::make_unique<VulkanDevice>(*vulkanInstance, surface, deviceExtensions));
        physicalDevice = vulkanDevice->physical_handle();
    }

    void createLogicalDevice()
    {
        device = vulkanDevice->handle();
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily, 0, &presentationQueue);
    }

    void createSwapChain()
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

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        auto const queueFamilyIndices = make_array(
            (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily
        );

        if (indices.graphicsFamily != indices.presentFamily) {
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

    void createImageViews()
    {
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

    void createRenderPass()
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

    void createGraphicsPipeline()
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

    void createFramebuffers()
    {
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

    void createCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo const createInfo{
            VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            nullptr,
            0,
            static_cast<std::uint32_t>(queueFamilyIndices.graphicsFamily)
        };

        if (auto result = vkCreateCommandPool(device, &createInfo, nullptr, &commandPool); result != VK_SUCCESS)
            throw std::runtime_error("failed to create a command buffer: "s + std::to_string(result));
    }

    void createCommandBuffers()
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

    void createSemaphores()
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

    void drawFrame()
    {
        std::uint32_t imageIndex;

        if (auto result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<std::uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex); result != VK_SUCCESS)
            throw std::runtime_error("failed to acquire next image index: "s + std::to_string(result));

        std::array<VkPipelineStageFlags, 1> constexpr waitStages{
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        };

        auto const waitSemaphores = make_array(imageAvailableSemaphore);
        auto const signalSemaphores = make_array(renderFinishedSemaphore);

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


    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (queueFamily.queueCount > 0 && presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

};

int main()
{
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
#endif