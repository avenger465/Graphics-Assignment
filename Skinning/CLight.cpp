#include "CLight.h"

CLight::CLight(Mesh* Mesh, float Strength, CVector3 Colour, CVector3 Position, float Scale)
{

	LightModel = new Model(Mesh);
	LightStrength = Strength;
	LightColour = Colour;
	LightModel->SetPosition(Position);
	LightModel->SetScale(Scale);

}

CLight::~CLight()
{

}

void CLight::SetPosition(CVector3 Position)
{
	LightModel->SetPosition(Position);
}

CVector3 CLight::GetLightColour()
{
	return LightColour;
}

void CLight::SetLightStates(ID3D11BlendState* blendSate, ID3D11DepthStencilState* depthState, ID3D11RasterizerState* rasterizerState)
{
	gD3DContext->OMSetBlendState(blendSate, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(depthState, 0);
	gD3DContext->RSSetState(rasterizerState);
}

void CLight::RenderLight()
{
	LightModel->Render();


}
