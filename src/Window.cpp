/// @file
/// @version 3.5
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include <gtypes/Rectangle.h>
#include <gtypes/Vector2.h>
#include <hltypes/hlog.h>
#include <hltypes/hresource.h>
#include <hltypes/hstring.h>
#include <hltypes/hthread.h>

#include "april.h"
#include "ControllerDelegate.h"
#include "Cursor.h"
#include "KeyboardDelegate.h"
#include "Keys.h"
#include "MouseDelegate.h"
#include "Platform.h"
#include "RenderSystem.h"
#include "SystemDelegate.h"
#include "TextureAsync.h"
#include "TouchDelegate.h"
#include "UpdateDelegate.h"
#include "Window.h"

#define INPUT_MODE_NAME(value) \
	hstr(value == MOUSE ? "MOUSE" : \
	(value == TOUCH ? "TOUCH" : \
	(value == CONTROLLER ? "CONTROLLER" : "UNDEFINED")))

namespace april
{
	Window::MouseEventType Window::AMOUSEEVT_DOWN = Window::MOUSE_DOWN; // DEPRECATED
	Window::MouseEventType Window::AMOUSEEVT_UP = Window::MOUSE_UP; // DEPRECATED
	Window::MouseEventType Window::AMOUSEEVT_CANCEL = Window::MOUSE_CANCEL; // DEPRECATED
	Window::MouseEventType Window::AMOUSEEVT_MOVE = Window::MOUSE_MOVE; // DEPRECATED
	Window::MouseEventType Window::AMOUSEEVT_SCROLL = Window::MOUSE_SCROLL; // DEPRECATED
	Window::KeyEventType Window::AKEYEVT_DOWN = Window::KEY_DOWN; // DEPRECATED
	Window::KeyEventType Window::AKEYEVT_UP = Window::KEY_UP; // DEPRECATED
	Window::ControllerEventType Window::ACTRLEVT_DOWN = Window::CONTROLLER_DOWN; // DEPRECATED
	Window::ControllerEventType Window::ACTRLEVT_UP = Window::CONTROLLER_UP; // DEPRECATED
		
	// TODOaa - refactor
	void (*Window::msLaunchCallback)(void*) = NULL;
	void Window::handleLaunchCallback(void* args)
	{
		if (msLaunchCallback != NULL)
		{
			(*msLaunchCallback)(args);
		}
	}
	//////////////////

	Window::MouseInputEvent::MouseInputEvent()
	{
		this->type = MOUSE_MOVE;
		this->keyCode = AK_NONE;
	}
		
	Window::MouseInputEvent::MouseInputEvent(Window::MouseEventType type, gvec2 position, Key keyCode)
	{
		this->type = type;
		this->position = position;
		this->keyCode = keyCode;
	}
		
	Window::KeyInputEvent::KeyInputEvent()
	{
		this->type = KEY_UP;
		this->keyCode = AK_NONE;
		this->charCode = 0;
	}

	Window::KeyInputEvent::KeyInputEvent(Window::KeyEventType type, Key keyCode, unsigned int charCode)
	{
		this->type = type;
		this->keyCode = keyCode;
		this->charCode = charCode;
	}

	Window::TouchInputEvent::TouchInputEvent()
	{
	}
		
	Window::TouchInputEvent::TouchInputEvent(harray<gvec2>& touches)
	{
		this->touches = touches;
	}
		
	Window::ControllerInputEvent::ControllerInputEvent()
	{
		this->type = CONTROLLER_UP;
		this->buttonCode = AB_NONE;
	}

	Window::ControllerInputEvent::ControllerInputEvent(Window::ControllerEventType type, Button buttonCode)
	{
		this->type = type;
		this->buttonCode = buttonCode;
	}

	Window::ControllerAxisInputEvent::ControllerAxisInputEvent()
	{
		this->type = CONTROLLER_AXIS;
		this->buttonCode = AB_NONE;
		this->axisValue = 0;
	}

