#pragma once

#include <utility>
#include <numeric>
#include <type_traits>

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
constexpr bool is_container_v = is_container<T>::value;

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
    return {{ std::forward<Ts>(t)... }};
}

namespace detail {
template <class T, std::size_t N, std::size_t... I>
constexpr std::array<std::remove_cv_t<T>, N> to_array_impl(T (&a)[N], std::index_sequence<I...>)
{
    return {{ a[I]... }};
}
}

template <class T, std::size_t N>
constexpr std::array<std::remove_cv_t<T>, N> to_array(T (&a)[N])
{
    return detail::to_array_impl(a, std::make_index_sequence<N>{});
}

template<class T, std::size_t N = 1>
struct type_instances_number {
    using type = T;
    static auto constexpr number = N;
};


struct vec2 {
    float xyz[2];

    /*template<class T, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, std::array<float, 3>>>...>
    constexpr vec3(T &&xyz) : xyz(std::forward<T>(xyz)) {}*/

    vec2(float x, float y)
    {
        xyz[0] = x;
        xyz[1] = y;
    }
};

struct vec3 {
    float xyz[3];

    /*template<class T, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, std::array<float, 3>>>...>
    constexpr vec3(T &&xyz) : xyz(std::forward<T>(xyz)) {}*/

    vec3(float x, float y, float z) {
        xyz[0] = x;
        xyz[1] = y;
        xyz[2] = z;
    }
};

struct Vertex {
    vec3 pos;
    vec3 color;
};