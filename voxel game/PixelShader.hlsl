Texture2D tex;
SamplerState sample;

float4 main(float2 tc : TexCoord) : SV_TARGET
{
    return tex.Sample(sample,tc);
}