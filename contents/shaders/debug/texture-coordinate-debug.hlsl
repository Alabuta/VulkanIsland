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

struct VS_INPUT
{
    [[vk::location(0)]] float3 POSITION : POSITION0;
    [[vk::location(2)]] float2 TEXCOORD_0 : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
    [[vk::location(0)]] float4 color : COLOR0;
};


VS_OUTPUT process(in float3 position, in float2 texcoord_0)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    output.position = mul(camera.view, mul(object[0].world, float4(position, 1.)));
    output.position = mul(camera.projection, output.position);

    output.color = float4(texcoord_0, 0., 1.);

    return output;
}

VS_OUTPUT main(VS_INPUT input)
{
    float2 texcoord_0 = input.TEXCOORD_0;//unpackAttribute(TEXCOORD_0);

    return process(input.POSITION, texcoord_0);
}