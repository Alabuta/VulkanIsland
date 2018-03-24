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
    static auto constexpr kFLAGS{VK_QUEUE_GRAPHICS_BIT};
};

class ComputeQueue final : public VulkanQueue<ComputeQueue> {
public:
    static auto constexpr kFLAGS{VK_QUEUE_COMPUTE_BIT};
};

class TransferQueue final : public VulkanQueue<TransferQueue> {
public:
    static auto constexpr kFLAGS{VK_QUEUE_TRANSFER_BIT};
};

class PresentationQueue final : public VulkanQueue<PresentationQueue> {};