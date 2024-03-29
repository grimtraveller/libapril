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

#include "VertexShader.h"

namespace april
{
	VertexShader::VertexShader()
	{
	}

	VertexShader::~VertexShader()
	{
	}

	bool VertexShader::_loadFileData(chstr filename, unsigned char** data, int* size)
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

	bool VertexShader::_loadResourceData(chstr filename, unsigned char** data, int* size)
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
