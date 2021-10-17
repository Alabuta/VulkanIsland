struct PER_CAMERA
{
    float4x4 view;
    float4x4 projection;

    float4x4 projectionView;

    float4x4 invertedView;
    float4x4 invertedProjection;
};

layout (set = 0, binding = 0) ConstantBuffer<PER_CAMERA> camera : register(b0, space0);

struct PER_OBJECT
{
    float4x4 world;
    float4x4 normal;
};

layout (set = 1, binding = 0) StructuredBuffer<PER_OBJECT> object : register(t0, space0);

struct VS_OUTPUT
{
	[[vk::location(0)]] float4 position : POSITION;
    [[vk::location(1)]] float3 normal : NORMAL;
};


VS_OUTPUT process(in float3 position, in float3 normal)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    output.position = mul(object[0].world, float4(position, 1.));
    output.position = mul(camera.view, output.position);

    float4 viewSpaceNormal = mul(object[0].normal, float4(normal, 0.0));
    output.normal = normalize(float3(viewSpaceNormal));

    return output;
}

#pragma technique(0)
VS_OUTPUT main(VS_INPUT input)
{
    float3 normal = unpackAttribute(input.NORMAL);

    return process(input.POSITION, normal);
}
