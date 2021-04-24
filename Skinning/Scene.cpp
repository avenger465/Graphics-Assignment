//--------------------------------------------------------------------------------------
// Scene geometry and layout preparation
// Scene rendering & update
//--------------------------------------------------------------------------------------

#include "Scene.h"
#include "CTexture.h"
#include "Mesh.h"
#include "Model.h"
#include "Camera.h"
#include "State.h"
#include "Shader.h"
#include "Input.h"
#include "Common.h"
#include "CLight.h"

#include "CVector2.h" 
#include "CVector3.h" 
#include "CMatrix4x4.h"
#include "MathHelpers.h"     // Helper functions for maths
#include "GraphicsHelpers.h" // Helper functions to unclutter the code here

#include "ColourRGBA.h" 

#include <sstream>
#include <memory>


//--------------------------------------------------------------------------------------
// Scene Data
//--------------------------------------------------------------------------------------
// Addition of Mesh, Model and Camera classes have greatly simplified this section
// Geometry data has gone to Mesh class. Positions, rotations, matrices have gone to Model and Camera classes

// Constants controlling speed of movement/rotation (measured in units per second because we're using frame time)
const float ROTATION_SPEED = 2.0f;  // 2 radians per second for rotation
const float MOVEMENT_SPEED = 50.0f; // 50 units per second for movement (what a unit of length is depends on 3D model - i.e. an artist decision usually)

//Strength of the wiggle effect 
const int WIGGLESTRENGTH = 2;

// Meshes, models and cameras, same meaning as TL-Engine. Meshes prepared in InitGeometry function, Models & camera in InitScene
Mesh* gTeapotMesh;
Mesh* gGroundMesh;
Mesh* gNormalMappingMesh;
Mesh* gLightMesh;
Mesh* gSphereMesh;
Mesh* gCubeMesh;
Mesh* gTrollMesh;

Model* gTeapot;
Model* gNormalMappingCube;
Model* gGround;
Model* gSphere;
Model* gLerpCube;
Model* gAdditiveBlendingModel;
Model* gMultiplicativeBlendingModel;
Model* gAlphaBlendingModel;
Model* gParallaxMappingCube;
Model* gTrollModel;

Camera* gCamera;

// Store lights in an array in this exercise
const int NUM_LIGHTS = 4;

//Array of all the lights in the scene 
CLight* gLights[NUM_LIGHTS]; 

//Strengths for all the lights used in the scene, 
//in an array because the light initialisation will be done in a loop
float LightsScale[NUM_LIGHTS] = { 10.0f, 10.0f, 10.0f, 0.6f};

//Colours for all the lights used in the scene, 
//in an array because the light initialisation will be done in a loop
CVector3 LightsColour[NUM_LIGHTS] = { {1.0f, 0.8f, 1.0f},
                                      {1.0f, 0.8f, 1.0f},
                                      {1.0f, 1.0f, 1.0f},
                                      {0.45f, 0.45f, 0.45f} };

//Positions for all the lights used in the scene, 
//in an array because the light initialisation will be done in a loop
CVector3 LightsPosition[NUM_LIGHTS] = { { 30, 10, 0 },
                                        { -30, 20, 80 },
                                        { 60, 20,0},
                                        { 100, 40, 40} };

// Additional light information
CVector3 gAmbientColour = { 0.2f, 0.2f, 0.3f }; // Background level of light (slightly bluish to match the far background, which is dark blue)
float    gSpecularPower = 256; // Specular power controls shininess - same for all models in this app

ColourRGBA gBackgroundColor = { 0.2f, 0.2f, 0.3f, 1.0f };

//Used for Cell shading 
CVector3 OutlineColour = {0,1,0};
float OutlineThickness = 0.03f;

// Variables controlling light1's orbiting of the cube
const float gLightOrbit = 20.0f;
const float gLightOrbitSpeed = 0.7f;

//how deep the texture will look
const float gParallaxDepth = 0.1f;

//angle of the spotlights Field of View
float gSpotlightConeAngle = 90.0f;

//Size of the shadow to use (size of the texture)
int gShadowMapSize = 1024;

