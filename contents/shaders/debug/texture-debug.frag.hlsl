struct PS_INPUT
{
	float4 sv_position : SV_POSITION;
    [[vk::location(0)]] float2 texcoord : TEXCOORD0;
};

Texture2D main_sampler : register(t0);
SamplerState main_sampler : register(s0);


#pragma technique(0)
[earlydepthstencil]
float4 main(PS_INPUT ps_input) : SV_TARGET
{
    return main_sampler.Sample(main_sampler, texcoord);
}
