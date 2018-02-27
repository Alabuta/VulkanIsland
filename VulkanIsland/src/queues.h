#pragma once



template<class T>
class VulkanQueue {
public:

    VkQueue handle() { return handle_; }
    VkQueue &handle() const { return handle_; }

    template<class... Args>
    [[nodiscard]] static bool constexpr supported_by_device(Args &&... args)
    {
        return T::supported_by_device(std::forward<Args>(args)...);
    }

private:
    VkQueue handle_{VK_NULL_HANDLE};
    std::uint32_t family_, index_;

};

class GraphicsQueue final : VulkanQueue<GraphicsQueue> {
public:

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
};

class PresentationQueue final : VulkanQueue<PresentationQueue> {
public:
};