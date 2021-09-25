struct PS_INPUT
{
    [[vk::location(0)]] float4 color : COLOR0;
};


[earlydepthstencil]
float4 main(PS_INPUT input) : SV_TARGET
{
    return input.color;
}