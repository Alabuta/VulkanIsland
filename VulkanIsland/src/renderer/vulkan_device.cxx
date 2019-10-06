#include <type_traits>
#include <functional>
#include <algorithm>
#include <bitset>

using namespace std::string_literals;
using namespace std::string_view_literals;

#include <fmt/format.h>

#include "vulkan_config.hxx"
#include "swapchain.hxx"
#include "renderer/vulkan_device.hxx"
#include "device_config.hxx"
#include "resources/buffer.hxx"
#include "resources/resource.hxx"
#include "queue_builder.hxx"


namespace
{
    auto constexpr device_extensions = std::array{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,

        VK_KHR_MAINTENANCE1_EXTENSION_NAME,
        VK_KHR_MAINTENANCE2_EXTENSION_NAME,
        VK_KHR_MAINTENANCE3_EXTENSION_NAME,

        VK_KHR_MULTIVIEW_EXTENSION_NAME,
        VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
        VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME,
        /*VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
        VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,*/
        VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME,

        VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME,
        VK_KHR_8BIT_STORAGE_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME,
        VK_KHR_16BIT_STORAGE_EXTENSION_NAME
        //VK_KHR_INLINE_UNIFORM_BLOCK_EXTENSION_NAME

    };
}

namespace
{
    struct required_device_queues final {
        std::vector<GraphicsQueue> &graphics_queues_;
        std::vector<ComputeQueue> &compute_queues_;
        std::vector<TransferQueue> &transfer_queues_;
        std::vector<PresentationQueue> &presentation_queues_;
    };

