

vec3 decode_oct_to_unit(in vec2 a)
{
    vec3 normal = vec3(a.xy, 1. - abs(a.x) - abs(a.y));

    normal.xy = normal.z < 0. ? ((1. - abs(normal.yx)) * (all(lessThan(normal.xy, vec2(0.))) ? -1. : +1.)) : normal.xy;

    return normalize(normal);
}

vec3 unpackAttribute_normal_rg8i_norm(in vec2 a)
{
    return decode_oct_to_unit(a);
}

vec3 unpackAttribute_normal_rg16i_norm(in vec2 a)
{
    return decode_oct_to_unit(a);
}

vec3 unpackAttribute_normal_rgb32f(in vec3 a)
{
    return a;
}

vec2 unpackAttribute_texcoord_rg16ui_norm(in vec2 a)
{
    return a;
}

vec2 unpackAttribute_texcoord_rg32f(in vec2 a)
{
    return a;
}

vec4 unpackAttribute_color_rgb8ui_norm(in vec3 a)
{
    return vec4(a, 1.);
}

vec4 unpackAttribute_color_rgba8ui_norm(in vec4 a)
{
    return a;
}
