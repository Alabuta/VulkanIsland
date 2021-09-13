#include "common.glsl"

layout (early_fragment_tests) in;

layout (location = 1) in GS_DATA {
    noperspective vec2 position;
    vec2 texcoord;

    noperspective vec4 edgeA;
    noperspective vec4 edgeB;

    flat uint mask;
} gs_data;

layout (location = 0) out vec4 outColor;

/*uniform*/ vec4 mainColor = vec4(0.8, 0.8, 0.8, 1.0);

/*uniform*/ vec4 wireColor = vec4(0, 0.64, 0, 1.0);

float getDistanceToEdges()
{
    float dist = 0.0;

    if (gs_data.mask == 0) {
        dist = min(min(gs_data.edgeA.x, gs_data.edgeA.y), gs_data.edgeA.z);
    }

    else {
        const vec2 AF = gs_data.position.xy - gs_data.edgeA.xy;
        const float sqAF = dot(AF, AF);
        const float AFcosA = dot(AF, gs_data.edgeA.zw);

        dist = abs(sqAF - AFcosA * AFcosA);

        const vec2 BF = gs_data.position.xy - gs_data.edgeB.xy;
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
{
    // Find the smallest distance
    const float dist = getDistanceToEdges();

    const float fading = clamp(FADE_DISTANCE / gl_FragCoord.w, 0, 1);

    const float mix_val = clamp(smoothstep(WIRE_WIDTH - 1, WIRE_WIDTH + 1, dist) + fading, 0, 1);

    outColor = mix(wireColor, mainColor, mix_val);
}
