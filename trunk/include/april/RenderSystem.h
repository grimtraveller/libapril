/************************************************************************************\
This source file is part of the Awesome Portable Rendering Interface Library         *
For latest info, see http://libapril.sourceforge.net/                                *
**************************************************************************************
Copyright (c) 2010 Kresimir Spes (kreso@cateia.com)                                  *
*                                                                                    *
* This program is free software; you can redistribute it and/or modify it under      *
* the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php   *
\************************************************************************************/
#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H

#include <string>
#include <vector>
#include <gtypes/Vector3.h>
#include "AprilExport.h"

namespace gtypes
{
	class Vector2;
}

namespace April
{
	
	class ImageSource;
	// render operations
	enum RenderOp
	{
		TriangleList=1,
		TriangleStrip=2,
		TriangleFan=3,
		LineList=4,
		LineStrip=5,
		LineLoop=6,
	};
	
	class AprilExport TexturedVertex : public gtypes::Vector3
	{
	public:
		float u,v;
	};

	struct AprilExport PlainVertex : public gtypes::Vector3
	{
	public:
	};

	struct AprilExport ColoredVertex : public gtypes::Vector3
	{
	public:
		unsigned int color;
	};

	enum BlendMode
	{
		ALPHA_BLEND,
		ADD,
		DEFAULT
	};

	class AprilExport Color
	{
	public:
		unsigned char r,g,b,a;
		Color(float r,float g,float b,float a=1);
		Color(unsigned int color);
		Color(std::string hex);
		Color();

		void setColor(float r,float g,float b,float a=1);
		void setColor(unsigned int color);
		void setColor(std::string hex);

		float r_float() { return (float) r/255.0f; }
		float g_float() { return (float) g/255.0f; }
		float b_float() { return (float) b/255.0f; }
		float a_float() { return (float) a/255.0f; }

	};

	class AprilExport Texture
	{
	protected:
		bool mDynamic;
		std::string mFilename;
		int mWidth,mHeight;
		float mUnusedTimer;
		std::vector<Texture*> mDynamicLinks;
	public:
		Texture();
		virtual ~Texture();
		virtual void unload()=0;
		virtual int getSizeInBytes()=0;
		
		virtual Color getPixel(int x,int y);
		virtual Color getInterpolatedPixel(float x,float y);
		
		void addDynamicLink(Texture* lnk);
		void removeDynamicLink(Texture* lnk);
		void _resetUnusedTimer(bool recursive=1);
		
		int getWidth() { return mWidth; };
		int getHeight() { return mHeight; };
		/// only used with dynamic textures since at chapter load you need it's dimensions for images, but you don't know them yet
		void _setDimensions(int w,int h) { mWidth=w; mHeight=h; }
		bool isDynamic() { return mDynamic; }
		virtual bool isLoaded()=0;
		
		void update(float time_increase);
		std::string getFilename() { return mFilename; }
	};
	
	class AprilExport RAMTexture : public Texture
	{
	protected:
		ImageSource* mBuffer;
	public:
		RAMTexture(std::string filename,bool dynamic);
		virtual ~RAMTexture();
		void load();
		void unload();
		bool isLoaded();
		Color getPixel(int x,int y);
		Color getInterpolatedPixel(float x,float y);
		int getSizeInBytes();
		
	};

	class AprilExport RenderSystem
	{
	protected:
		float mAlphaMultiplier;
		float mIdleUnloadTime;
		bool mDynamicLoading;
		
		bool (*mUpdateCallback)(float);
		void (*mMouseDownCallback)(float,float,int);
		void (*mMouseUpCallback)(float,float,int);
		void (*mMouseMoveCallback)(float,float);
		void (*mKeyDownCallback)(unsigned int,unsigned int);
		void (*mKeyUpCallback)(unsigned int,unsigned int);
		
		
	public:
		virtual std::string getName()=0;

		RenderSystem();
		virtual ~RenderSystem();

		// object creation
		virtual Texture* loadTexture(std::string filename,bool dynamic=false)=0;
		Texture* loadRAMTexture(std::string filename,bool dynamic=false);
		virtual Texture* createTextureFromMemory(unsigned char* rgba,int w,int h)=0;

		// modelview matrix transformation
		virtual void setIdentityTransform()=0;
		virtual void translate(float x,float y)=0;
		virtual void rotate(float angle)=0; // degrees!
		virtual void scale(float s)=0;
		virtual void pushTransform()=0;
		virtual void popTransform()=0;
		virtual void setBlendMode(BlendMode mode)=0;
		
		// projection matrix tronsformation
		virtual void setViewport(float w,float h,float x_offset=0,float y_offset=0)=0;
		
		// rendering
		virtual void clear(bool color=true,bool depth=false)=0;
		virtual void setTexture(Texture* t)=0;
		virtual void render(RenderOp renderOp,TexturedVertex* v,int nVertices)=0;
		virtual void render(RenderOp renderOp,TexturedVertex* v,int nVertices,float r,float g,float b,float a)=0;
		virtual void render(RenderOp renderOp,PlainVertex* v,int nVertices)=0;
		virtual void render(RenderOp renderOp,PlainVertex* v,int nVertices,float r,float g,float b,float a)=0;
		virtual void render(RenderOp renderOp,ColoredVertex* v,int nVertices)=0;
		
		void drawColoredQuad(float x,float y,float w,float h,float r,float g,float b,float a=1);

		float getIdleTextureUnloadTime() { return mIdleUnloadTime; }
		void setIdleTextureUnloadTime(float time) { mIdleUnloadTime=time; }

		void logMessage(std::string message,std::string prefix="[april] ");
		void setLogFunction(void (*fnptr)(std::string));

		virtual void setAlphaMultiplier(float value)=0;
		float getAlphaMultiplier() { return mAlphaMultiplier; }

		virtual int getWindowWidth()=0;
		virtual int getWindowHeight()=0;

		virtual void setWindowTitle(std::string title)=0;
		virtual gtypes::Vector2 getCursorPos()=0;
		virtual void showSystemCursor(bool b)=0;
		virtual bool isSystemCursorShown()=0;

		virtual void presentFrame()=0;
		virtual void enterMainLoop()=0;

		void registerUpdateCallback(bool (*callback)(float));
		void registerMouseCallbacks(void (*mouse_dn)(float,float,int),
									void (*mouse_up)(float,float,int),
									void (*mouse_move)(float,float));
		void registerKeyboardCallbacks(void (*key_dn)(unsigned int,unsigned int),
									   void (*key_up)(unsigned int,unsigned int));

		void forceDynamicLoading(bool value) { mDynamicLoading=value; }
		bool isDynamicLoadingForced() { return mDynamicLoading; }

		virtual void terminateMainLoop()=0;
	};

	AprilFnExport void init(std::string rendersystem_name,int w,int h,bool fullscreen,std::string title);
	AprilFnExport void destroy();
}
// global rendersys shortcut variable
AprilFnExport extern April::RenderSystem* rendersys;

#endif
