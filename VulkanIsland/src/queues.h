#pragma once



template<class T>
class VulkanQueue {
public:

    VkQueue handle() const noexcept { return handle_; }
    VkQueue &handle() noexcept { return handle_; }

    template<class... Args>
    [[nodiscard]] static bool constexpr is_supported_by_device(Args &&... args)
    {
        return T::is_supported_by_device(std::forward<Args>(args)...);
    }

private:
    VkQueue handle_{nullptr};
    std::uint32_t family_, index_;

};

class GraphicsQueue final : VulkanQueue<GraphicsQueue> {
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

class TransferQueue final : VulkanQueue<TransferQueue> {
public:
};

class PresentationQueue final : VulkanQueue<PresentationQueue> {
public:
};