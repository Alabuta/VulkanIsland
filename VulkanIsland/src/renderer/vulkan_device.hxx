#pragma once

#include <memory>

#include "utility/mpl.hxx"
#include "vulkan_instance.hxx"
#include "queues.hxx"

#define USE_DEBUG_MARKERS 0

class MemoryManager;
class ResourceManager;


namespace vulkan
{
    class device final {
    public:

        device(vulkan::instance &instance, VkSurfaceKHR surface);
        ~device();

        VkDevice handle() const noexcept { return handle_; };
        VkPhysicalDevice physical_handle() const noexcept { return physical_handle_; };

        template<class Q> requires mpl::derived_from<VulkanQueue<Q>, Q>
        Q const &queue() const noexcept;

        std::uint32_t samples_count() const noexcept { return samples_count_; }

        VkPhysicalDeviceProperties const &properties() const noexcept { return properties_; };

        MemoryManager &memory_manager() noexcept { return *memory_manager_; }
        MemoryManager const &memory_manager() const noexcept { return *memory_manager_; }

        ResourceManager &resource_manager() noexcept { return *resource_manager_; }
        ResourceManager const &resource_manager() const noexcept { return *resource_manager_; }

    private:

        VkPhysicalDevice physical_handle_{nullptr};
        VkDevice handle_{nullptr};

        std::vector<GraphicsQueue> graphics_queues_;
        std::vector<ComputeQueue> compute_queues_;
        std::vector<TransferQueue> transfer_queues_;
        std::vector<PresentationQueue> presentation_queues_;

        std::unique_ptr<MemoryManager> memory_manager_;
        std::unique_ptr<ResourceManager> resource_manager_;

        std::uint32_t samples_count_{1};

        VkPhysicalDeviceProperties properties_;

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