ID3D11Texture2D* gShadowMap1Texture = nullptr;
ID3D11DepthStencilView* gShadowMap1DepthStencil = nullptr;
ID3D11ShaderResourceView* gShadowMap1SRV = nullptr;

//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
// Variables sent over to the GPU each frame
// The structures are now in Common.h
// IMPORTANT: Any new data you add in C++ code (CPU-side) is not automatically available to the GPU
//            Anything the shaders need (per-frame or per-model) needs to be sent via a constant buffer

PerFrameConstants gPerFrameConstants;      // The constants that need to be sent to the GPU each frame (see common.h for structure)
ID3D11Buffer*     gPerFrameConstantBuffer; // The GPU buffer that will recieve the constants above

PerModelConstants gPerModelConstants;      // As above, but constant that change per-model (e.g. world matrix)
ID3D11Buffer*     gPerModelConstantBuffer; // --"--

//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

// DirectX objects controlling textures used in this lab

CTexture* CStoneTexture      = new CTexture();
CTexture* CSphereTexture     = new CTexture();
CTexture* CBrickTexture      = new CTexture();
CTexture* CGroundTexture     = new CTexture();
CTexture* CLightTexture      = new CTexture();
CTexture* CGlassTexture      = new CTexture();
CTexture* CMoogleTexture     = new CTexture();
CTexture* CWoodNormalTexture = new CTexture();
CTexture* CPatternTexture    = new CTexture();
CTexture* CPatternNormal     = new CTexture();
CTexture* CWallTexture       = new CTexture();
CTexture* CWallNormalHeight  = new CTexture();
CTexture* CCellMapTexture    = new CTexture();
CTexture* CTrollTexture      = new CTexture();

// Get "camera-like" view matrix for a spotlight
CMatrix4x4 CalculateLightViewMatrix(int lightIndex)
{
    return InverseAffine(gLights[lightIndex]->LightModel->WorldMatrix());
}

// Get "camera-like" projection matrix for a spotlight
CMatrix4x4 CalculateLightProjectionMatrix(int lightIndex)
{
    return MakeProjectionMatrix(1.0f, ToRadians(gSpotlightConeAngle)); // Helper function in Utility\GraphicsHelpers.cpp
}

//--------------------------------------------------------------------------------------
// Initialise scene geometry, constant buffers and states
//--------------------------------------------------------------------------------------

