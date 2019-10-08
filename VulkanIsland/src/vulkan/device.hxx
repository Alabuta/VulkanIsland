#pragma once

#include <memory>

#include "utility/mpl.hxx"
#include "instance.hxx"
#include "device_limits.hxx"
#include "renderer/queues.hxx"


namespace vulkan
{
    class device final {
    public:

        device(vulkan::instance &instance, VkSurfaceKHR surface);
        ~device();

        VkDevice handle() const noexcept { return handle_; };
        VkPhysicalDevice physical_handle() const noexcept { return physical_handle_; };

        vulkan::device_limits const &device_limits() const noexcept { return device_limits_; };

        template<class Q> requires mpl::derived_from<VulkanQueue<Q>, Q>
        Q const &queue() const noexcept;

    private:

        VkDevice handle_{nullptr};
        VkPhysicalDevice physical_handle_{nullptr};

        vulkan::device_limits device_limits_;

        std::vector<GraphicsQueue> graphics_queues_;
        std::vector<ComputeQueue> compute_queues_;
        std::vector<TransferQueue> transfer_queues_;
        std::vector<PresentationQueue> presentation_queues_;

        device() = delete;
        device(vulkan::device const &) = delete;
        device(vulkan::device &&) = delete;
    };

    template<class Q> requires mpl::derived_from<VulkanQueue<Q>, Q>
    inline Q const &device::queue() const noexcept
    {
        if constexpr (std::is_same_v<Q, GraphicsQueue>)
            return graphics_queues_.at(0);

        else if constexpr (std::is_same_v<Q, ComputeQueue>)
            return compute_queues_.at(0);

        else if constexpr (std::is_same_v<Q, TransferQueue>)
            return transfer_queues_.at(0);

        else if constexpr (std::is_same_v<Q, PresentationQueue>)
            return presentation_queues_.at(0);

        else static_assert(always_false<Q>::value, "unsupported queue type");
    }
}
