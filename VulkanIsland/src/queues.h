#pragma once
#include "device.h"

template<class T>
class VulkanQueue;

template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, Q>>...>
class QueueCreator;

template<class T>
class VulkanQueue {
public:

    VkQueue handle() const noexcept { return handle_; }

    template<class... Args>
    [[nodiscard]] static constexpr bool is_supported_by_device(Args &&... args)
    {
        return T::is_supported_by_device(std::forward<Args>(args)...);
    }

    template<class Q, typename std::enable_if_t<std::is_same_v<T, std::decay_t<Q>>>...>
    [[nodiscard]] constexpr bool operator== (Q &&queue) const noexcept
    {
        return family_ == queue.family_ && index_ == queue.index_;
    }

    template<class Q, typename std::enable_if_t<std::is_same_v<T, std::decay_t<Q>>>...>
    [[nodiscard]] constexpr bool operator!= (Q &&queue) const noexcept
    {
        return !(*this == queue);
    }

private:
    VkQueue handle_{nullptr};
    std::uint32_t family_, index_;

    friend QueueCreator;

};

class GraphicsQueue final : public VulkanQueue<GraphicsQueue> {
public:
    static auto constexpr kFLAGS{VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT};

    template<class...>
    GraphicsQueue()
    {
        ;
    }

    [[nodiscard]] static bool is_supported_by_device(VkPhysicalDevice physicalDevice)
    {
        return get_family_index(physicalDevice).has_value();
    }
    
    [[nodiscard]] static std::optional<std::uint32_t> get_family_index(VkPhysicalDevice physicalDevice)
    {
        std::uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, std::data(queueFamilies));

        if (queueFamilies.empty())
            return {};

        // Strict matching.
        auto it_family = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [] (auto &&queueFamily)
        {
            return queueFamily.queueCount > 0 && queueFamily.queueFlags == kFLAGS;
        });

        if (it_family != queueFamilies.cend())
            return static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_family));

        // Tolerant matching.
        it_family = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [] (auto &&queueFamily)
        {
            return queueFamily.queueCount > 0 && (queueFamily.queueFlags & kFLAGS) == kFLAGS;
        });

        if (it_family != queueFamilies.cend())
            return static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_family));

        return {};
    }
};

class TransferQueue final : public VulkanQueue<TransferQueue> {
public:
    static auto constexpr kFLAGS{VK_QUEUE_TRANSFER_BIT};
};

class PresentationQueue final : public VulkanQueue<PresentationQueue> {
public:
};


template<class Q, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, Q>>...>
class QueueCreator {
public:

    [[nodiscard]] constexpr Q Create(VulkanDevice const *device, VkSurfaceKHR surface) const
    {
        Q queue;
        queue.index_ = 0;

        if constexpr (std::is_same_v<Q, PresentationQueue>)
        {
            if (auto family = GetPresentationQueueFamilyIndex(physicalDevice, surface); !family)
                throw std::runtime_error("failed to create queue"s);

            else queue.family_ = index.value();

        }

        else {
            if (auto family = GetQueueFamilyIndex(physicalDevice); !family)
                throw std::runtime_error("failed to create queue"s);

            else queue.family_ = index.value();
        }

        vkGetDeviceQueue(device->handle(), queue.family_, queue.index_, &queue.handle_);

        return queue;
    }

private:

    [[nodiscard]] std::optional<std::uint32_t> GetQueueFamilyIndex(VkPhysicalDevice physicalDevice) const
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
        it_family = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [] (auto &&queueFamily)
        {
            return queueFamily.queueCount > 0 && (queueFamily.queueFlags & Q::kFLAGS) == Q::kFLAGS;
        });

        if (it_family != queueFamilies.cend())
            return static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_family));

        return {};
    }

    [[nodiscard]] std::optional<std::uint32_t> GetPresentationQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) const
    {
        std::uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, std::data(queueFamilies));

        if (queueFamilies.empty())
            return {};

        auto it_presentationQueue = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [physicalDevice, surface, size = queueFamilies.size()] (auto)
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