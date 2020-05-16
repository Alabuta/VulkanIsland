

vec3 decode_oct_to_unit(const in vec2 a)
{
    vec3 normal = vec3(a.xy, 1. - abs(a.x) - abs(a.y));

    normal.xy = normal.z < 0. ? ((1. - abs(normal.yx)) * (all(lessThan(normal.xy, vec2(0.))) ? -1. : +1.)) : normal.xy;

    return normalize(normal);
}

vec3 unpackAttribute_normal_rg8i_norm(const in vec2 a)
{
    return decode_oct_to_unit(a);
}

vec3 unpackAttribute_normal_rg16i_norm(const in vec2 a)
{
    return decode_oct_to_unit(a);
}
