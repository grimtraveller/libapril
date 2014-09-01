/// @file
/// @author  Kresimir Spes
/// @author  Boris Mikic
/// @author  Ivan Vucica
/// @version 3.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
/// 
/// @section DESCRIPTION
/// 
/// Defines a generic window.

#ifndef APRIL_WINDOW_H
#define APRIL_WINDOW_H

#include <gtypes/Vector2.h>
#include <hltypes/hmap.h>
#include <hltypes/hstring.h>

#include "aprilExport.h"
#include "Keys.h"
#include "Timer.h"

namespace april
{
	class KeyboardDelegate;
	class MouseDelegate;
	class RenderSystem;
	class SystemDelegate;
	class TouchDelegate;
	class UpdateDelegate;

	class aprilExport Window
	{
	public:
		enum MouseEventType
		{
			AMOUSEEVT_DOWN = 0,
			AMOUSEEVT_UP = 1,
			AMOUSEEVT_MOVE = 2,
			AMOUSEEVT_SCROLL = 3
		};
		
		enum KeyEventType
		{
			AKEYEVT_DOWN = 0,
			AKEYEVT_UP = 1
		};
		
		enum DeviceOrientation
		{
			ADEVICEORIENTATION_NONE = 0,
			ADEVICEORIENTATION_PORTRAIT,
			ADEVICEORIENTATION_PORTRAIT_UPSIDEDOWN,
			ADEVICEORIENTATION_LANDSCAPE_LEFT, // bottom of device is on the left
			ADEVICEORIENTATION_LANDSCAPE_RIGHT, // bottom of device is on the right
			ADEVICEORIENTATION_FACE_DOWN, // screen is facing the ground
			ADEVICEORIENTATION_FACE_UP // screen is facing the sky
		};
		
		Window();
		virtual ~Window();
		virtual bool create(int w, int h, bool fullscreen, chstr title, chstr options = "");
		virtual bool destroy();

		// generic getters/setters
		HL_DEFINE_GET(hstr, name, Name);
		HL_DEFINE_IS(bool, created, Created);
		HL_DEFINE_GET(hstr, title, Title);
		HL_DEFINE_IS(bool, fullscreen, Fullscreen);
		void setFullscreen(bool value) { } // TODO
		HL_DEFINE_IS(bool, focused, Focused);
		HL_DEFINE_IS(bool, running, Running);
		HL_DEFINE_GET(int, fps, Fps);
		HL_DEFINE_GETSET(float, fpsResolution, FpsResolution);
		HL_DEFINE_GET(gvec2, cursorPosition, CursorPosition);
		gvec2 getSize();
		float getAspectRatio();

		// callbacks
		HL_DEFINE_GETSET(UpdateDelegate*, updateDelegate, UpdateDelegate);
		HL_DEFINE_GETSET(KeyboardDelegate*, keyboardDelegate, KeyboardDelegate);
		HL_DEFINE_GETSET(MouseDelegate*, mouseDelegate, MouseDelegate);
		HL_DEFINE_GETSET(TouchDelegate*, touchDelegate, TouchDelegate);
		HL_DEFINE_GETSET(SystemDelegate*, systemDelegate, SystemDelegate);

		// virtual getters/setters
		virtual void setTitle(chstr value) { this->title = value; }
		virtual bool isVirtualKeyboardVisible() { return false; }
		virtual bool isCursorVisible() { return this->cursorVisible; }
		virtual void setCursorVisible(bool value) { this->cursorVisible = value; }
		virtual bool isCursorInside();

		virtual void _setResolution(int w, int h) { }
		
		// pure virtual getters/setters (window system dependent)
		virtual int getWidth() = 0;
		virtual int getHeight() = 0;
		virtual void setTouchEnabled(bool value) = 0;
		virtual bool isTouchEnabled() = 0;
		virtual void* getBackendId() = 0;

		// pure virtual methods (window system dependent)
		virtual void presentFrame() = 0;
		virtual void checkEvents() = 0;
		virtual bool updateOneFrame();
		virtual void terminateMainLoop();

		// misc virtuals
		virtual void beginKeyboardHandling() { }
		virtual void terminateKeyboardHandling() { }
		
		virtual bool isRotating() { return false; } // iOS/Android devices for example
		virtual hstr getParam(chstr param) { return ""; }
		virtual void setParam(chstr param, chstr value) { }
		
		// generic but overridable event handlers
		virtual void handleMouseEvent(MouseEventType type, gvec2 position, Key button);
		virtual void handleKeyEvent(KeyEventType type, Key keyCode, unsigned int charCode);
		virtual void handleTouchEvent(const harray<gvec2>& touches);
		virtual bool handleQuitRequest(bool canCancel);
		virtual void handleFocusChangeEvent(bool focused);
		virtual bool handleUrl(chstr url);
		virtual void handleLowMemoryWarning();

		void handleKeyOnlyEvent(KeyEventType type, Key keyCode);
		void handleCharOnlyEvent(unsigned int charCode);

		virtual void enterMainLoop();
		virtual bool performUpdate(float k);
		
		// TODO - refactor
		// the following functions should be temporary, it was added because I needed access to
		// iOS early initialization process. When april will be refactored this needs to be changed --kspes
		static void setLaunchCallback(void (*callback)(void*)) { msLaunchCallback = callback; }
		static void handleLaunchCallback(void* args);

	protected:
		bool created;
		hstr name;
		hstr title;
		bool fullscreen;
		bool focused;
		bool running;
		int fps;
		int fpsCount;
		float fpsTimer;
		float fpsResolution;
		gvec2 cursorPosition;
		bool cursorVisible;
		Timer timer;

		// TODO - refactor
		static void (*msLaunchCallback)(void*);

		UpdateDelegate* updateDelegate;
		KeyboardDelegate* keyboardDelegate;
		MouseDelegate* mouseDelegate;
		TouchDelegate* touchDelegate;
		SystemDelegate* systemDelegate;

		virtual float _calcTimeSinceLastFrame();

	};

	// global window shortcut variable
	aprilFnExport extern april::Window* window;

}
#endif