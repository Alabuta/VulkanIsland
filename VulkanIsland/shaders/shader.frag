#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform TRANSFORMS {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 modelView;
} transforms;

layout(binding = 1) uniform sampler2D textureSampler;

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
    { { -6, 1.4, 1.6 }, { 0, 1, 0 } },
    { { 2, 0, 2 }, { 0, 0.8, 1 } },
    { { 11.2, 1.2, -4.5 }, { 1, 0.24, 0 } },
    { { 5, 1, -2 }, { 1, 0.8, 0.4 } },
    { { -6, 1.4, -1.6 }, { 1, 1, 0 } }
};

void main()
{
    fragColor = vec4(fma(perVertexNormal, vec3(0.5), vec3(0.5)), 1.0);
    // fragColor = texture(textureSampler, perVertexUV);

    fragColor.rgb = vec3(0.0);

    vec3 lightPos;
    float dist, attenuation;

    for (int i = 0; i < kPOINT_LIGHTS; ++i) {
        lightPos = (transforms.view * vec4(pointLights[i].position, 1)).xyz;

        dist = distance(perVertexPos, lightPos);
        attenuation = 1.0 / (1.0 + dist * (0.7 + dist * 1.8));

        //fragColor.rgb += pointLights[i].color * attenuation;
        fragColor.rgb += pointLights[i].color * vec3(dist / 1000.0);
    }

    //fragColor.rgb = perVertexPos * 0.01;
}