#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(binding = 0) uniform TRANSFORMS {
    mat4 model;
    mat4 view;
    mat4 proj;
} transforms;

layout(location = 0) out vec3 perVertexNormal;
layout(location = 1) out vec2 perVertexUV;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    gl_Position = transforms.proj * transforms.view * transforms.model * vec4(inVertex, 1.0);
    perVertexNormal = inNormal;
    perVertexUV = vec2(inUV.x, 1.0 - inUV.y);
}