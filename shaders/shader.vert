#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inTangent;

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
    mat4 normal;  // Transposed of the inversed of the upper left 3x3 sub-matrix of world matrix.
} object;

layout(location = 0) out vec3 worldSpaceNormal;
layout(location = 1) out vec2 texCoord;
layout(location = 2) out vec3 worldSpacePosition;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    gl_Position = camera.view * object.world * vec4(inVertex, 1.0);

    worldSpacePosition = gl_Position.xyz;

    gl_Position = camera.projection * gl_Position;

    worldSpaceNormal = normalize((object.normal /* transpose(camera.invertedView)*/ * vec4(inNormal, 0.0)).xyz);
    texCoord = vec2(inUV.x, inUV.y);
}