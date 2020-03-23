#pragma once

#include <cstdint>
#include <numeric>
#include <limits>
#include <array>

#include "utility/mpl.hxx"

#include "math.hxx"


namespace math
{
    template<class O, class V>
    requires (mpl::one_of<O, std::int8_t, std::int16_t> && mpl::same_as<std::remove_cvref_t<V>, glm::vec3>)
    void encode_unit_vector_to_oct_fast(O (&oct)[2], V &&vec)
    {
        float const inv_l1_norm = 1.f / glm::l1Norm(vec);

        bool const is_bottom_hemisphere = vec.z < 0.f;

        if (is_bottom_hemisphere) {
            glm::vec2 sign = 1.f - 2.f * glm::vec2{glm::lessThan(glm::vec2{vec}, glm::vec2{0})};
            vec.xy = (1.f - glm::abs(vec.yx * inv_l1_norm)) * sign;
        }

        else vec.xy = vec.xy * inv_l1_norm;

        vec.xy = glm::clamp(glm::vec2{vec}, -1.f, 1.f);

        oct[0] = static_cast<O>(std::round(vec.x * std::numeric_limits<O>::max()));
        oct[1] = static_cast<O>(std::round(vec.y * std::numeric_limits<O>::max()));
    }

    template<class O, class... Ts>
    requires (sizeof...(Ts) == 3 && mpl::all_arithmetic<Ts> && mpl::one_of<O, std::int8_t, std::int16_t>)
    void encode_unit_vector_to_oct_fast(O (&oct)[2], Ts... values)
    {
        encode_unit_vector_to_oct_fast(oct, glm::vec3{static_cast<float>(values)...});
    }
}