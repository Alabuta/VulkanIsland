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
[[noreturn]] void set_tuple(T &&tuple, V value)
{
    std::get<i>(tuple) = value;

    if constexpr (i + 1 < std::tuple_size_v<std::decay_t<T>>)
        set_tuple<i + 1>(std::forward<T>(tuple), value);
}

template<class T, class U, typename std::enable_if_t<is_iterable_v<std::decay_t<T>> && std::is_same_v<std::decay_t<U>, VkQueueFamilyProperties>>...>
[[nodiscard]] std::optional<std::uint32_t> GetRequiredQueue(T &&queueFamilies, U &&requiredQueue)
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
        return queueFamily.queueCount > 0 && (queueFamily.queueFlags & requiredQueue.queueFlags) != 0;
    });

    if (it_family != queueFamilies.cend())
        return static_cast<std::uint32_t>(std::distance(queueFamilies.cbegin(), it_family));

    return {};
}