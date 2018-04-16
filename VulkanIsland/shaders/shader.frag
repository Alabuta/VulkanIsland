#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D textureSampler;

layout(location = 0) in vec3 perVertexNormal;
layout(location = 1) in vec2 perVertexUV;

layout(location = 0) out vec4 fragColor;

void main()
{
    // fragColor = vec4(perVertexUV, 0.0, 1.0);
    fragColor = texture(textureSampler, perVertexUV);
}