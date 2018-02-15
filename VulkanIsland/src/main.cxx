

#include "main.h"
using namespace std::string_literals;
using namespace std::string_view_literals;

#include <helpers.h>
#include <debug.h>

auto constexpr kWIDTH = 800u;
auto constexpr kHEIGHT = 600u;

std::array<VkQueueFamilyProperties, 2> constexpr requiredQueues{{
    {VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT},
    {VK_QUEUE_TRANSFER_BIT}
}};

std::array<const char *const, 1> constexpr deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

VkInstance instance;
VkDebugReportCallbackEXT debugReportCallback;
VkPhysicalDevice physicalDevice;
VkDevice device;
VkQueue graphicsQueue, transferQueue, presentationQueue;
VkSurfaceKHR surface;
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
[[nodiscard]] auto CheckRequiredExtensions(T &&_requiredExtensions)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, char const *const>, "iterable object does not contain null-terminated strings");

    std::vector<VkExtensionProperties> requiredExtensions;

    std::transform(_requiredExtensions.begin(), _requiredExtensions.end(), std::back_inserter(requiredExtensions), [] (auto &&name)
    {
        VkExtensionProperties prop{};
        std::uninitialized_copy_n(name, std::strlen(name), prop.extensionName);

        return prop;
    });

    auto constexpr extensionsComp = [] (auto &&lhs, auto &&rhs)
    {
        return std::lexicographical_compare(std::cbegin(lhs.extensionName), std::cend(lhs.extensionName), std::cbegin(rhs.extensionName), std::cend(rhs.extensionName));
    };

    std::sort(requiredExtensions.begin(), requiredExtensions.end(), extensionsComp);

    std::uint32_t extensionsCount = 0;
    if (auto result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve extensions count: "s + std::to_string(result));

    std::vector<VkExtensionProperties> supportedExtensions(extensionsCount);
    if (auto result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, std::data(supportedExtensions)); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve extensions: "s + std::to_string(result));

    std::sort(supportedExtensions.begin(), supportedExtensions.end(), extensionsComp);

    return std::includes(supportedExtensions.begin(), supportedExtensions.end(), requiredExtensions.begin(), requiredExtensions.end(), extensionsComp);
}