// Prepare the geometry required for the scene
// Returns true on success
bool InitGeometry()
{
    // Load mesh geometry data, just like TL-Engine this doesn't create anything in the scene. Create a Model for that.
    try 
    {
        gTeapotMesh           = new Mesh("Teapot.x");
        gNormalMappingMesh   = new Mesh("Cube.x", true);
        gGroundMesh          = new Mesh("Ground.x");
        gLightMesh           = new Mesh("Light.x");
        gSphereMesh          = new Mesh("Sphere.x");
        gCubeMesh            = new Mesh("Cube.x");
        gTrollMesh           = new Mesh("Troll.x");
    }
    catch (std::runtime_error e)  // Constructors cannot return error messages so use exceptions to catch mesh errors (fairly standard approach this)
    {
        gLastError = e.what(); // This picks up the error message put in the exception (see Mesh.cpp)
        return false;
    }

    // Load the shaders required for the geometry we will use (see Shader.cpp / .h)
    if (!LoadShaders())
    {
        gLastError = "Error loading shaders";
        return false;
    }

    // Create GPU-side constant buffers to receive the gPerFrameConstants and gPerModelConstants structures above
    // These allow us to pass data from CPU to shaders such as lighting information or matrices
    // See the comments above where these variable are declared and also the UpdateScene function
    gPerFrameConstantBuffer = CreateConstantBuffer(sizeof(gPerFrameConstants));
    gPerModelConstantBuffer = CreateConstantBuffer(sizeof(gPerModelConstants));
    if (gPerFrameConstantBuffer == nullptr || gPerModelConstantBuffer == nullptr)
    {
        gLastError = "Error creating constant buffers";
        return false;
    }

    //// Load / prepare textures on the GPU ////

    // Load textures and create DirectX objects for them
    // The LoadTexture function requires you to pass a ID3D11Resource* (e.g. &gCubeDiffuseMap), which manages the GPU memory for the
    // texture and also a ID3D11ShaderResourceView* (e.g. &gCubeDiffuseMapSRV), which allows us to use the texture in shaders
    // The function will fill in these pointers with usable data. The variables used here are globals found near the top of the file.
    if (!CStoneTexture->LoadTextureFromHelper("StoneDiffuseSpecular.dds") || !CSphereTexture->LoadTextureFromHelper("brick1.jpg") ||
        !CBrickTexture->LoadTextureFromHelper("brick1.jpg") || !CGroundTexture->LoadTextureFromHelper("WoodDiffuseSpecular.dds") ||
        !CLightTexture->LoadTextureFromHelper("Flare.jpg") || !CGlassTexture->LoadTextureFromHelper("Glass.jpg") ||
        !CMoogleTexture->LoadTextureFromHelper("Moogle.png") || !CWoodNormalTexture->LoadTextureFromHelper("WoodNormal.dds") ||
        !CPatternTexture->LoadTextureFromHelper("PatternDiffuseSpecular.dds") || !CPatternNormal->LoadTextureFromHelper("PatternNormal.dds") ||
        !CWallNormalHeight->LoadTextureFromHelper("WallNormalHeight.dds") || !CWallTexture->LoadTextureFromHelper("WallDiffuseSpecular.dds") ||
        !CTrollTexture->LoadTextureFromHelper("Red.png") || !CCellMapTexture->LoadTextureFromHelper("CellGradient.png"))
    {
        gLastError = "Error loading textures";
        return false;
    }

    //**** Create Shadow Map texture ****//

    // We also need a depth buffer to go with our portal
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = gShadowMapSize; // Size of the shadow map determines quality / resolution of shadows
    textureDesc.Height = gShadowMapSize;
    textureDesc.MipLevels = 1; // 1 level, means just the main texture, no additional mip-maps. Usually don't use mip-maps when rendering to textures (or we would have to render every level)
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R32_TYPELESS; // The shadow map contains a single 32-bit value [tech gotcha: have to say typeless because depth buffer and shaders see things slightly differently]
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D10_BIND_DEPTH_STENCIL | D3D10_BIND_SHADER_RESOURCE; // Indicate we will use texture as a depth buffer and also pass it to shaders
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;
    if (FAILED(gD3DDevice->CreateTexture2D(&textureDesc, NULL, &gShadowMap1Texture)))
    {
        gLastError = "Error creating shadow map texture";
        return false;
    }

    // Create the depth stencil view, i.e. indicate that the texture just created is to be used as a depth buffer
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // See "tech gotcha" above. The depth buffer sees each pixel as a "depth" float
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;
    dsvDesc.Flags = 0;
    if (FAILED(gD3DDevice->CreateDepthStencilView(gShadowMap1Texture, &dsvDesc, &gShadowMap1DepthStencil)))
    {
        gLastError = "Error creating shadow map depth stencil view";
        return false;
    }


    // We also need to send this texture (resource) to the shaders. To do that we must create a shader-resource "view"
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT; // See "tech gotcha" above. The shaders see textures as colours, so shadow map pixels are not seen as depths
                                           // but rather as "red" floats (one float taken from RGB). Although the shader code will use the value as a depth
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    if (FAILED(gD3DDevice->CreateShaderResourceView(gShadowMap1Texture, &srvDesc, &gShadowMap1SRV)))
    {
        gLastError = "Error creating shadow map shader resource view";
        return false;
    }


    //*****************************//

  	// Create all filtering modes, blending modes etc. used by the app (see State.cpp/.h)
	if (!CreateStates())
	{
		gLastError = "Error creating states";
		return false;
	}

	return true;
}

