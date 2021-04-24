#include "Common.hlsli"


float4 main(SimplePixelShaderInput input) : SV_Target
{
	// Output the value that would go in the depth buffer to the pixel colour (greyscale)
    // The pow just rescales the values so they are easier to visualise. Although depth values range from 0 to 1 most
    // values are close to 1, which might give the (mistaken) impression that the depth buffer is empty
    return pow(input.projectedPosition.z, 20);
}