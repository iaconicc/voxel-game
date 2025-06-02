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

float4 main(float2 tc : TexCoord, float3 WorldPos : WORLDPOS) : SV_TARGET
{   
    float4 tempColour = tex.Sample(sample, tc);
    
    float fogFactor = calculateExpfogFactor(WorldPos);
    tempColour = lerp(fogcolour, tempColour, fogFactor);
    
    return tempColour;
}