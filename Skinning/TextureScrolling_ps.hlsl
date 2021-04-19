#include "Common.hlsli"

Texture2D DiffuseSpecularMap : register(t0);
SamplerState TexSampler : register(s0);

float4 main(LightingPixelShaderInput input) : SV_TARGET
{
    float2 newUV = input.uv;

    newUV.x *= Wiggle / 4;
    newUV.y *= Wiggle / 2;
	
    float4 textureColour = DiffuseSpecularMap.Sample(TexSampler, newUV);	
	
	float4 finalColour;
	finalColour.rgb = textureColour.rgb;
	finalColour.y = 0.5;
	finalColour.w = textureColour.w;
	
	return finalColour;
}