	Window::ControllerAxisInputEvent::ControllerAxisInputEvent(Window::ControllerEventType type, Button buttonCode, float axisValue)
	{
		this->type = type;
		this->buttonCode = buttonCode;
		this->axisValue = axisValue;
	}
	
	Window* window = NULL;
	
	Window::Options::Options()
	{
		this->resizable = false;
		this->fpsCounter = false;
		this->hotkeyFullscreen = false;
		this->defaultWindowModeResolutionFactor = 0.85f;
	}
	
	Window::Options::~Options()
	{
	}

	hstr Window::Options::toString()
	{
		harray<hstr> options;
		if (this->resizable)
		{
			options += "resizable";
		}
		if (options.size() == 0)
		{
			options += "none";
		}
		return options.joined(',');
	}
	
	Window::Window()
	{
		this->name = "Generic";
		this->created = false;
		this->fullscreen = true;
		this->focused = true;
		this->running = true;
		this->lastWidth = 0;
		this->lastHeight = 0;
		this->fps = 0;
		this->fpsCount = 0;
		this->fpsTimer = 0.0f;
		this->fpsResolution = 0.5f;
		this->timeDeltaMaxLimit = 0.2f;
		this->cursor = NULL;
		this->cursorVisible = false;
		this->virtualKeyboardVisible = false;
		this->virtualKeyboardHeightRatio = 0.0f;
		this->multiTouchActive = false;
		this->inputMode = MOUSE;
		this->updateDelegate = NULL;
		this->keyboardDelegate = NULL;
		this->mouseDelegate = NULL;
		this->touchDelegate = NULL;
		this->controllerDelegate = NULL;
		this->systemDelegate = NULL;
	}
	
	Window::~Window()
	{
		this->destroy();
	}

	bool Window::create(int w, int h, bool fullscreen, chstr title, Window::Options options)
	{
		if (!this->created)
		{
			hlog::writef(logTag, "Creating window: '%s' (%d, %d) %s, '%s', (options: %s)",
				this->name.cStr(), w, h, fullscreen ? "fullscreen" : "windowed", title.cStr(), options.toString().cStr());
			this->fullscreen = fullscreen;
			this->title = title;
			this->options = options;
			this->created = true;
			if (options.hotkeyFullscreen)
			{
				if (!fullscreen)
				{
					this->lastWidth = w;
					this->lastHeight = h;
				}
				else
				{
					SystemInfo info = april::getSystemInfo();
					this->lastWidth = (int)(info.displayResolution.x * 0.6666667f);
					this->lastHeight = (int)(info.displayResolution.y * 0.6666667f);
				}
			}
			this->fps = 0;
			this->fpsCount = 0;
			this->fpsTimer = 0.0f;
			this->fpsResolution = 0.5f;
			this->multiTouchActive = false;
			this->cursor = NULL;
			this->virtualKeyboardVisible = false;
			this->virtualKeyboardHeightRatio = 0.0f;
			this->inputMode = MOUSE;
			return true;
		}
		return false;
	}
	
	bool Window::destroy()
	{
		if (this->created)
		{
			hlog::writef(logTag, "Destroying window '%s'.", this->name.cStr());
			this->created = false;
			this->fps = 0;
			this->fpsCount = 0;
			this->fpsTimer = 0.0f;
			this->fpsResolution = 0.5f;
			this->multiTouchActive = false;
			this->cursor = NULL;
			this->virtualKeyboardVisible = false;
			this->virtualKeyboardHeightRatio = 0.0f;
			this->inputMode = MOUSE;
			this->updateDelegate = NULL;
			this->keyboardDelegate = NULL;
			this->mouseDelegate = NULL;
			this->touchDelegate = NULL;
			this->controllerDelegate = NULL;
			this->systemDelegate = NULL;
			this->keyEvents.clear();
			this->mouseEvents.clear();
			this->touchEvents.clear();
			this->controllerEvents.clear();
			this->controllerAxisEvents.clear();
			this->touches.clear();
			this->controllerEmulationKeys.clear();
			return true;
		}
		return false;
	}

	void Window::unassign()
	{
	}