    template<class T, std::size_t I = 0>
    constexpr bool check_all_queues_support(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
    {
        using Q = std::variant_alternative_t<I, T>;

        if (!QueueHelper::IsSupportedByDevice<Q>(physical_device, surface))
            return false;

        if constexpr (I + 1 < std::variant_size_v<T>)
            return check_all_queues_support<T, I + 1>(physical_device, surface);

        return false;
    }

    template<bool check_on_duplicates = false>
    [[nodiscard]] bool check_required_device_extensions(VkPhysicalDevice physical_device, std::vector<std::string_view> &&extensions)
    {
        std::vector<VkExtensionProperties> required_extensions;

        auto constexpr extensions_compare = [] (auto &&lhs, auto &&rhs)
        {
            return std::lexicographical_compare(std::cbegin(lhs.extensionName), std::cend(lhs.extensionName),
                                                std::cbegin(rhs.extensionName), std::cend(rhs.extensionName));
        };

        std::transform(std::cbegin(extensions), std::cend(extensions), std::back_inserter(required_extensions), [] (auto &&name)
        {
            VkExtensionProperties prop{};
            std::uninitialized_copy_n(std::begin(name), std::size(name), prop.extensionName);

            return prop;
        });

        std::sort(std::begin(required_extensions), std::end(required_extensions), extensions_compare);

        if constexpr (check_on_duplicates) {
            auto it = std::unique(std::begin(required_extensions), std::end(required_extensions), [] (auto &&lhs, auto &&rhs)
            {
                return std::equal(std::cbegin(lhs.extensionName), std::cend(lhs.extensionName),
                                  std::cbegin(rhs.extensionName), std::cend(rhs.extensionName));
            });

            required_extensions.erase(it, std::end(required_extensions));
        }

        std::uint32_t extensions_count = 0;

        if (auto result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensions_count, nullptr); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to retrieve device extensions count: {0:#x}\n"s, result));

        std::vector<VkExtensionProperties> supported_extensions(extensions_count);

        if (auto result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensions_count, std::data(supported_extensions)); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to retrieve device extensions: {0:#x}\n"s, result));

        std::sort(std::begin(supported_extensions), std::end(supported_extensions), extensions_compare);

        return std::includes(std::cbegin(supported_extensions), std::cend(supported_extensions),
                             std::cbegin(required_extensions), std::cend(required_extensions), extensions_compare);
    }

    [[nodiscard]] VkPhysicalDevice
    pick_physical_device(VkInstance instance, VkSurfaceKHR surface, std::vector<std::string_view> &&extensions, required_device_queues &required_queues)
    {
        std::uint32_t devices_count = 0;

        if (auto result = vkEnumeratePhysicalDevices(instance, &devices_count, nullptr); result != VK_SUCCESS || devices_count == 0)
            throw std::runtime_error("failed to find physical device with Vulkan API support: "s + std::to_string(result));

        std::vector<VkPhysicalDevice> devices(devices_count);

        if (auto result = vkEnumeratePhysicalDevices(instance, &devices_count, std::data(devices)); result != VK_SUCCESS)
            throw std::runtime_error("failed to retrieve physical devices: "s + std::to_string(result));

        auto application_info = vulkan_config::application_info;

        // Matching by supported properties, features and extensions.
        auto it_end = std::remove_if(std::begin(devices), std::end(devices), [&extensions, &application_info] (auto &&device)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);

            if (properties.apiVersion < application_info.apiVersion)
                return true;

            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures(device, &features);

            if (!compare_physical_device_features(features))
                return true;

            return !check_required_device_extensions(device, std::move(extensions));
        });

        devices.erase(it_end, std::end(devices));

        // Removing unsuitable devices. Matching by required compute, graphics, transfer and presentation queues.
        it_end = std::remove_if(std::begin(devices), std::end(devices), [&] (auto &&device)
        {
            auto check_queue_pool_support = [device, surface, &required_queues] (auto &&queue_pool)
            {
                if (queue_pool.empty())
                    return true;

                return QueueHelper::IsSupportedByDevice<typename std::remove_cvref_t<decltype(queue_pool)>::value_type>(device, surface);
            };

            if (!check_queue_pool_support(required_queues.compute_queues_))
                return true;

            if (!check_queue_pool_support(required_queues.graphics_queues_))
                return true;

            if (!check_queue_pool_support(required_queues.presentation_queues_))
                return true;

            if (!check_queue_pool_support(required_queues.transfer_queues_))
                return true;

            return false;

        #if NOT_YET_IMPLEMENTED
            return !check_all_queues_support<Queues>(device, surface);
        #endif

        #if TEMPORARILY_DISABLED
            for (auto &&queue : queues) {
                auto supported = std::visit([=] (auto &&q)
                {
                    return QueueHelper<std::remove_cvref_t<decltype(q)>>::IsSupportedByDevice(device, surface);
                }, queue);

                if (!supported)
                    return true;
            }

            return false;
        #endif
        });

        devices.erase(it_end, std::end(devices));

        // Matchin by the swap chain properties support.
        it_end = std::remove_if(std::begin(devices), std::end(devices), [surface] (auto &&device)
        {
            auto const details = QuerySwapChainSupportDetails(device, surface);
            return details.formats.empty() || details.presentModes.empty();
        });

        devices.erase(it_end, std::end(devices));

        if (devices.empty())
            throw std::runtime_error("failed to pick physical device"s);

        auto constexpr device_types_priority = std::array{
            VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
            VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, VK_PHYSICAL_DEVICE_TYPE_CPU
        };

        // Sorting by device type.
        for (auto device_type : device_types_priority) {
            auto id_next_type = std::stable_partition(std::begin(devices), std::end(devices), [device_type] (auto &&device)
            {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(device, &properties);

                return properties.deviceType == device_type;
            });

            if (id_next_type != std::end(devices))
                break;
        }

        return devices.front();
    }

    [[nodiscard]] std::vector<VkDeviceQueueCreateInfo>
    get_queue_infos(VkPhysicalDevice physical_handle, VkSurfaceKHR surface, std::vector<char const *> &&extensions,
                    required_device_queues &required_queues, std::vector<std::vector<float>> &priorities)
    {
        QueueHelper queue_helper;

        for (auto &&queue : required_queues.compute_queues_)
            queue = queue_helper.Find<std::remove_cvref_t<decltype(queue)>>(physical_handle, surface);

        for (auto &&queue : required_queues.graphics_queues_)
            queue = queue_helper.Find<std::remove_cvref_t<decltype(queue)>>(physical_handle, surface);

        for (auto &&queue : required_queues.transfer_queues_)
            queue = queue_helper.Find<std::remove_cvref_t<decltype(queue)>>(physical_handle, surface);

        for (auto &&queue : required_queues.presentation_queues_)
            queue = queue_helper.Find<std::remove_cvref_t<decltype(queue)>>(physical_handle, surface);

        std::vector<VkDeviceQueueCreateInfo> queue_infos;

        for (auto [family, count] : queue_helper.GetRequestedFamilies()) {
            priorities.emplace_back(count, 1.f);

            VkDeviceQueueCreateInfo queue_info{
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                nullptr, 0,
                family, count,
                std::data(priorities.back())
            };

            queue_infos.push_back(std::move(queue_info));
        }

        return queue_infos;
    }
}

namespace vulkan
{
    device::device(vulkan::instance &instance, VkSurfaceKHR surface)
    {
        auto constexpr use_extensions = !device_extensions.empty();

        std::vector<std::string_view> extensions_view;
        std::vector<char const *> extensions;

        if constexpr (use_extensions) {
            auto const _extensions = device_extensions;

            std::copy(_extensions.begin(), _extensions.end(), std::back_inserter(extensions));

        #if USE_DEBUG_MARKERS
            extensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
        #endif

            std::copy(std::begin(extensions), std::end(extensions), std::back_inserter(extensions_view));
        }

        graphics_queues_.resize(1);
        compute_queues_.resize(1);
        transfer_queues_.resize(1);
        presentation_queues_.resize(1);

        required_device_queues required_queues{
            graphics_queues_, compute_queues_, transfer_queues_, presentation_queues_
        };

        physical_handle_ = pick_physical_device(instance.handle(), surface, std::move(extensions_view), required_queues);

        std::vector<std::vector<float>> priorities;

        auto queue_infos = get_queue_infos(physical_handle_, surface, std::move(extensions), required_queues, priorities);

        auto const device_features = kDEVICE_FEATURES;

        VkDeviceCreateInfo const device_info{
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            nullptr, 0,
            static_cast<std::uint32_t>(std::size(queue_infos)), std::data(queue_infos),
            0, nullptr,
            static_cast<std::uint32_t>(std::size(extensions)), std::data(extensions),
            &device_features
        };

        if (auto result = vkCreateDevice(physical_handle_, &device_info, nullptr, &handle_); result != VK_SUCCESS)
            throw std::runtime_error("failed to create logical device: "s + std::to_string(result));

        for (auto &&queue : compute_queues_)
            vkGetDeviceQueue(handle_, queue.family_, queue.index_, &queue.handle_);

        for (auto &&queue : graphics_queues_)
            vkGetDeviceQueue(handle_, queue.family_, queue.index_, &queue.handle_);

        for (auto &&queue : transfer_queues_)
            vkGetDeviceQueue(handle_, queue.family_, queue.index_, &queue.handle_);

        for (auto &&queue : presentation_queues_)
            vkGetDeviceQueue(handle_, queue.family_, queue.index_, &queue.handle_);

        vkGetPhysicalDeviceProperties(physical_handle_, &properties_);

        auto samples_count_bits = std::min(properties_.limits.framebufferColorSampleCounts, properties_.limits.framebufferDepthSampleCounts);
        samples_count_ = static_cast<std::uint32_t>(std::pow(2, std::floor(std::log2(samples_count_bits))));

        memory_manager_ = std::make_unique<MemoryManager>(*this, properties_.limits.bufferImageGranularity);
        resource_manager_ = std::make_unique<ResourceManager>(*this);
    }

    device::~device()
    {
        if (resource_manager_)
            resource_manager_.reset();

        if (memory_manager_)
            memory_manager_.reset();

        if (handle_) {
            vkDeviceWaitIdle(handle_);
            vkDestroyDevice(handle_, nullptr);
        }

        handle_ = nullptr;
        physical_handle_ = nullptr;
    }
}