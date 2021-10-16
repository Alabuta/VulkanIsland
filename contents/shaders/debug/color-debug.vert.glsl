
layout(set = 0, binding = 0, scalar) uniform PER_CAMERA
{
    mat4 view;
    mat4 projection;

    mat4 projectionView;

    mat4 invertedView;
    mat4 invertedProjection;
} camera;

layout(set = 1, binding = 0, scalar) readonly buffer PER_OBJECT
{
    mat4 world;
    mat4 normal;
} object;

layout(location = 0) out vec4 outColor;


out gl_PerVertex {
    vec4 gl_Position;
};


#pragma technique(0)
{
    gl_Position = camera.projectionView * object.world * vec4(POSITION, 1.);

    outColor = unpackAttribute(COLOR_0);
}

#pragma technique(1)
{
    gl_Position = camera.projectionView * object.world * vec4(POSITION, 1.);

    outColor = unpackAttribute(COLOR_0);
    outColor.rgb *= COLOR_MULTIPLIER;
}
