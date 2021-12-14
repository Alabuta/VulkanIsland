#include "common.hlsl"

layout (set = 0, binding = 0) ConstantBuffer<PER_CAMERA> camera : register(b0, space0);
layout (set = 1, binding = 0) StructuredBuffer<PER_OBJECT> object : register(t0, space0);

struct PER_VIEWPORT
{
    int4 rect;
};

layout (set = 0, binding = 1) ConstantBuffer<PER_VIEWPORT> viewport : register(b0, space1);

struct VS_OUTPUT
{
	float4 sv_position : SV_POSITION;
	[[vk::location(0)]] float2 vs_position : POSITION;
};


VS_OUTPUT process(in float3 position)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    output.sv_position = mul(object[0].world, float4(position, 1.0f));
    output.sv_position = mul(camera.projectionView, output.sv_position);

    // Transform each vertex from clip space into viewport space.
    output.vs_position = normalizedToViewport(viewport.rect, output.sv_position.xy / output.sv_position.w);

    return output;
}

#pragma technique(0)
VS_OUTPUT main(VS_INPUT input)
{
    return process(input.POSITION);
}
