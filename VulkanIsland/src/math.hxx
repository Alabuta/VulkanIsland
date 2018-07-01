#pragma once

#include <array>
#include "helpers.hxx"

struct vec2 {
    std::array<float, 2> xy;

    vec2() = default;

    template<class T, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, std::array<float, 2>>>...>
    constexpr vec2(T &&xy) noexcept : xy(std::forward<T>(xy)) {}
    constexpr vec2(float x, float y) noexcept : xy({ x, y }) {}
};

struct vec3 {
    std::array<float, 3> xyz;

    vec3() = default;

    template<class T, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, std::array<float, 3>>>...>
    constexpr vec3(T &&xyz) noexcept : xyz(std::forward<T>(xyz)) {}
    constexpr vec3(float x, float y, float z = 0) noexcept : xyz({ x, y, z }) {}

    float length() const
    {
        return std::sqrt(xyz[0] * xyz[0] + xyz[1] * xyz[1] + xyz[2] * xyz[2]);
    }

    vec3 const &normalize()
    {
        if (auto const length = this->length(); std::abs(length) > std::numeric_limits<float>::min())
            for (auto &v : xyz) v /= length;

        return *this;
    }

    template<class T, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, vec3>>...>
    vec3 operator+ (T &&rhs) const
    {
        return { xyz[0] + rhs.xyz[0], xyz[1] + rhs.xyz[1], xyz[2] + rhs.xyz[2] };
    }

    template<class T, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, vec3>>...>
    vec3 operator- (T &&rhs) const
    {
        return { xyz[0] - rhs.xyz[0], xyz[1] - rhs.xyz[1], xyz[2] - rhs.xyz[2] };
    }

    vec3 &operator+ () { return *this; };
    vec3 operator- () const { return { -xyz[0], -xyz[1], -xyz[2] }; };

    template<class T, typename std::enable_if_t<std::is_arithmetic_v<std::decay_t<T>>>...>
    vec3 operator* (T scalar) const
    {
        return { xyz[0] * scalar, xyz[1] * scalar, xyz[2] * scalar };
    }

    template<class T, typename std::enable_if_t<std::is_arithmetic_v<std::decay_t<T>>>...>
    vec3 operator/ (T scalar) const
    {
        return { xyz[0] / scalar, xyz[1] / scalar, xyz[2] / scalar };
    }

    template<class T, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, vec3>>...>
    vec3 cross(T &&rhs) const
    {
        return {
            xyz[1] * rhs.xyz[2] - xyz[2] * rhs.xyz[1],
            xyz[2] * rhs.xyz[0] - xyz[0] * rhs.xyz[2],
            xyz[0] * rhs.xyz[1] - xyz[1] * rhs.xyz[0]
        };
    }

    template<class T, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, vec3>>...>
    float dot(T &&rhs) const
    {
        return xyz[0] * rhs.xyz[0] + xyz[1] * rhs.xyz[1] + xyz[2] * rhs.xyz[2];
    }
};

struct vec4 {
    std::array<float, 4> xyzw;

    vec4() = default;

    template<class T, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, std::array<float, 4>>>...>
    constexpr vec4(T &&xyzw) noexcept : xyzw(std::forward<T>(xyzw)) { }
    constexpr vec4(float x, float y, float z, float w) noexcept : xyzw({x, y, z, w}) { }
};

struct mat4 {
    std::array<float, 16> m;

    mat4() = default;

    template<class T, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, std::decay_t<decltype(m)>>>...>
    constexpr mat4(T &&array) noexcept : m(std::forward<T>(array)) { }

    template<class T0, class T1, class T2, class T3, typename std::enable_if_t<are_same_v<vec3, T0, T1, T2, T3>>* = 0>
    constexpr mat4(T0 &&xAxis, T1 &&yAxis, T2 &&zAxis, T3 &&translation)
    {
        std::fill(std::begin(m), std::end(m), 0.f);
        m.back() = 1.f;

        auto tr = -std::forward<T3>(translation);

        std::uninitialized_copy_n(std::begin(xAxis.xyz), std::size(xAxis.xyz), std::begin(m));
        std::uninitialized_copy_n(std::begin(yAxis.xyz), std::size(yAxis.xyz), std::begin(m) + 4);
        std::uninitialized_copy_n(std::begin(zAxis.xyz), std::size(zAxis.xyz), std::begin(m) + 4 * 2);
        std::uninitialized_copy_n(std::begin(tr.xyz), std::size(tr.xyz), std::begin(m) + 4 * 3);
    }

    template<class... Ts, typename std::enable_if_t<std::conjunction_v<std::is_arithmetic<Ts>...> && sizeof...(Ts) == 16, int> = 0>
    constexpr mat4(Ts... values) noexcept : m({{ static_cast<std::decay_t<decltype(m)>::value_type>(values)... }}) { }
};

template<class T, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, vec3>>...>
mat4 lookAt(T &&eye, T &&center, T &&up)
{
    auto zAxis = -(center-eye);

    if (std::abs(zAxis.xyz.at(0)) < std::numeric_limits<float>::min()&&std::abs(zAxis.xyz.at(2)) < std::numeric_limits<float>::min())
        zAxis.xyz.at(2) += std::numeric_limits<float>::min();

    zAxis.normalize();

    auto const xAxis = up.cross(zAxis).normalize();
    auto const yAxis = zAxis.cross(xAxis).normalize();

    auto const position = vec3{xAxis.dot(eye), yAxis.dot(eye), zAxis.dot(eye)};

    return mat4(xAxis, yAxis, zAxis, position);
}

struct quat {
    std::array<float, 4> xyzw;

    quat() = default;

    template<class T, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, std::array<float, 4>>>...>
    constexpr quat(T &&xyzw) : xyzw(std::forward<T>(xyzw)) { }
    constexpr quat(float x, float y, float z, float w) : xyzw({x, y, z, w}) { }
};


struct Vertex {
    vec3 pos;
    vec3 normal;
    vec2 uv;

    Vertex() = default;

    template<class P, class N, class UV, typename std::enable_if_t<are_same_v<vec3, std::decay_t<P>, std::decay_t<N>> && std::is_same_v<vec2, std::decay_t<UV>>>...>
    constexpr Vertex(P &&_position, N &&_normal, UV &&_uv)
    {
        pos = std::forward<P>(_position);
        normal = std::forward<N>(_normal);
        uv = std::forward<UV>(_uv);
    }

    template<class P, class N, class UV,
        typename std::enable_if_t<are_same_v<std::array<float, 3>, std::decay_t<P>, std::decay_t<N>> && std::is_same_v<std::array<float, 2>, std::decay_t<UV>>>...>
    constexpr Vertex(P &&_position, N &&_normal, UV &&_uv)
    {
        pos = vec3{std::forward<P>(_position)};
        normal = vec3{std::forward<N>(_normal)};
        uv = vec2{std::forward<UV>(_uv)};
    }

    template<class T, typename std::enable_if_t<std::is_same_v<Vertex, std::decay_t<T>>>...>
    constexpr bool operator== (T &&rhs) const
    {
        return pos == rhs.pos && normal == rhs.normal && uv == rhs.uv;
    }
};

#ifndef _MSC_VER
#define GLM_FORCE_CXX17
#endif

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#ifdef _MSC_VER
#pragma warning(push, 3)
#pragma warning(disable: 4201)
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif