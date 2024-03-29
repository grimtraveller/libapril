/// @file
/// @version 3.5
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include <hltypes/hfile.h>
#include <hltypes/hresource.h>
#include <hltypes/hstring.h>

#include "PixelShader.h"

namespace april
{
	PixelShader::PixelShader()
	{
	}

	PixelShader::~PixelShader()
	{
	}

	bool PixelShader::_loadFileData(chstr filename, unsigned char** data, int* size)
	{
		if (!hfile::exists(filename))
		{
			return false;
		}
		hfile stream;
		stream.open(filename);
		*size = (int)stream.size();
		*data = new unsigned char[*size];
		stream.readRaw(*data, *size);
		return true;
	}

	bool PixelShader::_loadResourceData(chstr filename, unsigned char** data, int* size)
	{
		if (!hresource::exists(filename))
		{
			return false;
		}
		hresource stream;
		stream.open(filename);
		*size = (int)stream.size();
		*data = new unsigned char[*size];
		stream.readRaw(*data, *size);
		return true;
	}

}
