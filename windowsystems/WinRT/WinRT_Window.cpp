/// @file
/// @author  Boris Mikic
/// @version 2.5
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php

#ifdef _WIN32
#include <hltypes/hplatform.h>
#if _HL_WINRT
#include <hltypes/hltypesUtil.h>
#include <hltypes/hthread.h>

#include "april.h"
#include "RenderSystem.h"
#include "Timer.h"
#include "WinRT_View.h"
#include "WinRT_ViewSource.h"
#include "WinRT_Window.h"

using namespace Windows::ApplicationModel::Core;

namespace april
{
	WinRT_Window::WinRT_Window() : Window()
	{
		this->name = APRIL_WS_WINRT;
		this->width = 0;
		this->height = 0;
		this->multiTouchActive = false;
	}

	WinRT_Window::~WinRT_Window()
	{
		this->destroy();
	}

	bool WinRT_Window::create(int w, int h, bool fullscreen, chstr title)
	{
		if (!Window::create(w, h, fullscreen, title))
		{
			return false;
		}
		this->width = w;
		this->height = h;
		this->multiTouchActive = false;
		this->setCursorVisible(true);
		return true;
	}
	
	bool WinRT_Window::destroy()
	{
		if (!Window::destroy())
		{
			return false;
		}
		return true;
	}

	/*
	void WinRT_Window::setTitle(chstr title)
	{
		this->title = title;
		hstr t = this->title;
#ifdef _DEBUG
		t += this->_fpsTitle;
#endif
		SetWindowTextW(this->hWnd, t.w_str().c_str());
	}
	
	bool WinRT_Window::isCursorVisible()
	{
		return (Window::isCursorVisible() || !this->isCursorInside());
	}
	
	void WinRT_Window::setCursorVisible(bool value)
	{
		Window::setCursorVisible(value);
		this->isCursorVisible() ? SetCursor(LoadCursor(0, IDC_ARROW)) : SetCursor(0);
	}
	*/
	
	void* WinRT_Window::getBackendId()
	{
		// TODO ?
		return 0;
	}

	/*
	void WinRT_Window::_setResolution(int w, int h)
	{
		int x = 0;
		int y = 0;
		DWORD style = WS_EX_TOPMOST | WS_POPUP;
		if (!this->fullscreen)
		{
			x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2,
			y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
			style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
		}
		if (!this->fullscreen)
		{
			RECT rcClient;
			RECT rcWindow;
			POINT ptDiff;
			GetClientRect(this->hWnd, &rcClient);
			GetWindowRect(this->hWnd, &rcWindow);
			ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
			ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
			MoveWindow(this->hWnd, rcWindow.left, rcWindow.top, ptDiff.x + w, ptDiff.y + h, TRUE);
		}
		// display the window on the screen
		ShowWindow(this->hWnd, 1);
		UpdateWindow(this->hWnd);
	}
	*/
	
	bool WinRT_Window::updateOneFrame()
	{
		static bool result;
		this->checkEvents();
		// rendering
		result = Window::updateOneFrame();
		this->setTitle(this->title); // has to come after Window::updateOneFrame(), otherwise FPS value in title would be late one frame
		return result;
	}
	
	void WinRT_Window::presentFrame()
	{
	}
	
	void WinRT_Window::checkEvents()
	{
		april::WinRT::View->checkEvents();
		while (this->mouseEvents.size() > 0)
		{
			MouseInputEvent e = this->mouseEvents.pop_first();
			this->cursorPosition = e.position;
			Window::handleMouseEvent(e.type, e.position, e.button);
		}
		while (this->keyEvents.size() > 0)
		{
			KeyInputEvent e = this->keyEvents.pop_first();
			Window::handleKeyEvent(e.type, e.keyCode, e.charCode);
		}
		while (this->touchEvents.size() > 0)
		{
			TouchInputEvent e = this->touchEvents.pop_first();
			Window::handleTouchEvent(e.touches);
		}
	}

	void WinRT_Window::handleTouchEvent(MouseEventType type, gvec2 position, int index)
	{
		switch (type)
		{
		case AMOUSEEVT_DOWN:
			if (index < this->touches.size()) // DOWN event of an already indexed touch, never happened so far
			{
				return;
			}
			this->touches += position;
			break;
		case AMOUSEEVT_UP:
			if (index >= this->touches.size()) // redundant UP event, can happen
			{
				return;
			}
			this->touches.remove_at(index);
			break;
		case AMOUSEEVT_MOVE:
			if (index >= this->touches.size()) // MOVE event of an unindexed touch, never happened so far
			{
				return;
			}
			this->touches[index] = position;
			break;
		}
		if (this->multiTouchActive || this->touches.size() > 1)
		{
			this->multiTouchActive = (this->touches.size() > 0);
		}
		else
		{
			this->handleMouseEvent(type, position, AMOUSEBTN_LEFT);
		}
		this->touchEvents += TouchInputEvent(this->touches);
	}

	void WinRT_Window::handleMouseEvent(MouseEventType type, gvec2 position, MouseButton button)
	{
		this->mouseEvents += MouseInputEvent(type, position, button);
	}

	void WinRT_Window::handleKeyEvent(KeyEventType type, KeySym keyCode, unsigned int charCode)
	{
		this->keyEvents += KeyInputEvent(type, keyCode, charCode);
	}

}
#endif
#endif