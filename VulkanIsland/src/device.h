#pragma once

#include "instance.h"
#include "device_defaults.h"
#include "queues.h"
#include "queue_builder.h"



auto constexpr deviceExtensions = make_array(
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_MAINTENANCE1_EXTENSION_NAME
);

class VulkanDevice final {
public:

    template<class E, class... Qs>
    VulkanDevice(VulkanInstance &instance, VkSurfaceKHR surface, E &&extensions, QueuePool<Qs...> &&qpool);
    ~VulkanDevice();

    VkDevice handle() const noexcept { return device_; };
    VkPhysicalDevice physical_handle() const noexcept { return physicalDevice_; };

    template<class Q, std::size_t I = 0, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, Q>>...>
    Q const &Get() const;


#if NOT_YET_IMPLEMENTED
    template<VkCommandBufferLevel L>
    struct VulkanCmdBuffer {
        static auto constexpr level{L};
    };

    template<class Q, std::size_t I = 0, VkCommandBufferLevel L, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, Q>>...>
    std::vector<VulkanCmdBuffer<L>> AllocateCmdBuffers(std::size_t count)
    {
        std::vector<VulkanCmdBuffer<L>> commandBuffers(count);

        VkCommandBufferAllocateInfo const allocateInfo{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            nullptr,
            VkCommandPool{}, //graphicsCommandPool,
            L,
            static_cast<std::uint32_t>(std::size(commandBuffers))
        };

        if (auto result = vkAllocateCommandBuffers(device, &allocateInfo, std::data(commandBuffers)); result != VK_SUCCESS)
            throw std::runtime_error("failed to create allocate command buffers: "s + std::to_string(result));

        return commandBuffers;
    }
#endif

private:

    VkPhysicalDevice physicalDevice_{nullptr};
    VkDevice device_{nullptr};

    struct QueuePoolImpl final {
        std::vector<GraphicsQueue> graphicsQueues_;
        std::vector<ComputeQueue> computeQueues_;
        std::vector<TransferQueue> transferQueues_;
        std::vector<PresentationQueue> presentationQueues_;
    } queuePool_;


    VulkanDevice() = delete;
    VulkanDevice(VulkanDevice const &) = delete;
    VulkanDevice(VulkanDevice &&) = delete;

    void PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, std::vector<std::string_view> &&extensions);
    void CreateDevice(VkSurfaceKHR surface, std::vector<char const *> &&extensions);
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

    using Queues = typename std::decay_t<decltype(qpool)>::Tuple;

    queuePool_.computeQueues_.resize(get_type_instances_number<ComputeQueue, Queues>());
    queuePool_.graphicsQueues_.resize(get_type_instances_number<GraphicsQueue, Queues>());
    queuePool_.transferQueues_.resize(get_type_instances_number<TransferQueue, Queues>());
    queuePool_.presentationQueues_.resize(get_type_instances_number<PresentationQueue, Queues>());

    PickPhysicalDevice(instance.handle(), surface, std::move(extensions_view));
    CreateDevice(surface, std::move(extensions_));
}

template<class Q, std::size_t I, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, Q>>...>
inline Q const &VulkanDevice::Get() const
{
    if constexpr (std::is_same_v<Q, GraphicsQueue>)
        return queuePool_.graphicsQueues_.at(I);

    else if constexpr (std::is_same_v<Q, ComputeQueue>)
        return queuePool_.computeQueues_.at(I);

    else if constexpr (std::is_same_v<Q, TransferQueue>)
        return queuePool_.transferQueues_.at(I);

    else if constexpr (std::is_same_v<Q, PresentationQueue>)
        return queuePool_.presentationQueues_.at(I);

    else static_assert(always_false<Q>::value, "unsupported queue type");
}
