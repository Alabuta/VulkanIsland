#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform TRANSFORMS {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 modelView;
} transforms;

layout(set = 0, binding = 1) uniform sampler2D textureSampler;

layout(location = 0) in vec3 perVertexNormal;
layout(location = 1) in vec2 perVertexUV;
layout(location = 2) in vec3 perVertexPos;

layout(location = 0) out vec4 fragColor;

struct PointLight {
    vec3 position;
    vec3 color;
};

const int kPOINT_LIGHTS = 5;
const PointLight pointLights[kPOINT_LIGHTS] = {
    { { -6, 8.4, -10.6 }, { 0, 1, 0 } },
    { { -7, 4, 16 }, { 0, 0.8, 1 } },
    { { 8, 4, -22 }, { 1, 0.24, 0 } },
    { { 8, 1, 8 }, { 1, 0.8, 0.4 } },
    { { -9, 4.4, 1.6 }, { 1, 1, 0 } }
};

void main()
{
    //fragColor = vec4(vec3(perVertexNormal * 0.5 + 0.5), 1.0);
    //fragColor = vec4(perVertexUV, perVertexUV.y / perVertexUV.x, 1.0);
    //fragColor = texture(textureSampler, perVertexUV);

    //fragColor.rgb = perVertexPos / 100.0;
    //fragColor.a = 1.0;

    fragColor = vec4(vec3(0.16), 1);

    vec4 lightPos;
    float dist, attenuation;

    for (int i = 0; i < kPOINT_LIGHTS; ++i) {
        lightPos = transforms.view * vec4(pointLights[i].position * 2.0, 1.0);

        dist = distance(perVertexPos, lightPos.xyz);
        attenuation = 1.0 / (1.0 + dist * (0.09 + dist * 0.032));

        fragColor.rgb += pointLights[i].color * attenuation;
    }
}