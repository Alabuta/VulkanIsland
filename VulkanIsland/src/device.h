#pragma once

#include "instance.h"
#include "device_defaults.h"
#include "queues.h"
#include "queue_builder.h"


class VulkanDevice final {
public:

    template<class E, class... Qs>
    VulkanDevice(VulkanInstance &instance, VkSurfaceKHR surface, E &&extensions, QueuePool<Qs...> &&qpool);
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

    QueuePoolImpl queuePool_;

    std::vector<GraphicsQueue> graphicsQueues_;
    std::vector<ComputeQueue> computeQueues_;
    std::vector<TransferQueue> transferQueues_;
    std::vector<PresentationQueue> presentationQueues_;

    void PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, std::vector<Queues> const &queues, std::vector<std::string_view> &&extensions);
    void CreateDevice(VkSurfaceKHR surface, std::vector<Queues> &&queues, std::vector<char const *> &&extensions);
};

template<class E, class... Qs>
inline VulkanDevice::VulkanDevice(VulkanInstance &instance, VkSurfaceKHR surface, E &&extensions, QueuePool<Qs...> &&qpool)
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

    using Tuple = typename std::decay_t<decltype(qpool)>::Tuple;
    queuePool_.graphicsQueues_.resize(get_type_instances_number<GraphicsQueue, Tuple>());
    queuePool_.computeQueues_.resize(get_type_instances_number<ComputeQueue, Tuple>());
    queuePool_.transferQueues_.resize(get_type_instances_number<TransferQueue, Tuple>());
    queuePool_.presentationQueues_.resize(get_type_instances_number<PresentationQueue, Tuple>());

    /*using Q = std::decay_t<Qs>;
    static_assert(is_iterable_v<Q>, "'queues' must be an iterable container");
    static_assert(std::is_same_v<typename std::decay_t<Q>::value_type, Queues>, "'queues' must contain 'Queues' instances");*/

    std::vector<Queues> queues;// { std::cbegin(queues), std::cend(queues) };

    PickPhysicalDevice(instance.handle(), surface, queues, std::move(extensions_view));
    CreateDevice(surface, std::move(queues), std::move(extensions_));
}


template<class Q, std::size_t I, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, Q>>...>
inline Q const &VulkanDevice::Get() const
{
    if constexpr (std::is_same_v<Q, GraphicsQueue>)
        return graphicsQueues_.at(I);

    else if constexpr (std::is_same_v<Q, ComputeQueue>)
        return computeQueues_.at(I);

    else if constexpr (std::is_same_v<Q, TransferQueue>)
        return transferQueues_.at(I);

    else if constexpr (std::is_same_v<Q, PresentationQueue>)
        return presentationQueues_.at(I);

    else static_assert(std::false_type::type, "unsupported queue type");
}
