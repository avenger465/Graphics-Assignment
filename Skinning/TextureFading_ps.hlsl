#include "Common.hlsli"

Texture2D BrickDiffuseSpecularMap : register(t0);
Texture2D WoodDiffuseSpecularMap : register(t1);
SamplerState TexSampler : register(s0);


float4 main(LightingPixelShaderInput input) : SV_TARGET
{
	input.worldNormal		 = normalize(input.worldNormal);

	///////////////////////
	// Calculate lighting
    
    // Direction from pixel to camera
	float3 cameraDirection	 = normalize(gCameraPosition - input.worldPosition);

	//// Light 1 ////

	// Direction and distance from pixel to light
	float3 light1Direction	 = normalize(gLight1Position - input.worldPosition);
	float3 light1Dist		 = length(gLight1Position - input.worldPosition);
    
    // Equations from lighting lecture
	float3 diffuseLight1	 = gLight1Colour * max(dot(input.worldNormal, light1Direction), 0) / light1Dist;
	float3 halfway			 = normalize(light1Direction + cameraDirection);
	float3 specularLight1	 = diffuseLight1 * pow(max(dot(input.worldNormal, halfway), 0), gSpecularPower); // Multiplying by diffuseLight instead of light colour - my own personal preference


	//// Light 2 ////

	float3 light2Direction	 = normalize(gLight2Position - input.worldPosition);
	float3 light2Dist	     = length(gLight2Position - input.worldPosition);
	float3 diffuseLight2	 = gLight2Colour * max(dot(input.worldNormal, light2Direction), 0) / light2Dist;
	halfway					 = normalize(light2Direction + cameraDirection);
	float3 specularLight2	 = diffuseLight2 * pow(max(dot(input.worldNormal, halfway), 0), gSpecularPower);


	// Sum the effect of the lights - add the ambient at this stage rather than for each light (or we will get too much ambient)
	float3 diffuseLight		 = gAmbientColour + diffuseLight1 + diffuseLight2;
	float3 specularLight	 = specularLight1 + specularLight2;


	////////////////////
	// Combine textures

    // Sample diffuse material and specular material colour for this pixel from a texture using a given sampler that you set up in the C++ code
	float4 BrickColour = BrickDiffuseSpecularMap.Sample(TexSampler, input.uv);	
	float4 WoodColour  = WoodDiffuseSpecularMap.Sample(TexSampler, input.uv);
	
	float4 Lerpresult  = lerp(BrickColour, WoodColour, (sin(Wiggle)));
	
	float3 DiffuseMaterialColour = Lerpresult.rgb;
	float SpecularMaterialColour = Lerpresult.a;
	

    // Combine lighting with texture colours
	float3 finalColour = diffuseLight * DiffuseMaterialColour + specularLight * SpecularMaterialColour;

	return float4(finalColour, 1.0f); // Always use 1.0f for output alpha - no alpha blending in this lab
}