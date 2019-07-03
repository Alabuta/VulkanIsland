#include "math.hxx"


namespace math {
glm::mat4 reversedPerspective(float yFOV, float aspect, float znear, float zfar)
{
    auto const f = 1.f / std::tan(yFOV * .5f);

    auto const kA = zfar / (zfar - znear) - 1.f;
    auto const kB = zfar * znear / (zfar - znear);

    return glm::mat4{
        f / aspect, 0, 0, 0,
        0, f, 0, 0,
        0, 0, kA, -1,
        0, 0, kB, 0
    };
}
}
