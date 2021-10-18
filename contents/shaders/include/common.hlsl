
float2 normalizedToViewport(in int4 screenRect, in float2 position)
{
    return float2(screenRect.z * 0.5 * (position.x + 1.0) + screenRect.x,
                  screenRect.w * 0.5 * (position.y + 1.0) + screenRect.y);
}