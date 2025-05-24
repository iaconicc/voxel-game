cbuffer TransformationBuff{
    float4x4 transform;
    float4x4 projection;
    float4x4 view;
};

float4 main( float3 pos : POSITION ) : SV_POSITION
{
    float4 worldpos = float4(pos.x, pos.y, pos.z, 1.0f);
    worldpos = mul(worldpos, transform);
    float4 viewspace = mul(worldpos, view);
    return mul(viewspace, projection);
}