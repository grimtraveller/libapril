/// @file
/// @author  Kresimir Spes
/// @author  Boris Mikic
/// @author  Ivan Vucica
/// @version 1.5
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php

#include <hltypes/harray.h>
#include <hltypes/hltypesUtil.h>
#include <hltypes/hstring.h>

#include "april.h"
#include "Color.h"
#include "ImageSource.h"
#include "Texture.h"
#include "TextureManager.h"
#include "RenderSystem.h"

namespace april
{
	static void (*msTextureLoadingListener)(Texture*) = NULL;

	Texture::Texture()
	{
		mDynamic = false;
		mFilename = "";
		mWidth = 0;
		mHeight = 0;
		mBpp = 3;
		mUnusedTimer = 0.0f;
		mTextureFilter = Linear;
		mTextureWrapping = true;
		april::rendersys->getTextureManager()->registerTexture(this);
	}

	Texture::~Texture()
	{
		foreach (Texture*, it, mDynamicLinks)
		{
			(*it)->removeDynamicLink(this);
		}
		april::rendersys->getTextureManager()->unregisterTexture(this);
	}

	void Texture::setTextureLoadingListener(void (*listener)(Texture*))
	{
		msTextureLoadingListener = listener;
	}
	
	void Texture::notifyLoadingListener(Texture* t)
	{
		if (msTextureLoadingListener != NULL) msTextureLoadingListener(t);
	}

	hstr Texture::_getInternalName()
	{
		return (mFilename != "" ? mFilename : "UserTexture");
	}
	
	void Texture::fillRect(grect rect, Color color)
	{
		fillRect((int)rect.x, (int)rect.y, (int)rect.w, (int)rect.h, color);
	}
	
	Color Texture::getInterpolatedPixel(float x, float y)
	{
		Color result;
		int x0 = (int)x;
		int y0 = (int)x;
		int x1 = x0 + 1;
		int y1 = y0 + 1;
		float rx0 = x - x0;
		float ry0 = y - y0;
		float rx1 = 1.0f - rx0;
		float ry1 = 1.0f - ry0;
		if (rx0 != 0.0f && ry0 != 0.0f)
		{
			Color tl = this->getPixel(x0, y0);
			Color tr = this->getPixel(x1, y0);
			Color bl = this->getPixel(x0, y1);
			Color br = this->getPixel(x1, y1);
			Color l = tl * ry1 + bl * ry0;
			Color r = tr * ry1 + br * ry0;
			result = l * rx1 + r * rx0;
			//result = (tl * ry1 + bl * ry0) * rx1 + (tr * ry1 + br * ry0) * rx0; // causes a weird conversion from "float" to "unsigned int" warning
		}
		else if (rx0 != 0.0f)
		{
			Color tl = this->getPixel(x0, y0);
			Color tr = this->getPixel(x1, y0);
			result = tl * rx1 + tr * rx0;
		}
		else if (ry0 != 0.0f)
		{
			Color tl = this->getPixel(x0, y0);
			Color bl = this->getPixel(x0, y1);
			result = tl * ry1 + bl * ry0;
		}
		else
		{
			result = this->getPixel(x0, y0);
		}
		return result;
	}
	
	void Texture::update(float time_increase)
	{
		if (mDynamic && isLoaded())
		{
			float max_time = rendersys->getIdleTextureUnloadTime();
			if (max_time > 0)
			{
				if (mUnusedTimer > max_time)
				{
					unload();
				}
				mUnusedTimer += time_increase;
			}
		}
	}
	
	void Texture::addDynamicLink(Texture* lnk)
	{
		if (!mDynamicLinks.contains(lnk))
		{
			mDynamicLinks += lnk;
			lnk->addDynamicLink(this);
		}
	}
	
	void Texture::removeDynamicLink(Texture* lnk)
	{
		if (mDynamicLinks.contains(lnk))
		{
			mDynamicLinks -= lnk;
		}
	}
	
	void  Texture::_resetUnusedTimer(bool recursive)
	{
		mUnusedTimer = 0;
		if (recursive)
		{
			foreach (Texture*, it, mDynamicLinks)
			{
				(*it)->_resetUnusedTimer(0);
			}
		}
	}

	void Texture::insertAsAlphaMap(Texture* texture, unsigned char median, int ambiguity)
	{
	}

/************************************************************************************/
	RamTexture::RamTexture(chstr filename, bool dynamic) : Texture()
	{
		mFilename = filename;
		mBuffer = NULL;
		if (!dynamic)
		{
			load();
		}
	}

	RamTexture::RamTexture(int w, int h) : Texture()
	{
		mWidth = w;
		mHeight = h;
		mBuffer = createEmptyImage(w, h);
		mFilename = "";
	}

	RamTexture::~RamTexture()
	{
		unload();
	}

	bool RamTexture::load()
	{
		if (mBuffer == NULL)
		{
			april::log("loading RAM texture '" + _getInternalName() + "'");
			mBuffer = loadImage(mFilename);
			mWidth = mBuffer->w;
			mHeight = mBuffer->h;
			return true;
		}
		return false;
	}
	
	void RamTexture::unload()
	{
		if (mBuffer != NULL)
		{
			april::log("unloading RAM texture '" + _getInternalName() + "'");
			delete mBuffer;
			mBuffer = NULL;
		}
	}
	
	bool RamTexture::isLoaded()
	{
		return (mBuffer != NULL);
	}
	
	Color RamTexture::getPixel(int x, int y)
	{
		if (mBuffer == NULL)
		{
			load();
		}
		mUnusedTimer = 0;
		return mBuffer->getPixel(x, y);
	}
	
	void RamTexture::setPixel(int x, int y, Color c)
	{
		if (mBuffer == NULL)
		{
			load();
		}
		mUnusedTimer = 0;
		mBuffer->setPixel(x, y, c);
	}
	
	Color RamTexture::getInterpolatedPixel(float x, float y)
	{
		if (mBuffer == NULL)
		{
			load();
		}
		mUnusedTimer = 0;
		return mBuffer->getInterpolatedPixel(x, y);
	}
	
	int RamTexture::getSizeInBytes()
	{
		return (mWidth * mHeight * 3); // TODO - should use BPP instead of 3?
	}

	bool RamTexture::isValid()
	{
		return (mBuffer != NULL);
	}

}