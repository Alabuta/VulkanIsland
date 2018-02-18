#pragma once

#include <utility>
#include <numeric>

template<class C, class = void>
struct is_iterable : std::false_type {};

template<class C>
struct is_iterable<C, std::void_t<decltype(std::cbegin(std::declval<C>()), std::cend(std::declval<C>()))>> : std::true_type {};

template<class T>
constexpr bool is_iterable_v = is_iterable<T>::value;

template<class C, class = void>
struct is_container : std::false_type {};

template<class C>
struct is_container<C, std::void_t<decltype(std::size(std::declval<C>()), std::data(std::declval<C>()))>> : std::true_type {};

template<class T>
constexpr bool is_container_v = is_iterable<T>::value;

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
