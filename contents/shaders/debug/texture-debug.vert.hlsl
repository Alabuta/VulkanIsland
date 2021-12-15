#include "common.hlsl"

layout (set = 0, binding = 0) ConstantBuffer<PER_CAMERA> camera : register(b0, space0);
layout (set = 1, binding = 0) StructuredBuffer<PER_OBJECT> object : register(t0, space0);

struct VS_OUTPUT
{
	float4 sv_position : SV_POSITION;
    [[vk::location(0)]] float2 texcoord : TEXCOORD0;
};


VS_OUTPUT process(in float3 position, in float2 texcoord_0)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    output.sv_position = mul(object[0].world, float4(position, 1.));
    output.sv_position = mul(camera.projectionView, output.sv_position);

    output.texcoord = float2(texcoord_0.x, 1.f - texcoord_0.y);

    return output;
}

#pragma technique(0)
VS_OUTPUT main(VS_INPUT input)
{
    float2 texcoord_0 = unpackAttribute(input.TEXCOORD_0);

    return process(input.POSITION, texcoord_0);
}