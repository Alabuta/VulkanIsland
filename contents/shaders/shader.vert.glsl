#version 460
#extension GL_ARB_separate_shader_objects : enable

#pragma attribute(POSITION, vec3, required)
#pragma attribute(NORMAL, vec3, required)
#pragma attribute(TEXCOORD_0, vec2, required)
#pragma attribute(TANGENT, vec4, required)

layout (location = 0) in vec3 POSITION;
layout (location = 1) in vec3 NORMAL;
layout (location = 2) in vec2 TEXCOORD_0;
layout (location = 4) in vec4 TANGENT;

layout (set = 0, binding = 0, std430) readonly buffer PER_CAMERA
{
    mat4 view;
    mat4 projection;

    mat4 projectionView;

    mat4 invertedView;
    mat4 invertedProjection;
} camera;

layout (set = 0, binding = 2, std430) readonly buffer PER_OBJECT
{
    mat4 world;
    mat4 normal;  // Transposed of the inversed of the upper left 3x3 sub-matrix of world matrix.
} object;

layout (location = 0) out vec3 worldSpaceNormal;
layout (location = 1) out vec2 texCoord;
layout (location = 2) out vec3 worldSpacePosition;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    gl_Position = camera.view * object.world * vec4(POSITION, 1.0);

    worldSpacePosition = gl_Position.xyz;

    gl_Position = camera.projection * gl_Position;

    worldSpaceNormal = normalize((object.normal /* transpose(camera.invertedView)*/ * vec4(NORMAL, 0.0)).xyz);
    texCoord = vec2(TEXCOORD_0.x, TEXCOORD_0.y);
}