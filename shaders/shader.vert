#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(set = 0, binding = 0) uniform TRANSFORMS {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 modelView;
} transforms;

layout(set = 0, binding = 2) uniform PER_CAMERA
{
    mat4 view;
    mat4 projection;

    mat4 projectionView;

    mat4 invertedView;
    mat4 invertedProjection;
} camera;

layout(location = 0) out vec3 viewSpaceNormal;
layout(location = 1) out vec2 texCoord;
layout(location = 2) out vec3 viewSpacePosition;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    gl_Position = camera.view * transforms.model * vec4(inVertex, 1.0);

    viewSpacePosition = gl_Position.xyz;

    gl_Position = camera.projection * gl_Position;

    viewSpaceNormal = normalize((transpose(inverse(transforms.modelView)) * vec4(inNormal, 0.0)).xyz);
    texCoord = vec2(inUV.x, inUV.y);
}