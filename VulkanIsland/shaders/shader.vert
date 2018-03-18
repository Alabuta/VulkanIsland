#version 450
#extension GL_ARB_separate_shader_objects : enable

const vec2 positions[3] = vec2[](vec2(1, 1), vec2(0, -1), vec2(-1, 1));
const vec3 colors[3] = vec3[](vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0));

out gl_PerVertex{
    vec4 gl_Position;
};

layout(location = 0) out vec3 perVertexColor;

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    perVertexColor = colors[gl_VertexIndex];
}