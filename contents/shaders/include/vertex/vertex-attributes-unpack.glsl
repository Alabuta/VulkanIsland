

//vec3 unpackAttribute_position_rgb32f(const in vec3 a)
//{
//    return a;
//}
//
//vec3 unpackAttribute_normal_rgb32f(const in vec3 a)
//{
//    return a;
//}
//
//vec3 unpackAttribute_normal_rg16f(const in vec2 a)
//{
//    return a;
//}
//
//vec3 unpackAttribute_normal_rg16snorm(const in vec2 a)
//{
//    return a;
//}

vec3 unpackAttribute_normal_rg8snorm(const in vec2 a)
{
    vec3 normal = vec3(a.xy, 1. - abs(a.x) - abs(a.y));

    normal.xy = normal.z < 0. ? ((1. - abs(normal.yx)) * (all(lessThan(normal.xy, vec2(0.))) ? -1. : +1.)) : normal.xy;

    return normalize(normal);
}
