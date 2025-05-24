cbuffer TransformationBuff{
    float4x4 transform;
    float4x4 projection;
};

float4 main( float3 pos : POSITION ) : SV_POSITION
{
    float4 worldpos = float4(pos.x, pos.y, pos.z, 1.0f);
    worldpos = mul(worldpos, transform);
    return mul(worldpos, projection);
}