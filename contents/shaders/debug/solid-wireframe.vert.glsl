#include "common.glsl"

layout (set = 0, binding = 0, scalar) uniform PER_CAMERA
{
    mat4 view;
    mat4 projection;

    mat4 projectionView;

    mat4 invertedView;
    mat4 invertedProjection;
} camera;

layout (set = 0, binding = 1, scalar) uniform PER_VIEWPORT
{
    ivec4 rect;
} viewport;

layout (set = 1, binding = 0, scalar) readonly buffer PER_OBJECT
{
    mat4 world;
    mat4 normal;
} object;

layout (location = 0) out VS_DATA
{
    vec2 position;
} vs_data;

out gl_PerVertex {
    vec4 gl_Position;
};


void process(in vec3 position)
{
    gl_Position = camera.view * object.world * vec4(position, 1.0);
    gl_Position = camera.projection * gl_Position;

    // Transform each vertex from clip space into viewport space.
    vs_data.position = normalizedToViewport(viewport.rect, gl_Position.xy / gl_Position.w);
}

#pragma technique(0)
{
    process(POSITION);
}
