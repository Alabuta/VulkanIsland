

vec3 unpackAttribute_position_rgb32f(const in vec3 attribute)
{
    return attribute;
}

vec3 unpackAttribute_normal_rgb32f(const in vec3 attribute)
{
    return attribute;
}

vec3 unpackAttribute_normal_rg32f(const in vec2 attribute)
{
    return attribute;
}

vec3 unpackAttribute_normal_rgb16f(const in vec3 attribute)
{
    return attribute;
}

vec3 unpackAttribute_normal_rg16f(const in vec2 attribute)
{
    return attribute;
}

vec3 unpackAttribute_normal_rg16snorm(const in vec2 attribute)
{
    return attribute;
}

vec3 unpackAttribute_normal_rg8snorm(const in vec2 attribute)
{
    vec3 normal = vec3(attribute.xy, 1. - abs(attribute.x) - abs(attribute.y));

    normal.xy = normal.z < 0 ? ((1. - abs(normal.yx)) * (normal.xy < 0. ? -1. : +1.)) : normal.xy;

    return normalize(normal);
}
