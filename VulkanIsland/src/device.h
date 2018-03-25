#pragma once

#include "instance.h"
#include "device_defaults.h"
#include "queues.h"


class VulkanDevice final {
public:

    template<class Qs, class E>
    VulkanDevice(VulkanInstance &instance, VkSurfaceKHR surface, Qs &queues, E &&extensions);
    ~VulkanDevice();

    VkDevice handle() const noexcept { return device_; };
    VkPhysicalDevice physical_handle() const noexcept { return physicalDevice_; };

    template<class Q, std::size_t I = 0, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, Q>>...>
    Q const &Get() const;

private:

    VulkanDevice() = delete;
    VulkanDevice(VulkanDevice const &) = delete;
    VulkanDevice(VulkanDevice &&) = delete;

    VkPhysicalDevice physicalDevice_{nullptr};
    VkDevice device_{nullptr};

#if NOT_YET_IMPLEMENTED
    GraphicsQueue graphicsQueue_;
    TransferQueue transferQueue_;
    PresentationQueue presentationQueue_;
#endif

    void PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, std::vector<Queues> const &queues, std::vector<std::string_view> &&extensions);
    void CreateDevice(VkSurfaceKHR surface, std::vector<Queues> &queues, std::vector<char const *> &&extensions);
};

template<class Qs, class E>
inline VulkanDevice::VulkanDevice(VulkanInstance &instance, VkSurfaceKHR surface, Qs &queues, E &&extensions)
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

    using Q = std::decay_t<Qs>;
    static_assert(is_iterable_v<Q>, "'queues' must be an iterable container");
    static_assert(std::is_same_v<typename std::decay_t<Q>::value_type, Queues>, "'queues' must contain 'Queues' instances");


template<class Q, std::size_t I, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, Q>>...>
inline Q const &VulkanDevice::Get() const
{
    if constexpr (std::is_same_v<Q, GraphicsQueue>)
        return graphicsQueues_.at(I);

    else if constexpr (std::is_same_v<Q, ComputeQueue>)
        return computeQueues_.at(I);

    std::vector<Queues> queues_{std::cbegin(queues), std::cend(queues)};
    else if constexpr (std::is_same_v<Q, TransferQueue>)
        return transferQueue_.at(I);

    PickPhysicalDevice(instance.handle(), surface, queues_, std::move(extensions_view));
    CreateDevice(surface, queues_, std::move(extensions_));
    else if constexpr (std::is_same_v<Q, PresentationQueue>)
        return presentationQueue_.at(I);

    std::move(std::cbegin(queues_), std::cend(queues_), std::begin(queues));
    else static_assert(std::false_type::type, "unsupported queue type");
}
