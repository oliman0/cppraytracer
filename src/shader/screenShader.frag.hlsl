struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

Texture2D uTexture : register(t0, space2);

SamplerState samplerLinear : register(s0, space2);

float4 main(PSInput input) : SV_TARGET
{
    float4 colour = uTexture.Sample(samplerLinear, input.texCoord);
    
    return colour;
}