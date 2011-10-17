/************************************************************************************\
This source file is part of the Awesome Portable Rendering Interface Library         *
For latest info, see http://libapril.sourceforge.net/                                *
**************************************************************************************
Copyright (c) 2010 Ivan Vucica (ivan@vucica.net)                                     *
*                                                                                    *
* This program is free software; you can redistribute it and/or modify it under      *
* the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php   *
\************************************************************************************/

#ifndef APRIL_GLUTWINDOW_H_INCLUDED
#define APRIL_GLUTWINDOW_H_INCLUDED

#include <hltypes/hstring.h>
#include "Window.h"
#include "AprilExport.h"
namespace April
{
	class GLUTWindow : public Window
	{
	private:
		static void _handleKeyUp(unsigned char key, int x, int y);
		static void _handleKeyDown(unsigned char key, int x, int y);
		static void _handleKeySpecial(int key, int x, int y);
		
		static void _handleMouseButton(int button, int state, int x,int y);		
		static void _handleMouseMove(int x,int y);
		
		static void _handleDisplayAndUpdate();
		
		static void _handleQuitRequest();
		
	public:
		
		GLUTWindow(int w, int h, bool fullscreen, chstr title);
		
		// implementations
		void enterMainLoop();
		void terminateMainLoop();
		void showSystemCursor(bool visible);
		bool isSystemCursorShown();
		int getWindowWidth();
		int getWindowHeight();
		void setWindowTitle(chstr title);
		gtypes::Vector2 getCursorPos();
		void presentFrame();
		void* getIDFromBackend();
		void doEvents();
		
		// override functions
		void handleKeyEvent(Window::KeyEventType type, unsigned int keycode, unsigned int unicode);
		
		
		
		static GLUTWindow *_instance;

		float mCursorX, mCursorY; // TODO turn into private
	};
}

#endif