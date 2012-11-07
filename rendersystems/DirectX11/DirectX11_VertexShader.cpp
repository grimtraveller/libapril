/// @file
/// @author  Boris Mikic
/// @version 2.5
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php

#ifdef _DIRECTX11
#include <d3d11.h>
#include <d3dcompiler.h>

#include <hltypes/hlog.h>
#include <hltypes/hstring.h>

#include "april.h"
#include "DirectX11_RenderSystem.h"
#include "DirectX11_VertexShader.h"

#define APRIL_D3D_DEVICE (((DirectX11_RenderSystem*)april::rendersys)->d3dDevice)

namespace april
{
	DirectX11_VertexShader::DirectX11_VertexShader(chstr filename) : VertexShader(filename), dx11Shader(NULL)
	{
	}

	DirectX11_VertexShader::DirectX11_VertexShader() : VertexShader(), dx11Shader(NULL)
	{
	}

	DirectX11_VertexShader::~DirectX11_VertexShader()
	{
		if (this->dx11Shader != NULL)
		{
			this->dx11Shader->Release();
			this->dx11Shader = NULL;
		}
	}

	bool DirectX11_VertexShader::compile(chstr shaderCode)
	{
		if (shaderCode == "")
		{
			hlog::error(april::logTag, "No vertex shader code given!");
			return false;
		}
		DWORD shaderFlags = (D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_ENABLE_STRICTNESS);
#ifdef _DEBUG
		shaderFlags |= D3DCOMPILE_DEBUG;
#endif
		ID3D10Blob* bufferShader = NULL;
		ID3D10Blob* bufferError = NULL;
		HRESULT hr = D3DCompile(shaderCode.c_str(), shaderCode.size() + 1, "VS", NULL, NULL,
			"VS", "vs_2_0", shaderFlags, 0, &bufferShader, &bufferError);
		if (FAILED(hr))
		{
			hlog::error(april::logTag, "Failed to compile vertex shader!");
			if (bufferError != NULL)
			{
				hlog::error(april::logTag, (char*)bufferError->GetBufferPointer());
				bufferError->Release();
			}
			return false;
		}
		hr = APRIL_D3D_DEVICE->CreateVertexShader(bufferShader->GetBufferPointer(),
			bufferShader->GetBufferSize(), NULL, &this->dx11Shader);
		if (FAILED(hr))
		{
			hlog::error(april::logTag, "Failed to create vertex shader!");
			return false;
		}
		return true;
	}

	void DirectX11_VertexShader::setConstantsB(const int* quadVectors, unsigned int quadCount)
	{
	}

	void DirectX11_VertexShader::setConstantsI(const int* quadVectors, unsigned int quadCount)
	{
	}

	void DirectX11_VertexShader::setConstantsF(const float* quadVectors, unsigned int quadCount)
	{
	}

}

#endif