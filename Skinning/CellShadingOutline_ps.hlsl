#include "Common.hlsli"

//Return the outline colour that was define in the C++ side for each pixel
float4 main(SimplePixelShaderInput input) : SV_TARGET
{
	return float4(gOutlineColour, 1.0f);
}