/// @file
/// @author  Boris Mikic
/// @version 3.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php

#ifdef _WINRT_WINDOW
#include <hltypes/hfile.h>
#include <hltypes/hltypesUtil.h>
#include <hltypes/hlog.h>
#include <hltypes/hthread.h>

#include "april.h"
#include "RenderSystem.h"
#include "SystemDelegate.h"
#include "Timer.h"
#include "WinRT_View.h"
#include "WinRT_Window.h"

using namespace Windows::ApplicationModel::Core;

#define MANIFEST_FILENAME "AppxManifest.xml"
#define SNAP_VIEW_WIDTH 320 // as defined by Microsoft

namespace april
{
	WinRT_Window::WinRT_Window() : Window()
	{
		this->name = APRIL_WS_WINRT;
		this->width = 0;
		this->height = 0;
		this->hasStoredProjectionMatrix = false;
		this->backgroundColor = april::Color::Black;
		this->logoTexture = NULL;
	}

	WinRT_Window::~WinRT_Window()
	{
		this->destroy();
	}

	bool WinRT_Window::create(int w, int h, bool fullscreen, chstr title, Window::Options options)
	{
		if (!Window::create(w, h, fullscreen, title, options))
		{
			return false;
		}
		this->width = w;
		this->height = h;
		this->hasStoredProjectionMatrix = false;
		this->backgroundColor = april::Color::Black;
		this->logoTexture = NULL;
		this->setCursorVisible(true);
		return true;
	}
	
	bool WinRT_Window::destroy()
	{
		if (!Window::destroy())
		{
			return false;
		}
		this->hasStoredProjectionMatrix = false;
		return true;
	}

	void WinRT_Window::unassign()
	{
		Window::unassign();
		_HL_TRY_DELETE(this->logoTexture);
	}

	void WinRT_Window::setTitle(chstr title)
	{
		hlog::warn(april::logTag, "Window::setTitle() does nothing on WinRT.");
	}
	
	void WinRT_Window::setCursorVisible(bool value)
	{
		Window::setCursorVisible(value);
		WinRT::App->setCursorVisible(value);
	}
	
	void* WinRT_Window::getBackendId()
	{
		// TODO ?
		return 0;
	}

	void WinRT_Window::setResolution(int w, int h, bool fullscreen)
	{
		hlog::error(april::logTag, "Cannot change resolution on window system: " + this->name);
	}
	
	bool WinRT_Window::updateOneFrame()
	{
		static grect drawRect(0.0f, 0.0f, 1.0f, 1.0f);
		static grect srcRect(0.0f, 0.0f, 1.0f, 1.0f);
		static grect viewport(0.0f, 0.0f, 1.0f, 1.0f);
		// TODOr
		/*
		WinRT::View->updateViewState();
		if (WinRT::View->isFilled() || WinRT::View->isSnapped())
		{
			if (!this->hasStoredProjectionMatrix)
			{
				this->storedProjectionMatrix = april::rendersys->getProjectionMatrix();
				this->hasStoredProjectionMatrix = true;
			}
			this->_calcTimeSinceLastFrame();
			this->checkEvents();
			this->_tryLoadLogoTexture();
			april::rendersys->clear();
			viewport.setSize((float)this->width, (float)this->height);
			april::rendersys->setOrthoProjection(viewport);
			april::rendersys->drawFilledRect(viewport, this->backgroundColor);
			if (this->logoTexture != NULL)
			{
				drawRect.set(0.0f, (float)((this->height - this->logoTexture->getHeight()) / 2),
					(float)this->logoTexture->getWidth(), (float)this->logoTexture->getHeight());
				april::rendersys->setTexture(this->logoTexture);
				if (WinRT::View->isFilled())
				{
					// render texture in center
					drawRect.x = (float)((this->width - SNAP_VIEW_WIDTH - this->logoTexture->getWidth()) / 2);
				}
				if (WinRT::View->isSnapped())
				{
					// render texture twice on each side of the snapped view
					drawRect.x = (float)((SNAP_VIEW_WIDTH - this->logoTexture->getWidth()) / 2);
				}
				april::rendersys->drawTexturedRect(drawRect, srcRect);
			}
			return this->running;
		}
		*/
		if (this->hasStoredProjectionMatrix)
		{
			april::rendersys->setProjectionMatrix(this->storedProjectionMatrix);
			this->hasStoredProjectionMatrix = false;
		}
		return Window::updateOneFrame();
	}
	
	void WinRT_Window::presentFrame()
	{
	}
	
	void WinRT_Window::beginKeyboardHandling()
	{
		WinRT::Interface->showKeyboard();
		this->virtualKeyboardVisible = true;
		if (this->systemDelegate != NULL)
		{
			this->systemDelegate->onVirtualKeyboardVisibilityChanged(true);
		}
	}

	void WinRT_Window::terminateKeyboardHandling()
	{
		WinRT::Interface->hideKeyboard();
		this->virtualKeyboardVisible = false;
		if (this->systemDelegate != NULL)
		{
			this->systemDelegate->onVirtualKeyboardVisibilityChanged(false);
		}
	}

	void WinRT_Window::_tryLoadLogoTexture()
	{
		if (this->logoTexture != NULL)
		{
			return;
		}
		if (!hfile::exists(MANIFEST_FILENAME))
		{
			return;
		}
		hstr data = hfile::hread(MANIFEST_FILENAME); // lets hope Microsoft does not change the format of these
		// locating the right entry in XML
		int index = data.find("<Applications>");
		if (index < 0)
		{
			return;
		}
		data = data(index, data.size() - index);
		index = data.find("<Application ");
		if (index < 0)
		{
			return;
		}
		data = data(index, data.size() - index);
		index = data.find("<VisualElements ");
		if (index < 0)
		{
			return;
		}
		// finding the logo entry in XML
		data = data(index, data.size() - index);
		int logoIndex = data.find("Logo=\"");
		if (logoIndex >= 0)
		{
			index = logoIndex + hstr("Logo=\"").size();
			hstr logoFilename = data(index, data.size() - index);
			index = logoFilename.find("\"");
			if (index >= 0)
			{
				// loading the logo file
				logoFilename = logoFilename(0, index);
				// adding that ".scale-100" thing there, because my prayers went unanswered and Microsoft decided to change the format after all
				index = logoFilename.rfind('.');
				logoFilename = logoFilename(0, index) + ".scale-100" + logoFilename(index, -1);
				this->logoTexture = april::rendersys->createTexture(logoFilename, false);
				if (this->logoTexture != NULL)
				{
					try
					{
						this->logoTexture->load();
					}
					catch (hltypes::exception&)
					{
						delete this->logoTexture;
						this->logoTexture = NULL;
					}
				}
			}
		}
		// finding the color entry in XML
		int colorIndex = data.find("BackgroundColor=\"");
		if (colorIndex >= 0)
		{
			index = colorIndex + hstr("BackgroundColor=\"").size();
			hstr colorString = data(index, data.size() - index);
			index = colorString.find("\"");
			if (index >= 0)
			{
				// loading the color string
				colorString = colorString(0, index).ltrim('#');
				if (colorString.size() >= 6)
				{
					if (colorString.size() > 6)
					{
						colorString = colorString(0, 6);
					}
					this->backgroundColor.set(colorString);
				}
			}
		}
	}

}
#endif
