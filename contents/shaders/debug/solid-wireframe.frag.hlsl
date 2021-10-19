struct GS_DATA
{
	float4 sv_position : SV_POSITION;
    
    [[vk::location(0)]] noperspective float2 position;

    [[vk::location(1)]] noperspective float4 edgeA;
    [[vk::location(2)]] noperspective float4 edgeB;

    [[vk::location(3)]] linear uint mask;
};

const float4 mainColor = float4(0.8, 0.8, 0.8, 1.0);
const float4 wireColor = float4(0.0, 0.64, 0.0, 1.0);


float getDistanceToEdges(in GS_DATA gs_data)
{
    float dist = 0.0;

    if (gs_data.mask == 0) {
        dist = min(min(gs_data.edgeA.x, gs_data.edgeA.y), gs_data.edgeA.z);
    }

    else {
        const float2 AF = gs_data.position.xy - gs_data.edgeA.xy;
        const float sqAF = dot(AF, AF);
        const float AFcosA = dot(AF, gs_data.edgeA.zw);

        dist = abs(sqAF - AFcosA * AFcosA);

        const float2 BF = gs_data.position.xy - gs_data.edgeB.xy;
        const float sqBF = dot(BF, BF);
        const float BFcosB = dot(BF, gs_data.edgeB.zw);

        dist = min(dist, abs(sqBF - BFcosB * BFcosB));

        if (gs_data.mask == 1 || gs_data.mask == 2 || gs_data.mask == 4) {
            const float AFcosA0 = dot(AF, normalize(gs_data.edgeB.xy - gs_data.edgeA.xy));
            dist = min(dist, abs(sqAF - AFcosA0 * AFcosA0));
        }

        dist = sqrt(dist);
    }

    return dist;
}

#pragma technique(0)
[earlydepthstencil]
float4 main(GS_DATA gs_data) : SV_TARGET
{
    // Find the smallest distance
    const float dist = getDistanceToEdges(gs_data);

    const float fading = saturate(FADE_DISTANCE / gs_data.sv_position.w);

    const float mix_val = saturate(smoothstep(WIRE_WIDTH - 1.0, WIRE_WIDTH + 1.0, dist) + fading);

    return lerp(wireColor, mainColor, mix_val);
}