// Prepare the scene
// Returns true on success
bool InitScene()
{
    //// Set up scene ////

    gTeapot = new Model(gTeapotMesh);
    gGround = new Model(gGroundMesh);
    gNormalMappingCube = new Model(gNormalMappingMesh);
    gSphere = new Model(gSphereMesh);
    gLerpCube = new Model(gCubeMesh);
    gAdditiveBlendingModel = new Model(gCubeMesh);
    gMultiplicativeBlendingModel = new Model(gCubeMesh);
    gAlphaBlendingModel = new Model(gCubeMesh);
    gParallaxMappingCube = new Model(gNormalMappingMesh);
    gTrollModel = new Model(gTrollMesh);

    // Initial positions
    gTeapot->SetPosition({ 60, 0, 25 });
    gTeapot->SetScale(0.9f);
    gTeapot->SetRotation({ 0.0f, 0/*ToRadians(-50.0f) */, 0.0f });

    gNormalMappingCube->SetPosition({ 18, 5, 68 });
    gNormalMappingCube->SetScale(1.0f);
    gNormalMappingCube->SetRotation({ 0.0f, 0/*ToRadians(-50.0f) */, 0.0f });

    gSphere->SetPosition({ 32, 5, 18 });
    gSphere->SetScale(0.5f);
    gSphere->SetRotation({ 0.0f, 0/*ToRadians(-50.0f) */, 0.0f });

    gLerpCube->SetPosition({ 18, 5, 32 });
    gLerpCube->SetScale(1.0f);
    gLerpCube->SetRotation({ 0.0f, 0.0f, 0.0f });

    gAdditiveBlendingModel->SetPosition({ -10, 5, 32 });
    gAdditiveBlendingModel->SetScale(1.0f);
    gAdditiveBlendingModel->SetRotation({ 0.0f, 0.0f, 0.0f });

    gMultiplicativeBlendingModel->SetPosition({ -10, 6, 50 });
    gMultiplicativeBlendingModel->SetScale(1.0f);
    gMultiplicativeBlendingModel->SetRotation({ 0.0f, 0.0f, 0.0f });

    gAlphaBlendingModel->SetPosition({ -10, 6, 14 });
    gAlphaBlendingModel->SetScale(1.0f);
    gAlphaBlendingModel->SetRotation({ 0.0f, 0.0f, 0.0f });

    gParallaxMappingCube->SetPosition({ 18, 5, 50 });
    gParallaxMappingCube->SetScale(1.0f);
    gParallaxMappingCube->SetRotation({ 0.0f, 0.0f, 0.0f });

    gTrollModel->SetPosition({ 32, 0.5f, 50 });
    gTrollModel->SetScale(4.0f);
    gTrollModel->SetRotation({ 0.0f, 0.0f, 0.0f });

    // Light set-up - using an array this time
    for (int i = 0; i < NUM_LIGHTS; ++i)
    {
        gLights[i] = new CLight(gLightMesh, LightsScale[i], LightsColour[i], LightsPosition[i], pow(LightsScale[i], 0.7f));
    }
    gLights[3]->LightModel->SetScale(pow(10, 0.7));

    gLights[2]->LightModel->SetRotation({ ToRadians(40.0f), 0.0f, 0.0f });
    gLights[3]->LightModel->SetRotation({0.0f, 0.0f, ToRadians(40)});
    //// Set up camera ////

    gCamera = new Camera();
    gCamera->SetPosition({ 25, 20,-20 });
    gCamera->SetRotation({ ToRadians(15.0f), 0, 0.0f });

    return true;
}

