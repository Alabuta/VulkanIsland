#pragma once

#include <algorithm>
#include <iostream>
#include <cstdint>
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
        auto constexpr type_max = static_cast<float>(std::numeric_limits<T>::max());

        vec = glm::vec3{oct[0], oct[1], 0};
        vec.xy() += type_max + 1.f;
        vec.xy() /= type_max - .5f;
        vec.xy() -= 1.f;
        vec.z = 1.f - std::abs(vec.x) - std::abs(vec.y);

        auto tt = std::max(-vec.z, 0.f);
        vec.x += (vec.x > 0.f) ? -tt : tt;
        vec.y += (vec.y > 0.f) ? -tt : tt;

        vec = glm::normalize(vec);
    }

    template<class T, class V>
    requires (std::same_as<std::remove_cvref_t<V>, glm::vec3> && mpl::one_of<T, std::int8_t, std::int16_t>)
    void encode_unit_vector_to_oct_fast(std::span<T, 2> oct, V &&vec)
    {
        vec.xy = vec.xy() / glm::l1Norm(vec);

        auto const is_hemisphere_bottom = vec.z < 0.f;

        if (is_hemisphere_bottom) {
            auto sign = 1.f - 2.f * glm::vec2{glm::lessThan(vec.xy(), glm::vec2{0})};
            vec.xy = (1.f - glm::abs(vec.yx())) * sign;
        }

        auto constexpr type_max = static_cast<float>(std::numeric_limits<T>::max());

        glm::vec<2, std::int32_t> d{glm::round((1.f + glm::vec2{vec}) * (.5f + type_max) - (type_max + 1))};

        assert(d.x <= std::numeric_limits<T>::max() && d.x >= std::numeric_limits<T>::min());
        assert(d.y <= std::numeric_limits<T>::max() && d.x >= std::numeric_limits<T>::min());

        oct[0] = static_cast<T>(d.x);
        oct[1] = static_cast<T>(d.y);
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
        encode_unit_vector_to_oct_fast(oct, vec);

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