	void Window::setInputMode(InputMode value)
	{
		if (this->inputModeTranslations.hasKey(value))
		{
			value = this->inputModeTranslations[value];
		}
		if (this->inputMode != value)
		{
			this->inputMode = value;
			hlog::write(logTag, "Changing Input Mode to: " + INPUT_MODE_NAME(this->inputMode));
			if (this->inputMode == CONTROLLER)
			{
				this->cursorPosition.set(-10000.0f, -10000.0f);
			}
			if (this->systemDelegate != NULL)
			{
				this->systemDelegate->onInputModeChanged(value);
			}
		}
	}

	void Window::setInputModeTranslations(hmap<InputMode, InputMode> value)
	{
		this->inputModeTranslations = value;
		if (this->inputModeTranslations.hasKey(this->inputMode))
		{
			this->inputMode = this->inputModeTranslations[this->inputMode];
			hlog::write(logTag, "Forcing Input Mode to: " + INPUT_MODE_NAME(this->inputMode));
			if (this->inputMode == CONTROLLER)
			{
				this->cursorPosition.set(-10000.0f, -10000.0f);
			}
			if (this->systemDelegate != NULL)
			{
				this->systemDelegate->onInputModeChanged(this->inputMode);
			}
		}
	}

	gvec2 Window::getSize()
	{
		return gvec2((float)this->getWidth(), (float)this->getHeight());
	}
	
	float Window::getAspectRatio()
	{
		return ((float)this->getWidth() / this->getHeight());
	}
	
	void Window::setCursorVisible(bool value)
	{
		this->cursorVisible = value;
		this->_refreshCursor();
	}

	void Window::setCursor(Cursor* value)
	{
		this->cursor = value;
		this->_refreshCursor();
	}

	bool Window::isCursorInside()
	{
		return grect(0.0f, 0.0f, this->getSize()).isPointInside(this->getCursorPosition());
	}

	void Window::setFullscreen(bool value)
	{
		SystemInfo info = april::getSystemInfo();
		int w = hround(info.displayResolution.x);
		int h = hround(info.displayResolution.y);
		if (!value)
		{
			float factor = this->getOptions().defaultWindowModeResolutionFactor;
			w = (int)(w * factor);
			h = (int)(h * factor);
		}
		this->setResolution(w, h, value);
		this->fullscreen = value;
	}

	void Window::setResolution(int w, int h)
	{
		this->setResolution(w, h, this->isFullscreen());
	}

	void Window::setResolution(int w, int h, bool fullscreen)
	{
		hlog::warnf(logTag, "setResolution() is not available in '%s'.", this->name.cStr());
	}

	void Window::toggleHotkeyFullscreen()
	{
		if (this->options.hotkeyFullscreen)
		{
			if (!this->fullscreen)
			{
				this->lastWidth = this->getWidth();
				this->lastHeight = this->getHeight();
			}
			this->setResolution(this->lastWidth, this->lastHeight, !this->fullscreen);
		}
	}

	void Window::_setRenderSystemResolution()
	{
		this->_setRenderSystemResolution(this->getWidth(), this->getHeight(), this->fullscreen);
	}
	
	void Window::_setRenderSystemResolution(int w, int h, bool fullscreen)
	{
		hlog::writef(logTag, "Setting window resolution: (%d,%d); fullscreen: %s", w, h, fullscreen ? "yes" : "no");
		april::rendersys->_setResolution(w, h, fullscreen);
		if (this->systemDelegate != NULL)
		{
			this->systemDelegate->onWindowSizeChanged(w, h, fullscreen);
		}
	}
	
	void Window::enterMainLoop()
	{
		this->fps = 0;
		this->fpsCount = 0;
		this->fpsTimer = 0.0f;
		this->running = true;
		while (this->running)
		{
			if (!this->updateOneFrame())
			{
				this->running = false;
			}
			if (this->updateDelegate != NULL)
			{
				this->updateDelegate->onPresentFrame();
			}
		}
	}

