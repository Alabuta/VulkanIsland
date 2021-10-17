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

layout (location = 0) in VS_DATA
{
    vec3 normal;
} vs_data[];

in gl_PerVertex {
    vec4 gl_Position;
} gl_in[];

layout (location = 0) out vec4 outColor;

const vec4 normalsColor = vec4(1.0, 1.0, 0.0, 1.0);


void process(int index)
{
    outColor = normalsColor;

    gl_Position = camera.projection * gl_in[index].gl_Position;
    EmitVertex();

    outColor = normalsColor;

    gl_Position = camera.projection * (gl_in[index].gl_Position + vec4(vs_data[index].normal, 0.0) * MAGNITUDE);
    EmitVertex();

    EndPrimitive();
}

#pragma technique(0)
{
    process(0);
    process(1);
    process(2);
}