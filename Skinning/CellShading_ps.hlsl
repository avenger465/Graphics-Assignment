#include "Common.hlsli"

Texture2D DiffuseMap : register(t0); // Diffuse map only
Texture2D CellMap : register(t1); // CellMap is a 1D map that is used to limit the range of colours used in cell shading

SamplerState TexSampler : register(s0); // Sampler for use on textures
SamplerState PointSampleClamp : register(s1); // No filtering of cell maps (otherwise the cell edges would be blurred)




float4 main(LightingPixelShaderInput input) : SV_TARGET
{
    // Lighting equations
    input.worldNormal = normalize(input.worldNormal); // Normal might have been scaled by model scaling or interpolation so renormalise
    float3 cameraDirection = normalize(gCameraPosition - input.worldPosition);

    // Light 1
    float3 light1Vector = gLight1Position - input.worldPosition;
    float light1Distance = length(light1Vector);
    float3 light1Direction = light1Vector / light1Distance; // Quicker than normalising as we have length for attenuation
	
	//****| INFO |*************************************************************************************//
	// To make a cartoon look to the lighting, we clamp the basic light level to just a small range of
	// colours. This is done by using the light level itself as the U texture coordinate to look up
	// a colour in a special 1D texture (a single line). This could be done with if statements, but
	// GPUs are much faster at looking up small textures than if statements
	//*************************************************************************************************//
    float diffuseLevel1 = max(dot(input.worldNormal, light1Direction), 0);
    float cellDiffuseLevel1 = CellMap.Sample(PointSampleClamp, diffuseLevel1).r;
    float3 diffuseLight1 = gLight1Colour * cellDiffuseLevel1 / light1Distance;

    float3 halfway = normalize(light1Direction + cameraDirection);
    float3 specularLight1 = diffuseLight1 * pow(max(dot(input.worldNormal, halfway), 0), gSpecularPower); // Multiplying by diffuseLight instead of light colour - my own personal preference

    // Light 2
    float3 light2Vector = gLight2Position - input.worldPosition;
    float light2Distance = length(light2Vector);
    float3 light2Direction = light2Vector / light2Distance;

    float diffuseLevel2 = max(dot(input.worldNormal, light2Direction), 0);
    float cellDiffuseLevel2 = CellMap.Sample(PointSampleClamp, diffuseLevel2).b;
    float3 diffuseLight2 = gLight2Colour * cellDiffuseLevel2 / light2Distance;

    halfway = normalize(light2Direction + cameraDirection);
    float3 specularLight2 = diffuseLight2 * pow(max(dot(input.worldNormal, halfway), 0), gSpecularPower);

    // Sample diffuse material colour for this pixel from a texture using a given sampler that you set up in the C++ code
    // Ignoring any alpha in the texture, just reading RGB
    float4 textureColour = DiffuseMap.Sample(TexSampler, input.uv);
    float3 diffuseMaterialColour = textureColour.rgb;
    float specularMaterialColour = textureColour.a;

    float3 finalColour = (gAmbientColour + diffuseLight1 + diffuseLight2) * diffuseMaterialColour +
                         (specularLight1 + specularLight2) * specularMaterialColour;

    return float4(finalColour, 1.0f); // Always use 1.0f for alpha - no alpha blending in this lab
}