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
	float4 position : SV_POSITION;
    [[vk::location(0)]] float4 color : COLOR0;
};


#pragma technique(0)
VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    output.position = mul(object[0].world, float4(input.POSITION, 1.));
    output.position = mul(camera.projectionView, output.position);

    output.color = unpackAttribute(input.COLOR_0);

    return output;
}

#pragma technique(1)
VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    output.position = mul(object[0].world, float4(input.POSITION, 1.));
    output.position = mul(camera.projectionView, output.position);

    output.color = unpackAttribute(input.COLOR_0);
    output.color.rgb *= COLOR_MULTIPLIER;

    return output;
}
