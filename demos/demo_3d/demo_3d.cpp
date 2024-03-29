/// @file
/// @version 3.5
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#ifndef _ANDROID
#ifndef _WINRT
#define RESOURCE_PATH "../../demos/media/"
#else
#define RESOURCE_PATH "media/"
#endif
#elif defined(__APPLE__)
#define RESOURCE_PATH "media/"
#else
#define RESOURCE_PATH "./"
#endif

#include <april/april.h>
#include <april/Cursor.h>
#include <april/main.h>
#include <april/Platform.h>
#include <april/RenderSystem.h>
#include <april/SystemDelegate.h>
#include <april/UpdateDelegate.h>
#include <april/Window.h>
#include <hltypes/hlog.h>

#define LOG_TAG "demo_3d"

april::Cursor* cursor = NULL;
april::Texture* texture = NULL;
april::TexturedVertex v[14];

#if !defined(_ANDROID) && !defined(_IOS) && !defined(_WINP8)
grect drawRect(0.0f, 0.0f, 800.0f, 600.0f);
#else
grect drawRect(0.0f, 0.0f, 480.0f, 320.0f);
#endif

class UpdateDelegate : public april::UpdateDelegate
{
public:
	UpdateDelegate() : april::UpdateDelegate(), angle(0.0f)
	{
	}

	bool onUpdate(float timeDelta)
	{
		this->angle += timeDelta * 90.0f;
		april::rendersys->clear(true, true);
		april::rendersys->setPerspective(60.0f, 1.0f / drawRect.getAspect(), 1.0f, 1000.0f);
		april::rendersys->lookAt(gvec3(0.0f, 2.0f, 5.0f), gvec3(0.0f, 0.0f, 0.0f), gvec3(0.0f, 1.0f, 0.0f));
		april::rendersys->rotate(this->angle, 0.0f, 1.0f, 0.0f);
		april::rendersys->setTexture(texture);
		april::rendersys->render(april::RO_TRIANGLE_STRIP, v, 14);
		return true;
	}

protected:
	float angle;

};

class SystemDelegate : public april::SystemDelegate
{
public:
	SystemDelegate() : april::SystemDelegate()
	{
	}

	void onWindowSizeChanged(int width, int height, bool fullScreen)
	{
		hlog::writef(LOG_TAG, "window size changed: %dx%d", width, height);
		april::rendersys->setViewport(drawRect);
	}

};

static UpdateDelegate* updateDelegate = NULL;
static SystemDelegate* systemDelegate = NULL;

void april_init(const harray<hstr>& args)
{
#ifdef __APPLE__
	// On MacOSX, the current working directory is not set by
	// the Finder, since you are expected to use Core Foundation
	// or ObjC APIs to find files. 
	// So, when porting you probably want to set the current working
	// directory to something sane (e.g. .../Resources/ in the app
	// bundle).
	// In this case, we set it to parent of the .app bundle.
	{	// curly braces in order to localize variables 

		CFURLRef url = CFBundleCopyBundleURL(CFBundleGetMainBundle());
		CFStringRef path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
		// let's hope chdir() will be happy with utf8 encoding
		const char* cpath = CFStringGetCStringPtr(path, kCFStringEncodingUTF8);
		char* cpath_alloc = NULL;
		if (cpath == NULL)
		{
			// CFStringGetCStringPtr is allowed to return NULL. bummer.
			// we need to use CFStringGetCString instead.
			cpath_alloc = (char*)malloc(CFStringGetLength(path) + 1);
			CFStringGetCString(path, cpath_alloc, CFStringGetLength(path) + 1, kCFStringEncodingUTF8);
		}
		else
		{
			// even though it didn't return NULL, we still want to slice off bundle name.
			cpath_alloc = (char*)malloc(CFStringGetLength(path) + 1);
			strcpy(cpath_alloc, cpath);
		}
		// just in case / is appended to .app path for some reason
		if (cpath_alloc[CFStringGetLength(path) - 1] == '/')
		{
			cpath_alloc[CFStringGetLength(path) - 1] = 0;
		}
		// replace pre-.app / with a null character, thus
		// cutting off .app's name and getting parent of .app.
		strrchr(cpath_alloc, '/')[0] = 0;
		// change current dir using posix api
		chdir(cpath_alloc);
		free(cpath_alloc); // even if null, still ok
		CFRelease(path);
		CFRelease(url);
	}
#endif
	srand((unsigned int)htime());
	updateDelegate = new UpdateDelegate();
	systemDelegate = new SystemDelegate();
#if defined(_ANDROID) || defined(_IOS) || defined(_WINRT)
	drawRect.setSize(april::getSystemInfo().displayResolution);
#endif
	april::init(april::RS_DEFAULT, april::WS_DEFAULT);
	april::RenderSystem::Options options;
	options.depthBuffer = true;
	april::createRenderSystem(options);
	april::createWindow((int)drawRect.w, (int)drawRect.h, false, "APRIL: Simple 3D");
#ifdef _WINRT
	april::window->setParam("cursor_mappings", "101 " RESOURCE_PATH "cursor\n102 " RESOURCE_PATH "simple");
#endif
	april::rendersys->setDepthBuffer(true, true);
	april::window->setUpdateDelegate(updateDelegate);
	april::window->setSystemDelegate(systemDelegate);
	cursor = april::window->createCursor(RESOURCE_PATH "cursor");
	april::window->setCursor(cursor);
	texture = april::rendersys->createTextureFromResource(RESOURCE_PATH "texture");
	april::TexturedVertex _v[8];
	_v[0].x = -1.0f;	_v[0].y = -1.0f;	_v[0].z = 1.0f;		_v[0].u = 0.0f;	_v[0].v = 1.0f;
	_v[1].x = 1.0f;		_v[1].y = -1.0f;	_v[1].z = 1.0f;		_v[1].u = 1.0f;	_v[1].v = 1.0f;
	_v[2].x = -1.0f;	_v[2].y = 1.0f;		_v[2].z = 1.0f;		_v[2].u = 0.0f;	_v[2].v = 0.0f;
	_v[3].x = 1.0f;		_v[3].y = 1.0f;		_v[3].z = 1.0f;		_v[3].u = 1.0f;	_v[3].v = 0.0f;
	_v[4].x = -1.0f;	_v[4].y = -1.0f;	_v[4].z = -1.0f;	_v[4].u = 1.0f;	_v[4].v = 0.0f;
	_v[5].x = 1.0f;		_v[5].y = -1.0f;	_v[5].z = -1.0f;	_v[5].u = 0.0f;	_v[5].v = 0.0f;
	_v[6].x = -1.0f;	_v[6].y = 1.0f;		_v[6].z = -1.0f;	_v[6].u = 1.0f;	_v[6].v = 1.0f;
	_v[7].x = 1.0f;		_v[7].y = 1.0f;		_v[7].z = -1.0f;	_v[7].u = 0.0f;	_v[7].v = 1.0f;
	v[0] = _v[0];
	v[1] = _v[1];
	v[2] = _v[4];
	v[3] = _v[5];
	v[4] = _v[7];
	v[5] = _v[1];
	v[6] = _v[3];
	v[7] = _v[0];
	v[8] = _v[2];
	v[9] = _v[4];
	v[10] = _v[6];
	v[11] = _v[7];
	v[12] = _v[2];
	v[13] = _v[3];
}

void april_destroy()
{
	april::window->setCursor(NULL);
	delete cursor;
	delete texture;
	april::destroy();
	delete updateDelegate;
	updateDelegate = NULL;
	delete systemDelegate;
	systemDelegate = NULL;
}