template<class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
[[nodiscard]] auto CheckRequiredLayers(T &&_requiredLayers)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, char const *const>, "iterable object does not contain null-terminated strings");

    std::vector<VkLayerProperties> requiredLayers;

    std::transform(_requiredLayers.begin(), _requiredLayers.end(), std::back_inserter(requiredLayers), [] (auto &&name)
    {
        VkLayerProperties prop{};
        std::uninitialized_copy_n(name, std::strlen(name), prop.layerName);

        return prop;
    });

    auto constexpr layersComp = [] (auto &&lhs, auto &&rhs)
    {
        return std::lexicographical_compare(std::cbegin(lhs.layerName), std::cend(lhs.layerName), std::cbegin(rhs.layerName), std::cend(rhs.layerName));
    };

    std::sort(requiredLayers.begin(), requiredLayers.end(), layersComp);

    std::uint32_t layersCount = 0;
    if (auto result = vkEnumerateInstanceLayerProperties(&layersCount, nullptr); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve layers count: "s + std::to_string(result));

    std::vector<VkLayerProperties> supportedLayers(layersCount);
    if (auto result = vkEnumerateInstanceLayerProperties(&layersCount, std::data(supportedLayers)); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve layers: "s + std::to_string(result));

    std::sort(supportedLayers.begin(), supportedLayers.end(), layersComp);

    return std::includes(supportedLayers.begin(), supportedLayers.end(), requiredLayers.begin(), requiredLayers.end(), layersComp);
}

template<bool check_on_duplicates = false, class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
[[nodiscard]] auto CheckRequiredDeviceExtensions(VkPhysicalDevice physicalDevice, T &&_requiredExtensions)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, char const *const>, "iterable object does not contain null-terminated strings");

    std::vector<VkExtensionProperties> requiredExtensions;

    auto constexpr extensionsComp = [] (auto &&lhs, auto &&rhs)
    {
        return std::lexicographical_compare(std::cbegin(lhs.extensionName), std::cend(lhs.extensionName), std::cbegin(rhs.extensionName), std::cend(rhs.extensionName));
    };

    std::transform(_requiredExtensions.begin(), _requiredExtensions.end(), std::back_inserter(requiredExtensions), [] (auto &&name)
    {
        VkExtensionProperties prop{};
        std::uninitialized_copy_n(name, std::strlen(name), prop.extensionName);

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


struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

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

[[nodiscard]] VkPhysicalDevice PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
{
    std::uint32_t devicesCount = 0;
    
    if (auto result = vkEnumeratePhysicalDevices(instance, &devicesCount, nullptr); result != VK_SUCCESS || devicesCount == 0)
        throw std::runtime_error("failed to find physical device with Vulkan API support: "s + std::to_string(result));

    std::vector<VkPhysicalDevice> devices(devicesCount);
    if (auto result = vkEnumeratePhysicalDevices(instance, &devicesCount, std::data(devices)); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve physical devices: "s + std::to_string(result));

    VkPhysicalDeviceFeatures features{};

    auto requiredFeatures = std::tie(
        features.geometryShader,
        features.tessellationShader,
        features.depthClamp,
        features.shaderUniformBufferArrayDynamicIndexing,
        features.shaderSampledImageArrayDynamicIndexing,
        features.shaderStorageBufferArrayDynamicIndexing,
        features.shaderStorageImageArrayDynamicIndexing
    );

    set_tuple(requiredFeatures, static_cast<VkBool32>(1));

    // Matching by supported features, extensions, device type and supported Vulkan API version.
    auto it_end = std::remove_if(devices.begin(), devices.end(), [&requiredFeatures] (auto &&device)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || properties.apiVersion < kVULKAN_VERSION)
            return true;

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);

        auto const deviceFeatures = std::tie(
            features.geometryShader,
            features.tessellationShader,
            features.depthClamp,
            features.shaderUniformBufferArrayDynamicIndexing,
            features.shaderSampledImageArrayDynamicIndexing,
            features.shaderStorageBufferArrayDynamicIndexing,
            features.shaderStorageImageArrayDynamicIndexing
        );

        if (deviceFeatures != requiredFeatures)
            return true;

        return !CheckRequiredDeviceExtensions(device, deviceExtensions);
    });

    devices.erase(it_end, devices.end());

    // Matching by required graphics, transfer and presentation queues. Also by presentation capabilities.
    it_end = std::remove_if(devices.begin(), devices.end(), [surface] (auto &&device)
    {
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

    it_end = std::remove_if(devices.begin(), devices.end(), [surface] (auto &&device)
    {
        auto const details = QuerySwapChainSupportDetails(device, surface);
        return details.formats.empty() || details.presentModes.empty();
    });

    devices.erase(it_end, devices.end());

    if (devices.empty())
        throw std::runtime_error("failed to pick physical device"s);

    return devices.front();
}

[[nodiscard]] VkDevice CreateDevice(VkInstance instance, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    std::uint32_t queueFamilyPropertyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, std::data(queueFamilies));

    if (queueFamilies.empty())
        throw std::runtime_error("there's no queue families on device"s);

    supportedQueuesIndices.clear();

    for (auto &&queueProp : requiredQueues)
        if (auto const queueIndex = GetRequiredQueueFamilyIndex(queueFamilies, queueProp); queueIndex)
            supportedQueuesIndices.emplace_back(queueIndex.value());

    if (supportedQueuesIndices.empty())
        throw std::runtime_error("device does not support required queues"s);

    if (auto const presentationFamilyQueueIndex = GetPresentationQueueFamilyIndex(queueFamilies, physicalDevice, surface); presentationFamilyQueueIndex)
        supportedQueuesIndices.emplace_back(presentationFamilyQueueIndex.value());

    else throw std::runtime_error("picked queue does not support presentation surface"s);

    std::set<std::uint32_t> uniqueFamilyQueueIndices{supportedQueuesIndices.cbegin(), supportedQueuesIndices.cend()};
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    auto constexpr queuePriority = 1.f;

    for (auto &&queueFamilyIndex : uniqueFamilyQueueIndices) {
        VkDeviceQueueCreateInfo queueCreateInfo = {
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            nullptr, 0,
            queueFamilyIndex, 1,
            &queuePriority
        };

        queueCreateInfos.push_back(std::move(queueCreateInfo));
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo const createInfo = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        nullptr, 0,
        static_cast<std::uint32_t>(std::size(queueCreateInfos)), std::data(queueCreateInfos),
        0, nullptr,
        static_cast<std::uint32_t>(std::size(deviceExtensions)), std::data(deviceExtensions),
        &deviceFeatures
    };

    VkDevice device;
    if (auto result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device); result != VK_SUCCESS)
        throw std::runtime_error("failed to create logical device: "s + std::to_string(result));

    vkGetDeviceQueue(device, supportedQueuesIndices.at(0), 0, &graphicsQueue);
    vkGetDeviceQueue(device, supportedQueuesIndices.at(1), 0, &transferQueue);
    vkGetDeviceQueue(device, supportedQueuesIndices.at(2), 0, &presentationQueue);

    return device;
}

void InitVulkan(GLFWwindow *window)
{
    VkApplicationInfo constexpr appInfo = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,
        "VulkanIsland", VK_MAKE_VERSION(1, 0, 0),
        "VulkanIsland", VK_MAKE_VERSION(1, 0, 0),
        kVULKAN_VERSION
    };

    std::array<const char *const, 3> constexpr extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#if USE_WIN32
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#else
        "VK_KHR_win32_surface",
#endif
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME
    };

    if (auto supported = CheckRequiredExtensions(extensions); !supported)
        throw std::runtime_error("not all required extensions are supported"s);

