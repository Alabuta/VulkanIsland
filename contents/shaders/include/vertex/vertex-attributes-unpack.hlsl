

float3 decode_oct_to_unit(in float2 a)
{
    float3 normal = float3(a.xy, 1. - abs(a.x) - abs(a.y));

    normal.xy = normal.z < 0. ? ((1. - abs(normal.yx)) * (all(normal.xy < float2(0.)) ? -1. : +1.)) : normal.xy;

    return normalize(normal);
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
