﻿/// @file
/// @version 3.4
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#ifdef _WINRT_WINDOW
#include "pch.h"

#include <hltypes/hfile.h>
#include <hltypes/hlog.h>
#include <hltypes/hltypesUtil.h>
#include <hltypes/hstring.h>
#include <hltypes/hthread.h>

#include "april.h"
#include "IWinRT.h"
#include "Platform.h"
#include "RenderSystem.h"
#include "Window.h"
#include "WinRT.h"
#include "WinRT_BaseApp.h"
#include "WinRT_Cursor.h"
#include "WinRT_Window.h"
#include "WinRT_XamlApp.h"

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Media;

#define MANIFEST_FILENAME "AppxManifest.xml"

namespace april
{
	WinRT_XamlApp::WinRT_XamlApp() : Application()
	{
		this->app = ref new WinRT_BaseApp();
		this->running = true;
#ifndef _WINP8
		this->defaultCursor = ref new CoreCursor(CoreCursorType::Arrow, 0);
#else
		this->defaultCursor = nullptr;
#endif
		this->backgroundColor = april::Color::Black;
		this->launched = false;
		this->activated = false;
		this->Suspending += ref new SuspendingEventHandler(this->app, &WinRT_BaseApp::OnSuspend);
		this->Resuming += ref new EventHandler<Object^>(this->app, &WinRT_BaseApp::OnResume);
#ifdef _DEBUG
		this->UnhandledException += ref new UnhandledExceptionEventHandler([](Object^ sender, UnhandledExceptionEventArgs^ args)
		{
			hlog::error("FATAL", _HL_PSTR_TO_HSTR(args->Message));
		});
#endif
	}

	WinRT_XamlApp::~WinRT_XamlApp()
	{
	}

	void WinRT_XamlApp::unassignWindow()
	{
		this->backgroundColor = april::Color::Black;
	}

	void WinRT_XamlApp::refreshCursor()
	{
		if (april::window != NULL)
		{
			CoreCursor^ cursor = nullptr;
			if (april::window->isCursorVisible())
			{
				Cursor* windowCursor = april::window->getCursor();
				if (windowCursor != NULL)
				{
					cursor = ((WinRT_Cursor*)windowCursor)->getCursor();
				}
				if (cursor == nullptr)
				{
					cursor = this->defaultCursor;
				}
			}
#ifndef _WINP8
			Windows::UI::Xaml::Window::Current->CoreWindow->PointerCursor = cursor;
#endif
		}
	}

	void WinRT_XamlApp::checkEvents()
	{
	}

	void WinRT_XamlApp::showKeyboard()
	{
		WinRT::XamlOverlay->showKeyboard();
	}

	void WinRT_XamlApp::hideKeyboard()
	{
		WinRT::XamlOverlay->hideKeyboard();
	}

	void WinRT_XamlApp::Connect(int connectionId, Object^ target)
	{
	}

	void WinRT_XamlApp::OnLaunched(LaunchActivatedEventArgs^ args)
	{
		// don't repeat app initialization when already launched
		if (!this->launched)
		{
			this->launched = true;
			this->app->assignEvents(Windows::UI::Core::CoreWindow::GetForCurrentThread());
			this->refreshCursor();
			WinRT::XamlOverlay = ref new WinRT_XamlOverlay();
			Windows::UI::Xaml::Window::Current->Content = WinRT::XamlOverlay;
			Windows::UI::Xaml::Window::Current->Activated += ref new WindowActivatedEventHandler(this, &WinRT_XamlApp::OnWindowActivationChanged);
			(*WinRT::Init)(WinRT::Args);
			if (april::rendersys != NULL && april::window != NULL)
			{
				float delaySplash = (float)april::window->getParam(WINRT_DELAY_SPLASH);
				if (delaySplash > 0.0f && delaySplash - (get_system_tick_count() - this->app->getStartTime()) * 0.001f > 0.0f)
				{
					this->_tryLoadSplashTexture();
					this->_tryRenderSplashTexture();
				}
			}
		}
		Windows::UI::Xaml::Window::Current->Activate();
	}

	void WinRT_XamlApp::OnWindowActivationChanged( _In_ Object^ sender, _In_ WindowActivatedEventArgs^ args)
	{
		if (!this->activated)
		{
			this->activated = true;
			this->eventToken = CompositionTarget::Rendering::add(ref new EventHandler<Object^>(this, &WinRT_XamlApp::OnRender));
			if (april::rendersys != NULL)
			{
				bool rendered = false;
				if (april::window != NULL)
				{
					float delaySplash = (float)april::window->getParam(WINRT_DELAY_SPLASH);
					if (delaySplash > 0.0f)
					{
						float delay = delaySplash - (get_system_tick_count() - this->app->getStartTime()) * 0.001f;
						if (delay > 0.0f)
						{
							hlog::write(april::logTag, "Rendering splash screen for: " + hstr(delay));
							this->_tryLoadSplashTexture();
							this->_tryRenderSplashTexture();
							if (this->splashTexture != NULL)
							{
								rendered = true;
							}
							delay = delaySplash - (get_system_tick_count() - this->app->getStartTime()) / 1000.0f;
							if (delay > 0.0f) // if there's still time left after rendering
							{
								hthread::sleep(delay * 1000.0f);
							}
						}
					}
				}
				if (this->splashTexture != NULL)
				{
					delete this->splashTexture;
					this->splashTexture = NULL;
				}
				if (!rendered)
				{
					april::rendersys->clear();
					april::rendersys->presentFrame();
					april::rendersys->reset();
				}
			}
		}
		else if (args->WindowActivationState == CoreWindowActivationState::Deactivated)
		{
			this->app->handleFocusChange(false);
			if (this->eventToken.Value != 0)
			{
				CompositionTarget::Rendering::remove(this->eventToken);
				this->eventToken.Value = 0;
			}
		}
		else if (args->WindowActivationState == CoreWindowActivationState::CodeActivated ||
			args->WindowActivationState == CoreWindowActivationState::PointerActivated)
		{
			this->app->handleFocusChange(true);
			if (this->eventToken.Value == 0)
			{
				this->eventToken = CompositionTarget::Rendering::add(ref new EventHandler<Object^>(this, &WinRT_XamlApp::OnRender));
			}
		}
	}

