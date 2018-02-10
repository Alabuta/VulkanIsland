#include <iostream>
#include <memory>
#include <vector>
#include <array>
#include <string>
using namespace std::string_literals;

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

auto constexpr kVULKAN_VERSION = VK_API_VERSION_1_0;

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib, "glfw3.lib")

#include <helpers.h>
#include <debug.h>

VkInstance vkInstance;
VkDebugReportCallbackEXT vkDebugReportCallback;
VkPhysicalDevice vkPhysicalDevice;
VkDevice vkDevice;
VkQueue vkQueue;

template<class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
auto CheckRequiredExtensions(T &&_requiredExtensions)
{
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
auto CheckRequiredLayers(T &&_requiredLayers)
{
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



VkPhysicalDevice PickPhysicalDevice(VkInstance instance)
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
        features.shaderUniformBufferArrayDynamicIndexing,
        features.shaderSampledImageArrayDynamicIndexing,
        features.shaderStorageBufferArrayDynamicIndexing,
        features.shaderStorageImageArrayDynamicIndexing
    );

    set_tuple(requiredFeatures, static_cast<VkBool32>(1));

    auto it_end = std::remove_if(devices.begin(), devices.end(), [&requiredFeatures] (auto &&device)
    {
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;

        vkGetPhysicalDeviceProperties(device, &properties);
        vkGetPhysicalDeviceFeatures(device, &features);

        auto const deviceFeatures = std::tie(
            features.geometryShader,
            features.tessellationShader,
            features.shaderUniformBufferArrayDynamicIndexing,
            features.shaderSampledImageArrayDynamicIndexing,
            features.shaderStorageBufferArrayDynamicIndexing,
            features.shaderStorageImageArrayDynamicIndexing
        );

        return deviceFeatures != requiredFeatures || properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || properties.apiVersion < kVULKAN_VERSION;
    });

    devices.erase(it_end, devices.end());

    it_end = std::remove_if(devices.begin(), devices.end(), [] (auto &&device)
    {
        std::uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyPropertyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyPropertyCount, std::data(queueFamilies));

        if (queueFamilies.empty())
            return true;

        auto it_family = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [] (auto &&queueFamily)
        {
            return queueFamily.queueCount > 0 && (queueFamily.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT));
        });

        return it_family == queueFamilies.cend();
    });

    devices.erase(it_end, devices.end());

    if (devices.empty())
        throw std::runtime_error("failed to pick physical device"s);

    return devices.front();
}

VkDevice CreateDevice(VkInstance instance, VkPhysicalDevice physicalDevice)
{
    std::uint32_t queueFamilyPropertyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, std::data(queueFamilies));

    if (queueFamilies.empty())
        throw std::runtime_error("there's no queue families on device"s);

    auto it_family = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [] (auto &&queueFamily)
    {
        return queueFamily.queueCount > 0 && (queueFamily.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT));
    });

    if (it_family == queueFamilies.cend())
        throw std::runtime_error("there's no queue familiy with required properties"s);

    auto const familyIndex = static_cast<decltype(VkDeviceQueueCreateInfo::queueFamilyIndex)>(std::distance(queueFamilies.cbegin(), it_family));

    auto constexpr queuePriority = 1.f;

    VkDeviceQueueCreateInfo const queueCreateInfo = {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        nullptr, 0,
        familyIndex, 1,
        &queuePriority
    };

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo const createInfo = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        nullptr, 0,
        1, &queueCreateInfo,
        0, nullptr,
        0, nullptr,
        &deviceFeatures
    };

    VkDevice device;
    if (auto result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device); result != VK_SUCCESS)
        throw std::runtime_error("failed to create logical device: "s + std::to_string(result));

    vkGetDeviceQueue(device, familyIndex, 0, &vkQueue);

    return device;
}

void InitVulkan()
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
        "VK_KHR_win32_surface",
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME
    };

    if (auto supported = CheckRequiredExtensions(extensions); !supported)
        throw std::runtime_error("not all required extensions are supported"s);

#if _DEBUG
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
#if _DEBUG
        static_cast<std::uint32_t>(std::size(layers)), std::data(layers),
#else
        0, nullptr,
#endif
        static_cast<std::uint32_t>(std::size(extensions)), std::data(extensions)
    };

    if (auto result = vkCreateInstance(&createInfo, nullptr, &vkInstance); result != VK_SUCCESS)
        throw std::runtime_error("failed to create instance"s);

    CreateDebugReportCallback(vkInstance, vkDebugReportCallback);

    vkPhysicalDevice = PickPhysicalDevice(vkInstance);
    vkDevice = CreateDevice(vkInstance, vkPhysicalDevice);
}

int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(800, 600, "VulkanIsland", nullptr, nullptr);

    InitVulkan();

    while (!glfwWindowShouldClose(window))
        glfwPollEvents();

    vkDeviceWaitIdle(vkDevice);

    if (vkDevice)
        vkDestroyDevice(vkDevice, nullptr);

    if (vkDebugReportCallback)
        vkDestroyDebugReportCallbackEXT(vkInstance, vkDebugReportCallback, nullptr);

    if (vkInstance)
        vkDestroyInstance(vkInstance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}