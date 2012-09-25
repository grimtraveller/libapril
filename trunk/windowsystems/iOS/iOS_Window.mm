/// @file
/// @author  Kresimir Spes
/// @author  Ivan Vucica
/// @author  Boris Mikic
/// @version 2.32
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php

#import <UIKit/UIKit.h>
#import <QuartzCore/CAEAGLLayer.h>
#import <hltypes/exception.h>
#import "AprilViewController.h"
#import "EAGLView.h"
#import "iOS_Window.h"
#import "RenderSystem.h"
#import "ApriliOSAppDelegate.h"
#include "april.h"

static ApriliOSAppDelegate* appDelegate;
static UIWindow* uiwindow = NULL;
EAGLView* glview = NULL;
static AprilViewController* viewcontroller = NULL;

extern bool g_wnd_rotating;

namespace april
{
	// TODO - convert to gvec2 so it can be included in the class
	static harray<UITouch*> g_touches;
	
	void updateCursorPosition(gvec2 touch)
	{
		float scale = ((iOS_Window*) window)->_getTouchScale();
		// return value stored in cursorX and cursorY		
		//For "primary" landscape orientation, this is how we calc it
		((iOS_Window*) window)->_setCursorPosition(touch.x * scale, touch.y * scale);		
	}
	
	static harray<UITouch*> _convertTouchesToCoordinates(void* nssetTouches)
	{
		// return value stored in cursorX and cursorY
		harray<UITouch*> coordinates;
		NSSet* touches = (NSSet*)nssetTouches;
		UITouch* touch;

		for (touch in touches)
		{
			coordinates += touch;
		}
		
		return coordinates;
	}
	
	InputEvent::InputEvent()
	{
	
	}
	
	InputEvent::~InputEvent()
	{
	
	}

	InputEvent::InputEvent(Window* window)
	{
		this->window = window;
	}

	class MouseInputEvent : public InputEvent
	{
	public:
		MouseInputEvent(Window* window, Window::MouseEventType type, gvec2 position, Window::MouseButton button) : InputEvent(window)
		{
			this->type = type;
			this->position = position;
			this->button = button;
		}
		
		void execute()
		{
			updateCursorPosition(this->position);
			this->window->handleMouseEvent(this->type, this->position, this->button);
		}
		
	protected:
		Window::MouseEventType type;
		gvec2 position;
		Window::MouseButton button;
		
	};
	
	class TouchInputEvent : public InputEvent
	{
	public:
		TouchInputEvent(Window* window, harray<gvec2>& touches) : InputEvent(window)
		{
			this->touches = touches;
		}
		
		void execute()
		{
			this->window->handleTouchEvent(this->touches);
		}
		
	protected:
		harray<gvec2> touches;
		
	};
	
	iOS_Window::iOS_Window() : Window()
	{
		this->name = APRIL_WS_IOS;
	}
	
	bool iOS_Window::create(int w, int h, bool fullscreen, chstr title)
	{
		if (!Window::create(w, h, fullscreen, title))
		{
			return false;
		}
		this->firstFrameDrawn = false; // show window after drawing first frame
		this->keyboardRequest = 0;
		this->retainLoadingOverlay = false;
		this->focused = true;
		this->inputEventsMutex = false;
		this->multiTouchActive = false;
		appDelegate = ((ApriliOSAppDelegate*)[[UIApplication sharedApplication] delegate]);
		viewcontroller = [appDelegate viewController];
		uiwindow = appDelegate.uiwnd;
		[UIApplication sharedApplication].statusBarHidden = fullscreen ? YES : NO;		
		this->fullscreen = true; // iOS apps are always fullscreen
		this->firstFrameDrawn = false; // show window after drawing first frame
		this->running = true;
		return true;
	}
	
	iOS_Window::~iOS_Window()
	{
		destroy();
	}
		
	void iOS_Window::enterMainLoop()
	{
		NSLog(@"Fatal error: Using enterMainLoop on iOS!");
		exit(-1);
	}
	
