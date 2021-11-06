uint pack_snorm2x12(in float2 v)
{
    uint2 d = uint2(round(2047.5 + v * 2047.5));
    return d.x | (d.y << 12u);
}

uint pack_snorm2x8(in float2 v)
{
    uint2 d = uint2(round(127.5 + v * 127.5));
    return d.x | (d.y << 8u);
}

float2 unpack_snorm2x8(in uint d)
{
    return float2(uint2(d, d >> 8u) & 255u) / 127.5 - 1.0;
}

float2 unpack_snorm2x12(in uint d)
{
    return float2(uint2(d, d >> 12u) & 4095u) / 2047.5 - 1.0;
}

// https://www.shadertoy.com/view/llfcRl
float3 decode_oct_to_unit(in float2 v)
{
    float3 n = float3(v, 1. - abs(v.x) - abs(v.y));     // Rune Stubbe's version,
    float t = max(-n.z, 0.);                        // much faster than original
    n.x += (n.x > 0.) ? -t : t;                     // implementation of this
    n.y += (n.y > 0.) ? -t : t;                     // technique

    // n.xy = n.z < 0. ? ((1. - abs(n.yx)) * (all(lessThan(n.xy, float2(0.))) ? -1. : +1.)) : n.xy;
    return normalize(n);
}

// float3 decode_oct_to_unit(in float2 a)
// {
//     float3 normal = float3(a.xy, 1. - abs(a.x) - abs(a.y));

//     normal.xy = normal.z < 0. ? ((1. - abs(normal.yx)) * (all(normal.xy < float2(0.)) ? -1. : +1.)) : normal.xy;

//     return normalize(normal);
// }

float3 unpackAttribute_normal_r16ui(in uint a)
{
    return decode_oct_to_unit(unpack_snorm2x8(a));
}

float3 unpackAttribute_normal_r32ui(in uint a)
{
    float2 v = (uint2(a, a > 16u) & 65535u) / 32767.5 - 1.;

    // return float3(v.xy, 0) * 0.5 + 0.5;
    return decode_oct_to_unit(v);
}

float3 unpackAttribute_normal_rg8i_norm(in float2 a)
{
    return decode_oct_to_unit(a);
}

float3 unpackAttribute_normal_rg16i_norm(in float2 a)
{
    return decode_oct_to_unit(a);
}

float3 unpackAttribute_normal_rgb32f(in float3 a)
{
    return a;
}

float2 unpackAttribute_texcoord_rg16ui_norm(in float2 a)
{
    return a;
}

float2 unpackAttribute_texcoord_rg32f(in float2 a)
{
    return a;
}

float4 unpackAttribute_color_rgb8ui_norm(in float3 a)
{
    return float4(a, 1.);
}

float4 unpackAttribute_color_rgba8ui_norm(in float4 a)
{
    return a;
}
