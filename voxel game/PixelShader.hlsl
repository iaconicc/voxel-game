cbuffer FogBuff{
    float4 fogcolour;
    float3 cameraPos;
    float FogDesity;
    float FogEnd;
};

Texture2D tex;
SamplerState sample;

float calculateExpfogFactor(float3 worldPos)
{
    float CameraToPixelDist = length(worldPos - cameraPos);
    float DistRatio = 4.0 * CameraToPixelDist / FogEnd;
    float fogFactor = exp(-DistRatio * FogDesity);
    return fogFactor;
}

float4 main(float2 textureCoords : TEXCOORD, float3 WorldPos : WORLDPOS) : SV_TARGET
{   
    float4 tempcolour = tex.Sample(sample, textureCoords);
    
    float fogFactor = calculateExpfogFactor(WorldPos);
    tempcolour = lerp(fogcolour, tempcolour, fogFactor);
    
    return tempcolour;
}