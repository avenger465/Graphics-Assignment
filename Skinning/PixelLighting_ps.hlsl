//--------------------------------------------------------------------------------------
// Per-Pixel Lighting Pixel Shader
//--------------------------------------------------------------------------------------
// Pixel shader receives position and normal from the vertex shader and uses them to calculate
// lighting per pixel. Also samples a samples a diffuse + specular texture map and combines with light colour.

#include "Common.hlsli" // Shaders can also use include files - note the extension


//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// Here we allow the shader access to a texture that has been loaded from the C++ side and stored in GPU memory.
// Note that textures are often called maps (because texture mapping describes wrapping a texture round a mesh).
// Get used to people using the word "texture" and "map" interchangably.
Texture2D DiffuseSpecularMap : register(t0); // Textures here can contain a diffuse map (main colour) in their rgb channels and a specular map (shininess) in the a channel
SamplerState TexSampler      : register(s0); // A sampler is a filter for a texture like bilinear, trilinear or anisotropic - this is the sampler used for the texture above

Texture2D ShadowMapLight1 : register(t1);
SamplerState PointClamp : register(s1);


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Pixel shader entry point - each shader has a "main" function
// This shader just samples a diffuse texture map
float4 main(LightingPixelShaderInput input) : SV_Target
{
    const float DepthAdjust = 0.0005f;
    
    // Normal might have been scaled by model scaling or interpolation so renormalise
    input.worldNormal = normalize(input.worldNormal); 

	///////////////////////
	// Calculate lighting
    
    // Direction from pixel to camera
    float3 cameraDirection = normalize(gCameraPosition - input.worldPosition);

	//// Light 1 ////

	// Direction and distance from pixel to light
	float3 light1Direction = normalize(Light1.Position - input.worldPosition);
    float3 light1Dist = length(Light1.Position - input.worldPosition);
    
    // Equations from lighting lecture
    float3 diffuseLight1 = Light1.Colour * max(dot(input.worldNormal, light1Direction), 0) / light1Dist;
    float3 halfway = normalize(light1Direction + cameraDirection);
    float3 specularLight1 =  diffuseLight1 * pow(max(dot(input.worldNormal, halfway), 0), gSpecularPower); // Multiplying by diffuseLight instead of light colour - my own personal preference


	//// Light 2 ////

	float3 light2Direction = normalize(Light2.Position - input.worldPosition);
    float3 light2Dist = length(Light2.Position - input.worldPosition);
    float3 diffuseLight2 = Light2.Colour * max(dot(input.worldNormal, light2Direction), 0) / light2Dist; 
    halfway = normalize(light2Direction + cameraDirection);
    float3 specularLight2 =  diffuseLight2 * pow(max(dot(input.worldNormal, halfway), 0), gSpecularPower);

    //// Light 3 ////
    
    float3 light3Direction = normalize(Light3.Position - input.worldPosition);
    float3 diffuseLight3 = 0;
    float3 specularLight3 = 0;
    
    if (dot(Light3.Direction, -light3Direction) > Light3.CosHalfAngle)
    {
        
        float3 light3Direction = normalize(Light3.Position - input.worldPosition);
        float3 light3Dist = length(Light3.Position - input.worldPosition);
        diffuseLight3 = Light3.Colour * max(dot(input.worldNormal, light3Direction), 0) / light3Dist;
        halfway = normalize(light3Direction + cameraDirection);
        specularLight3 = diffuseLight3 * pow(max(dot(input.worldNormal, halfway), 0), gSpecularPower);
    }
    

	// Sum the effect of the lights - add the ambient at this stage rather than for each light (or we will get too much ambient)
    float3 diffuseLight = gAmbientColour + diffuseLight1 + diffuseLight2 + diffuseLight3;
	float3 specularLight = specularLight1 + specularLight2 + specularLight3;


	////////////////////
	// Combine lighting and textures

    // Sample diffuse material and specular material colour for this pixel from a texture using a given sampler that you set up in the C++ code
    float4 textureColour = DiffuseSpecularMap.Sample(TexSampler, input.uv);
    float3 diffuseMaterialColour = textureColour.rgb; // Diffuse material colour in texture RGB (base colour of model)
    float specularMaterialColour = textureColour.a;   // Specular material colour in texture A (shininess of the surface)

    //if (textureColour.a < 0.5)
    //{
    //    discard;
    //}
    
    
    // Combine lighting with texture colours
        float3 finalColour = diffuseLight * diffuseMaterialColour + specularLight * specularMaterialColour;

    return float4(finalColour, 1.0f); // Always use 1.0f for output alpha - no alpha blending in this lab
}