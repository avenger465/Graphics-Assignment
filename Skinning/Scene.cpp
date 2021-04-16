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


// Meshes, models and cameras, same meaning as TL-Engine. Meshes prepared in InitGeometry function, Models & camera in InitScene
//Mesh* gCharacterMesh;
Mesh* gCrateMesh;
Mesh* gGroundMesh;
Mesh* gLightMesh;
Mesh* gSphereMesh;
Mesh* gLerpCubeMesh;
Mesh* gAdditiveBlendingMesh;
Mesh* gMultiplicativeBlendingMesh;
Mesh* gAlphaBlendingMesh;

//Model* gCharacter;
Model* gCrate;
Model* gGround;
Model* gSphere;
Model* gLerpCube;
Model* gAdditiveBlendingModel;
Model* gMultiplicativeBlendingModel;
Model* gAlphaBlendingModel;


Camera* gCamera;


// Store lights in an array in this exercise
const int NUM_LIGHTS = 2;
CLight* gLights[NUM_LIGHTS]; 
float LightsScale[NUM_LIGHTS] = { 10.0f, 10.0f};

CVector3 LightsColour[NUM_LIGHTS] = { {1.0f, 0.8f, 1.0f},
                                      {1.0f, 0.8f, 1.0f} };

CVector3 LightsPosition[NUM_LIGHTS] = { { 30, 10, 0 },
                                        { 10, 20, 50 }};


// Additional light information
CVector3 gAmbientColour = { 0.2f, 0.2f, 0.3f }; // Background level of light (slightly bluish to match the far background, which is dark blue)
float    gSpecularPower = 256; // Specular power controls shininess - same for all models in this app

ColourRGBA gBackgroundColor = { 0.2f, 0.2f, 0.3f, 1.0f };

// Variables controlling light1's orbiting of the cube
const float gLightOrbit = 20.0f;
const float gLightOrbitSpeed = 0.7f;


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
//ID3D11Resource*           gCharacterDiffuseSpecularMap    = nullptr; // This object represents the memory used by the texture on the GPU
//ID3D11ShaderResourceView* gCharacterDiffuseSpecularMapSRV = nullptr; // This object is used to give shaders access to the texture above (SRV = shader resource view)

CTexture* StoneTexture = new CTexture();

CTexture* SphereTexture = new CTexture();

CTexture* BrickTexture = new CTexture();

CTexture* GroundTexture = new CTexture();

CTexture* LightTexture = new CTexture();

CTexture* GlassTexture = new CTexture();

