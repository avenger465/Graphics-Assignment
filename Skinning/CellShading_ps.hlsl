#include "Common.hlsli"

Texture2D DiffuseMap : register(t0); // Diffuse map only
Texture2D CellMap : register(t2); // CellMap is a 1D map that is used to limit the range of colours used in cell shading

Texture2D ShadowMapLight1 : register(t1);
SamplerState TexSampler : register(s0); // Sampler for use on textures
SamplerState PointSampleClamp : register(s1); // No filtering of cell maps (otherwise the cell edges would be blurred)

float4 main(LightingPixelShaderInput input) : SV_TARGET
{
    // Lighting equations
    input.worldNormal = normalize(input.worldNormal); // Normal might have been scaled by model scaling or interpolation so renormalise
    float3 cameraDirection = normalize(gCameraPosition - input.worldPosition);

    // Light 1
    float3 light1Vector = Light1.Position - input.worldPosition;
    float light1Distance = length(light1Vector);
    float3 light1Direction = light1Vector / light1Distance; // Quicker than normalising as we have length for attenuation
	
    float diffuseLevel1 = max(dot(input.worldNormal, light1Direction), 0);
    float cellDiffuseLevel1 = CellMap.Sample(PointSampleClamp, diffuseLevel1).r;
    float3 diffuseLight1 = Light1.Colour * cellDiffuseLevel1 / light1Distance;

    float3 halfway = normalize(light1Direction + cameraDirection);
    float3 specularLight1 = diffuseLight1 * pow(max(dot(input.worldNormal, halfway), 0), gSpecularPower); // Multiplying by diffuseLight instead of light colour - my own personal preference

    // Light 2
    float3 light2Vector = Light2.Position - input.worldPosition;
    float light2Distance = length(light2Vector);
    float3 light2Direction = light2Vector / light2Distance;

    float diffuseLevel2 = max(dot(input.worldNormal, light2Direction), 0);
    float cellDiffuseLevel2 = CellMap.Sample(PointSampleClamp, diffuseLevel2).r;
    float3 diffuseLight2 = Light2.Colour * cellDiffuseLevel2 / light2Distance;

    halfway = normalize(light2Direction + cameraDirection);
    float3 specularLight2 = diffuseLight2 * pow(max(dot(input.worldNormal, halfway), 0), gSpecularPower);
    
    //Light 3
    float3 light3Direction = normalize(Light3.Position - input.worldPosition);
    float3 diffuseLight3 = 0;
    float3 specularLight3 = 0;
    
    if (dot(Light3.Direction, -light3Direction) > Light3.CosHalfAngle)
    {
        float4 light3ViewPosition = mul(Light3.lightViewMatrix, float4(input.worldPosition, 1.0f));
        float4 light3Projection = mul(Light3.lightProjectionMatrix, light3ViewPosition);
        
        float2 ShadowMapUV = 0.5f * light3Projection.xy / light3Projection.w + float2(0.5f, 0.5f);
        ShadowMapUV.y = 1.0f - ShadowMapUV.y;
        
        float depthFromLight3 = light3Projection.z / light3Projection.w - DepthAdjust;
        if (depthFromLight3 < ShadowMapLight1.Sample(PointSampleClamp, ShadowMapUV).r)
        {
            float3 light3Dist = length(Light3.Position - input.worldPosition);
            diffuseLight3 = Light3.Colour * max(dot(input.worldNormal, light3Direction), 0) / light3Dist;
            halfway = normalize(light3Direction + cameraDirection);
            specularLight3 = diffuseLight3 * pow(max(dot(input.worldNormal, halfway), 0), gSpecularPower);
        }
    }
    
    //light 4
    float3 light4Vec = -(normalize(Light4.Direction));
    float3 diffuseLight4 = Light4.Colour * max(dot(light4Vec, input.worldNormal), 0.0f);
    halfway = normalize(light4Vec + cameraDirection);
    float3 specularLight4 = diffuseLight4 * pow(max(dot(input.worldNormal, halfway), 0), gSpecularPower);
    
    //total lighting
    float3 diffuseLight = gAmbientColour + diffuseLight1 + diffuseLight2 + diffuseLight3 + diffuseLight4;
    float3 specularLight = specularLight1 + specularLight2 + specularLight3 + specularLight4;
   
    // Sample diffuse material colour for this pixel from a texture using a given sampler that you set up in the C++ code
    // Ignoring any alpha in the texture, just reading RGB
    float4 textureColour = DiffuseMap.Sample(TexSampler, input.uv);
    float3 diffuseMaterialColour = textureColour.rgb;
    float specularMaterialColour = textureColour.a;

    float3 finalColour = diffuseLight * diffuseMaterialColour + specularLight * specularMaterialColour;

    return float4(finalColour, 1.0f); // Always use 1.0f for alpha - no alpha blending in this lab
}