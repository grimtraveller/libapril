/// @file
/// @version 3.5
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#ifdef _DIRECTX11
#include <d3d11.h>

#include <hltypes/hlog.h>
#include <hltypes/hresource.h>
#include <hltypes/hstring.h>

#include "april.h"
#include "DirectX11_RenderSystem.h"
#include "DirectX11_VertexShader.h"

#define APRIL_D3D_DEVICE (((DirectX11_RenderSystem*)april::rendersys)->d3dDevice)

using namespace Microsoft::WRL;

namespace april
{
	DirectX11_VertexShader::DirectX11_VertexShader(chstr filename) : VertexShader(), shaderData(NULL), shaderSize(0), dx11Shader(nullptr)
	{
		this->loadResource(filename);
	}

	DirectX11_VertexShader::DirectX11_VertexShader() : VertexShader(), shaderData(NULL), shaderSize(0), dx11Shader(nullptr)
	{
	}

	DirectX11_VertexShader::~DirectX11_VertexShader()
	{
		this->dx11Shader = nullptr;
		_HL_TRY_DELETE_ARRAY(this->shaderData);
	}

	bool DirectX11_VertexShader::loadFile(chstr filename)
	{
		if (this->dx11Shader != nullptr)
		{
			hlog::error(logTag, "Shader already loaded.");
			return false;
		}
		if (!this->_loadFileData(filename, &this->shaderData, &this->shaderSize))
		{
			hlog::error(logTag, "Shader file not found: " + filename);
			return false;
		}
		HRESULT hr = APRIL_D3D_DEVICE->CreateVertexShader(this->shaderData, this->shaderSize, NULL, &this->dx11Shader);
		if (FAILED(hr))
		{
			hlog::error(logTag, "Failed to create vertex shader!");
			return false;
		}
		return true;
	}

	bool DirectX11_VertexShader::loadResource(chstr filename)
	{
		if (this->dx11Shader != nullptr)
		{
			hlog::error(logTag, "Shader already loaded.");
			return false;
		}
		if (!this->_loadResourceData(filename, &this->shaderData, &this->shaderSize))
		{
			hlog::error(logTag, "Shader file not found: " + filename);
			return false;
		}
		HRESULT hr = APRIL_D3D_DEVICE->CreateVertexShader(this->shaderData, this->shaderSize, NULL, &this->dx11Shader);
		if (FAILED(hr))
		{
			hlog::error(logTag, "Failed to create vertex shader!");
			return false;
		}
		return true;
	}

	void DirectX11_VertexShader::setConstantsB(const int* quads, unsigned int quadCount)
	{
	}

	void DirectX11_VertexShader::setConstantsI(const int* quads, unsigned int quadCount)
	{
	}

	void DirectX11_VertexShader::setConstantsF(const float* quads, unsigned int quadCount)
	{
	}

}

#endif