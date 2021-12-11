struct PER_CAMERA
{
    float4x4 view;
    float4x4 projection;

    float4x4 projectionView;

    float4x4 invertedView;
    float4x4 invertedProjection;
};

struct PER_OBJECT
{
    float4x4 world;
    float4x4 normal;
};

float2 normalizedToViewport(in int4 screenRect, in float2 position)
{
    return float2(screenRect.z * 0.5 * (position.x + 1.0) + screenRect.x,
                  screenRect.w * 0.5 * (position.y + 1.0) + screenRect.y);
}