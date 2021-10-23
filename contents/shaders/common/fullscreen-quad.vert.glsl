layout (location = 0) out vec2 texCoord;


#pragma technique(0)
{
    // http://www.dhpoware.com/demos/gl3FullScreenQuad.html
    // inVertex is in normalized device coordinates (NDC).
    // The (x, y) coordinates are in the range [-1, 1].
    gl_Position = vec4(POSITION, 0., 1.0);

    // To generate the texture coordinates for the resulting full screen
    // quad we need to transform the vertex position's coordinates from the
    // [-1, 1] range to the [0, 1] range. This is achieved with a scale and
    // a bias.
    texCoord = fma(POSITION, .5, .5);
}