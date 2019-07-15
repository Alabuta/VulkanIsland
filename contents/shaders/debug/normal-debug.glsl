
layout (set = 0, binding = 0, scalar) readonly buffer PER_CAMERA
{
    mat4 view;
    mat4 projection;

    mat4 projectionView;

    mat4 invertedView;
    mat4 invertedProjection;
} camera;

layout (set = 0, binding = 2, scalar) readonly buffer PER_OBJECT
{
    mat4 world;
    mat4 normal;
} object;

layout(constant_id = 0) const int technique = 0;

layout (location = 0) out vec4 outColor;

out gl_PerVertex {
    vec4 gl_Position;
};


#pragma technique(0)
{
    gl_Position = camera.view * object.world * vec4(POSITION, 1.0);
    gl_Position = camera.projection * gl_Position;

    outColor = vec4(normalize(vec3(object.normal * vec4(NORMAL, 0.0))), 1.0);
}
