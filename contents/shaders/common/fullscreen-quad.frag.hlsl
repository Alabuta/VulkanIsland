struct VS_OUTPUT
{
	float4 sv_position : SV_POSITION;
    [[vk::location(0)]] float2 texCoord : TEXCOORD0;
};

Texture2D mainTexture : register(t0);
SamplerState mainSampler : register(s0);


#pragma technique(0)
[earlydepthstencil]
float4 main(VS_OUTPUT vs_output) : SV_TARGET
{
    fragColor = mainTexture.Sample(mainSampler, texCoord);
    fragColor.a = 1;
}