	bool Window::updateOneFrame()
	{
		TextureAsync::update();
		float timeDelta = this->_calcTimeSinceLastFrame();
		if (!this->focused)
		{
			hthread::sleep(40.0f);
		}
		this->checkEvents();
		return (this->performUpdate(timeDelta) && this->running);
	}
	
	void Window::checkEvents()
	{
		KeyInputEvent keyEvent;
		while (this->keyEvents.size() > 0)
		{
			keyEvent = this->keyEvents.removeFirst();
			this->handleKeyEvent(keyEvent.type, keyEvent.keyCode, keyEvent.charCode);
		}
		// due to possible problems with multiple scroll events in one frame, consecutive scroll events are merged (and so are move events for convenience)
		MouseInputEvent mouseEvent;
		gvec2 cumulativeScroll;
		while (this->mouseEvents.size() > 0)
		{
			mouseEvent = this->mouseEvents.removeFirst();
			if (mouseEvent.type != Window::MOUSE_CANCEL && mouseEvent.type != Window::MOUSE_SCROLL)
			{
				this->cursorPosition = mouseEvent.position;
			}
			if (mouseEvent.type == Window::MOUSE_SCROLL)
			{
				cumulativeScroll += mouseEvent.position;
				if (this->mouseEvents.size() == 0 || this->mouseEvents.first().type != Window::MOUSE_SCROLL)
				{
					this->handleMouseEvent(mouseEvent.type, cumulativeScroll, mouseEvent.keyCode);
					cumulativeScroll.set(0.0f, 0.0f);
				}
			}
			// if not a scroll event or final move event (because of merging)
			else if (mouseEvent.type != Window::MOUSE_MOVE || (mouseEvent.type == Window::MOUSE_MOVE &&
				(this->mouseEvents.size() == 0 || this->mouseEvents.first().type != Window::MOUSE_MOVE)))
			{
				this->handleMouseEvent(mouseEvent.type, mouseEvent.position, mouseEvent.keyCode);
			}
		}
		TouchInputEvent touchEvent;
		while (this->touchEvents.size() > 0)
		{
			touchEvent = this->touchEvents.removeFirst();
			this->handleTouchEvent(touchEvent.touches);
		}
		ControllerInputEvent controllerEvent;
		while (this->controllerEvents.size() > 0)
		{
			controllerEvent = this->controllerEvents.removeFirst();
			this->handleControllerEvent(controllerEvent.type, controllerEvent.buttonCode);
		}
		ControllerAxisInputEvent controllerAxisEvent;
		while (this->controllerAxisEvents.size() > 0)
		{
			controllerAxisEvent = this->controllerAxisEvents.removeFirst();
			this->handleControllerAxisEvent(controllerAxisEvent.type, controllerAxisEvent.buttonCode, controllerAxisEvent.axisValue);
		}
	}

	void Window::terminateMainLoop()
	{
		this->running = false;
	}
	
	bool Window::performUpdate(float timeDelta)
	{
		if (this->timeDeltaMaxLimit > 0.0f)
		{
			timeDelta = hmin(timeDelta, this->timeDeltaMaxLimit);
		}
		this->fpsTimer += timeDelta;
		if (this->fpsTimer > 0.0f)
		{
			++this->fpsCount;
			if (this->fpsTimer >= this->fpsResolution)
			{
				this->fps = hceil(this->fpsCount / this->fpsTimer);
				this->fpsCount = 0;
				this->fpsTimer = 0.0f;
			}
		}
		else
		{
			this->fps = 0;
			this->fpsCount = 0;
		}
		// returning true: continue execution
		// returning false: abort execution
		if (this->updateDelegate != NULL)
		{
			return this->updateDelegate->onUpdate(timeDelta);
		}
		april::rendersys->clear();
		return true;
	}
	
