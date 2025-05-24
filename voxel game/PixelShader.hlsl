float random(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

float4 main(float4 position : SV_POSITION) : SV_TARGET
{
    // Generate random values for red, green, and blue channels
    float r = position.x / position.y;
    float g = position.y / position.x;
    
	return float4(r, g, 0.0f, 1.0f);
}