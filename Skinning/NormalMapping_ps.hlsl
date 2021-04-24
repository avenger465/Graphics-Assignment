#include "Common.hlsli"

Texture2D DiffuseSpecularMap : register(t0);
SamplerState TexSampler : register(s0);

Texture2D ShadowMapLight1 : register(t1);
SamplerState PointClamp : register(s1);
Texture2D NormalMap : register(t2);


float4 main(NormalMappingPixelShaderInput input) : SV_Target
{
	//************************
	// Normal Map Extraction
	//************************

	// Will use the model normal/tangent to calculate matrix for tangent space. The normals for each pixel are *interpolated* from the
	// vertex normals/tangents. This means they will not be length 1, so they need to be renormalised (same as per-pixel lighting issue)
    float3 modelNormal = normalize(input.modelNormal);
    float3 modelTangent = normalize(input.modelTangent);

	// Calculate bi-tangent to complete the three axes of tangent space - then create the *inverse* tangent matrix to convert *from*
	// tangent space into model space. This is just a matrix built from the three axes (very advanced note - by default shader matrices
	// are stored as columns rather than in rows as in the C++. This means that this matrix is created "transposed" from what we would
	// expect. However, for a 3x3 rotation matrix the transpose is equal to the inverse, which is just what we require)
    float3 modelBiTangent = cross(modelNormal, modelTangent);
    float3x3 invTangentMatrix = float3x3(modelTangent, modelBiTangent, modelNormal);
	
	// Get the texture normal from the normal map. The r,g,b pixel values actually store x,y,z components of a normal. However, r,g,b
	// values are stored in the range 0->1, whereas the x, y & z components should be in the range -1->1. So some scaling is needed
    
    float3 textureNormal = (2.0f * NormalMap.Sample(TexSampler, input.uv).rgb - 1.0f) * 1000; // Scale from 0->1 to -1->1

	// Now convert the texture normal into model space using the inverse tangent matrix, and then convert into world space using the world
	// matrix. Normalise, because of the effects of texture filtering and in case the world matrix contains scaling
    float3 worldNormal = normalize(mul((float3x3) gWorldMatrix, mul(textureNormal, invTangentMatrix)));


	///////////////////////
	// Calculate lighting

   // Lighting equations
    float3 cameraDirection = normalize(gCameraPosition - input.worldPosition);

    // Light 1
    float3 light1Vector = Light1.Position - input.worldPosition;
    float light1Distance = length(light1Vector);
    float3 light1Direction = light1Vector / light1Distance; // Quicker than normalising as we have length for attenuation
    float3 diffuseLight1 = Light1.Colour * max(dot(worldNormal, light1Direction), 0) / light1Distance;

    float3 halfway = normalize(light1Direction + cameraDirection);
    float3 specularLight1 = diffuseLight1 * pow(max(dot(worldNormal, halfway), 0), gSpecularPower);


    // Light 2
    float3 light2Vector = Light2.Position - input.worldPosition;
    float light2Distance = length(light2Vector);
    float3 light2Direction = light2Vector / light2Distance;
    float3 diffuseLight2 = Light2.Colour * max(dot(worldNormal, light2Direction), 0) / light2Distance;

    halfway = normalize(light2Direction + cameraDirection);
    float3 specularLight2 = diffuseLight2 * pow(max(dot(worldNormal, halfway), 0), gSpecularPower);

    
    float3 light3Direction = normalize(Light3.Position - input.worldPosition);
    float3 diffuseLight3 = 0;
    float3 specularLight3 = 0;
    
    if (dot(Light3.Direction, -light3Direction) > Light3.CosHalfAngle)
    {
        if (dot(Light3.Direction, -light3Direction) > Light3.CosHalfAngle)
        {
            float4 light3ViewPosition = mul(Light3.lightViewMatrix, float4(input.worldPosition, 1.0f));
            float4 light3Projection = mul(Light3.lightProjectionMatrix, light3ViewPosition);
        
            float2 ShadowMapUV = 0.5f * light3Projection.xy / light3Projection.w + float2(0.5f, 0.5f);
            ShadowMapUV.y = 1.0f - ShadowMapUV.y;
        
            float depthFromLight3 = light3Projection.z / light3Projection.w - DepthAdjust;
            if (depthFromLight3 < ShadowMapLight1.Sample(PointClamp, ShadowMapUV).r)
            {
                float3 light3Dist = length(Light3.Position - input.worldPosition);
                diffuseLight3 = Light3.Colour * max(dot(worldNormal, light3Direction), 0) / light3Dist;
                halfway = normalize(light3Direction + cameraDirection);
                specularLight3 = diffuseLight3 * pow(max(dot(worldNormal, halfway), 0), gSpecularPower);
            }
        }
    }

    
    float3 light4Vec = -(normalize(Light4.Direction));
    float3 diffuseLight4 = Light4.Colour * max(dot(light4Vec, worldNormal), 0.0f);
    halfway = normalize(light4Vec + cameraDirection);
    float3 specularLight4 = diffuseLight4 * pow(max(dot(worldNormal, halfway), 0), gSpecularPower);

	// Sum the effect of the lights - add the ambient at this stage rather than for each light (or we will get too much ambient)
    float3 diffuseLight = gAmbientColour + diffuseLight1 + diffuseLight2 + diffuseLight3 + diffuseLight4;
    float3 specularLight = specularLight1 + specularLight2 + specularLight3 + specularLight4;

    // Sample diffuse material colour for this pixel from a texture using a given sampler that you set up in the C++ code
    // Ignoring any alpha in the texture, just reading RGB
    float4 textureColour = DiffuseSpecularMap.Sample(TexSampler, input.uv);
    float3 diffuseMaterialColour = textureColour.rgb;
    float specularMaterialColour = textureColour.a;

    float3 finalColour = diffuseLight * diffuseMaterialColour + specularLight * specularMaterialColour;

    return float4(finalColour, 1.0f); // Always use 1.0f for alpha - no alpha blending in this lab
}