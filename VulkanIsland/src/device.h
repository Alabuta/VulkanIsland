#pragma once

#include "instance.h"
#include "device_defaults.h"

auto constexpr requiredQueues = make_array(
    VkQueueFamilyProperties{VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT},
    VkQueueFamilyProperties{VK_QUEUE_TRANSFER_BIT}
);


class VulkanDevice final {
public:

    template<class E>
    VulkanDevice(VulkanInstance &instance, VkSurfaceKHR surface, E &&extensions);
    ~VulkanDevice();

    VkDevice handle();
    VkPhysicalDevice physical_handle();

    std::vector<std::uint32_t> const &supported_queues_indices() const;

public:

    VkDevice device_;
    VkPhysicalDevice physicalDevice_;

    std::vector<std::uint32_t> supportedQueuesIndices_;

    VulkanDevice() = delete;

    void PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, std::vector<std::string_view> &&extensions);
    void CreateDevice(VkInstance instance, VkSurfaceKHR surface, std::vector<char const *> &&extensions);
};

template<class E>
inline VulkanDevice::VulkanDevice(VulkanInstance &instance, VkSurfaceKHR surface, E &&extensions)
{
    auto constexpr use_extensions = !std::is_same_v<std::false_type, E>;

    std::vector<std::string_view> extensions_view;
    std::vector<char const *> extensions_;

    if constexpr (use_extensions)
    {
        using T = std::decay_t<E>;
        static_assert(is_container_v<T>, "'extensions' must be a container");
        static_assert(std::is_same_v<typename std::decay_t<T>::value_type, char const *>, "'extensions' must contain null-terminated strings");

        if constexpr (std::is_rvalue_reference_v<T>)
            std::move(extensions.begin(), extensions.end(), std::back_inserter(extensions_));

        else std::copy(extensions.begin(), extensions.end(), std::back_inserter(extensions_));

        std::copy(extensions_.begin(), extensions_.end(), std::back_inserter(extensions_view));
    }

    PickPhysicalDevice(instance.handle(), surface, std::move(extensions_view));
    CreateDevice(instance.handle(), surface, std::move(extensions_));
}

inline VkDevice VulkanDevice::handle()
{
    return device_;
}

inline VkPhysicalDevice VulkanDevice::physical_handle()
{
    return physicalDevice_;
}

inline std::vector<std::uint32_t> const &VulkanDevice::supported_queues_indices() const
{
    return supportedQueuesIndices_;
}
