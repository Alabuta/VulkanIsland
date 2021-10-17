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


VS_OUTPUT process(in float3 position, in float2 texcoord_0)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    output.position = mul(object[0].world, float4(position, 1.));
    output.position = mul(camera.view, output.position);
    output.position = mul(camera.projection, output.position);

    output.color = float4(texcoord_0, 0., 1.);

    return output;
}

#pragma technique(0)
VS_OUTPUT main(VS_INPUT input)
{
    float2 texcoord_0 = unpackAttribute(input.TEXCOORD_0);

    return process(input.POSITION, texcoord_0);
}

#pragma technique(1)
[earlydepthstencil]
VS_OUTPUT main(VS_INPUT input)
{
    asdasdas
    float2 texcoord_0 = unpackAttribute(input.TEXCOORD_0);
    //{}
    //}

    return process(input.POSITION, texcoord_0);
}

[earlydepthstencil2 x(0)]
#pragma technique(2)
[earlydepthstencil]
VS_OUTPUT main(VS_INPUT input)
{
    asdasdas
    float2 texcoord_0 = unpackAttribute(input.TEXCOORD_0);//;asdasdasdasd
    //{}
    //}

    return process(input.POSITION, texcoord_0);
}