CTexture* MoogleTexture = new CTexture();



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
        //gCharacterMesh = new Mesh("Woman.x");
        gCrateMesh     = new Mesh("Cube.x");
        gGroundMesh    = new Mesh("Ground.x");
        gLightMesh     = new Mesh("Light.x");
        gSphereMesh = new Mesh("Sphere.x");
        gLerpCubeMesh = new Mesh("Cube.x");
        gAdditiveBlendingMesh = new Mesh("Cube.x");
        gMultiplicativeBlendingMesh = new Mesh("Cube.x");
        gAlphaBlendingMesh = new Mesh("Cube.x");
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
    if (!StoneTexture->LoadTextureFromHelper("StoneDiffuseSpecular.dds") || !SphereTexture->LoadTextureFromHelper("brick1.jpg") ||
        !BrickTexture->LoadTextureFromHelper("brick1.jpg") || !GroundTexture->LoadTextureFromHelper("WoodDiffuseSpecular.dds") ||
        !LightTexture->LoadTextureFromHelper("Flare.jpg") || !GlassTexture->LoadTextureFromHelper("Glass.jpg") ||
        !MoogleTexture->LoadTextureFromHelper("Moogle.png"))
    {
        gLastError = "Error loading textures";
        return false;
    }



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

    //gCharacter = new Model(gCharacterMesh);
    gCrate     = new Model(gCrateMesh);
    gGround    = new Model(gGroundMesh);
    gSphere = new Model(gSphereMesh);
    gLerpCube = new Model(gLerpCubeMesh);
    gAdditiveBlendingModel = new Model(gAdditiveBlendingMesh);
    gMultiplicativeBlendingModel = new Model(gMultiplicativeBlendingMesh);
    gAlphaBlendingModel = new Model(gAlphaBlendingMesh);

	// Initial positions
	gCrate-> SetPosition({ 18, 5, 18 });
	gCrate-> SetScale( 1.0f );
	gCrate-> SetRotation({ 0.0f, 0/*ToRadians(-50.0f) */, 0.0f });

    gSphere->SetPosition({ 32, 5, 18 });
    gSphere->SetScale(0.5f);
    gSphere->SetRotation({ 0.0f, 0/*ToRadians(-50.0f) */, 0.0f });

    gLerpCube->SetPosition({18, 5, 32});
    gLerpCube->SetScale(1.0f);
    gLerpCube->SetRotation({0.0f, 0.0f, 0.0f});

    gAdditiveBlendingModel->SetPosition({ -10, 5, 32 });
    gAdditiveBlendingModel->SetScale(1.0f);
    gAdditiveBlendingModel->SetRotation({ 0.0f, 0.0f, 0.0f });

    gMultiplicativeBlendingModel->SetPosition({ -10, 6, 50 });
    gMultiplicativeBlendingModel->SetScale(1.0f);
    gMultiplicativeBlendingModel->SetRotation({ 0.0f, 0.0f, 0.0f });

    gAlphaBlendingModel->SetPosition({ -10, 6, 14 });
    gAlphaBlendingModel->SetScale(1.0f);
    gAlphaBlendingModel->SetRotation({ 0.0f, 0.0f, 0.0f });

    // Light set-up - using an array this time
    for (int i = 0; i < NUM_LIGHTS; ++i)
    {
        gLights[i] = new CLight(gLightMesh, LightsScale[i], LightsColour[i], LightsPosition[i], pow(LightsScale[i], 0.7f));
    }

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

    if (LightTexture->SRVMap)             LightTexture->SRVMap->Release();
    if (LightTexture->Map)                LightTexture->Map->Release();
    if (GlassTexture->SRVMap)             GlassTexture->SRVMap->Release();
    if (GlassTexture->Map)                GlassTexture->Map->Release();
    if (MoogleTexture->SRVMap)             MoogleTexture->SRVMap->Release();
    if (MoogleTexture->Map)                MoogleTexture->Map->Release();
    if (GroundTexture->SRVMap)    GroundTexture->SRVMap->Release();
    if (GroundTexture->Map)       GroundTexture->Map->Release();
    if (StoneTexture->SRVMap)            StoneTexture->SRVMap->Release();
    if (StoneTexture->Map)              StoneTexture->Map->Release();
    if (SphereTexture->SRVMap)    SphereTexture->SRVMap->Release();
    if (SphereTexture->Map)       SphereTexture->Map->Release();
    if (BrickTexture->SRVMap)     BrickTexture->SRVMap->Release();
    if (BrickTexture->Map)        BrickTexture->Map->Release();
    //if (gCharacterDiffuseSpecularMapSRV) gCharacterDiffuseSpecularMapSRV->Release();
    //if (gCharacterDiffuseSpecularMap)    gCharacterDiffuseSpecularMap->Release();

    if (gPerModelConstantBuffer)  gPerModelConstantBuffer->Release();
    if (gPerFrameConstantBuffer)  gPerFrameConstantBuffer->Release();

    ReleaseShaders();

    // See note in InitGeometry about why we're not using unique_ptr and having to manually delete
    for (int i = 0; i < NUM_LIGHTS; ++i)
    {
        delete gLights[i]->LightModel;  gLights[i]->LightModel = nullptr;
    }

    delete gCamera;     gCamera    = nullptr;
    delete gGround;     gGround    = nullptr;
    delete gCrate;      gCrate     = nullptr;
    delete gSphere;     gSphere    = nullptr;
    delete gLerpCube;   gLerpCube  = nullptr;
    delete gAdditiveBlendingModel; gAdditiveBlendingModel = nullptr;
    delete gMultiplicativeBlendingModel; gMultiplicativeBlendingModel = nullptr;
    delete gAlphaBlendingModel; gAlphaBlendingModel = nullptr;
    //delete gCharacter;  gCharacter = nullptr;

    delete gLightMesh;      gLightMesh     = nullptr;
    delete gGroundMesh;     gGroundMesh    = nullptr;
    delete gCrateMesh;      gCrateMesh     = nullptr;
    delete gSphereMesh;     gSphereMesh    = nullptr;
    delete gLerpCubeMesh;   gLerpCubeMesh  = nullptr;
    delete gAdditiveBlendingMesh;   gAdditiveBlendingMesh = nullptr;
    delete gMultiplicativeBlendingModel; gMultiplicativeBlendingModel = nullptr;
    delete gAlphaBlendingMesh; gAlphaBlendingMesh = nullptr;
    //delete gCharacterMesh;  gCharacterMesh = nullptr;
}



