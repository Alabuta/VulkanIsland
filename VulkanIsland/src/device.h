#pragma once

#include <variant>

#include "instance.h"
#include "device_defaults.h"
#include "queues.h"


class VulkanDevice final {
public:

    template<class E>
    VulkanDevice(VulkanInstance &instance, VkSurfaceKHR surface, E &&extensions);
    ~VulkanDevice();

    VkDevice handle() noexcept;
    VkPhysicalDevice physical_handle() noexcept;

    std::vector<std::uint32_t> const &supported_queues_indices() const noexcept;

#if NOT_YET_IMPLEMENTED
    template<class Q, std::enable_if_t<std::is_base_of_v<VulkanQueue<std::decay_t<Q>>, std::decay_t<Q>>>...>
    Q &GetQueue()
    {
        if constexpr (std::is_same_v<Q, GraphicsQueue>)
            return graphicsQueue_;

        else if constexpr (std::is_same_v<Q, TransferQueue>)
            return transferQueue_;

        else if constexpr (std::is_same_v<Q, PresentationQueue>)
            return presentationQueue_;

        else static_assert(std::false_type, "not implemented queue type");
    }
#endif

private:

    VulkanDevice() = delete;
    VulkanDevice(VulkanDevice const &) = delete;
    VulkanDevice(VulkanDevice &&) = delete;

    VkPhysicalDevice physicalDevice_{nullptr};
    VkDevice device_{nullptr};

    std::vector<std::uint32_t> supportedQueuesIndices_;

    GraphicsQueue graphicsQueue_;
    TransferQueue transferQueue_;
    PresentationQueue presentationQueue_;

    void PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, std::vector<std::string_view> &&extensions);
    void CreateDevice(VkSurfaceKHR surface, std::vector<char const *> &&extensions);
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
    CreateDevice(surface, std::move(extensions_));
}

inline VkDevice VulkanDevice::handle() noexcept
{
    return device_;
}

inline VkPhysicalDevice VulkanDevice::physical_handle() noexcept
{
    return physicalDevice_;
}

inline std::vector<std::uint32_t> const &VulkanDevice::supported_queues_indices() const noexcept
{
    return supportedQueuesIndices_;
}
