#pragma once

#include <cstdint>
#include <numeric>
#include <array>

#include "utility/mpl.hxx"

#include "math.hxx"


namespace math
{
    template<class O, class... Ts>// requires (sizeof...(Ts) == 3 && mpl::all_arithmetic<Ts>)
    std::array<O, 2> encode_unit_vector_to_oct_fast(Ts... values)
    {
        glm::vec3 vec{static_cast<float>(values)...};

        bool const is_bottom_hemisphere = vec.z < 0.f;

        glm::vec2 const inv_l1_norm = glm::vec2{1.f / (static_cast<float>(std::abs(values)) + ...)};

        if (is_bottom_hemisphere) {
            vec.xy = (1.f - glm::abs(glm::vec2{vec.yx} * inv_l1_norm)) * (1.f - 2.f * glm::vec2{glm::lessThan(glm::vec2{vec}, glm::vec2{0})});
        }

        else {
            ;
        }

        //m_bits = (T)round(clamp(f, -1.0f, 1.0f) * ((uint64_t(1) << (bitcount - 1)) - 1));

        return std::array<O, 2>{};
    }
}