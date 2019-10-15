#pragma once

#include <variant>

#include "utility/mpl.hxx"
#include "vulkan/device.hxx"
#include "graphics/graphics.hxx"


namespace vulkan
{
    class device;
}

namespace graphics
{
    class queue {
    public:

        /*template<graphics::QUEUE_CAPABILITY qc>
        auto constexpr operator< (queue<qc>) const noexcept { return SI < si; }

        template<graphics::QUEUE_CAPABILITY qc>
        auto constexpr operator== (queue<qc>) const noexcept { return SI == si; }*/

        /*template<class Q> requires std::same_as<queue<QC>, std::remove_cvref_t<Q>>
        bool constexpr operator== (Q &&queue) const noexcept
        {
            return family_ == queue.family_ && index_ == queue.index_;
        }*/

        // queue(VkQueue handle, std::uint32_t family, std::uint32_t index) noexcept : handle_{handle}, family_{family}, index_{index} { }

        VkQueue handle() const noexcept { return handle_; }

        std::uint32_t family() const noexcept { return family_; }
        std::uint32_t index() const noexcept { return index_; }

    private:

        VkQueue handle_{nullptr};
        std::uint32_t family_{0}, index_{0};

        friend vulkan::device;
    };

    struct graphics_queue final : public graphics::queue {
        static auto constexpr capability{graphics::QUEUE_CAPABILITY::GRAPHICS};
    };

    struct compute_queue final : public graphics::queue {
        static auto constexpr capability{graphics::QUEUE_CAPABILITY::COMPUTE};
    };

    struct transfer_queue final : public graphics::queue {
        static auto constexpr capability{graphics::QUEUE_CAPABILITY::TRANSFER};
    };
}

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

    VulkanQueue() = default;
    VulkanQueue(VulkanQueue &&) = default;

    VkQueue handle_{nullptr};
    std::uint32_t family_{0}, index_{0};

    friend vulkan::device;
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