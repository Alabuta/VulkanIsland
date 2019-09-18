#pragma once

#include <map>

#include "main.hxx"
#include "utility/mpl.hxx"
#include "device/device.hxx"
#include "queues.hxx"
#include "renderer/graphics_api.hxx"


class QueueHelper final {
public:

    template<class Q> requires mpl::derived_from<VulkanQueue<Q>, Q>
    [[nodiscard]] Q Find(VkPhysicalDevice physicalDevice, [[maybe_unused]] VkSurfaceKHR surface)
    {
        Q queue;

        std::optional<std::pair<VkQueueFamilyProperties, std::uint32_t>> pair;

        if constexpr (std::is_same_v<Q, PresentationQueue>)
            pair = GetPresentationQueueFamily<Q>(physicalDevice, surface);

        else pair = GetQueueFamily<Q>(physicalDevice);

        if (!pair)
            throw std::runtime_error("failed to create queue"s);

        auto &&[properties, family] = pair.value();

        family_and_index.try_emplace(family, 0);

        if (family_and_index[family] >= properties.queueCount)
            throw std::runtime_error("the number of suitable queues is exceeded"s);

        queue.family_ = family;
        queue.index_ = std::clamp(0u, family_and_index.at(family), properties.queueCount - 1);

        ++family_and_index[family];

        return queue;
    }

    [[nodiscard]] std::vector<std::pair<std::uint32_t, std::uint32_t>> GetRequestedFamilies() const
    {
        std::vector<std::pair<std::uint32_t, std::uint32_t>> families{family_and_index.cbegin(), family_and_index.cend()};

        return families;
    }

    template<class Q> requires mpl::derived_from<VulkanQueue<Q>, Q>
    [[nodiscard]] static bool IsSupportedByDevice(VkPhysicalDevice physicalDevice, [[maybe_unused]] VkSurfaceKHR surface)
    {
        if constexpr (std::is_same_v<Q, PresentationQueue>)
            return GetPresentationQueueFamily<Q>(physicalDevice, surface).has_value();

        else return GetQueueFamily<Q>(physicalDevice).has_value();
    }

private:
    std::map<std::uint32_t, std::uint32_t> family_and_index;
    
    template<class Q> requires mpl::derived_from<VulkanQueue<Q>, Q>
    [[nodiscard]] static std::optional<std::pair<VkQueueFamilyProperties, std::uint32_t>>
    GetQueueFamily(VkPhysicalDevice physicalDevice)
    {
        std::uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, std::data(queueFamilies));

        if (queueFamilies.empty())
            return {};

#if TEMPORARILY_DISABLED
        // Strict matching.
        auto it_family = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [] (auto &&queueFamily)
        {
            return queueFamily.queueCount > 0 && queueFamily.queueFlags == Q::kFLAGS;
        });

        if (it_family != queueFamilies.cend())
            return static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_family));
#endif

        // Tolerant matching.
        auto it_family = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [] (auto &&queueFamily)
        {
            auto const capability = convert_to::vulkan(Q::kCAPABILITY);
            return queueFamily.queueCount > 0 && (queueFamily.queueFlags & capability) == capability;
        });

        if (it_family != queueFamilies.cend())
            return std::pair{*it_family, static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_family))};

        return {};
    }

    template<class Q> requires mpl::derived_from<VulkanQueue<Q>, Q>
    [[nodiscard]] static std::optional<std::pair<VkQueueFamilyProperties, std::uint32_t>>
    GetPresentationQueueFamily(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        std::uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, std::data(queueFamilies));

        if (queueFamilies.empty())
            return {};

        auto it_family = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [physicalDevice, surface, size = queueFamilies.size()](auto)
        {
            std::vector<std::uint32_t> queueFamiliesIndices(size);
            std::iota(queueFamiliesIndices.begin(), queueFamiliesIndices.end(), 0);

            return std::find_if(queueFamiliesIndices.cbegin(), queueFamiliesIndices.cend(), [physicalDevice, surface] (auto queueIndex)
            {
                VkBool32 surfaceSupported = VK_FALSE;
                if (auto result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueIndex, surface, &surfaceSupported); result != VK_SUCCESS)
                    throw std::runtime_error("failed to retrieve surface support: "s + std::to_string(result));

                return surfaceSupported == VK_TRUE;

            }) != queueFamiliesIndices.cend();
        });

        if (it_family != queueFamilies.cend())
            return std::pair{*it_family, static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_family))};

        return {};
    }
};

template<class... Qs>
struct QueuePool final {
    using Tuple = std::tuple<Qs...>;
};