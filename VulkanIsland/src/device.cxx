#include "device.h"
#include "device_defaults.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

VulkanDevice::VulkanDevice(VulkanInstance const &instance, VkSurfaceKHR surface)
{
    ;
}

VulkanDevice::~VulkanDevice()
{
    //vkDestroyDevice(device_, nullptr);
}

template<bool check_on_duplicates = false, class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
[[nodiscard]] auto CheckRequiredDeviceExtensions(VkPhysicalDevice physicalDevice, T &&_requiredExtensions)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, char const *>, "iterable object does not contain null-terminated strings");

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


template<bool check_on_duplicates = false, class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
[[nodiscard]] VkPhysicalDevice PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, T &&requiredExtension)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, char const *>, "iterable object does not contain null-terminated strings");

    std::uint32_t devicesCount = 0;

    if (auto result = vkEnumeratePhysicalDevices(instance, &devicesCount, nullptr); result != VK_SUCCESS || devicesCount == 0)
        throw std::runtime_error("failed to find physical device with Vulkan API support: "s + std::to_string(result));

    std::vector<VkPhysicalDevice> devices(devicesCount);
    if (auto result = vkEnumeratePhysicalDevices(instance, &devicesCount, std::data(devices)); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve physical devices: "s + std::to_string(result));

    // Matching by supported features and extensions.
    auto it_end = std::remove_if(devices.begin(), devices.end(), [] (auto &&device)
    {
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);

        if (!ComparePhysicalDeviceFeatures(features))
            return true;

        return !CheckRequiredDeviceExtensions(device, requiredExtension);
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

    return devices.front();
}