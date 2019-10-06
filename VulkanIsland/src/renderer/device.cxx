#include <type_traits>
#include <functional>
#include <algorithm>
#include <bitset>

#include "vulkan_config.hxx"
#include "swapchain.hxx"
#include "renderer/device.hxx"
#include "device_config.hxx"
#include "resources/buffer.hxx"
#include "resources/resource.hxx"

using namespace std::string_literals;
using namespace std::string_view_literals;


namespace
{

    template<class T, std::size_t I = 0>
    constexpr bool check_all_queues_support(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        using Q = std::variant_alternative_t<I, T>;

        if (!QueueHelper::IsSupportedByDevice<Q>(physicalDevice, surface))
            return false;

        if constexpr (I + 1 < std::variant_size_v<T>)
            return check_all_queues_support<T, I + 1>(physicalDevice, surface);

        return false;
    }

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

        if constexpr (check_on_duplicates) {
            auto it = std::unique(requiredExtensions.begin(), requiredExtensions.end(), [] (auto &&lhs, auto &&rhs)
            {
                return std::equal(std::cbegin(lhs.extensionName), std::cend(lhs.extensionName), std::cbegin(rhs.extensionName), std::cend(rhs.extensionName));
            });

            requiredExtensions.erase(it, requiredExtensions.end());
        }

        std::uint32_t extensionsCount = 0;
        if (auto result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionsCount, nullptr); result != VK_SUCCESS)
            throw std::runtime_error("failed to retrieve device extensions count: "s + std::to_string(result));

        std::vector<VkExtensionProperties> supported_extensions(extensionsCount);
        if (auto result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionsCount, std::data(supported_extensions)); result != VK_SUCCESS)
            throw std::runtime_error("failed to retrieve device extensions: "s + std::to_string(result));

        std::sort(supported_extensions.begin(), supported_extensions.end(), extensionsComp);

        return std::includes(supported_extensions.begin(), supported_extensions.end(), requiredExtensions.begin(), requiredExtensions.end(), extensionsComp);
    }

    template<class T> requires iterable<std::remove_cvref_t<T>>
    [[nodiscard]] std::optional<std::uint32_t> GetPresentationQueueFamilyIndex(T &&queueFamilies, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        static_assert(std::is_same_v<typename std::remove_cvref_t<T>::value_type, VkQueueFamilyProperties>, "iterable object does not contain VkQueueFamilyProperties elements");

        auto it_presentationQueue = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [physicalDevice, surface, size = queueFamilies.size()](auto /*queueFamily*/)
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
}