	void Window::handleKeyEvent(KeyEventType type, Key keyCode, unsigned int charCode)
	{
		this->handleKeyOnlyEvent(type, keyCode); // doesn't do anything if keyCode is AK_NONE
		if (type == KEY_DOWN && charCode > 0) // ignores invalid chars
		{
			// according to the unicode standard, this range is undefined and reserved for system codes
			// for example, macosx maps keys up, down, left, right to this key, inducing wrong char calls to the app.
			// source: http://en.wikibooks.org/wiki/Unicode/Character_reference/F000-FFFF
			if (charCode < 0xE000 || charCode > 0xF8FF)
			{
				this->handleCharOnlyEvent(charCode);
			}
		}
	}
	
	void Window::handleKeyOnlyEvent(KeyEventType type, Key keyCode)
	{
		if (keyCode == AK_UNKNOWN)
		{
			keyCode = AK_NONE;
		}
		if (this->keyboardDelegate != NULL && keyCode != AK_NONE)
		{
			switch (type)
			{
			case KEY_DOWN:
				this->keyboardDelegate->onKeyDown(keyCode);
				break;
			case KEY_UP:
				this->keyboardDelegate->onKeyUp(keyCode);
				break;
			}
			// emulation of buttons using keyboard
			if (this->controllerEmulationKeys.hasKey(keyCode))
			{
				ControllerEventType buttonType = (type == KEY_DOWN ? CONTROLLER_DOWN : CONTROLLER_UP);
				this->handleControllerEvent(buttonType, this->controllerEmulationKeys[keyCode]);
			}
		}
	}
	
	void Window::handleCharOnlyEvent(unsigned int charCode)
	{
		if (this->keyboardDelegate != NULL && charCode >= 32 && charCode != 127) // special hack, backspace induces a character in some implementations
		{
			this->keyboardDelegate->onChar(charCode);
		}
	}
	
	void Window::handleMouseEvent(MouseEventType type, gvec2 position, Key keyCode)
	{
		if (this->mouseDelegate != NULL)
		{
			switch (type)
			{
			case MOUSE_DOWN:
				this->mouseDelegate->onMouseDown(keyCode);
				break;
			case MOUSE_UP:
				this->mouseDelegate->onMouseUp(keyCode);
				break;
			case MOUSE_CANCEL:
				this->mouseDelegate->onMouseCancel(keyCode);
				break;
			case MOUSE_MOVE:
				this->mouseDelegate->onMouseMove();
				break;
			case MOUSE_SCROLL:
				this->mouseDelegate->onMouseScroll(position.x, position.y);
				break;
			}
		}
	}
	
	void Window::handleTouchEvent(const harray<gvec2>& touches)
	{
		if (this->touchDelegate != NULL)
		{
			this->touchDelegate->onTouch(touches);
		}
	}

	void Window::handleControllerEvent(ControllerEventType type, Button buttonCode)
	{
		if (this->controllerDelegate != NULL && buttonCode != AB_NONE)
		{
			switch (type)
			{
			case CONTROLLER_DOWN:
				this->controllerDelegate->onButtonDown(buttonCode);
				break;
			case CONTROLLER_UP:
				this->controllerDelegate->onButtonUp(buttonCode);
				break;
			default:
				break;
			}
		}
	}

	void Window::handleControllerAxisEvent(ControllerEventType type, Button buttonCode, float axisValue)
	{
		if (this->controllerDelegate != NULL)
		{
			this->controllerDelegate->onControllerAxisChange(buttonCode, axisValue);
		}
	}
	
	bool Window::handleQuitRequest(bool canCancel)
	{
		// returns whether or not the windowing system is permitted to close the window
		return (this->systemDelegate == NULL || this->systemDelegate->onQuit(canCancel));
	}
	
	void Window::handleFocusChangeEvent(bool focused)
	{
		this->focused = focused;
		hlog::write(logTag, "Window " + hstr(focused ? "gained focus." : "lost focus."));
		if (this->systemDelegate != NULL)
		{
			this->systemDelegate->onWindowFocusChanged(focused);
		}
	}

	void Window::handleActivityChangeEvent(bool active)
	{
		hlog::warn(logTag, this->name + " does not implement activity change events!");
	}
	
