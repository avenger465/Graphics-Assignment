
#include "Common.hlsli"




TexturePixelShaderInput main(TextureVertex modelVertex)
{
    TexturePixelShaderInput output;
	
	float4 modelPosition = float4(modelVertex.position, 1);

    // Multiply by the world matrix passed from C++ to transform the model vertex position into world space. 
    // In a similar way use the view matrix to transform the vertex from world space into view space (camera's point of view)
    // and then use the projection matrix to transform the vertex to 2D projection space (project onto the 2D screen)
	float4 worldPosition = mul(gWorldMatrix, modelPosition);

    // Also transform model normals into world space using world matrix - lighting will be calculated in world space
    // Pass this normal to the pixel shader as it is needed to calculate per-pixel lighting
	float4 modelNormal = float4(modelVertex.normal, 0); // For normals add a 0 in the 4th element to indicate it is a vector
	float3 worldNormal = mul(gWorldMatrix, modelNormal).xyz; // Only needed the 4th element to do this multiplication by 4x4 matrix...
               
	worldNormal = normalize(worldNormal);
	//... it is not needed for lighting so discard afterwards with the .xyz
	worldPosition.x += sin(modelPosition.z + Wiggle) * 0.6f;
    worldPosition.y += sin(modelPosition.x + Wiggle) * 0.6f;
	
	output.worldNormal = worldNormal;
	output.worldPosition = worldPosition.xyz; // Also pass world position to pixel shader for lighting

    // Pass texture coordinates (UVs) on to the pixel shader, the vertex shader doesn't need them
	output.uv = modelVertex.uv;
	
	output.Dimensions.x = 0.6; //Wiggle / 2;
	output.Dimensions.y = 0.6;
	
    float4 viewPosition = mul(gViewMatrix, worldPosition);
    output.projectedPosition = mul(gProjectionMatrix, viewPosition);
	
	return output;
}