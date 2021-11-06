uint pack_snorm2x12(in vec2 v)
{
    uvec2 d = uvec2(round(2047.5 + v * 2047.5));
    return d.x | (d.y << 12u);
}

uint pack_snorm2x8(in vec2 v)
{
    uvec2 d = uvec2(round(127.5 + v * 127.5));
    return d.x | (d.y << 8u);
}

vec2 unpack_snorm2x8(in uint d)
{
    return vec2(uvec2(d, d >> 8u) & 255u) / 127.5 - 1.0;
}

vec2 unpack_snorm2x12(in uint d)
{
    return vec2(uvec2(d, d >> 12u) & 4095u) / 2047.5 - 1.0;
}

// https://www.shadertoy.com/view/llfcRl
vec3 decode_oct_to_unit(in vec2 v)
{
    vec3 n = vec3(v, 1. - abs(v.x) - abs(v.y));     // Rune Stubbe's version,
    float t = max(-n.z, 0.);                        // much faster than original
    n.x += (n.x > 0.) ? -t : t;                     // implementation of this
    n.y += (n.y > 0.) ? -t : t;                     // technique

    // n.xy = n.z < 0. ? ((1. - abs(n.yx)) * (all(lessThan(n.xy, vec2(0.))) ? -1. : +1.)) : n.xy;
    return normalize(n);
}

vec3 unpackAttribute_normal_r16ui(in uint a)
{
    return decode_oct_to_unit(unpack_snorm2x8(a));
}

vec3 unpackAttribute_normal_r32ui(in uint a)
{
    vec2 v = (uvec2(a, a > 16u) & 65535u) / 32767.5 - 1.;

    // return vec3(v.xy, 0) * 0.5 + 0.5;
    return decode_oct_to_unit(v);
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
