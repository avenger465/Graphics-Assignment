#include "CTexture.h"
#include "GraphicsHelpers.h"
#include "../Shader.h"
//#include <cmath>
#include <cctype>
//#include <atlbase.h>

CTexture::CTexture()
{
	Map = nullptr;
	SRVMap = nullptr;
}


bool CTexture::LoadTextureFromHelper(std::string filename)
{
	return LoadTexture(filename, &Map, &SRVMap);
}
