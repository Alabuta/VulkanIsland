struct VS_OUTPUT
{
	float4 sv_position : SV_POSITION;
    [[vk::location(0)]] float2 texCoord : TEXCOORD0;
};


#pragma technique(0)
VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    // http://www.dhpoware.com/demos/gl3FullScreenQuad.html
    // inVertex is in normalized device coordinates (NDC).
    // The (x, y) coordinates are in the range [-1, 1].
    output.sv_position = vec4(POSITION, 0., 1.0);

    // To generate the texture coordinates for the resulting full screen
    // quad we need to transform the vertex position's coordinates from the
    // [-1, 1] range to the [0, 1] range. This is achieved with a scale and
    // a bias.
    output.texCoord = fma(POSITION, .5, .5);

    return output;
}