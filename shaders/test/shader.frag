#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(early_fragment_tests) in;


layout(set = 0, binding = 0, std430) readonly buffer PER_CAMERA
{
    mat4 view;
    mat4 projection;

    mat4 projectionView;

    mat4 invertedView;
    mat4 invertedProjection;
} camera;


layout(location = 0) in vec4 inColor;

layout(location = 0) out vec4 fragColor;


void main()
{
    fragColor = inColor;
}