#include "common.hlsl"

layout (set = 0, binding = 0) ConstantBuffer<PER_CAMERA> camera : register(b0, space0);
layout (set = 1, binding = 0) StructuredBuffer<PER_OBJECT> object : register(t0, space0);

struct VS_OUTPUT
{
	float4 sv_position : SV_POSITION;
    [[vk::location(0)]] float3 normal : NORMAL0;
    [[vk::location(1)]] float3 light_vector : COLOR0;
    [[vk::location(2)]] float3 position : COLOR1;
};

const float4 light_position = float4(float3(3.), .0);


#pragma technique(0)
VS_OUTPUT main(VS_INPUT input)
{
    float3 in_position = input.POSITION;
    float3 in_normal = unpackAttribute(input.NORMAL);

    VS_OUTPUT output = (VS_OUTPUT)0;

    float4 position = mul(object[0].world, float4(in_position, 1.));
    position = mul(camera.view, position);

    output.position = position.xyz;

    output.sv_position = mul(camera.projection, position);

    output.normal = normalize(mul(float3x3(object[0].normal), in_normal));

    float4 vs_light_pos = mul(camera.view, light_position);
    output.light_vector = normalize((vs_light_pos - position).xyz);

    return output;
}
