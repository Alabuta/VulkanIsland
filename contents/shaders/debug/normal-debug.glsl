
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


void process(const in vec3 position, const in vec3 normal)
{
    gl_Position = camera.view * object.world * vec4(position, 1.0);
    gl_Position = camera.projection * gl_Position;

    vec4 worldSpaceNormal = object.normal * vec4(normal, 0.0);
    vec4 viewSpaceNormal = camera.view * worldSpaceNormal;

    outColor = vec4(normalize(vec3(viewSpaceNormal)), 1.0);
}


#pragma technique(0)
{
    process(POSITION, NORMAL);
}

#pragma technique(1)
{
    vec3 normal = unpackAttribute(NORMAL);

    process(POSITION, normal);
}
