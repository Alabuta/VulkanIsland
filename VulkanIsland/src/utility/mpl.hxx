#pragma once

#include <type_traits>
#include <utility>
#include <numeric>
#include <cstddef>
#include <variant>
#include <vector>
#include <tuple>

#if defined(_MSC_VER)
    #include <concepts>
#endif


namespace mpl
{
    template<class C, class = void>
    struct is_iterable : std::false_type{ };

    template<class C>
    struct is_iterable<C, std::void_t<decltype(std::cbegin(std::declval<C>()), std::cend(std::declval<C>()))>> : std::true_type{ };

    template<class C>
    constexpr bool is_iterable_v = is_iterable<C>::value;

    template<class C, class = void>
    struct is_container : std::false_type{ };

    template<class C>
    struct is_container<C, std::void_t<
        decltype(std::size(std::declval<C>()), std::data(std::declval<C>())), typename C::value_type
    >> : std::true_type{ };

    template<class C>
    constexpr bool is_container_v = is_container<C>::value;

    template<class T> struct always_false : std::false_type{ };

    template<class T, class... Ts>
    struct are_same {
        static auto constexpr value_type = std::conjunction_v<std::is_same<T, Ts>...>;
    };

    template<class T, class... Ts>
    inline auto constexpr are_same_v = are_same<T, Ts...>::value_type;

    template<class T, class... Ts>
    struct is_one_of {
        static auto constexpr value = std::disjunction_v<std::is_same<T, Ts>...>;
    };

    template<class T, class... Ts>
    auto constexpr is_one_of_v = is_one_of<T, Ts...>::value;

    template<std::size_t i = 0, class T, class V>
    constexpr void set_tuple(T &&tuple, V value)
    {
        std::get<i>(tuple) = value;

        if constexpr (i + 1 < std::tuple_size_v<std::remove_cvref_t<T>>)
            set_tuple<i + 1>(std::forward<T>(tuple), value);
    }

    namespace detail
    {
        template<class T, class... Ts> struct return_type_helper { using type = T; };
        template<class... Ts> struct return_type_helper<void, Ts...> : std::common_type<Ts...>{ };

        template<class T, class... Ts>
        using return_type_helper_t = typename return_type_helper<T, Ts...>::type;
    }

    template<class T = void, class... Ts>
    constexpr std::array<detail::return_type_helper_t<T, Ts...>, sizeof...(Ts)> make_array(Ts &&...t)
    {
        return {{ std::forward<Ts>(t)... }};
    }

    namespace detail
    {
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

    template<class T, class V>
    struct is_variant_alternative;

    template<class T, class... Ts>
    struct is_variant_alternative<T, std::variant<Ts...>> {
        static auto constexpr value = is_one_of_v<T, Ts...>;
    };

    template<class T, class... Ts>
    auto constexpr is_variant_alternative_v = is_variant_alternative<T, Ts...>::value;

    template<class T>
    struct variant_from_tuple;

    template<class... Ts>
    struct variant_from_tuple<std::tuple<Ts...>> {
        using type = std::variant<Ts...>;
    };

    template<class T>
    concept integral = std::is_integral_v<T>;

    template<class T>
    concept iterable = is_iterable_v<T>;

    template<class T>
    concept container = is_container_v<T>;

    template<class T, class... Ts>
    concept one_of = is_one_of_v<T, Ts...>;

    template<class T1, class T2>
    concept same_as = are_same_v<T1, T2>;

    template<class T, class... Ts>
    concept all_same = are_same_v<T, Ts...>;

    template<class T, class U>
    concept derived_from = std::is_base_of_v<T, U>;

    template<class T>
    concept enumeration = std::is_enum_v<T>;

    template<class T>
    concept arithmetic = std::is_arithmetic_v<T>;

    template<class... Ts, std::size_t N>
    concept length_equal_to = sizeof...(Ts) == N;

    template<class T, class... Ts>
    concept variant_alternative = is_variant_alternative_v<T, Ts...>;
}
