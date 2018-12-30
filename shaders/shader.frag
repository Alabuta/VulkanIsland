#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(early_fragment_tests) in;

layout(set = 0, binding = 1) uniform sampler2D textureSampler;

layout(set = 0, binding = 0, std430) readonly buffer PER_CAMERA
{
    mat4 view;
    mat4 projection;

    mat4 projectionView;

    mat4 invertedView;
    mat4 invertedProjection;
} camera;

layout(location = 0) in vec3 worldSpaceNormal;
layout(location = 1) in vec2 texCoord1;
layout(location = 2) in vec3 worldSpacePosition;

layout(location = 0) out vec4 fragColor;

struct PointLight {
    vec3 position;
    vec3 color;
};

const int kPOINT_LIGHTS = 5;
const PointLight pointLights[kPOINT_LIGHTS] = {
    { { -6.0, 8.4, -10.6 }, { 0, 1, 0 } },
    { { -7.0, 4.0, 16 }, { 0, 0.8, 1 } },
    { { +8.0, 4.0, -22 }, { 1, 0.24, 0 } },
    { { +8.0, 1.0, 8 }, { 1, 0.8, 0.4 } },
    { { -9.0, 4.4, 1.6 }, { 1, 1, 0 } }
};

const vec4 lightDir = vec4(-1., -1., 1., 0.);

void main()
{
    //fragColor = vec4(vec3(worldSpaceNormal * 0.5 + 0.5), 1.0);
    //fragColor = vec4(texCoord, texCoord.y / texCoord.x, 1.0);
    /*vec2 texCoord = texCoord1 * vec2(1, -1);
    fragColor = texture(textureSampler, texCoord);*/

    //fragColor.rgb = worldSpacePosition / 100.0;
    //fragColor.a = 1.0;

    fragColor = vec4(1.);

    vec3 worldSpaceLightDir = normalize((1. * lightDir).xyz);
    fragColor.rgb *= dot(normalize(worldSpaceNormal), worldSpaceLightDir);

    fragColor.rgb = fragColor.rgb + vec3(.24);

#if 0
    vec4 lightPos;

    float dist = 0.0, luminosity = 0.0;

    for (int i = 0; i < kPOINT_LIGHTS; ++i) {
        lightPos = camera.view * vec4(pointLights[i].position * 2.0, 1.0);

        dist = distance(worldSpacePosition, lightPos.xyz);
        luminosity = 1.0 / (1.0 + dist * (0.09 + dist * 0.032));

        fragColor.rgb += pointLights[i].color * luminosity;
    }
#endif
}