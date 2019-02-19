#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 POSITION;
layout(location = 1) in vec4 COLOR_0;

layout(set = 0, binding = 0, std430) readonly buffer PER_CAMERA
{
    mat4 view;
    mat4 projection;

    mat4 projectionView;

    mat4 invertedView;
    mat4 invertedProjection;
} camera;

layout(set = 0, binding = 2, std430) readonly buffer PER_OBJECT
{
    mat4 world;
    mat4 normal;
} object;

layout(location = 0) out vec4 color_0;

out gl_PerVertex{
    vec4 gl_Position;
};

void main()
{
    gl_Position = camera.view * object.world * vec4(POSITION, 1.0);

    gl_Position = camera.projection * gl_Position;

    color_0 = COLOR_0;
}