	void Window::handleVirtualKeyboardChangeEvent(bool visible, float heightRatio)
	{
		this->virtualKeyboardVisible = visible;
		this->virtualKeyboardHeightRatio = heightRatio;
		if (this->systemDelegate != NULL)
		{
			this->systemDelegate->onVirtualKeyboardChanged(this->virtualKeyboardVisible, this->virtualKeyboardHeightRatio);
		}
	}

	void Window::handleLowMemoryWarning()
	{
		if (this->systemDelegate != NULL)
		{
			this->systemDelegate->onLowMemoryWarning();
		}
	}

	void Window::queueKeyEvent(KeyEventType type, Key keyCode, unsigned int charCode)
	{
		this->keyEvents += KeyInputEvent(type, keyCode, charCode);
	}

	void Window::queueMouseEvent(MouseEventType type, gvec2 position, Key keyCode)
	{
		this->mouseEvents += MouseInputEvent(type, position, keyCode);
	}

	void Window::queueTouchEvent(MouseEventType type, gvec2 position, int index)
	{
		int previousTouchesSize = this->touches.size();
		switch (type)
		{
		case MOUSE_DOWN:
			if (index < this->touches.size()) // DOWN event of an already indexed touch, never happened so far
			{
				return;
			}
			this->touches += position;
			break;
		case MOUSE_UP:
			if (index >= this->touches.size()) // redundant UP event, can happen
			{
				return;
			}
			this->touches.removeAt(index);
			break;
		case MOUSE_MOVE:
			if (index >= this->touches.size()) // MOVE event of an unindexed touch, never happened so far
			{
				return;
			}
			this->touches[index] = position;
			break;
		case MOUSE_CANCEL: // canceling a particular pointer, required by specific systems (e.g. WinRT)
			if (index < this->touches.size())
			{
				this->touches.removeAt(index);
				if (this->touches.size() == 0)
				{
					this->multiTouchActive = false;
				}
			}
			return;
		default:
			break;
		}
		if (this->multiTouchActive || this->touches.size() > 1)
		{
			if (!this->multiTouchActive && previousTouchesSize == 1)
			{
				// cancel (notify the app) the previously called mousedown event so we can begin the multi touch event properly
				this->queueMouseEvent(MOUSE_CANCEL, position, AK_LBUTTON);
			}
			this->multiTouchActive = (this->touches.size() > 0);
		}
		else
		{
			this->queueMouseEvent(type, position, AK_LBUTTON);
		}
		this->touchEvents.clear();
		this->touchEvents += TouchInputEvent(this->touches);
	}

	void Window::queueControllerEvent(ControllerEventType type, Button buttonCode)
	{
		this->controllerEvents += ControllerInputEvent(type, buttonCode);
	}

	void Window::queueControllerAxisEvent(ControllerEventType type, Button buttonCode, float axisValue)
	{
		this->controllerAxisEvents += ControllerAxisInputEvent(type, buttonCode, axisValue);
	}

	float Window::_calcTimeSinceLastFrame()
	{
		float timeDelta = this->timer.diff(true);
		if (timeDelta > 0.5f)
		{
			timeDelta = 0.05f; // prevent jumps from e.g, waiting on device reset or super low framerate
		}
		if (!this->focused)
		{
			timeDelta = 0.0f;
		}
		return timeDelta;
	}

	hstr Window::findCursorFile(chstr filename)
	{
		hstr _filename;
		foreach (hstr, it, this->cursorExtensions)
		{
			_filename = filename + (*it);
			if (hresource::exists(_filename))
			{
				return _filename;
			}
		}
		return "";
	}
	
	Cursor* Window::createCursor(chstr filename)
	{
		hstr name = this->findCursorFile(filename);
		if (name == "")
		{
			return NULL;
		}
		Cursor* cursor = this->_createCursor();
		if (cursor != NULL && !cursor->_create(name))
		{
			delete cursor;
			return NULL;
		}
		return cursor;
	}

	april::Cursor* Window::_createCursor()
	{
		hlog::warnf(logTag, "Cursors are not available in '%s'.", this->name.cStr());
		return NULL;
	}

	void Window::_refreshCursor()
	{
	}

}