	bool iOS_Window::updateOneFrame()
	{
		// call input events
		InputEvent* e;
		while ((e = this->popInputEvent()) != 0)
		{
			e->execute();
			delete e;
		}	
		if (this->keyboardRequest != 0 && g_touches.size() == 0) // only process keyboard when there is no interaction with the screen
		{
			bool visible = this->isVirtualKeyboardVisible();
			if (visible && this->keyboardRequest == -1)
			{
				[glview terminateKeyboardHandling];
			}
			else if (!visible && this->keyboardRequest == 1)
			{
				[glview beginKeyboardHandling];
			}
			this->keyboardRequest = 0;
		}
		
		float k = this->timer.diff(true);
		return (this->performUpdate(k) && this->running);	
	}

	void iOS_Window::destroyWindow()
	{
		// just stopping the animation on iOS
		[glview stopAnimation];
	}
	
	void iOS_Window::setCursorVisible(bool visible)
	{
		// no effect on iOS
	}
	
	void iOS_Window::addInputEvent(InputEvent* event)
	{
		// TODO - use a real mutex, this is unsafe
		while (this->inputEventsMutex); // wait it out
		this->inputEventsMutex = true;
		this->inputEvents += event;
		this->inputEventsMutex = false;
	}

	InputEvent* iOS_Window::popInputEvent()
	{
		// TODO - use a real mutex, this is unsafe
		while (this->inputEventsMutex); // wait it out
		if (this->inputEvents.size() == 0)
		{
			return NULL;
		}
		this->inputEventsMutex = true;
		InputEvent* e = this->inputEvents.pop_front();
		this->inputEventsMutex = false;
		return e;
	}

	void iOS_Window::_setCursorPosition(float x, float y)
	{
		this->cursorPosition.set(x, y);
	}

	bool iOS_Window::isCursorVisible()
	{
		return false; // iOS never shows system cursor
	}
	
	int iOS_Window::getWidth()
	{
		// TODO dont swap width and height in case display is in portrait mode
#if __IPHONE_3_2 //__IPHONE_OS_VERSION_MIN_REQUIRED >= 30200
		CAEAGLLayer *caeagllayer = ((CAEAGLLayer*)glview.layer);
		if ([caeagllayer respondsToSelector:@selector(contentsScale)])
		{
			return uiwindow.bounds.size.height * caeagllayer.contentsScale;
		}
#endif
		return uiwindow.bounds.size.height;
	}
	
	int iOS_Window::getHeight()
	{
		// TODO dont swap width and height in case display is in portrait mode
#if __IPHONE_3_2 //__IPHONE_OS_VERSION_MIN_REQUIRED >= 30200
		CAEAGLLayer *caeagllayer = ((CAEAGLLayer*)glview.layer);
		if ([caeagllayer respondsToSelector:@selector(contentsScale)])
		{
			return uiwindow.bounds.size.width * caeagllayer.contentsScale;
		}
#endif
		return uiwindow.bounds.size.width;
	}

	void iOS_Window::setTitle(chstr value)
	{
		// no effect on iOS
	}
	
	void iOS_Window::presentFrame()
	{
		if (this->firstFrameDrawn)
		{
			[glview swapBuffers];
		}
		else
		{
			this->checkEvents();
			if (!this->retainLoadingOverlay)
			{
				[viewcontroller removeImageView];
			}
			this->firstFrameDrawn = true;
		}
	}

	void* iOS_Window::getBackendId()
	{
		return viewcontroller;
	}

