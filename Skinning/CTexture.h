#include "Common.h"
#include "../Shader.h"
#include <cmath>
#include <cctype>
#include <atlbase.h>

#pragma once
class CTexture
{
public:
	CTexture();
	bool LoadTextureFromHelper(std::string filename);

	ID3D11Resource* Map;
	ID3D11ShaderResourceView* SRVMap;
};

