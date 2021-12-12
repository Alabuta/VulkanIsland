struct VS_OUTPUT
{
	float4 sv_position : SV_POSITION;
    [[vk::location(0)]] float2 texcoord : TEXCOORD0;
};

Texture2D main_texture : register(t0);
SamplerState main_sampler : register(s0);


#pragma technique(0)
[earlydepthstencil]
float4 main(VS_OUTPUT vs_output) : SV_TARGET
{
    float4 color = main_texture.Sample(main_sampler, texcoord);

    return float4(color.rgb, 1.);
}
