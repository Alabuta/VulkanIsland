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
    vec2 position;
    vec2 texcoord;
} vs_data;

out gl_PerVertex {
    vec4 gl_Position;
};


void process(in vec3 position, in vec2 texcoord)
{
    gl_Position = camera.view * object.world * vec4(position, 1.0);
    gl_Position = camera.projection * gl_Position;

    // Transform each vertex from clip space into viewport space.
    vs_data.position = normalizedToViewport(vec4(0, 0, 1920, 1080), gl_Position.xy / gl_Position.w);

    vs_data.texcoord = texcoord;
}

#pragma technique(0)
{
    vec2 texcoord_0 = unpackAttribute(TEXCOORD_0);

    process(POSITION, texcoord_0);
}
