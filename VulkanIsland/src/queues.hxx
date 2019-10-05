#pragma once

#include <variant>

#include "main.hxx"
#include "utility/mpl.hxx"
#include "device/device.hxx"
#include "graphics/graphics.hxx"

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

    template<class Q> requires std::same_as<T, std::remove_cvref_t<Q>>
    [[nodiscard]] constexpr bool operator== (Q &&queue) const noexcept
    {
        return family_ == queue.family_ && index_ == queue.index_;
    }

    template<class Q> requires std::same_as<T, std::remove_cvref_t<Q>>
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
    std::uint32_t family_{0}, index_{0};

    friend VulkanDevice;
    friend QueueHelper;
};

class GraphicsQueue final : public VulkanQueue<GraphicsQueue> {
public:
    static auto constexpr kCAPABILITY{ graphics::QUEUE_CAPABILITY::GRAPHICS };
};

class ComputeQueue final : public VulkanQueue<ComputeQueue> {
public:
    static auto constexpr kCAPABILITY{ graphics::QUEUE_CAPABILITY::COMPUTE };
};

class TransferQueue final : public VulkanQueue<TransferQueue> {
public:
    static auto constexpr kCAPABILITY{ graphics::QUEUE_CAPABILITY::TRANSFER };
};

class PresentationQueue final : public VulkanQueue<PresentationQueue> {};

using queues_t = std::variant<GraphicsQueue, ComputeQueue, TransferQueue, PresentationQueue>;