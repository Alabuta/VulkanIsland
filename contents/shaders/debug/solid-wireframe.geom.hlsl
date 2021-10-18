

struct VS_DATA
{
	float4 sv_position : SV_POSITION;
	[[vk::location(0)]] float2 vs_position : POSITION;
};

struct GS_OUTPUT
{
	float4 sv_position : SV_POSITION;
    
    [[vk::location(0)]] noperspective float2 position;

    [[vk::location(1)]] noperspective float4 edgeA;
    [[vk::location(2)]] noperspective float4 edgeB;

    [[vk::location(3)]] linear uint mask;
};

const uint infoA[] = {0, 0, 0, 0, 1, 1, 2};
const uint infoB[] = {1, 1, 2, 0, 2, 1, 2};

const uint infoAd[] = {2, 2, 1, 1, 0, 0, 0};
const uint infoBd[] = {2, 2, 1, 2, 0, 2, 1};


void simpleCase(triangle VS_DATA input[3], inout GS_OUTPUT output, inout TriangleStream<GS_OUTPUT> outstream)
{
    // Edge vectors of the transformed triangle.
    const float2 ab = input[1].vs_position - input[0].vs_position;
    const float2 bc = input[2].vs_position - input[1].vs_position;
    const float2 ca = input[0].vs_position - input[2].vs_position;

    const float area = abs(fma(ab.x, ca.y, -ab.y * ca.x));

    // Vertices' heights.
    const float ha = area / length(bc);
    const float hb = area / length(ca);
    const float hc = area / length(ab);

    output.edgeA = float4(ha, 0.0, 0.0, 0.0);
    output.sv_position = input[0].sv_position;
    outstream.Append(output);

    output.edgeA = float4(0.0, hb, 0.0, 0.0);
    output.sv_position = input[1].sv_position;
    outstream.Append(output);

    output.edgeA = float4(0.0, 0.0, hc, 0.0);
    output.sv_position = input[2].sv_position;
    outstream.Append(output);
}

#pragma technique(0)
[maxvertexcount(3)]
void main(triangle VS_DATA input[3], inout TriangleStream<GS_OUTPUT> outstream)
{
    GS_OUTPUT output = (GS_OUTPUT)0;

    output.mask = (input[0].sv_position.z < 0.0 ? 4 : 0) + (input[1].sv_position.z < 0.0 ? 2 : 0) + (input[2].sv_position.z < 0.0 ? 1 : 0);

    // All vertices behind the viewport.
    /*if (output.mask == 7)
        return;*/

    if (output.mask == 0) {
        simpleCase(input, output, outstream);
    }

    else {
        output.edgeA.xy = input[infoA[output.mask]].vs_position;
        output.edgeB.xy = input[infoB[output.mask]].vs_position;

        output.edgeA.zw = normalize(output.edgeA.xy - input[infoAd[output.mask]].vs_position);
        output.edgeB.zw = normalize(output.edgeB.xy - input[infoBd[output.mask]].vs_position);

        for (int i = 0; i < 3; ++i) {
            output.position = input[i].vs_position;
            output.sv_position = input[i].sv_position;

            outstream.Append(output);
        }
    }

    outstream.RestartStrip();
}
