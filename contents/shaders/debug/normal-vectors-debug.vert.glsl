#include "common.glsl"

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

layout (location = 0) out VS_DATA
{
    vec3 normal;
} vs_data;

out gl_PerVertex {
    vec4 gl_Position;
};


void process(in vec3 position, in vec3 normal)
{
    gl_Position = camera.view * object.world * vec4(position, 1.0);

    vec4 viewSpaceNormal = object.normal * vec4(normal, 0.0);
    vs_data.normal = normalize(vec3(viewSpaceNormal));
}

#pragma technique(0)
{
    vec3 normal = unpackAttribute(NORMAL);

    process(POSITION, normal);
}