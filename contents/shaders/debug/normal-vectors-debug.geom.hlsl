struct PER_CAMERA
{
    float4x4 view;
    float4x4 projection;

    float4x4 projectionView;

    float4x4 invertedView;
    float4x4 invertedProjection;
};

layout (set = 0, binding = 0) ConstantBuffer<PER_CAMERA> camera : register(b0, space0);

struct VS_INPUT
{
	float4 sv_position : SV_POSITION;
    [[vk::location(0)]] float3 normal : NORMAL;
};

struct GS_OUTPUT
{
	float4 position : SV_POSITION;
    [[vk::location(0)]] float4 color : COLOR;
};


const float4 normalsColor = float4(0., .4, .8, 1.);


#pragma technique(0)
[maxvertexcount(6)]
void main(triangle VS_INPUT input[3], inout LineStream<GS_OUTPUT> outstream)
{
    for (int i = 0; i < 3; ++i) {
        float4 sv_position = input[i].sv_position;
		float3 normal = input[i].normal;

		GS_OUTPUT output = (GS_OUTPUT)0;

        output.position = mul(camera.projection, sv_position);
        output.color = normalsColor;
        outstream.Append(output);

        output.position = mul(camera.projection, (sv_position + float4(normal, 0.0) * MAGNITUDE));
        output.color = normalsColor;
        outstream.Append(output);

        outstream.RestartStrip();
    }
}
