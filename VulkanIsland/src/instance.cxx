
#include "main.h"
#include "instance.h"
#include "device.h"

namespace {

template<class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
[[nodiscard]] auto CheckRequiredExtensions(T &&_requiredExtensions)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, char const *>, "iterable object does not contain null-terminated strings");

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
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, char const *>, "iterable object does not contain null-terminated strings");

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

}

VulkanInstance::~VulkanInstance()
{
    if (debugReportCallback_ != VK_NULL_HANDLE)
        vkDestroyDebugReportCallbackEXT(instance_, debugReportCallback_, nullptr);

    debugReportCallback_ = VK_NULL_HANDLE;

    vkDestroyInstance(instance_, nullptr);
    instance_ = VK_NULL_HANDLE;
}

void VulkanInstance::CreateInstance(std::vector<char const *> &&extensions, std::vector<char const *> &&layers)
{
    VkInstanceCreateInfo createInfo{
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr, 0,
        &app_info,
        0, nullptr,
        0, nullptr
    };

    if (auto supported = CheckRequiredExtensions(extensions); !supported)
        throw std::runtime_error("not all required extensions are supported"s);

    createInfo.enabledExtensionCount = static_cast<std::uint32_t>(std::size(extensions));
    createInfo.ppEnabledExtensionNames = std::data(extensions);

    if (auto supported = CheckRequiredLayers(layers); !supported)
        throw std::runtime_error("not all required layers are supported"s);

    createInfo.enabledLayerCount = static_cast<std::uint32_t>(std::size(layers));
    createInfo.ppEnabledLayerNames = std::data(layers);

    if (auto result = vkCreateInstance(&createInfo, nullptr, &instance_); result != VK_SUCCESS)
        throw std::runtime_error("failed to create instance"s);

    if (!layers.empty())
        CreateDebugReportCallback(instance_, debugReportCallback_);
}