	void WinRT_XamlApp::OnRender(_In_ Object^ sender, _In_ Object^ args)
	{
		if (!this->running || april::window == NULL)
		{
			return;
		}
		this->running = april::window->updateOneFrame();
		april::rendersys->presentFrame();
		if (!this->running)
		{
			(*WinRT::Destroy)();
			CompositionTarget::Rendering::remove(this->eventToken);
			hlog::warn(april::logTag, "Manual closing in WinRT apps should not be used!");
		}
	}

	void WinRT_XamlApp::_tryRenderSplashTexture()
	{
		if (this->splashTexture != NULL)
		{
			grect storedOrthoProjection = april::rendersys->getOrthoProjection();
			gmat4 storedProjectionMatrix = april::rendersys->getProjectionMatrix();
			grect drawRect(0.0f, 0.0f, 1.0f, 1.0f);
			grect viewport(0.0f, 0.0f, 1.0f, 1.0f);
			int width = (int)(april::window->getWidth() * 96 / april::getSystemInfo().displayDpi);
			int height = (int)(april::window->getHeight() * 96 / april::getSystemInfo().displayDpi);
			viewport.setSize((float)width, (float)height);
			april::rendersys->setOrthoProjection(viewport);
			april::rendersys->drawFilledRect(viewport, this->backgroundColor);
			drawRect.set((float)((width - this->splashTexture->getWidth()) / 2), (float)((height - this->splashTexture->getHeight()) / 2),
				(float)this->splashTexture->getWidth(), (float)this->splashTexture->getHeight());
			april::rendersys->setTexture(this->splashTexture);
			april::rendersys->drawTexturedRect(drawRect, grect(0.0f, 0.0f, 1.0f, 1.0f));
			april::rendersys->presentFrame();
			april::rendersys->reset();
			april::rendersys->setOrthoProjection(storedOrthoProjection);
			april::rendersys->setProjectionMatrix(storedProjectionMatrix);
		}
	}

	april::Texture* WinRT_XamlApp::_tryLoadTexture(chstr nodeName, chstr attributeName)
	{
		if (april::rendersys == NULL)
		{
			return NULL;
		}
		hstr data;
		int index = 0;
		if (!this->_findVisualElements(nodeName, data, index))
		{
			return NULL;
		}
		// finding the logo entry in XML
		int logoIndex = data.find(attributeName + "=\"");
		if (logoIndex >= 0)
		{
			index = logoIndex + hstr(attributeName + "=\"").size();
			hstr logoFilename = data(index, -1);
			index = logoFilename.find("\"");
			if (index >= 0)
			{
				logoFilename = logoFilename(0, index);
				harray<hstr> filenames;
				filenames += logoFilename(0, index);
				// adding that ".scale-100" thing here, because my prayers went unanswered and Microsoft decided to change the format after all
				index = logoFilename.rfind('.');
				filenames += logoFilename(0, index) + ".scale-100" + logoFilename(index, -1);
				foreach (hstr, it, filenames)
				{
					// loading the logo file
					april::Texture* texture = april::rendersys->createTextureFromResource(logoFilename, Texture::TYPE_IMMUTABLE, false);
					if (texture != NULL)
					{
						try
						{
							texture->load();
							return texture;
						}
						catch (hltypes::exception&)
						{
							delete texture;
						}
					}
				}
			}
		}
		return NULL;
	}

	void WinRT_XamlApp::_tryLoadSplashTexture()
	{
		if (this->splashTexture == NULL)
		{
			this->splashTexture = this->_tryLoadTexture("SplashScreen", "Image");
		}
		this->_tryLoadBackgroundColor();
	}

	void WinRT_XamlApp::_tryLoadBackgroundColor()
	{
		hstr data;
		int index = 0;
		if (!this->_findVisualElements("VisualElements", data, index))
		{
			return;
		}
		// finding the color entry in XML
		int colorIndex = data.find("BackgroundColor=\"");
		if (colorIndex >= 0)
		{
			index = colorIndex + hstr("BackgroundColor=\"").size();
			hstr colorString = data(index, -1);
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

	bool WinRT_XamlApp::_findVisualElements(chstr nodeName, hstr& data, int& index)
	{
		if (!hfile::exists(MANIFEST_FILENAME))
		{
			return false;
		}
		data = hfile::hread(MANIFEST_FILENAME); // lets hope Microsoft does not change the format of these
		// locating the right entry in XML
		index = data.find("<Applications>");
		if (index < 0)
		{
			return false;
		}
		data = data(index, -1);
		index = data.find("<Application ");
		if (index < 0)
		{
			return false;
		}
		data = data(index, -1);
		index = data.find("<" + nodeName + " ");
		if (index < 0)
		{
			return false;
		}
		data = data(index, -1);
		return true;
	}

}
#endif
