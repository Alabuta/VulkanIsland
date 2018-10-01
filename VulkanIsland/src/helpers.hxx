#pragma once

#include <utility>
#include <numeric>
#include <variant>
#include <tuple>
#include <cmath>
#include <type_traits>
#include <chrono>

template<class C, class = void>
struct is_iterable : std::false_type {};

template<class C>
struct is_iterable<C, std::void_t<decltype(std::cbegin(std::declval<C>()), std::cend(std::declval<C>()))>> : std::true_type {};

template<class C>
constexpr bool is_iterable_v = is_iterable<C>::value;

template<class C, class = void>
struct is_container : std::false_type {};

template<class C>
struct is_container<C, std::void_t<decltype(std::size(std::declval<C>()), std::data(std::declval<C>()))>> : std::true_type {};

template<class C>
constexpr bool is_container_v = is_container<C>::value;

template<class T> struct always_false : std::false_type {};

template<class T, class... Ts>
struct are_same {
    static auto constexpr value_type = std::conjunction_v<std::is_same<T, std::decay_t<Ts>>...>;
};

template<class T, class... Ts>
inline auto constexpr are_same_v = are_same<T, Ts...>::value_type;

template<class T, class... Ts>
struct is_one_of {
    static auto constexpr value = std::disjunction_v<std::is_same<T, Ts>...>;
};

template<class T, class... Ts>
auto constexpr is_one_of_v = is_one_of<T, Ts...>::value;

//template<class T, typename std::enable_if_t<std::is_integral_v<std::decay_t<T>>>...>
constexpr std::uint16_t operator"" _ui16(unsigned long long value)
{
    return static_cast<std::uint16_t>(value);
}

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
struct instances_number {
    using type = T;
    static auto constexpr number = N;
};

template<class T, class Tuple, std::size_t I = 0>
constexpr std::size_t get_type_instances_number()
{
    using E = std::tuple_element_t<I, Tuple>;

    if constexpr (std::is_same_v<T, typename E::type>)
        return E::number;

    else if constexpr (I + 1 < std::tuple_size_v<Tuple>)
        return get_type_instances_number<T, Tuple, I + 1>();

    else return 0;
}

template<class V>
struct wrap_variant_by_vector;

template<class... Ts>
struct wrap_variant_by_vector<std::variant<Ts...>> {
    using type = std::variant<std::vector<Ts>...>;
};


// A function execution duration measurement.
template<typename TimeT = std::chrono::milliseconds>
struct measure {
    template<typename F, typename... Args>
    static auto execution(F func, Args &&... args)
    {
        auto start = std::chrono::system_clock::now();

        func(std::forward<Args>(args)...);

        auto duration = std::chrono::duration_cast<TimeT>(std::chrono::system_clock::now() - start);

        return duration.count();
    }
};