// Release the geometry and scene resources created above
void ReleaseResources()
{
    ReleaseStates();

    if (CLightTexture->SRVMap)  CLightTexture->SRVMap->Release();
    if (CLightTexture->Map)     CLightTexture->Map->Release();
    if (CGlassTexture->SRVMap)  CGlassTexture->SRVMap->Release();
    if (CGlassTexture->Map)     CGlassTexture->Map->Release();
    if (CMoogleTexture->SRVMap) CMoogleTexture->SRVMap->Release();
    if (CMoogleTexture->Map)    CMoogleTexture->Map->Release();
    if (CGroundTexture->SRVMap) CGroundTexture->SRVMap->Release();
    if (CGroundTexture->Map)    CGroundTexture->Map->Release();
    if (CStoneTexture->SRVMap)  CStoneTexture->SRVMap->Release();
    if (CStoneTexture->Map)     CStoneTexture->Map->Release();
    if (CSphereTexture->SRVMap) CSphereTexture->SRVMap->Release();
    if (CSphereTexture->Map)    CSphereTexture->Map->Release();
    if (CBrickTexture->SRVMap)  CBrickTexture->SRVMap->Release();
    if (CBrickTexture->Map)     CBrickTexture->Map->Release();

    if (gPerModelConstantBuffer)  gPerModelConstantBuffer->Release();
    if (gPerFrameConstantBuffer)  gPerFrameConstantBuffer->Release();

    ReleaseShaders();

    // See note in InitGeometry about why we're not using unique_ptr and having to manually delete
    for (int i = 0; i < NUM_LIGHTS; ++i)
    {
        delete gLights[i]->LightModel;  gLights[i]->LightModel = nullptr;
    }

    delete gCamera;                      gCamera                      = nullptr;
    delete gGround;                      gGround                      = nullptr;
    delete gTeapot;                       gTeapot                       = nullptr;
    delete gNormalMappingCube;           gNormalMappingCube           = nullptr;
    delete gSphere;                      gSphere                      = nullptr;
    delete gLerpCube;                    gLerpCube                    = nullptr;
    delete gAdditiveBlendingModel;       gAdditiveBlendingModel       = nullptr;
    delete gMultiplicativeBlendingModel; gMultiplicativeBlendingModel = nullptr;
    delete gAlphaBlendingModel;          gAlphaBlendingModel          = nullptr;
    delete gParallaxMappingCube;         gParallaxMappingCube         = nullptr;
    delete gTrollModel;                  gTrollModel                  = nullptr;

    delete gLightMesh;         gLightMesh         = nullptr;
    delete gGroundMesh;        gGroundMesh        = nullptr;
    delete gTeapotMesh;         gTeapotMesh         = nullptr;
    delete gNormalMappingMesh; gNormalMappingMesh = nullptr;
    delete gSphereMesh;        gSphereMesh        = nullptr;
    delete gCubeMesh;          gCubeMesh          = nullptr;
    delete gTrollMesh;         gTrollMesh         = nullptr;
}


//--------------------------------------------------------------------------------------
// Scene Rendering
//--------------------------------------------------------------------------------------

// Render the scene from the given light's point of view. Only renders depth buffer
void RenderDepthBufferFromLight(int lightIndex)
{
    // Get camera-like matrices from the spotlight, seet in the constant buffer and send over to GPU
    gPerFrameConstants.viewMatrix = CalculateLightViewMatrix(lightIndex);
    gPerFrameConstants.projectionMatrix = CalculateLightProjectionMatrix(lightIndex);
    gPerFrameConstants.viewProjectionMatrix = gPerFrameConstants.viewMatrix * gPerFrameConstants.projectionMatrix;
    UpdateConstantBuffer(gPerFrameConstantBuffer, gPerFrameConstants);

    // Indicate that the constant buffer we just updated is for use in the vertex shader (VS) and pixel shader (PS)
    gD3DContext->VSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer); // First parameter must match constant buffer number in the shader 
    gD3DContext->PSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer);


    //// Only render models that cast shadows ////

    // Use special depth-only rendering shaders
    gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
    gD3DContext->PSSetShader(gDepthOnlyPixelShader, nullptr, 0);

    // States - no blending, normal depth buffer and culling
    gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
    gD3DContext->OMSetDepthStencilState(gUseDepthBufferState, 0);
    gD3DContext->RSSetState(gCullBackState);

    // Render models - no state changes required between each object in this situation (no textures used in this step)
    gGround->Render();
    gTeapot->Render();
    gAdditiveBlendingModel->Render();
    gAlphaBlendingModel->Render();
    gSphere->Render();
    gLerpCube->Render();
    gNormalMappingCube->Render();
    gParallaxMappingCube->Render();
    gTrollModel->Render();
    gMultiplicativeBlendingModel->Render();
}

