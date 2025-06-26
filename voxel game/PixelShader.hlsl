cbuffer FogBuff{
    float4 fogcolour;
    float3 cameraPos;
    float FogStart;
    float FogEnd;
};

Texture2D tex;
SamplerState sample;

float ComputePlanarFog(float3 worldPos)
{
    float height = worldPos.y;
    float fogFactor = saturate((FogEnd - height) / (FogEnd - FogStart));
    return fogFactor;
}

float4 main(float2 textureCoords : TEXCOORD, float3 WorldPos : WORLDPOS) : SV_TARGET
{   
    float4 tempcolour = tex.Sample(sample, textureCoords);
    
    float fogFactor = ComputePlanarFog(WorldPos);
    tempcolour = lerp(fogcolour, tempcolour, fogFactor);
    
    return tempcolour;
}