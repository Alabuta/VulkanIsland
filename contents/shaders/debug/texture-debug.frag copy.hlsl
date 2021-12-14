struct PS_INPUT
{
	float4 sv_position : SV_POSITION;
    [[vk::location(0)]] float2 texcoord : TEXCOORD0;
};

layout (set = 2, binding = 0) Texture2D main_texture : register(t1);
layout (set = 2, binding = 1) SamplerState main_sampler : register(s1);


#pragma technique(0)
[earlydepthstencil]
float4 main(PS_INPUT ps_input) : SV_TARGET
{
    return main_texture.Sample(main_sampler, ps_input.texcoord);
}
