
vec2 normalizedToViewport(in ivec4 screenRect, in vec2 position)
{
    return vec2(screenRect.z * 0.5 * (position.x + 1.0) + screenRect.x,
                screenRect.w * 0.5 * (position.y + 1.0) + screenRect.y);
}