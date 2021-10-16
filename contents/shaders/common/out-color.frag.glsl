
layout (early_fragment_tests) in;

layout (location = 0) in vec4 inColor;

layout (location = 0) out vec4 fragColor;


#pragma technique(0)
{
    fragColor = inColor;
}