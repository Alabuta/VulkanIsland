#if 0
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

/*layout (location = 0)*/ in VS_DATA
{
    vec2 position;
    vec2 texcoord;
} vs_data[];

out GS_DATA {
    noperspective vec2 position;
    vec2 texCoord;

    noperspective vec4 edgeA;
    noperspective vec4 edgeB;

    flat uint mask;
} gs_data;

const uint infoA[] = {0, 0, 0, 0, 1, 1, 2};
const uint infoB[] = {1, 1, 2, 0, 2, 1, 2};

const uint infoAd[] = {2, 2, 1, 1, 0, 0, 0};
const uint infoBd[] = {2, 2, 1, 2, 0, 2, 1};


void process(in vec3 position, in vec3 normal)
{
    gl_Position = camera.view * object.world * vec4(position, 1.0);
    gl_Position = camera.projection * gl_Position;

    vec4 worldSpaceNormal = object.normal * vec4(normal, 0.0);
    vec4 viewSpaceNormal = camera.view * worldSpaceNormal;

    outColor = vec4(normalize(vec3(viewSpaceNormal)), 1.0);
}

#pragma technique(0)
{
    vec3 normal = unpackAttribute(NORMAL);

    process(POSITION, normal);
}
#endif
