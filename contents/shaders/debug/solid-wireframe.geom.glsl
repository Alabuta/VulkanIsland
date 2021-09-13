layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

layout (location = 0) in VS_DATA
{
    vec2 position;
    vec2 texcoord;
} vs_data[];

layout (location = 1) out GS_DATA {
    noperspective vec2 position;
    vec2 texcoord;

    noperspective vec4 edgeA;
    noperspective vec4 edgeB;

    flat uint mask;
} gs_data;

const uint infoA[] = {0, 0, 0, 0, 1, 1, 2};
const uint infoB[] = {1, 1, 2, 0, 2, 1, 2};

const uint infoAd[] = {2, 2, 1, 1, 0, 0, 0};
const uint infoBd[] = {2, 2, 1, 2, 0, 2, 1};


void simpleCase()
{
    // Edge vectors of the transformed triangle.
    const vec2 ab = vs_data[1].position - vs_data[0].position;
    const vec2 bc = vs_data[2].position - vs_data[1].position;
    const vec2 ca = vs_data[0].position - vs_data[2].position;

    const float area = abs(fma(ab.x, ca.y, -ab.y * ca.x));

    // Vertices' heights.
    const float ha = area / length(bc);
    const float hb = area / length(ca);
    const float hc = area / length(ab);

    gs_data.edgeA = vec4(ha, 0.0, 0.0, 0.0);
    gs_data.texcoord = vs_data[0].texcoord;

    gl_Position = gl_in[0].gl_Position;
    EmitVertex();


    gs_data.edgeA = vec4(0.0, hb, 0.0, 0.0);
    gs_data.texcoord = vs_data[1].texcoord;

    gl_Position = gl_in[1].gl_Position;
    EmitVertex();


    gs_data.edgeA = vec4(0.0, 0.0, hc, 0.0);
    gs_data.texcoord = vs_data[2].texcoord;

    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
}


void process()
{
    gs_data.mask = (gl_in[0].gl_Position.z < 0.0 ? 4 : 0) + (gl_in[1].gl_Position.z < 0.0 ? 2 : 0) + (gl_in[2].gl_Position.z < 0.0 ? 1 : 0);

    // All vertices behind the viewport.
    /*if (gs_data.mask == 7)
        return;*/

    if (gs_data.mask == 0) {
        simpleCase();
    }

    else {
        gs_data.edgeA.xy = vs_data[infoA[gs_data.mask]].position;
        gs_data.edgeB.xy = vs_data[infoB[gs_data.mask]].position;

        gs_data.edgeA.zw = normalize(gs_data.edgeA.xy - vs_data[infoAd[gs_data.mask]].position);
        gs_data.edgeB.zw = normalize(gs_data.edgeB.xy - vs_data[infoBd[gs_data.mask]].position);

        for (int i = 0; i < 3; ++i) {
            gs_data.position = vs_data[i].position;
            gs_data.texcoord = vs_data[i].texcoord;

            gl_Position = gl_in[i].gl_Position;

            EmitVertex();
        }
    }

    EndPrimitive();
}

#pragma technique(0)
{
    process();
}
