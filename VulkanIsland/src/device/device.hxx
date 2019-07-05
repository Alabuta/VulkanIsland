#pragma once

#include "instance.hxx"
#include "queues.hxx"
#include "queueBuilder.hxx"

#define USE_DEBUG_MARKERS 0

class MemoryManager;
class ResourceManager;


namespace config {
auto constexpr deviceExtensions = make_array(
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,

    VK_KHR_MAINTENANCE1_EXTENSION_NAME,
    VK_KHR_MAINTENANCE2_EXTENSION_NAME,
    VK_KHR_MAINTENANCE3_EXTENSION_NAME,

    VK_KHR_16BIT_STORAGE_EXTENSION_NAME,
#if USE_WIN32
    VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME,
#else
    "VK_EXT_scalar_block_layout",
#endif

    VK_KHR_MULTIVIEW_EXTENSION_NAME,
    VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
    VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME,
    /*VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
    VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,*/
    VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME

);
}

class VulkanDevice final {
public:

    template<class E, class... Qs>
    VulkanDevice(VulkanInstance &instance, VkSurfaceKHR surface, E &&extensions, QueuePool<Qs...> &&qpool);
    ~VulkanDevice();

    VkDevice handle() const noexcept { return device_; };
    VkPhysicalDevice physical_handle() const noexcept { return physicalDevice_; };

    template<class Q, std::size_t I = 0, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, Q>>* = nullptr>
    Q const &queue() const noexcept;

    MemoryManager &memoryManager() noexcept { return *memoryManager_; }
    MemoryManager const &memoryManager() const noexcept { return *memoryManager_; }

    ResourceManager &resourceManager() noexcept { return *resourceManager_; }
    ResourceManager const &resourceManager() const noexcept { return *resourceManager_; }

    VkSampleCountFlagBits samplesCount() const noexcept { return samplesCount_; }

    VkPhysicalDeviceProperties const &properties() const noexcept { return properties_; };

#if NOT_YET_IMPLEMENTED
    template<VkCommandBufferLevel L>
    struct VulkanCmdBuffer {
        static auto constexpr level{L};
    };

    template<class Q, std::size_t I = 0, VkCommandBufferLevel L, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, Q>>* = nullptr>
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

    std::unique_ptr<MemoryManager> memoryManager_;
    std::unique_ptr<ResourceManager> resourceManager_;

    VkSampleCountFlagBits samplesCount_{VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT};

    VkPhysicalDeviceProperties properties_;

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

#if USE_DEBUG_MARKERS
        extensions_.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
#endif

        std::copy(extensions_.begin(), extensions_.end(), std::back_inserter(extensions_view));
    }

    using Queues = typename std::decay_t<decltype(qpool)>::Tuple;

    queuePool_.computeQueues_.resize(get_type_instances_number<ComputeQueue, Queues>());
    queuePool_.graphicsQueues_.resize(get_type_instances_number<GraphicsQueue, Queues>());
    queuePool_.transferQueues_.resize(get_type_instances_number<TransferQueue, Queues>());
    queuePool_.presentationQueues_.resize(get_type_instances_number<PresentationQueue, Queues>());

    PickPhysicalDevice(instance.handle(), surface, std::move(extensions_view));
    CreateDevice(surface, std::move(extensions_));

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice_, &properties);

    memoryManager_ = std::make_unique<MemoryManager>(*this, properties.limits.bufferImageGranularity);
    resourceManager_ = std::make_unique<ResourceManager>(*this);
}

template<class Q, std::size_t I, typename std::enable_if_t<std::is_base_of_v<VulkanQueue<Q>, Q>>* = nullptr>
inline Q const &VulkanDevice::queue() const noexcept
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
