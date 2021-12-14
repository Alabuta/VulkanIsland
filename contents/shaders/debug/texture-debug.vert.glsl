
layout (set = 0, binding = 0, scalar) uniform PER_CAMERA
{
    mat4 view;
    mat4 projection;

    mat4 projectionView;

    mat4 invertedView;
    mat4 invertedProjection;
} camera;

layout (set = 1, binding = 0, scalar) readonly buffer PER_OBJECT
{
    mat4 world;
    mat4 normal;
} object;

layout (location = 0) out vec2 texcoord;

out gl_PerVertex {
    vec4 gl_Position;
};


void process(in vec3 position, in vec2 texcoord_0)
{
    gl_Position = camera.view * object.world * vec4(position, 1.);
    gl_Position = camera.projection * gl_Position;

    texcoord = vec2(texcoord_0.x, 1.f - texcoord_0.y);
}

#pragma technique(0)
{
    vec2 texcoord_0 = unpackAttribute(TEXCOORD_0);

    process(POSITION, texcoord_0);
}
