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

// template<class T>
// requires std::is_integral_v<std::remove_cvref_t<T>>
constexpr std::uint16_t operator"" _ui16(unsigned long long value)
{
    return static_cast<std::uint16_t>(value);
}

// A function execution duration measurement.
template<typename TimeT = std::chrono::milliseconds>
struct measure {
    template<class F, class... Args>
    static auto execution(F func, Args &&... args)
    {
        auto const start = std::chrono::system_clock::now();

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

    strided_forward_iterator(pointer data, std::size_t stride) noexcept : stride_{stride}, position_{reinterpret_cast<std::byte *>(data)} { }

    strided_forward_iterator<T> &operator++ () noexcept
    {
        position_ += stride_;

        return *this;
    }

    strided_forward_iterator<T> operator++ (int) noexcept
    {
        auto copy = *this;

        position_ += stride_;
        
        return copy;
    }

    reference operator* () noexcept { return *reinterpret_cast<pointer>(position_); }
    pointer operator-> () noexcept { return reinterpret_cast<pointer>(position_); }

    bool operator== (strided_forward_iterator<T> const &rhs) const noexcept
    {
        return stride_ == rhs.stride_ && position_ == rhs.position_;
    }

    bool operator!= (strided_forward_iterator<T> const &rhs) const noexcept
    {
        return !(*this == rhs);
    }

    bool operator< (strided_forward_iterator<T> const &rhs) const
    {
        return position_ < rhs.position_;
    }

protected:

    std::size_t stride_{sizeof(T)};
    std::byte *position_{nullptr};
};

template<class T>
class strided_bidirectional_iterator : public strided_forward_iterator<T> {
public:

    using iterator_category = std::bidirectional_iterator_tag;

    using strided_forward_iterator<T>::strided_forward_iterator;

    strided_bidirectional_iterator<T> &operator-- () noexcept
    {
        strided_forward_iterator<T>::position_ -= strided_forward_iterator<T>::stride_;

        return *this;
    }

    strided_bidirectional_iterator<T> operator-- (int) noexcept
    {
        auto copy = *this;

        strided_forward_iterator<T>::position_ -= strided_forward_iterator<T>::stride_;

        return copy;
    }
};
