
layout (set = 0, binding = 0, scalar) uniform PER_CAMERA
{
    mat4 view;
    mat4 projection;

    mat4 projectionView;

    mat4 invertedView;
    mat4 invertedProjection;
} camera;

layout (set = 1, binding = 0, scalar) readonly buffer PER_OBJECT
{
    mat4 world;
    mat4 normal;
} object;

layout (location = 0) out vec4 outColor;

out gl_PerVertex {
    vec4 gl_Position;
};


void process(in vec3 position, in vec3 normal, bool transfromToViewSpace)
{
    gl_Position = camera.view * object.world * vec4(position, 1.0);
    gl_Position = camera.projection * gl_Position;

    vec4 n = transfromToViewSpace ? object.normal * vec4(normal, 0.0) : vec4(normal, 0.0);
    outColor = vec4(normalize(vec3(n)), 1.0);
    if (!transfromToViewSpace)
        outColor = vec4(outColor.xyz * .5 + .5, 1.);
}

#pragma technique(0)
{
    vec3 normal = unpackAttribute(NORMAL);

    process(POSITION, normal, true);
}

#pragma technique(1)
{
    vec3 normal = unpackAttribute(NORMAL);

    process(POSITION, normal, false);
}
