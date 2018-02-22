#pragma once

#include "instance.h"
#include "device_defaults.h"

auto constexpr requiredQueues = make_array(
    VkQueueFamilyProperties{VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT},
    VkQueueFamilyProperties{VK_QUEUE_TRANSFER_BIT}
);


class VulkanDevice final {
public:

    //VulkanDevice() = default;
    VulkanDevice(VulkanDevice const &) = default;
    VulkanDevice(VulkanDevice &&) = default;

    template<class E>
    VulkanDevice(VulkanInstance &instance, VkSurfaceKHR surface, E &&extensions);
    ~VulkanDevice();

    VkPhysicalDevice &physical_handle()
    {
        return physicalDevice_;
    }

public:

    VkPhysicalDevice physicalDevice_;
    VkDevice device_;

    void PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, std::vector<std::string_view> &&extension);
};

template<class E>
inline VulkanDevice::VulkanDevice(VulkanInstance &instance, VkSurfaceKHR surface, E &&extensions)
{
    auto constexpr use_extensions = !std::is_same_v<std::false_type, E>;

    std::vector<std::string_view> exts;

    if constexpr (use_extensions)
    {
        using T = std::decay_t<E>;
        static_assert(is_container_v<T>, "'extensions' must be a container");
        static_assert(std::is_same_v<typename std::decay_t<T>::value_type, char const *>, "'extensions' must contain null-terminated strings");

        std::copy(extensions.begin(), extensions.end(), std::back_inserter(exts));
    }

    PickPhysicalDevice(instance.handle(), surface, std::move(exts));
}
