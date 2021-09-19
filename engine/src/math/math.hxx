#pragma once

#ifdef max
    #undef max
#endif

#ifdef min
    #undef min
#endif

#include <array>
#include <iomanip>
#include <numbers>

#define GLM_FORCE_CXX17
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_SWIZZLE
#define GLM_GTX_norm

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/polar_coordinates.hpp> 
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/norm.hpp>

#include "utility/mpl.hxx"


namespace math
{
    template<std::size_t N, class T>
    struct vec final {
        static auto constexpr size = N;
        using value_type = T;

        std::array<T, N> array;

        vec() = default;

        template<mpl::arithmetic... Ts> requires (sizeof...(Ts) == size)
        constexpr vec(Ts... values) noexcept : array{static_cast<T>(values)...} { }
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

    glm::mat4 reversed_perspective(float vertical_fov, float aspect, float znear, float zfar);
}
