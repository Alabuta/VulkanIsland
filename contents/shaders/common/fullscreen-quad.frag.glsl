layout (early_fragment_tests) in;

layout (binding = 0) uniform sampler2D mainTexture;

layout (location = 0) in vec2 texCoord;

layout (location = 0) out vec4 fragColor;

void main()
{
    fragColor = texture(mainTexture, texCoord);
    fragColor.a = 1;
}