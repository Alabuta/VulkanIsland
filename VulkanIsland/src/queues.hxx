#pragma once
#include <variant>

#include "main.hxx"
#include "device.hxx"

class VulkanDevice;

template<class T>
class VulkanQueue;

class QueueHelper;

template<class T>
class VulkanQueue {
public:
    VulkanQueue(VulkanQueue const &) = default;
    ~VulkanQueue() = default;

    VulkanQueue &operator= (VulkanQueue const &) = default;

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

    VkQueue handle() const noexcept { return handle_; }
    std::uint32_t family() const noexcept { return family_; }

protected:
    VulkanQueue() = default;
    VulkanQueue(VulkanQueue &&) = default;

private:
    VkQueue handle_{nullptr};
    std::uint32_t family_, index_;

    friend VulkanDevice;
    friend QueueHelper;
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

using Queues = std::variant<GraphicsQueue, ComputeQueue, TransferQueue, PresentationQueue>;