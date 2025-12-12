[[vk::image_format("rgba8")]]
RWTexture2D<float4> uOutputTexture : register(u0, space1);

Texture2D<float4> uFrame : register(t0, space0);

cbuffer uDenoiserParameters : register(b0, space2)
{
    float emaSmoothingFactor;
};

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // Temporal Accumulation using expontential average
    uOutputTexture[DTid.xy] = (uFrame[DTid.xy] * emaSmoothingFactor) + (uOutputTexture[DTid.xy] * (1 - emaSmoothingFactor));
}