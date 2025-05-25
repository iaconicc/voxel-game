cbuffer TransformationBuff{
    float4x4 transform;
    float4x4 projection;
    float4x4 view;
};

struct VSOut
{
    float2 tex : TexCoord;
    float4 pos : SV_Position;
};

VSOut main(float3 pos : POSITION, float2 tex : TexCoord)
{
    VSOut vso;
    //expand to 4 floats
    float4 worldpos = float4(pos.x, pos.y, pos.z, 1.0f);
    
    //translate to world space
    worldpos = mul(worldpos, transform);
    
    //translate to screenspace
    float4 viewspace = mul(worldpos, view);
    vso.pos = mul(viewspace, projection);
        
    //add tex coord to output    
    vso.tex = tex;
    
    return vso;
}