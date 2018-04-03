#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec3 inColor;

layout(binding = 0) uniform TRANSFORMS {
    mat4 model;
    mat4 view;
    mat4 proj;
} transforms;

layout(location = 0) out vec3 perVertexColor;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    gl_Position = transforms.proj * transforms.view * transforms.model * vec4(inVertex, 1.0);
    perVertexColor = inColor;
}