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
        /*
            glm::vec3 x{(1.f + glm::vec3{vec}) * 32767.5f - glm::vec3{32768.f}};
            glm::vec<2, std::int32_t> d{glm::round(x)};*/
        vec = glm::vec3{oct[0], oct[1], 0};
        vec += glm::vec3{32768.f};
        vec /= glm::vec3{32767.5f};
        vec -= 1.f;
        vec.z = 1.f - std::abs(vec.x) - std::abs(vec.y);

        float tt = std::max(-vec.z, 0.f);
        vec.x += (vec.x > 0.) ? -tt : tt;
        vec.y += (vec.y > 0.) ? -tt : tt;

#if 0
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
#endif

        vec = glm::normalize(vec);
    }

    inline glm::vec2 msign(glm::vec2 v)
    {
        return glm::vec2((v.x >= 0.0) ? 1.0 : -1.0, (v.y >= 0.0) ? 1.0 : -1.0);
    }

    template<class T, class V>
    requires (std::same_as<std::remove_cvref_t<V>, glm::vec3> && mpl::one_of<T, std::int8_t, std::int16_t>)
    void encode_unit_vector_to_oct_fast(std::span<T, 2> oct, V &&vec)
    {
        //std::cout << "v0 " << vec[0] << ' ' << vec[1] << ' ' << vec[2] << std::endl;
        //vec = glm::vec3{ 0, 0, -1 };
        vec /= glm::l1Norm(vec);
        //std::cout << "v1 " << vec[0] << ' ' << vec[1] << ' ' << vec[2] << std::endl;

        bool const is_hemisphere_bottom = vec.z < 0.f;

        if (is_hemisphere_bottom) {
            //glm::vec2 sign = 1.f - 2.f * glm::vec2{glm::lessThan(glm::vec2{vec}, glm::vec2{0})};
            vec.xy = (glm::vec2{1.f} - glm::abs(glm::vec2{vec.y, vec.x})) * msign(glm::vec2{vec});
        }

        //vec.xy = glm::clamp(glm::vec2{vec}, -1.f, 1.f);
        //std::cout << "is_hemisphere_bottom " << std::boolalpha << is_hemisphere_bottom << "v2 " << vec[0] << ' ' << vec[1] << ' ' << vec[2] << std::endl;

        if constexpr (std::is_same_v<T, std::int8_t>) {
            glm::vec<2, std::int16_t> d{glm::round((1.f + vec.xy()) * 127.5f)};
            d -= glm::vec<2, std::int16_t>{128};

            assert(d.x <= std::numeric_limits<T>::max() && d.x >= std::numeric_limits<T>::min());
            assert(d.y <= std::numeric_limits<T>::max() && d.x >= std::numeric_limits<T>::min());

            oct[0] = static_cast<T>(d.x);
            oct[1] = static_cast<T>(d.y);
        }

        else if constexpr (std::is_same_v<T, std::int16_t>) {
            auto a = (1.f + glm::vec2{vec});
            auto b = a * 32767.5f;
            glm::vec2 x{b - glm::vec2{32768.f}};
            glm::vec<2, std::int32_t> d{glm::round(x)};

            assert(d.x <= std::numeric_limits<T>::max() && d.x >= std::numeric_limits<T>::min());
            assert(d.y <= std::numeric_limits<T>::max() && d.x >= std::numeric_limits<T>::min());

            oct[0] = static_cast<T>(d.x);
            oct[1] = static_cast<T>(d.y);

            glm::vec3 decoded;
            decode_oct_to_vec(std::span{oct}, decoded);
        }

        /*float error = 0.f;
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
        }*/

        /*oct[0] = static_cast<T>(std::round(vec.x * std::numeric_limits<T>::max()));
        oct[1] = static_cast<T>(std::round(vec.y * std::numeric_limits<T>::max()));*/
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
        vec /= glm::l1Norm(vec);

        bool const is_hemisphere_bottom = vec.z <= 0.f;

        if (is_hemisphere_bottom) {
            //glm::vec2 sign = 1.f - 2.f * glm::vec2{glm::lessThan(glm::vec2{vec}, glm::vec2{0})};
            vec.xy = (1.f - glm::abs(vec.yx())) * glm::sign(vec.xy());
        }

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