#if USE_LAYERS
    std::array<const char *const, 6> constexpr layers = {
        //"VK_LAYER_LUNARG_api_dump",
        "VK_LAYER_LUNARG_core_validation",
        "VK_LAYER_LUNARG_object_tracker",
        "VK_LAYER_LUNARG_parameter_validation",
        "VK_LAYER_GOOGLE_threading",
        "VK_LAYER_GOOGLE_unique_objects",

        "VK_LAYER_NV_nsight"
    };

    if (auto supported = CheckRequiredLayers(layers); !supported)
        throw std::runtime_error("not all required layers are supported"s);
#endif

    VkInstanceCreateInfo const createInfo = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr, 0,
        &appInfo,
#if USE_LAYERS
        static_cast<std::uint32_t>(std::size(layers)), std::data(layers),
#else
        0, nullptr,
#endif
        static_cast<std::uint32_t>(std::size(extensions)), std::data(extensions)
    };

    if (auto result = vkCreateInstance(&createInfo, nullptr, &instance); result != VK_SUCCESS)
        throw std::runtime_error("failed to create instance"s);

#if USE_LAYERS
    CreateDebugReportCallback(instance, debugReportCallback);
#endif

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

    physicalDevice = PickPhysicalDevice(instance, surface);
    device = CreateDevice(instance, physicalDevice, surface);
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
        std::clamp(kWIDTH, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
        std::clamp(kHEIGHT, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)
    };
}


void CreateSwapChain(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface)
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

    std::array<decltype(supportedQueuesIndices)::value_type, 2> const queueFamilyIndices{
        supportedQueuesIndices.at(0), supportedQueuesIndices.at(2)
    };

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

[[nodiscard]] std::vector<std::byte> ReadShaderFile(std::string_view path)
{
    std::ifstream file(std::data(path), std::ios::binary);

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
    auto const vertShaderByteCode = ReadShaderFile(R"(shaders\vert.spv)"sv);

    if (vertShaderByteCode.empty())
        throw std::runtime_error("failed to open vertex shader file");

    auto const vertShaderModule = CreateShaderModule(device, vertShaderByteCode);

    auto const fragShaderByteCode = ReadShaderFile(R"(shaders\frag.spv)"sv);

    if (fragShaderByteCode.empty())
        throw std::runtime_error("failed to open fragment shader file");

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

    std::array<VkPipelineShaderStageCreateInfo, 2> const shaderStages{
        vertShaderCreateInfo, fragShaderCreateInfo
    };

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
        VK_TRUE,
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

    VkPipelineColorBlendAttachmentState constexpr colorBlendAttachement{
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
        &colorBlendAttachement,
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

    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
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

    VkRenderPassCreateInfo const renderPassCreateInfo{
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        nullptr, 0,
        1, &colorAttachment,
        1, &subpassDescription,
        0, nullptr
    };

    if (auto result = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass); result != VK_SUCCESS)
        throw std::runtime_error("failed to create render pass: "s + std::to_string(result));
}

template<class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
void CreateFramebuffers(VkRenderPass renderPass, T &&swapChainImageViews)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkImageView>, "iterable object does not contain VkImageView elements");

    for (auto &&imageView : swapChainImageViews) {
        std::array<VkImageView, 1> const attachement{ imageView };

        VkFramebufferCreateInfo const createInfo{
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            nullptr, 0,
            renderPass,
            static_cast<std::uint32_t>(std::size(attachement)), std::data(attachement),
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


int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(kWIDTH, kHEIGHT, "VulkanIsland", nullptr, nullptr);

    InitVulkan(window);

    CreateSwapChain(physicalDevice, device, surface);
    CreateSwapChainImageViews(device, swapChainImages);

    CreateRenderPass(device);
    CreateGraphicsPipeline(device);

    CreateFramebuffers(renderPass, swapChainImageViews);

    CreateCommandPool(device);
    CreateCommandBuffers(device, renderPass, commandPool);

    while (!glfwWindowShouldClose(window))
        glfwPollEvents();

    if (commandPool)
        vkDestroyCommandPool(device, commandPool, nullptr);

    for (auto &&swapChainFramebuffer : swapChainFramebuffers)
        vkDestroyFramebuffer(device, swapChainFramebuffer, nullptr);

    if (graphicsPipeline)
        vkDestroyPipeline(device, graphicsPipeline, nullptr);

    if (pipelineLayout)
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

    if (renderPass)
        vkDestroyRenderPass(device, renderPass, nullptr);

    for (auto &&swapChainImageView : swapChainImageViews)
        vkDestroyImageView(device, swapChainImageView, nullptr);

    if (swapChain)
        vkDestroySwapchainKHR(device, swapChain, nullptr);

    if (surface)
        vkDestroySurfaceKHR(instance, surface, nullptr);
    
    if (device) {
        vkDeviceWaitIdle(device);
        vkDestroyDevice(device, nullptr);
    }

    if (debugReportCallback)
        vkDestroyDebugReportCallbackEXT(instance, debugReportCallback, nullptr);

    if (instance)
        vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}