// Render everything in the scene from the given camera
void RenderSceneFromCamera(Camera* camera)
{
    // Set camera matrices in the constant buffer and send over to GPU
    gPerFrameConstants.viewMatrix           = camera->ViewMatrix();
    gPerFrameConstants.projectionMatrix     = camera->ProjectionMatrix();
    gPerFrameConstants.viewProjectionMatrix = camera->ViewProjectionMatrix();
    UpdateConstantBuffer(gPerFrameConstantBuffer, gPerFrameConstants);

    // Indicate that the constant buffer we just updated is for use in the vertex shader (VS) and pixel shader (PS)
    gD3DContext->VSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer); // First parameter must match constant buffer number in the shader 
    gD3DContext->PSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer);

    ///////////////////////////////
    //// Render skinned models ////
    ///////////////////////////////

    ///////////////////////////////////
    //// Render non-skinned models ////
    ///////////////////////////////////
    
    // Render lit models, only change textures for each onee
    // States - no blending, normal depth buffer and culling
    gGround->Setup(gPixelLightingVertexShader, gPixelLightingPixelShader);
    gGround->SetStates(gNoBlendingState, gUseDepthBufferState, gCullBackState);
    gD3DContext->PSSetSamplers(0, 1, &gAnisotropic4xSampler);
    gGround->SetShaderResources(0, CGroundTexture->SRVMap);
    gGround->Render();

    gTeapot->SetShaderResources(0, CStoneTexture->SRVMap);
    gTeapot->Render();

    //-------------------//
    // Additive Blending //
    //-------------------//

    gAdditiveBlendingModel->Setup(gBlendingPixelShader);
    gAdditiveBlendingModel->SetStates(gAdditiveBlendingState, gDepthReadOnlyState, gCullBackState);
    gAdditiveBlendingModel->SetShaderResources(0, CLightTexture->SRVMap);
    gAdditiveBlendingModel->Render();

    //----------------//
    // Alpha Blending //
    //----------------//

    gAlphaBlendingModel->SetShaderResources(0, CMoogleTexture->SRVMap);
    gAlphaBlendingModel->SetStates(gAlphaBlending, gUseDepthBufferState, gCullBackState);
    gAlphaBlendingModel->Render();

    //-------------------//
    // Texture Scrolling //
    //-------------------//

    gSphere->Setup(gWigglingVertexShader, gTextureScrollingPixelShader);
    gSphere->SetShaderResources(0, CSphereTexture->SRVMap);
    gSphere->SetStates(gNoBlendingState, gUseDepthBufferState, gCullBackState);
    gSphere->Render();

    //----------------//
    // Texture Fading //
    //----------------//

    gLerpCube->Setup(gPixelLightingVertexShader, gTextureFadingPixelShader);
    gLerpCube->SetShaderResources(0,CBrickTexture->SRVMap , 2, CGroundTexture->SRVMap);
    gLerpCube->SetStates(gNoBlendingState, gUseDepthBufferState, gCullBackState);
    gLerpCube->Render();

    //----------------//
    // Normal Mapping //
    //----------------//

    gNormalMappingCube->Setup(gNormalMappingVertexShader, gNormalMappingPixelShader);
    gNormalMappingCube->SetShaderResources(0, CPatternTexture->SRVMap,2, CPatternNormal->SRVMap);
    gNormalMappingCube->Render();

    gParallaxMappingCube->Setup(gParallaxMappingPixelShader);
    gParallaxMappingCube->SetShaderResources(0, CWallTexture->SRVMap, 2, CWallNormalHeight->SRVMap);
    gParallaxMappingCube->Render();

    //-----------------------------------//
    // Cell Shading - First Pass Through //
    //-----------------------------------//

    gTrollModel->Setup(gCellShadingOutlineVertexShader, gCellShadingOutlinePixelShader);
    gTrollModel->SetStates(gNoBlendingState, gUseDepthBufferState, gCullFrontState);
    gTrollModel->Render();

    //------------------------------------//
    // Cell Shading - Second Pass Through //
    //------------------------------------//

    gTrollModel->Setup(gPixelLightingVertexShader, gCellShadingPixelShader);
    gTrollModel->SetStates(gNoBlendingState, gUseDepthBufferState, gCullBackState);
    gTrollModel->SetShaderResources(0, CTrollTexture->SRVMap, 2, CCellMapTexture->SRVMap);
    gD3DContext->PSSetSamplers(0, 1, &gAnisotropic4xSampler);
    gD3DContext->PSSetSamplers(1, 1, &gPointSampler);
    gTrollModel->Render();

    //-------------------------//
    // Multiplicative Blending //
    //-------------------------//

    gMultiplicativeBlendingModel->Setup(gPixelLightingVertexShader, gBlendingPixelShader);
    gMultiplicativeBlendingModel->SetStates(gMultiplicativeBlend, gDepthReadOnlyState, gCullNoneState);
    gMultiplicativeBlendingModel->SetShaderResources(0, CGlassTexture->SRVMap);
    gMultiplicativeBlendingModel->Render();

    //// Render lights ////

    // Select which shaders to use next
    gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
    gD3DContext->PSSetShader(gLightModelPixelShader,      nullptr, 0);

    // Select the texture and sampler to use in the pixel shader
    gD3DContext->PSSetShaderResources(0, 1, &CLightTexture->SRVMap); // First parameter must match texture slot number in the shaer
    gD3DContext->PSSetSamplers(0, 1, &gAnisotropic4xSampler);

    // States - additive blending, read-only depth buffer and no culling (standard set-up for blending
    gD3DContext->OMSetBlendState(gAdditiveBlendingState, nullptr, 0xffffff);
    gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
    gD3DContext->RSSetState(gCullNoneState);

    // Render all the lights in the array
    for (int i = 0; i < NUM_LIGHTS; ++i)
    {
        gPerModelConstants.objectColour = gLights[i]->LightColour; // Set any per-model constants apart from the world matrix just before calling render (light colour here)
        gLights[i]->RenderLight();
    }
}

