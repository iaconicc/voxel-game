cbuffer TransformationBuff{
    float4x4 transform;
    float4x4 projection;
    float4x4 view;
};

struct VSOut
{
    float2 textureCoords : TEXCOORD;
    float3 WorldPos : WORLDPOS;
    float4 pos : SV_Position;
};

VSOut main(int4 pos : POSITION, uint texID : TEXMETA)
{
    VSOut vso;
   
    float4 localPos = float4(float3(pos.xyz) - 0.5f, 1.0f);
    
    //translate to world space
    float4 worldpos = mul(localPos, transform);
    vso.WorldPos = worldpos;
    
    //translate to screenspace
    float4 viewspace = mul(worldpos, view);
    vso.pos = mul(viewspace, projection);
    
    //calculate tex coords
    
    //extracting local uv from top 2 bits of texID
    uint textureID = texID & 0x3FFF;
    uint uvBits = texID >> 14;
    float2 localUV = float2(uvBits & 1, (uvBits>>1) & 1);
    
    //scale down the uv to one block of the texture atlas   
    float2 scaledUV = localUV * 0.25f;
    
    //calculate offsets on atlas
    float tileX = textureID % (uint) 4;
    float tileY = textureID / (uint) 4;
    
    //add offsets to scaled offsets
    float2 uvOffsets = float2(tileX * 0.25, tileY * 0.25) + scaledUV;
    
    vso.textureCoords = uvOffsets;
    
    return vso;
}