	void iOS_Window::checkEvents()
	{
		SInt32 result;
		do
		{
			result = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, TRUE);
		} while (result == kCFRunLoopRunHandledSource);
	}
	
	void iOS_Window::callTouchCallback()
	{
		if (this->touchCallback == NULL)
		{
			return;
		}
		harray<gvec2> coordinates;
		gvec2 position;
		CGPoint point;
		float scale = this->_getTouchScale();
		
		foreach (UITouch*, it, g_touches)
		{
			point = [*it locationInView:glview];
			position.x = point.x * scale;
			position.y = point.y * scale;
			coordinates += position;
		}
		this->inputEvents += new TouchInputEvent(this, coordinates);
	}
	
	bool iOS_Window::isRotating()
	{
		return g_wnd_rotating;
	}
	
	hstr iOS_Window::getParam(chstr param)
	{
		if (param == "retain_loading_overlay")
		{
			return this->retainLoadingOverlay ? "1" : "0";
		}
		return "";
	}
	
	void iOS_Window::setParam(chstr param, chstr value)
	{
		if (param == "retain_loading_overlay")
		{
			bool prev = this->retainLoadingOverlay;
			this->retainLoadingOverlay = (value != "0");
			if (!this->retainLoadingOverlay && prev && this->firstFrameDrawn)
			{
				[viewcontroller removeImageView];
			}
		}
	}

	float iOS_Window::_getTouchScale()
	{
#if __IPHONE_3_2 //__IPHONE_OS_VERSION_MIN_REQUIRED >= 30200
		static float scale = -1;
		if (scale == -1)
		{
			CAEAGLLayer* caeagllayer = (CAEAGLLayer*)[glview layer];
			if ([caeagllayer respondsToSelector:@selector(contentsScale)])
			{
				scale = [caeagllayer contentsScale];
			}
			else
			{
				scale = 1; // prior to ios 3.2
			}
		}
		return scale;
#else
		return 1;
#endif
	}
	
	void iOS_Window::touchesBegan_withEvent_(void* nssetTouches, void* uieventEvent)
	{
		harray<UITouch*> touches = _convertTouchesToCoordinates(nssetTouches);
		
		int prev_len = g_touches.size();
		g_touches += touches;
		if (g_touches.size() > 1)
		{
			if (!this->multiTouchActive && prev_len == 1)
			{
				// cancel (notify the app) the previously called mousedown event so we can begin the multi touch event properly
				this->addInputEvent(new MouseInputEvent(this, AMOUSEEVT_UP, gvec2(-10000, -10000), AMOUSEBTN_LEFT));
			}
			this->multiTouchActive = true;
		}
		else
		{
			CGPoint pt = [g_touches[0] locationInView:glview];
			this->addInputEvent(new MouseInputEvent(this, AMOUSEEVT_DOWN, gvec2(pt.x, pt.y), AMOUSEBTN_LEFT));
		}
		this->callTouchCallback();
	}

	void iOS_Window::touchesEnded_withEvent_(void* nssetTouches, void* uieventEvent)
	{
		harray<UITouch*> touches = _convertTouchesToCoordinates(nssetTouches);
		int num_touches = g_touches.size();
		g_touches /= touches;
		
		if (this->multiTouchActive)
		{
			if (num_touches == touches.size())
			{
				this->multiTouchActive = false;
			}
		}
		else
		{
			CGPoint pt = [touches[0] locationInView:glview];
			this->addInputEvent(new MouseInputEvent(this, AMOUSEEVT_UP, gvec2(pt.x, pt.y), AMOUSEBTN_LEFT));
		}
		this->callTouchCallback();
	}
	
	void iOS_Window::touchesCancelled_withEvent_(void* nssetTouches, void* uieventEvent)
	{
		// FIXME needs to cancel touches, not treat them as "release"
		this->touchesEnded_withEvent_(nssetTouches, uieventEvent);
	}
	
	void iOS_Window::touchesMoved_withEvent_(void* nssetTouches, void* uieventEvent)
	{
		if (!this->multiTouchActive)
		{
			UITouch* touch = [[(NSSet*) nssetTouches allObjects] objectAtIndex:0];
			CGPoint pt = [touch locationInView:glview];			
			this->addInputEvent(new MouseInputEvent(this, AMOUSEEVT_MOVE, gvec2(pt.x, pt.y), AMOUSEBTN_NONE));
		}
		this->callTouchCallback();
	}
	
	bool iOS_Window::isVirtualKeyboardVisible()
	{
		return [glview isKeyboardActive];
	}
	
	void iOS_Window::beginKeyboardHandling()
	{
		this->keyboardRequest = 1;
	}
	
	void iOS_Window::terminateKeyboardHandling()
	{
		this->keyboardRequest = -1;
	}
	
	void iOS_Window::injectiOSChar(unsigned int inputChar)
	{
		if (inputChar == 0)
		{
			// deploy backspace
			this->handleKeyEvent(AKEYEVT_DOWN, AK_BACK, 8);
			this->handleKeyEvent(AKEYEVT_UP, AK_BACK, 8);
		}
		if (inputChar >= 32)
		{
			// deploy keypress
			april::KeySym keycode = AK_NONE; // TODO - FIXME incorrect, might cause a nasty bug. 
											 // however, writing a translation table atm 
											 // isn't the priority.
		
			this->handleKeyEvent(AKEYEVT_DOWN, keycode, inputChar);
			this->handleKeyEvent(AKEYEVT_UP, keycode, inputChar);
		}
	}
	
	void iOS_Window::keyboardWasShown()
	{
		if (this->virtualKeyboardCallback != NULL)
		{
			(*this->virtualKeyboardCallback)(true);
		}
	}
	void iOS_Window::keyboardWasHidden()
	{
		if (this->virtualKeyboardCallback != NULL)
		{
			(*this->virtualKeyboardCallback)(false);
		}
	}
	
	void iOS_Window::setDeviceOrientationCallback(void (*do_callback)(DeviceOrientation))
	{
		if (do_callback != NULL)
		{
			[[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
		}
		else
		{
			[[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
		}
		// update mDeviceOrientationCallback
		Window::setDeviceOrientationCallback(do_callback);
	}
	
	//////////////
	void iOS_Window::handleDisplayAndUpdate()
	{
		bool result = this->updateOneFrame();
		april::rendersys->presentFrame();
		if (!result)
		{
			// TODO - should exit application here
		}
	}
	
	void iOS_Window::deviceOrientationDidChange()
	{
		if (this->deviceOrientationCallback != NULL)
		{
			DeviceOrientation newOrientation;
			switch ([[UIDevice currentDevice] orientation])
			{
			case UIDeviceOrientationUnknown:
				newOrientation = ADEVICEORIENTATION_NONE;
			case UIDeviceOrientationPortrait:
				newOrientation = ADEVICEORIENTATION_PORTRAIT;
			case UIDeviceOrientationPortraitUpsideDown:
				newOrientation = ADEVICEORIENTATION_PORTRAIT_UPSIDEDOWN;
			case UIDeviceOrientationLandscapeLeft:
				newOrientation = ADEVICEORIENTATION_LANDSCAPE_LEFT;
			case UIDeviceOrientationLandscapeRight:
				newOrientation = ADEVICEORIENTATION_LANDSCAPE_RIGHT;
			case UIDeviceOrientationFaceUp:
				newOrientation = ADEVICEORIENTATION_FACE_UP;
			case UIDeviceOrientationFaceDown:
				newOrientation = ADEVICEORIENTATION_FACE_DOWN;
			// not adding default, so we get a warning in case
			// a new orientation is added to backing API
			}
			newOrientation = ADEVICEORIENTATION_NONE;
			(*this->deviceOrientationCallback)(newOrientation);
		}
	}
	void iOS_Window::applicationWillResignActive()
	{
		if (!this->firstFrameDrawn)
		{
			log("April iOS Window: received app suspend request before first frame was drawn, quitting app.");
			this->destroy();
			exit(0);
		}
		if (this->focused)
		{
			this->focused = false;
			if (this->focusChangeCallback != NULL)
			{
				(*this->focusChangeCallback)(false);
			}
			[glview stopAnimation];
		}
	}
	
	void iOS_Window::applicationDidBecomeActive()
	{
		if (!this->focused)
		{
			this->focused = true;
			if (glview != NULL) [glview startAnimation];
			if (this->focusChangeCallback != NULL)
			{
				(*this->focusChangeCallback)(true);
			}
		}
	}

}