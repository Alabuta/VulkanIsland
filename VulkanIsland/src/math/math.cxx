#include "math.hxx"


namespace math
{
    glm::mat4 reversed_perspective(float vertical_fov, float aspect, float znear, float zfar)
    {
        auto const f = 1.f / std::tan(vertical_fov * .5f);

        auto const A = zfar / (zfar - znear) - 1.f;
        auto const B = zfar * znear / (zfar - znear);

        return glm::mat4{
            f / aspect, 0, 0, 0,
            0, f, 0, 0,
            0, 0, A, -1,
            0, 0, B, 0
        };
    }
}
