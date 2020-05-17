
layout (set = 0, binding = 0, scalar) readonly buffer PER_CAMERA
{
    mat4 view;
    mat4 projection;

    mat4 projectionView;

    mat4 invertedView;
    mat4 invertedProjection;
} camera;

layout (set = 0, binding = 1, scalar) readonly buffer PER_OBJECT
{
    mat4 world;
    mat4 normal;
} object;

layout (location = 0) out vec4 outColor;

out gl_PerVertex {
    vec4 gl_Position;
};


void process(in vec3 position, in vec2 texcoord_0)
{
    gl_Position = camera.view * object.world * vec4(position, 1.);
    gl_Position = camera.projection * gl_Position;

    outColor = vec4(texcoord_0, 0., 1.);
}

#pragma technique(0)
{
    process(POSITION, TEXCOORD_0);
}

#pragma technique(1)
{
    vec2 texcoord_0 = unpackAttribute(TEXCOORD_0);

    process(POSITION, texcoord_0);
}
