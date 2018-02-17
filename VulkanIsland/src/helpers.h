#pragma once

#include <utility>
#include <numeric>

template<class C, class = void>
struct is_iterable : std::false_type {};

template<class C>
struct is_iterable<C, std::void_t<decltype(std::cbegin(std::declval<C>()), std::cend(std::declval<C>()))>> : std::true_type {};

template<class T>
constexpr bool is_iterable_v = is_iterable<T>::value;

template<std::size_t i = 0, typename T, typename V>
constexpr void set_tuple(T &&tuple, V value)
{
    std::get<i>(tuple) = value;

    if constexpr (i + 1 < std::tuple_size_v<std::decay_t<T>>)
        set_tuple<i + 1>(std::forward<T>(tuple), value);
}


template<class... Ts>
constexpr std::array<std::decay_t<std::tuple_element_t<0, std::tuple<Ts...>>>, sizeof...(Ts)> make_array(Ts &&...t)
{
    return {std::forward<Ts>(t)...};
}


template<class T, class U, typename std::enable_if_t<is_iterable_v<std::decay_t<T>> && std::is_same_v<std::decay_t<U>, VkQueueFamilyProperties>>...>
[[nodiscard]] std::optional<std::uint32_t> GetRequiredQueueFamilyIndex(T &&queueFamilies, U &&requiredQueue)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkQueueFamilyProperties>, "iterable object does not contain VkQueueFamilyProperties elements");

    // Strict matching.
    auto it_family = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [&requiredQueue] (auto &&queueFamily)
    {
        return queueFamily.queueCount > 0 && queueFamily.queueFlags == requiredQueue.queueFlags;
    });

    if (it_family != queueFamilies.cend())
        return static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_family));

    // Tolerant matching.
    it_family = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [&requiredQueue] (auto &&queueFamily)
    {
        return queueFamily.queueCount > 0 && (queueFamily.queueFlags & requiredQueue.queueFlags) == requiredQueue.queueFlags;
    });

    if (it_family != queueFamilies.cend())
        return static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_family));

    return {};
}

template<class T,typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
[[nodiscard]] std::optional<std::uint32_t> GetPresentationQueueFamilyIndex(T &&queueFamilies, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, VkQueueFamilyProperties>, "iterable object does not contain VkQueueFamilyProperties elements");

    auto it_presentationQueue = std::find_if(queueFamilies.cbegin(), queueFamilies.cend(), [physicalDevice, surface, size = queueFamilies.size()](auto queueFamily)
    {
        std::vector<std::uint32_t> queueFamiliesIndices(size);
        std::iota(queueFamiliesIndices.begin(), queueFamiliesIndices.end(), 0);

        return std::find_if(queueFamiliesIndices.crbegin(), queueFamiliesIndices.crend(), [physicalDevice, surface] (auto queueIndex)
        {
            VkBool32 surfaceSupported = 0;
            if (auto result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueIndex, surface, &surfaceSupported); result != VK_SUCCESS)
                throw std::runtime_error("failed to retrieve surface support: "s + std::to_string(result));

            return surfaceSupported != VK_TRUE;

        }) != queueFamiliesIndices.crend();
    });

    if (it_presentationQueue != queueFamilies.cend())
        return static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_presentationQueue));

    return {};
}