//--------------------------------------------------------------------------------------
// Scene Rendering
//--------------------------------------------------------------------------------------

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

    // Select which shaders to use next
    gD3DContext->PSSetShader(gPixelLightingPixelShader, nullptr, 0);
    
    // States - no blending, normal depth buffer and culling
    gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
    gD3DContext->OMSetDepthStencilState(gUseDepthBufferState, 0);
    gD3DContext->RSSetState(gCullBackState);

    // Select the approriate textures and sampler to use in the pixel shader
    gD3DContext->PSSetSamplers(0, 1, &gAnisotropic4xSampler);

    ///////////////////////////////////
    //// Render non-skinned models ////
    ///////////////////////////////////

    // Select which shaders to use next
    gD3DContext->VSSetShader(gPixelLightingVertexShader, nullptr, 0); // Only need to change the vertex shader from skinning
    
    // Render lit models, only change textures for each onee
    gD3DContext->PSSetShaderResources(0, 1, &GroundTexture->SRVMap); // First parameter must match texture slot number in the shader
    gGround->Render();

    gD3DContext->PSSetShaderResources(0, 1, &StoneTexture->SRVMap);
    gCrate->Render();

    gD3DContext->PSSetShader(gBlendingPixelShader, nullptr, 0);
    gD3DContext->PSSetShaderResources(0, 1, &LightTexture->SRVMap);
    gD3DContext->OMSetBlendState(gAdditiveBlendingState, nullptr, 0xffffff);
    gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
    gD3DContext->RSSetState(gCullBackState);
    gAdditiveBlendingModel->Render();

    gD3DContext->PSSetShaderResources(0, 1, &GlassTexture->SRVMap);
    gD3DContext->OMSetBlendState(gMultiplicativeBlend, nullptr, 0xffffff);
    gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
    gD3DContext->RSSetState(gCullNoneState);
    gMultiplicativeBlendingModel->Render();

    gD3DContext->PSSetShaderResources(0, 1, &MoogleTexture->SRVMap);
    gD3DContext->OMSetBlendState(gAlphaBlending, nullptr, 0xffffff);
    gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
    gD3DContext->RSSetState(gCullBackState);
    gAlphaBlendingModel->Render();

    gD3DContext->VSSetShader(gWiggleVertexShader, nullptr, 0);
    gD3DContext->PSSetShader(gTintPixelShader, nullptr, 0);
    gD3DContext->PSSetShaderResources(0, 1, &SphereTexture->SRVMap);
    gD3DContext->PSSetSamplers(0, 1, &gAnisotropic4xSampler);

    gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
    gD3DContext->OMSetDepthStencilState(gUseDepthBufferState, 0);
    gD3DContext->RSSetState(gCullBackState);
    gSphere->Render();

    //// Render Lerp Cube ////

    gD3DContext->VSSetShader(gPixelLightingVertexShader, nullptr, 0);
    gD3DContext->PSSetShader(gTextureLerpPixelShader, nullptr, 0);
    gD3DContext->PSSetShaderResources(0, 1, &BrickTexture->SRVMap);
    gD3DContext->PSSetShaderResources(1, 1, &GroundTexture->SRVMap);
    gD3DContext->PSSetSamplers(0, 1, &gAnisotropic4xSampler);
    gLerpCube->Render();


    //// Render lights ////

    // Select which shaders to use next
    gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
    gD3DContext->PSSetShader(gLightModelPixelShader,      nullptr, 0);

    // Select the texture and sampler to use in the pixel shader
    gD3DContext->PSSetShaderResources(0, 1, &LightTexture->SRVMap); // First parameter must match texture slot number in the shaer
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
    // Don't send to the GPU yet, the function RenderSceneFromCamera will do that
    gPerFrameConstants.light1Colour   = gLights[0]->LightColour * gLights[0]->LightStrength;
    gPerFrameConstants.light1Position = gLights[0]->LightModel->Position();
                                                  
                                                  
    gPerFrameConstants.light2Colour   = gLights[1]->LightColour * gLights[1]->LightStrength;
    gPerFrameConstants.light2Position = gLights[1]->LightModel->Position();

    gPerFrameConstants.ambientColour  = gAmbientColour;
    gPerFrameConstants.specularPower  = gSpecularPower;
    gPerFrameConstants.cameraPosition = gCamera->Position();



    //// Main scene rendering ////

    // Set the back buffer as the target for rendering and select the main depth buffer.
    // When finished the back buffer is sent to the "front buffer" - which is the monitor.
    gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);

    // Clear the back buffer to a fixed colour and the depth buffer to the far distance
    gD3DContext->ClearRenderTargetView(gBackBufferRenderTarget, &gBackgroundColor.r);
    gD3DContext->ClearDepthStencilView(gDepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // Setup the viewport to the size of the main window
    D3D11_VIEWPORT vp;
    vp.Width  = static_cast<FLOAT>(gViewportWidth);
    vp.Height = static_cast<FLOAT>(gViewportHeight);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    gD3DContext->RSSetViewports(1, &vp);

    // Render the scene from the main camera
    RenderSceneFromCamera(gCamera);


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
    //gCharacter->Control(36, frameTime, Key_I, Key_K, Key_J, Key_L, Key_U, Key_O, Key_Period, Key_Comma );

    // Orbit the light - a bit of a cheat with the static variable [ask the tutor if you want to know what this is]
    static float rotate = 0.0f;
    static bool go = true;
    //gLights[0].model->SetPosition( gCrate->Position() + CVector3{ cos(rotate) * gLightOrbit, 10, sin(rotate) * gLightOrbit } );
    if (go)  rotate -= gLightOrbitSpeed * frameTime;
    if (KeyHit(Key_1))  go = !go;

    float wig = sin(((2 * rotate + 3) * 3.14 / 2) + 1);
    float wig2 = cos(((2 * rotate + 3) * 3.14 / 2) + 1);

    gLights[0]->LightColour = CVector3 {0.3, wig2 / 3, wig / 3};
              
    gLights[1]->LightStrength = float((sin((2 * rotate + 3) * 3.14 / 2) + 1) / 2) * 50;

    gPerFrameConstants.Wiggle += 6 * frameTime;

	// Control camera (will update its view matrix)
	gCamera->Control(frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D );
    gAdditiveBlendingModel->Control(NULL, frameTime, Key_I, Key_K, Key_J, Key_L, Key_U, Key_O, Key_Period, Key_Comma);

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
                                  "ms, FPS: " + std::to_string(static_cast<int>(1 / avgFrameTime + 0.5f));
        SetWindowTextA(gHWnd, windowTitle.c_str());
        totalFrameTime = 0;
        frameCount = 0;
    }
}
