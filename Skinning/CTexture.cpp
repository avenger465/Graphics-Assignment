#include "CTexture.h"
#include "GraphicsHelpers.h"
#include "../Shader.h"
#include <cctype>

CTexture::CTexture()
{
	Map = nullptr;
	SRVMap = nullptr;
}


bool CTexture::LoadTextureFromHelper(std::string filename)
{
	return LoadTexture(filename, &Map, &SRVMap);
}
