
#include "main.h"
#include "device.h"
#include "device_defaults.h"

using namespace std::string_literals;
using namespace std::string_view_literals;


namespace {


auto constexpr requiredQueues = make_array(
    VkQueueFamilyProperties{VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0, 0, {0, 0, 0}},
    VkQueueFamilyProperties{VK_QUEUE_TRANSFER_BIT, 0, 0, {0, 0, 0}}
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

#if TEMPORARILY_DISABLED
    // Strict matching.
    auto it_family = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [&requiredQueue] (auto &&queueFamily)
    {
        return queueFamily.queueCount > 0 && queueFamily.queueFlags == requiredQueue.queueFlags;
    });

    if (it_family != queueFamilies.cend())
        return static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_family));
#endif

    // Tolerant matching.
    auto it_family = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [&requiredQueue] (auto &&queueFamily)
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

    auto it_presentationQueue = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [physicalDevice, surface, size = queueFamilies.size()] (auto /*queueFamily*/)
    {
        std::vector<std::uint32_t> queueFamiliesIndices(size);
        std::iota(queueFamiliesIndices.begin(), queueFamiliesIndices.end(), 0);

        return std::find_if(queueFamiliesIndices.cbegin(), queueFamiliesIndices.cend(), [physicalDevice, surface] (auto queueIndex)
        {
            VkBool32 surfaceSupported = VK_FALSE;
            if (auto result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueIndex, surface, &surfaceSupported); result != VK_SUCCESS)
                throw std::runtime_error("failed to retrieve surface support: "s + std::to_string(result));

            return surfaceSupported == VK_TRUE;

        }) != queueFamiliesIndices.cend();
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
    if (device_) {
        vkDeviceWaitIdle(device_);
        vkDestroyDevice(device_, nullptr);
    }

    device_ = nullptr;
    physicalDevice_ = nullptr;
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
        if (!GraphicsQueue::is_supported_by_device(device))
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

void VulkanDevice::CreateDevice(VkSurfaceKHR surface, std::vector<char const *> &&extensions)
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

    VkPhysicalDeviceFeatures constexpr deviceFeatures{kDEVICE_FEATURES};

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
