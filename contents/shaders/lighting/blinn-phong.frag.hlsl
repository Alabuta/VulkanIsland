struct PS_INPUT
{
	float4 sv_position : SV_POSITION;
    [[vk::location(0)]] float3 normal : NORMAL0;
    [[vk::location(1)]] float3 light_vector : COLOR0;
};

const float4 ambient_color = float4(.25, .25, .25, 1.0);
const float4 diffuse_color = float4(0.8, 0.8, 0.8, 1.0);


#pragma technique(0)
[earlydepthstencil]
float4 main(PS_INPUT input) : SV_TARGET
{
    float3 color = ambient_color + max(dot(input.normal, input.light_vector), 0.) * diffuse_color;

    return float4(color, 1.);
}
