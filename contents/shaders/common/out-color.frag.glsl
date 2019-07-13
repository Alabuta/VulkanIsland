#version 460

#extension GL_ARB_separate_shader_objects : enable

layout (early_fragment_tests) in;

layout(constant_id = 0) const int technique = 0;

layout (location = 0) in vec4 inColor;

layout (location = 0) out vec4 fragColor;


#pragma technique(0)
{
    fragColor = inColor;
}