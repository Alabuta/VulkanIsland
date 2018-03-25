#pragma once
#include "main.h"
#include "device.h"
#include "queues.h"


template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, Q>>...>
class QueueBuilder final {
public:

    [[nodiscard]] static Q Build(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface)
    {
        Q queue;
        queue.index_ = 0;

        std::optional<std::uint32_t> family;

        if constexpr (std::is_same_v<Q, PresentationQueue>)
            family = GetPresentationQueueFamilyIndex(physicalDevice, surface);

        else family = GetQueueFamilyIndex(physicalDevice);

        if (!family)
            throw std::runtime_error("failed to create queue"s);

        queue.family_ = family.value();

        vkGetDeviceQueue(device, queue.family_, queue.index_, &queue.handle_);

        return queue;
    }

    [[nodiscard]] static bool IsSupportedByDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        if constexpr (std::is_same_v<Q, PresentationQueue>)
            return GetPresentationQueueFamilyIndex(physicalDevice, surface).has_value();

        else return GetQueueFamilyIndex(physicalDevice).has_value();
    }

private:

    [[nodiscard]] static std::optional<std::uint32_t> GetQueueFamilyIndex(VkPhysicalDevice physicalDevice)
    {
        std::uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, std::data(queueFamilies));

        if (queueFamilies.empty())
            return {};

#if TEMPORARILY_DISABLED
        // Strict matching.
        auto it_family = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [] (auto &&queueFamily)
        {
            return queueFamily.queueCount > 0 && queueFamily.queueFlags == Q::kFLAGS;
        });

        if (it_family != queueFamilies.cend())
            return static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_family));
#endif

        // Tolerant matching.
        auto it_family = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [] (auto &&queueFamily)
        {
            return queueFamily.queueCount > 0 && (queueFamily.queueFlags & Q::kFLAGS) == Q::kFLAGS;
        });

        if (it_family != queueFamilies.cend())
            return static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_family));

        return {};
    }

    [[nodiscard]] static std::optional<std::uint32_t> GetPresentationQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        std::uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, std::data(queueFamilies));

        if (queueFamilies.empty())
            return {};

        auto it_presentationQueue = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [physicalDevice, surface, size = queueFamilies.size()](auto)
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
};

template<class Q, class Tuple, std::size_t I = 0>
constexpr std::size_t get_queue_number()
{
    using T = std::tuple_element_t<I, Tuple>;

    if constexpr (std::is_same_v<Q, typename T::type>)
        return T::number;

    else if constexpr (I + 1 < std::tuple_size_v<Tuple>)
        return get_queue_number<Q, Tuple, I + 1>();

    return 0;
}

template<class... Qs>
class QueuePool final {
    using Tuple = std::tuple<Qs...>;

    std::array<GraphicsQueue, get_queue_number<GraphicsQueue, Tuple>()> graphicsQueues_;
    std::array<ComputeQueue, get_queue_number<ComputeQueue, Tuple>()> computeQueues_;
    std::array<TransferQueue, get_queue_number<TransferQueue, Tuple>()> transferQueue_;
    std::array<PresentationQueue, get_queue_number<PresentationQueue, Tuple>()> presentationQueue_;
};