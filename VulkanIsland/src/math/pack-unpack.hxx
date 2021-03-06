#pragma once

#include <cstdint>
#include <algorithm>
#include <numeric>
#include <limits>
#include <array>
#include <span>

#include "utility/mpl.hxx"

#include "math.hxx"


namespace math
{
    template<class T, class V>
    requires (std::same_as<std::remove_cvref_t<V>, glm::vec3> && mpl::one_of<T, std::int8_t, std::int16_t>)
    void decode_oct_to_vec(std::span<T, 2> const oct, V &vec)
    {
        vec = glm::vec3{
            oct[0] / static_cast<float>(std::numeric_limits<T>::max()),
            oct[1] / static_cast<float>(std::numeric_limits<T>::max()),
            0
        };

        vec.xy() = glm::clamp(glm::vec2{vec}, -1.f, 1.f);
        vec.z = 1.f - std::abs(vec.x) - std::abs(vec.y);

        if (vec.z < 0.f) {
            glm::vec2 sign = 1.f - 2.f * glm::vec2{glm::lessThan(glm::vec2{vec}, glm::vec2{0})};

            vec.xy() = (1.f - glm::abs(glm::vec2{vec.yx()})) * sign;
        }

        vec = glm::normalize(vec);
    }

    template<class T, class V>
    requires (std::same_as<std::remove_cvref_t<V>, glm::vec3> && mpl::one_of<T, std::int8_t, std::int16_t>)
    void encode_unit_vector_to_oct_fast(std::span<T, 2> oct, V &&vec)
    {
        float const inv_l1_norm = 1.f / glm::l1Norm(vec);

        bool const is_hemisphere_bottom = vec.z < 0.f;

        if (is_hemisphere_bottom) {
            glm::vec2 sign = 1.f - 2.f * glm::vec2{glm::lessThan(glm::vec2{vec}, glm::vec2{0})};
            vec.xy = (1.f - glm::abs(vec.yx() * inv_l1_norm)) * sign;
        }

        else vec.xy = vec.xy * inv_l1_norm;

        vec.xy = glm::clamp(glm::vec2{vec}, -1.f, 1.f);

        oct[0] = static_cast<T>(std::round(vec.x * std::numeric_limits<T>::max()));
        oct[1] = static_cast<T>(std::round(vec.y * std::numeric_limits<T>::max()));
    }

    template<class T, mpl::arithmetic... Ts>
    requires (sizeof...(Ts) == 3 && mpl::one_of<T, std::int8_t, std::int16_t>)
    void encode_unit_vector_to_oct_fast(std::span<T, 2> oct, Ts... values)
    {
        encode_unit_vector_to_oct_fast(oct, glm::vec3{static_cast<float>(values)...});
    }

    template<class T, class V>
    requires (std::same_as<std::remove_cvref_t<V>, glm::vec3> && mpl::one_of<T, std::int8_t, std::int16_t>)
    void encode_unit_vector_to_oct_precise(std::span<T, 2> oct, V &&vec)
    {
        float const inv_l1_norm = 1.f / glm::l1Norm(vec);

        bool const is_hemisphere_bottom = vec.z <= 0.f;

        if (is_hemisphere_bottom) {
            glm::vec2 sign = 1.f - 2.f * glm::vec2{glm::lessThan(glm::vec2{vec}, glm::vec2{0})};
            vec.xy() = (1.f - glm::abs(vec.yx() * inv_l1_norm)) * sign;
        }

        else vec.xy() = vec.xy() * inv_l1_norm;

        vec.xy() = glm::clamp(glm::vec2{vec}, -1.f, 1.f);

        oct[0] = static_cast<T>(std::round(vec.x * std::numeric_limits<T>::max()));
        oct[1] = static_cast<T>(std::round(vec.y * std::numeric_limits<T>::max()));

        float error = 0.f;
        glm::vec3 decoded;

        std::array<T, 2> projected;

        for (auto index : {0, 1, 2, 3}) {
            projected[0] = static_cast<T>(oct[0] + (index / 2));
            projected[1] = static_cast<T>(oct[1] + (index % 2));

            decode_oct_to_vec(std::span{projected}, decoded);

            auto sq_distance = glm::distance2(vec, decoded);

            if (sq_distance > error) {
                oct[0] = projected[0];
                oct[1] = projected[1];

                error = sq_distance;
            }
        }
    }

    template<class T, mpl::arithmetic... Ts>
    requires (sizeof...(Ts) == 3 && mpl::one_of<T, std::int8_t, std::int16_t>)
    void encode_unit_vector_to_oct_precise(std::span<T, 2> oct, Ts... values)
    {
        encode_unit_vector_to_oct_precise(oct, glm::vec3{static_cast<float>(values)...});
    }
}