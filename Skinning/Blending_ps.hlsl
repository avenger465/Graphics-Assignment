#include "Common.hlsli"

Texture2D DiffuseSpecularMap : register(t0);
SamplerState TexSampler : register(s0);

float4 main(LightingPixelShaderInput input) : SV_TARGET
{
    float4 diffuseMapColour = DiffuseSpecularMap.Sample(TexSampler, input.uv);
    if (diffuseMapColour.a < 0.5)
    {
        discard;
    }
    return diffuseMapColour;
}