VulkanDevice::~VulkanDevice()
{
    if (resourceManager_)
        resourceManager_.reset();

    if (memoryManager_)
        memoryManager_.reset();

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

    auto application_info = vulkan_config::application_info;

    // Matching by supported properties, features and extensions.
    auto it_end = std::remove_if(devices.begin(), devices.end(), [&extensions, &application_info] (auto &&device)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        if (properties.apiVersion < application_info.apiVersion)
            return true;

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);

        if (!ComparePhysicalDeviceFeatures(features))
            return true;

        return !CheckRequiredDeviceExtensions(device, std::move(extensions));
    });

    devices.erase(it_end, devices.end());

    auto const &queuePool = queuePool_;

    // Removing unsuitable devices. Matching by required compute, graphics, transfer and presentation queues.
    it_end = std::remove_if(devices.begin(), devices.end(), [surface, queuePool] (auto &&device)
    {
        auto check_queue_pool_support = [device, surface] (auto &&queuePool)
        {
            if (queuePool.empty())
                return true;

            return QueueHelper::IsSupportedByDevice<typename std::remove_cvref_t<decltype(queuePool)>::value_type>(device, surface);
        };

        if (!check_queue_pool_support(queuePool.computeQueues_))
            return true;

        if (!check_queue_pool_support(queuePool.graphicsQueues_))
            return true;

        if (!check_queue_pool_support(queuePool.presentationQueues_))
            return true;

        if (!check_queue_pool_support(queuePool.transferQueues_))
            return true;

        return false;

    #if NOT_YET_IMPLEMENTED
        return !check_all_queues_support<Queues>(device, surface);
    #endif

    #if TEMPORARILY_DISABLED
        for (auto &&queue : queues) {
            auto supported = std::visit([=] (auto &&q)
            {
                return QueueHelper<std::remove_cvref_t<decltype(q)>>::IsSupportedByDevice(device, surface);
            }, queue);

            if (!supported)
                return true;
        }

        return false;
    #endif
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

    auto constexpr deviceTypesPriority = mpl::make_array(
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
    vkGetPhysicalDeviceProperties(physicalDevice_, &properties_);

    auto samplesCountBits = std::min(properties_.limits.framebufferColorSampleCounts, properties_.limits.framebufferDepthSampleCounts);

    samplesCount_ = static_cast<std::uint32_t>(std::pow(2, std::floor(std::log2(samplesCountBits))));

    QueueHelper queueHelper;

    for (auto &&queue : queuePool_.computeQueues_)
        queue = queueHelper.Find<std::remove_cvref_t<decltype(queue)>>(physicalDevice_, surface);

    for (auto &&queue : queuePool_.graphicsQueues_)
        queue = queueHelper.Find<std::remove_cvref_t<decltype(queue)>>(physicalDevice_, surface);

    for (auto &&queue : queuePool_.transferQueues_)
        queue = queueHelper.Find<std::remove_cvref_t<decltype(queue)>>(physicalDevice_, surface);

    for (auto &&queue : queuePool_.presentationQueues_)
        queue = queueHelper.Find<std::remove_cvref_t<decltype(queue)>>(physicalDevice_, surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::vector<std::vector<float>> priorities;

    for (auto [family, count] : queueHelper.GetRequestedFamilies()) {
        priorities.emplace_back(std::vector<float>(count, 1.f));

        VkDeviceQueueCreateInfo queueCreateInfo{
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            nullptr, 0,
            family, count,
            std::data(priorities.back())
        };

        queueCreateInfos.push_back(std::move(queueCreateInfo));
    }

#if TEMPORARILY_DISABLED
    std::set<std::uint32_t> uniqueQueueFamilyIndices;

    for (auto &&queue : queues)
        uniqueQueueFamilyIndices.emplace(std::visit([] (auto &&q) { return q.family(); }, queue));

    for (auto &&queueFamilyIndex : uniqueQueueFamilyIndices) {
        auto constexpr queuePriority = 1.f;

        VkDeviceQueueCreateInfo queueCreateInfo{
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            nullptr, 0,
            queueFamilyIndex, 1,
            &queuePriority
        };

        queueCreateInfos.push_back(std::move(queueCreateInfo));
    }
#endif

    VkPhysicalDeviceFeatures const deviceFeatures{kDEVICE_FEATURES};

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

    for (auto &&queue : queuePool_.computeQueues_)
        vkGetDeviceQueue(device_, queue.family_, queue.index_, &queue.handle_);

    for (auto &&queue : queuePool_.graphicsQueues_)
        vkGetDeviceQueue(device_, queue.family_, queue.index_, &queue.handle_);

    for (auto &&queue : queuePool_.transferQueues_)
        vkGetDeviceQueue(device_, queue.family_, queue.index_, &queue.handle_);

    for (auto &&queue : queuePool_.presentationQueues_)
        vkGetDeviceQueue(device_, queue.family_, queue.index_, &queue.handle_);

#if TEMPORARILY_DISABLED
    for (auto &&queue : queues) {
        std::visit([=] (auto &&q)
        {
            using Q = std::remove_cvref_t<decltype(q)>;
            q = std::move(QueueHelper::Find<Q>(physicalDevice_, device_, surface));

            if constexpr (std::is_same_v<Q, GraphicsQueue>)
                graphicsQueues_.push_back(std::move(q));

            else if constexpr (std::is_same_v<Q, ComputeQueue>)
                computeQueues_.push_back(std::move(q));

            else if constexpr (std::is_same_v<Q, TransferQueue>)
                transferQueues_.push_back(std::move(q));

            else if constexpr (std::is_same_v<Q, PresentationQueue>)
                presentationQueues_.push_back(std::move(q));

            else static_assert(std::false_type::type, "unsupported queue type");

        }, queue);
    }
#endif
}
