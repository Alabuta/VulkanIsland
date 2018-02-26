#pragma once



template<class T>
class VulkanQueue {
public:

    VkQueue handle() { return handle_; }
    VkQueue &handle() const { return handle_; }

    template<class... Args>
    [[nodiscard]] constexpr bool StrictMatching(Args &&... args)
    {
        return static_cast<T *>(this)->StrictMatching(std::forward<Args>(args)...);
    }

    template<class... Args>
    [[nodiscard]] constexpr bool TolerantMatching(Args &&... args)
    {
        return static_cast<T *>(this)->TolerantMatching(std::forward<Args>(args)...);
    }

    /*template<class... Args>
    VulkanQueue(Args &&... args)
    {
    *static_cast<T *>(this) = std::move(T(std::forward<Args>(args)...));
    }*/

    template<class... Args>
    [[nodiscard]] static bool constexpr has(Args &&... args)
    {
        return T::has(std::forward<Args>(args)...);
    }

private:
    VkQueue handle_{VK_NULL_HANDLE};
    std::uint32_t family_, index_;

};

class GraphicsQueue final : VulkanQueue<GraphicsQueue> {
public:

    template<class P, std::enable_if_t<std::is_same_v<std::decay_t<P>, VkQueueFamilyProperties>>...>
    [[nodiscard]] constexpr bool StrictMatching(P &&properties)
    {
        return properties.queueCount > 0 && properties.queueFlags == (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
    }

    template<class P, std::enable_if_t<std::is_same_v<std::decay_t<P>, VkQueueFamilyProperties>>...>
    [[nodiscard]] constexpr bool TolerantMatching(P &&properties)
    {
        return properties.queueCount > 0 && (properties.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
    }

    /*template<class T, class...>
    GraphicsQueue(T &&queueFamilies)
    {
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkQueueFamilyProperties>, "iterable object does not contain VkQueueFamilyProperties elements");
    }*/

    template<class...>
    [[nodiscard]] static std::optional<std::uint32_t> supported_by_device(VkPhysicalDevice physicalDevice)
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
            return queueFamily.queueCount > 0 && queueFamily.queueFlags == (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
        });

        if (it_family != queueFamilies.cend())
            return static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_family));

        // Tolerant matching.
        it_family = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [] (auto &&queueFamily)
        {
            return queueFamily.queueCount > 0 && (queueFamily.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
        });

        if (it_family != queueFamilies.cend())
            return static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_family));

        return {};
    }
};

class TransferQueue final : VulkanQueue<TransferQueue> {
public:

    template<class P, std::enable_if_t<std::is_same_v<std::decay_t<P>, VkQueueFamilyProperties>>...>
    [[nodiscard]] constexpr bool StrictMatching(P &&properties)
    {
        return properties.queueCount > 0 && properties.queueFlags == VK_QUEUE_TRANSFER_BIT;
    }

    template<class P, std::enable_if_t<std::is_same_v<std::decay_t<P>, VkQueueFamilyProperties>>...>
    [[nodiscard]] constexpr bool TolerantMatching(P &&properties)
    {
        return properties.queueCount > 0 && (properties.queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT;
    }
};

class PresentationQueue final : VulkanQueue<PresentationQueue> {
public:

    template<class...>
    [[nodiscard]] constexpr bool StrictMatching(/*VulkanDevice const &device,*/VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::uint32_t queueIndex)
    {
        VkBool32 surfaceSupported = 0;
        if (auto result = vkGetPhysicalDeviceSurfaceSupportKHR(/*device.physical_handle()*/physicalDevice, queueIndex, surface, &surfaceSupported); result != VK_SUCCESS)
            throw std::runtime_error("failed to retrieve surface support: "s + std::to_string(result));

        return surfaceSupported == VK_TRUE;
    }
};