// Rendering the scene
void RenderScene()
{
    //// Common settings ////

    // Set up the light information in the constant buffer

    gPerFrameConstants.light1.Colour = gLights[0]->LightColour * gLights[0]->LightStrength;
    gPerFrameConstants.light1.Position = gLights[0]->LightModel->Position();

    gPerFrameConstants.light2.Colour = gLights[1]->LightColour * gLights[1]->LightStrength;
    gPerFrameConstants.light2.Position = gLights[1]->LightModel->Position();

    gPerFrameConstants.light3.Colour = gLights[2]->LightColour * gLights[2]->LightStrength;
    gPerFrameConstants.light3.Position = gLights[2]->LightModel->Position();
    gPerFrameConstants.light3.Direction = Normalise(gLights[2]->LightModel->WorldMatrix().GetZAxis());
    gPerFrameConstants.light3.CosHalfAngle = cos(ToRadians(gSpotlightConeAngle / 4));
    gPerFrameConstants.light3.lightViewMatrix = CalculateLightViewMatrix(2);
    gPerFrameConstants.light3.lightProjectionMatrix = CalculateLightProjectionMatrix(2);

    gPerFrameConstants.light4.Colour = gLights[3]->LightColour * gLights[3]->LightStrength;
    gPerFrameConstants.light4.Position = gLights[3]->LightModel->Position();
    gPerFrameConstants.light4.Direction = Normalise(-gLights[3]->LightModel->WorldMatrix().GetXAxis());

    gPerFrameConstants.ambientColour  = gAmbientColour;
    gPerFrameConstants.specularPower  = gSpecularPower;
    gPerFrameConstants.cameraPosition = gCamera->Position();

    gPerFrameConstants.parallaxDepth = gParallaxDepth;
    gPerFrameConstants.outlineColour = OutlineColour;
    gPerFrameConstants.outlineThickness = OutlineThickness;

    gPerFrameConstants.DepthAdjust = 0.0005f;


    // Setup the viewport to the size of the shadow map texture
    D3D11_VIEWPORT vp;
    vp.Width = static_cast<FLOAT>(gShadowMapSize);
    vp.Height = static_cast<FLOAT>(gShadowMapSize);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    gD3DContext->RSSetViewports(1, &vp);

    // Select the shadow map texture as the current depth buffer. We will not be rendering any pixel colours
    // Also clear the the shadow map depth buffer to the far distance
    gD3DContext->OMSetRenderTargets(0, nullptr, gShadowMap1DepthStencil);
    gD3DContext->ClearDepthStencilView(gShadowMap1DepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // Render the scene from the point of view of light 1 (only depth values written)
    RenderDepthBufferFromLight(2);

    //// Main scene rendering ////

    // Set the back buffer as the target for rendering and select the main depth buffer.
    // When finished the back buffer is sent to the "front buffer" - which is the monitor.
    gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);

    // Clear the back buffer to a fixed colour and the depth buffer to the far distance
    gD3DContext->ClearRenderTargetView(gBackBufferRenderTarget, &gBackgroundColor.r);
    gD3DContext->ClearDepthStencilView(gDepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // Setup the viewport to the size of the main window
    vp.Width  = static_cast<FLOAT>(gViewportWidth);
    vp.Height = static_cast<FLOAT>(gViewportHeight);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    gD3DContext->RSSetViewports(1, &vp);

    gD3DContext->PSSetShaderResources(1, 1, &gShadowMap1SRV);
    gD3DContext->PSSetSamplers(1, 1, &gPointSampler);

    // Render the scene from the main camera
    RenderSceneFromCamera(gCamera);


    // Unbind shadow maps from shaders - prevents warnings from DirectX when we try to render to the shadow maps again next frame
    ID3D11ShaderResourceView* nullView = nullptr;
    gD3DContext->PSSetShaderResources(1, 1, &nullView);

    //// Scene completion ////

    // When drawing to the off-screen back buffer is complete, we "present" the image to the front buffer (the screen)
    gSwapChain->Present(0, 0);
}


//--------------------------------------------------------------------------------------
// Scene Update
//--------------------------------------------------------------------------------------

// Update models and camera. frameTime is the time passed since the last frame
void UpdateScene(float frameTime)
{
    // Control character part. First parameter is node number - index from flattened depth-first array of model parts. 0 is root

    // Orbit the light - a bit of a cheat with the static variable [ask the tutor if you want to know what this is]
    static float rotate = 0.0f;
    static bool go = true;
    static float PI = 3.14f;
    //gLights[0].model->SetPosition( gCrate->Position() + CVector3{ cos(rotate) * gLightOrbit, 10, sin(rotate) * gLightOrbit } );
    if (go)  rotate -= gLightOrbitSpeed * frameTime;
    if (KeyHit(Key_1))  go = !go;

    //Performs a sin and cos calculation and clamps the value between -1 and 1
    float sinBlueColour = sin(((rotate + 3) * PI) + 1);
    float cosGreenColour = cos(((rotate + 3) * PI) + 1);

    //Divide by three to allow the colours to change gradually
    gLights[0]->LightColour = CVector3 {0.3, cosGreenColour / 3, sinBlueColour / 3};
              
    gLights[1]->LightStrength = float((sin((rotate + 3) * PI) + 1)) * 50;

    gPerFrameConstants.Wiggle += WIGGLESTRENGTH * frameTime;

	// Control camera (will update its view matrix)
	gCamera->Control(frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D );
    gTeapot->Control(NULL, frameTime, Key_I, Key_K, Key_J, Key_L, Key_U, Key_O, Key_Period, Key_Comma);
    gLights[2]->LightModel->Control(NULL, frameTime, Key_T, Key_G, Key_F, Key_H, Key_R, Key_Y, Key_B, Key_N);

    // Show frame time / FPS in the window title //
    const float fpsUpdateTime = 0.5f; // How long between updates (in seconds)
    static float totalFrameTime = 0;
    static int frameCount = 0;
    totalFrameTime += frameTime;
    ++frameCount;
    if (totalFrameTime > fpsUpdateTime)
    {
        // Displays FPS rounded to nearest int, and frame time (more useful for developers) in milliseconds to 2 decimal places
        float avgFrameTime = totalFrameTime / frameCount;
        std::ostringstream frameTimeMs;
        frameTimeMs.precision(2);
        frameTimeMs << std::fixed << avgFrameTime * 1000;
        std::string windowTitle = "CO2409 Week 22: Skinning - Frame Time: " + frameTimeMs.str() +
                                  "ms, FPS: " + std::to_string(static_cast<int>(1 / avgFrameTime + 0.5f)) + " FFFFF " + std::to_string(static_cast<float>(gLights[1]->LightStrength));
        SetWindowTextA(gHWnd, windowTitle.c_str());
        totalFrameTime = 0;
        frameCount = 0;
    }
}
