/// @file
/// @author  Kresimir Spes
/// @author  Ivan Vucica
/// @author  Boris Mikic
/// @version 3.3
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php

#ifdef _OPENGL
#include <hltypes/hplatform.h>
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
#if TARGET_OS_IPHONE
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#elif _OPENGLES
#include <GLES/gl.h>
#else
#ifndef __APPLE__
#include <gl/GL.h>
#else
#include <OpenGL/gl.h>
#endif
#endif

#include <hltypes/hlog.h>
#include <hltypes/hstring.h>

#include "april.h"
#include "Image.h"
#include "OpenGL_RenderSystem.h"
#include "OpenGL_State.h"
#include "OpenGL_Texture.h"

#define APRIL_OGL_RENDERSYS ((OpenGL_RenderSystem*)april::rendersys)

namespace april
{
    static inline bool isPower2(int x)
    {
        return (x > 0) && ((x & (x - 1)) == 0);
    }

	OpenGL_Texture::OpenGL_Texture() : Texture(), textureId(0), glFormat(0), internalFormat(0)
	{
	}

	bool OpenGL_Texture::_createInternalTexture(unsigned char* data, int size)
	{
		glGenTextures(1, &this->textureId);
		this->_setCurrentTexture();
		// required first call of glTexImage2D() to prevent problems
#if TARGET_OS_IPHONE
		if (this->dataFormat == GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG || this->dataFormat == GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG)
		{
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, this->dataFormat, this->width, this->height, 0, size, data);
		}
		else
#endif
		{
			int size = this->getByteSize();
			unsigned char* clearColor = new unsigned char[size];
			memset(clearColor, 0, size);
			glTexImage2D(GL_TEXTURE_2D, 0, this->internalFormat, this->width, this->height, 0, this->glFormat, GL_UNSIGNED_BYTE, clearColor);
			delete [] clearColor;
		}
		// Non power of 2 textures in OpenGL, must have addressing mode set to clamp, otherwise they won't work.
		if (!isPower2(this->width) || !isPower2(this->height))
		{
			this->addressMode = ADDRESS_CLAMP;
		}
		return true;
	}
	
	void OpenGL_Texture::_assignFormat()
	{
		switch (this->format)
		{
		case Image::FORMAT_ARGB:
		case Image::FORMAT_XRGB:
		case Image::FORMAT_RGBA:
		case Image::FORMAT_RGBX:
		case Image::FORMAT_ABGR:
		case Image::FORMAT_XBGR:
			this->glFormat = this->internalFormat = GL_RGBA;
			break;
		// for optimizations
		case Image::FORMAT_BGRA:
		case Image::FORMAT_BGRX:
#if !defined(_ANDROID) && !defined(_WIN32)
#ifndef __APPLE__
			this->glFormat = GL_BGRA;
#else
			this->glFormat = GL_BGRA_EXT;
#endif
#else
			this->glFormat = GL_RGBA;
#endif
			this->internalFormat = GL_RGBA;
			break;
		case Image::FORMAT_RGB:
			this->glFormat = this->internalFormat = GL_RGB;
			break;
		// for optimizations
		case Image::FORMAT_BGR:
#if !defined(_ANDROID) && !defined(_WIN32)
#ifndef __APPLE__
			this->glFormat = GL_BGR;
#else
			this->glFormat = GL_BGR_EXT;
#endif
#else
			this->glFormat = GL_RGB;
#endif
			this->internalFormat = GL_RGB;
			break;
		case Image::FORMAT_ALPHA:
			this->glFormat = this->internalFormat = GL_ALPHA;
			break;
		case Image::FORMAT_GRAYSCALE:
			this->glFormat = this->internalFormat = GL_LUMINANCE;
			break;
		case Image::FORMAT_PALETTE: // TODOaa - does palette use RGBA?
			this->glFormat = this->internalFormat = GL_RGBA;
			break;
		}
	}

	OpenGL_Texture::~OpenGL_Texture()
	{
		this->unload();
	}

	void OpenGL_Texture::_setCurrentTexture()
	{
		APRIL_OGL_RENDERSYS->currentState.textureId = APRIL_OGL_RENDERSYS->deviceState.textureId = this->textureId;
		glBindTexture(GL_TEXTURE_2D, this->textureId);
		APRIL_OGL_RENDERSYS->currentState.textureFilter = APRIL_OGL_RENDERSYS->deviceState.textureFilter = this->filter;
		APRIL_OGL_RENDERSYS->_setTextureFilter(this->filter);
		APRIL_OGL_RENDERSYS->currentState.textureAddressMode = APRIL_OGL_RENDERSYS->deviceState.textureAddressMode = this->addressMode;
		APRIL_OGL_RENDERSYS->_setTextureAddressMode(this->addressMode);
	}

	bool OpenGL_Texture::isLoaded()
	{
		return (this->textureId != 0);
	}

	void OpenGL_Texture::unload()
	{
		if (this->textureId != 0)
		{
			hlog::write(april::logTag, "Unloading GL texture: " + this->_getInternalName());
			glDeleteTextures(1, &this->textureId);
			this->textureId = 0;
		}
	}

	void OpenGL_Texture::clear()
	{
		if (this->data != NULL)
		{
			Texture::clear();
			return;
		}
		int size = this->getByteSize();
		unsigned char* clearColor = new unsigned char[size];
		memset(clearColor, 0, size);
		this->_setCurrentTexture();
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->width, this->height, GL_RGBA, GL_UNSIGNED_BYTE, clearColor);
		delete [] clearColor;
	}

	Color OpenGL_Texture::getPixel(int x, int y)
	{
		if (this->data != NULL)
		{
			return Texture::getPixel(x, y);
		}
		Color result = april::Color::Clear;
		Image::Format format = april::rendersys->getNativeTextureFormat(this->format);
		unsigned char* pixels = NULL;
		if (this->copyPixelData(&pixels, format)) // it's not possible to get just one pixel on OpenGL so the entire texture has to be retrieved (expensive!)
		{
			result = Image::getPixel(x, y, pixels, this->width, this->height, format);
			delete [] pixels;
		}
		return result;
	}

	void OpenGL_Texture::setPixel(int x, int y, Color color)
	{
		if (this->data != NULL)
		{
			Texture::setPixel(x, y, color);
			return;
		}
		if (!Image::checkRect(x, y, this->width, this->height))
		{
			return;
		}
		unsigned char writeData[4] = {color.r, color.g, color.b, color.a};
		this->_setCurrentTexture();
		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, writeData);
	}
	
	void OpenGL_Texture::fillRect(int x, int y, int w, int h, Color color)
	{
		if (w == 1 && h == 1)
		{
			this->setPixel(x, y, color);
			return;
		}
		if (this->data != NULL)
		{
			Texture::fillRect(x, y, w, h, color);
			return;
		}
		if (!Image::correctRect(x, y, w, h, this->width, this->height))
		{
			return;
		}
		Image::Format nativeFormat = april::rendersys->getNativeTextureFormat(this->format);
		int size = w * h * Image::getFormatBpp(nativeFormat);
		unsigned char* writeData = new unsigned char[size];
		if (Image::fillRect(0, 0, w, h, color, writeData, w, h, nativeFormat))
		{
			this->_setCurrentTexture();
			glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, this->glFormat, GL_UNSIGNED_BYTE, writeData);
		}
		delete [] writeData;
	}
	
	void OpenGL_Texture::write(int sx, int sy, int sw, int sh, int dx, int dy, unsigned char* srcData, int srcWidth, int srcHeight, Image::Format srcFormat)
	{
		if (this->data != NULL)
		{
			Texture::write(sx, sy, sw, sh, dx, dy, srcData, srcWidth, srcHeight, srcFormat);
			return;
		}
		if (!Image::correctRect(sx, sy, sw, sh, srcWidth, srcHeight, dx, dy, this->width, this->height))
		{
			return;
		}
		int srcBpp = Image::getFormatBpp(srcFormat);
		Image::Format nativeFormat = april::rendersys->getNativeTextureFormat(this->format);
		int destBpp = Image::getFormatBpp(nativeFormat);
		if (!Image::needsConversion(format, nativeFormat) && sx == 0 && dx == 0 && srcWidth == this->width && sw == this->width) // to avoid unnecessary copying
		{
			this->_setCurrentTexture();
			glTexSubImage2D(GL_TEXTURE_2D, 0, dx, dy, sw, sh, this->glFormat, GL_UNSIGNED_BYTE, &srcData[(sx + sy * srcWidth) * srcBpp]);
			return;
		}
		unsigned char* data = new unsigned char[sw * sh * destBpp];
		if (Image::write(sx, sy, sw, sh, 0, 0, srcData, srcWidth, srcHeight, srcFormat, data, sw, sh, nativeFormat))
		{
			this->_setCurrentTexture();
			glTexSubImage2D(GL_TEXTURE_2D, 0, dx, dy, sw, sh, this->glFormat, GL_UNSIGNED_BYTE, data);
		}
		delete [] data;
	}

	void OpenGL_Texture::writeStretch(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh, unsigned char* srcData, int srcWidth, int srcHeight, Image::Format srcFormat)
	{
		if (this->data != NULL)
		{
			Texture::writeStretch(sx, sy, sw, sh, dx, dy, dw, dh, srcData, srcWidth, srcHeight, srcFormat);
			return;
		}
		if (!Image::correctRect(sx, sy, sw, sh, srcWidth, srcHeight, dx, dy, dw, dh, this->width, this->height))
		{
			return;
		}
		int srcBpp = Image::getFormatBpp(srcFormat);
		Image::Format nativeFormat = april::rendersys->getNativeTextureFormat(this->format);
		int destBpp = Image::getFormatBpp(nativeFormat);
		unsigned char* data = new unsigned char[dw * dh * destBpp];
		if (Image::writeStretch(sx, sy, sw, sh, 0, 0, dw, dh, srcData, srcWidth, srcHeight, srcFormat, data, dw, dh, nativeFormat))
		{
			this->_setCurrentTexture();
			glTexSubImage2D(GL_TEXTURE_2D, 0, dx, dy, dw, dh, this->glFormat, GL_UNSIGNED_BYTE, data);
		}
		delete [] data;
	}

	void OpenGL_Texture::blit(int sx, int sy, int sw, int sh, int dx, int dy, unsigned char* srcData, int srcWidth, int srcHeight, Image::Format srcFormat, unsigned char alpha)
	{
		if (this->data != NULL)
		{
			Texture::blit(sx, sy, sw, sh, dx, dy, srcData, srcWidth, srcHeight, srcFormat, alpha);
			return;
		}
		if (!Image::correctRect(sx, sy, sw, sh, srcWidth, srcHeight, dx, dy, this->width, this->height))
		{
			return;
		}
		Image::Format nativeFormat = april::rendersys->getNativeTextureFormat(this->format);
		unsigned char* data = NULL;
		if (!this->copyPixelData(&data, nativeFormat))
		{
			return;
		}
		if (!Image::blit(sx, sy, sw, sh, dx, dy, srcData, srcWidth, srcHeight, srcFormat, data, this->width, this->height, nativeFormat, alpha))
		{
			delete [] data;
			return;
		}
		int dataBpp = Image::getFormatBpp(nativeFormat);
		if (dx == 0 && sw == this->width)
		{
			this->_setCurrentTexture();
			glTexSubImage2D(GL_TEXTURE_2D, 0, dx, dy, sw, sh, this->glFormat, GL_UNSIGNED_BYTE, &data[(dx + dy * this->width) * dataBpp]);
			delete [] data;
			return;
		}
		unsigned char* writeData = new unsigned char[sw * sh * dataBpp];
		if (Image::write(dx, dy, sw, sh, 0, 0, data, this->width, this->height, nativeFormat, writeData, sw, sh, nativeFormat))
		{
			this->_setCurrentTexture();
			glTexSubImage2D(GL_TEXTURE_2D, 0, dx, dy, sw, sh, this->glFormat, GL_UNSIGNED_BYTE, writeData);
			delete [] writeData;
		}
		delete [] data;
	}

	// TODOaa - writeStretch goes here

	void OpenGL_Texture::stretchBlit(int x, int y, int w, int h, Texture* texture, int sx, int sy, int sw, int sh, unsigned char alpha)
	{
		// TODO
	}

	void OpenGL_Texture::stretchBlit(int x, int y, int w, int h, unsigned char* data, int dataWidth, int dataHeight, int dataBpp, int sx, int sy, int sw, int sh, unsigned char alpha)
	{
		// TODO
	}

	void OpenGL_Texture::rotateHue(float degrees)
	{
		// TODO
	}

	void OpenGL_Texture::saturate(float factor)
	{
		// TODO
	}

	bool OpenGL_Texture::_uploadDataToGpu(int x, int y, int w, int h)
	{
		if (this->data == NULL)
		{
			return false;
		}
		if (!Image::correctRect(x, y, w, h, this->width, this->height))
		{
			return false;
		}
		int dataBpp = this->getBpp();
		Image::Format nativeFormat = april::rendersys->getNativeTextureFormat(this->format);
		int gpuBpp = Image::getFormatBpp(nativeFormat);
		if (!Image::needsConversion(this->format, nativeFormat) && x == 0 && w == this->width)
		{
			this->_setCurrentTexture();
			glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, this->glFormat, GL_UNSIGNED_BYTE, &this->data[(x + y * this->width) * dataBpp]);
			return true;
		}
		unsigned char* writeData = new unsigned char[w * h * gpuBpp];
		bool result = Image::write(x, y, w, h, 0, 0, this->data, this->width, this->height, this->format, writeData, w, h, nativeFormat);
		if (result)
		{
			this->_setCurrentTexture();
			glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, this->glFormat, GL_UNSIGNED_BYTE, writeData);
		}
		delete [] writeData;
		return result;
	}

}
#endif
