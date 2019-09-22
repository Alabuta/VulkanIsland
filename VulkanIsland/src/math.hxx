#pragma once

#ifdef max
    #undef max
#endif

#ifdef min
    #undef min
#endif

#include <array>
#include <iomanip>

#define GLM_FORCE_CXX17
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/polar_coordinates.hpp> 
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>

#include "utility/mpl.hxx"


namespace math {
template<std::size_t N, class T>
struct vec {
    static auto constexpr size = N;
    using value_type = T;

    std::array<T, N> array;

    vec() = default;

    template<class... Ts, typename = std::enable_if_t<std::conjunction_v<std::is_arithmetic<Ts>...> && sizeof...(Ts) == size>>
    constexpr vec(Ts... values) noexcept : array{static_cast<typename std::remove_cvref_t<decltype(array)>::value_type>(values)...} { }
};

template<class U, class V>
inline glm::quat from_two_vec3(U &&u, V &&v)
{
    auto norm_uv = std::sqrt(glm::dot(u, u) * glm::dot(v, v));
    auto real_part = norm_uv + glm::dot(u, v);

    glm::vec3 w{0};

    if (real_part < 1.e-6f * norm_uv) {
        real_part = 0.f;

        w = std::abs(u.x) > std::abs(u.z) ? glm::vec3{-u.y, u.x, 0} : glm::vec3{0, -u.z, u.y};
    }

    else w = glm::cross(u, v);

    return glm::normalize(glm::quat{real_part, w});
}

glm::mat4 reversedPerspective(float yFOV, float aspect, float znear, float zfar);
}


template<class T> requires std::same_as<std::remove_cvref_t<T>, glm::mat4>
inline std::ostream &operator<< (std::ostream &stream, T &&m)
{
    stream << std::setprecision(4) << std::fixed;

    stream << m[0][0] << ' ' << m[0][1] << ' ' << m[0][2] << ' ' << m[0][3] << '\n';
    stream << m[1][0] << ' ' << m[1][1] << ' ' << m[1][2] << ' ' << m[1][3] << '\n';
    stream << m[2][0] << ' ' << m[2][1] << ' ' << m[2][2] << ' ' << m[2][3] << '\n';
    stream << m[3][0] << ' ' << m[3][1] << ' ' << m[3][2] << ' ' << m[3][3];

    return stream;
}
