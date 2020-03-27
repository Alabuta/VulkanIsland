#pragma once

#include <type_traits>
#include <iterator>
#include <cstdint>
#include <utility>
#include <cstddef>
#include <cmath>
#include <chrono>


template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

//template<class T, typename std::enable_if_t<std::is_integral_v<std::remove_cvref_t<T>>>* = nullptr>
constexpr std::uint16_t operator"" _ui16(unsigned long long value)
{
    return static_cast<std::uint16_t>(value);
}

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


[[nodiscard]] std::size_t constexpr aligned_size(std::size_t size, std::size_t alignment) noexcept
{
    if (alignment > 0)
        return (size + alignment - 1) & ~(alignment - 1);

    return size;
}

template<class T>
class strided_forward_iterator {
public:

    using difference_type = std::ptrdiff_t;
    using value_type = std::remove_cv_t<T>;
    using pointer = T *;
    using reference = T &;
    using iterator_category = std::forward_iterator_tag;

    strided_forward_iterator() noexcept = default;
    ~strided_forward_iterator() = default;

    strided_forward_iterator(pointer data, std::size_t stride) noexcept : stride{stride}, position{reinterpret_cast<std::byte *>(data)} { };
    strided_forward_iterator(strided_forward_iterator<T> const &) noexcept = default;

    strided_forward_iterator<T> &operator++ () noexcept
    {
        position += stride;

        return *this;
    }

    strided_forward_iterator<T> operator++ (int) noexcept
    {
        auto copy = *this;

        position += stride;
        
        return copy;
    }

    reference operator* () noexcept { return *reinterpret_cast<pointer>(position); }
    pointer operator-> () noexcept { return reinterpret_cast<pointer>(position); }

    bool operator== (strided_forward_iterator<T> const &rhs) const noexcept
    {
        return stride == rhs.stride && position == rhs.position;
    }

    bool operator!= (strided_forward_iterator<T> const &rhs) const noexcept
    {
        return !(*this == rhs);
    }

    bool operator< (strided_forward_iterator<T> const &rhs) const
    {
        return position < rhs.position;
    }

protected:

    std::size_t stride{sizeof(T)};
    std::byte *position{nullptr};
};

template<class T>
class strided_bidirectional_iterator : public strided_forward_iterator<T> {
public:

    using iterator_category = std::bidirectional_iterator_tag;

    using strided_forward_iterator<T>::strided_forward_iterator;

    strided_bidirectional_iterator<T> &operator-- () noexcept
    {
        strided_forward_iterator<T>::position -= strided_forward_iterator<T>::stride;

        return *this;
    }

    strided_bidirectional_iterator<T> operator-- (int) noexcept
    {
        auto copy = *this;

        strided_forward_iterator<T>::position -= strided_forward_iterator<T>::stride;

        return copy;
    }
};
