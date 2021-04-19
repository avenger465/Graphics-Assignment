#include "Common.hlsli"

float4 main(SimplePixelShaderInput input) : SV_TARGET
{
	return float4(gOutlineColour, 1.0f);
}