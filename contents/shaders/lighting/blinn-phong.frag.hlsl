struct PS_INPUT
{
	float4 sv_position : SV_POSITION;
    [[vk::location(0)]] float3 normal : NORMAL0;
    [[vk::location(1)]] float3 light_vector : COLOR0;
    [[vk::location(2)]] float3 position : COLOR1;
};

const float4 ambient_component = float4(float3(.25), 1.);
const float4 light_color = float4(1.);
const float4 object_color = float4(.2, .8, .6, 1.);

const float shininess = 64.;


#pragma technique(0)
[earlydepthstencil]
float4 main(PS_INPUT input) : SV_TARGET
{
    float3 n = normalize(input.normal);
    float3 l = normalize(input.light_vector);
    float3 v = normalize(-input.position);
    float3 h = normalize(l + v);

    float diffuse_component = max(dot(n, l), 0.);
    float specular_component = pow(max(dot(h, n), 0.), shininess);
    
    float3 color = saturate((ambient_component + light_color * (diffuse_component + specular_component)) * object_color);

    return float4(color, 1.);
}
