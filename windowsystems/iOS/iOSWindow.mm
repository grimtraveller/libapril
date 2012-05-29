/// @file
/// @author  Kresimir Spes
/// @author  Ivan Vucica
/// @version 1.31
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php

#import <UIKit/UIKit.h>
#import <QuartzCore/CAEAGLLayer.h>
#import <hltypes/exception.h>
#include <sys/sysctl.h>
#import "AprilViewController.h"
#import "EAGLView.h"
#import "iOSWindow.h"
#import "RenderSystem.h"
#import "ApriliOSAppDelegate.h"
#include "april.h"

static ApriliOSAppDelegate *appDelegate;
static UIWindow *uiwnd = 0;
EAGLView *glview = 0;
static AprilViewController *viewcontroller = 0;

extern bool g_wnd_rotating;

namespace april
{
	InputEvent::InputEvent()
	{
	
	}
	
	InputEvent::~InputEvent()
	{
	
	}

	InputEvent::InputEvent(Window* wnd)
	{
		mWindow = wnd;
	}

	class MouseInputEvent : public InputEvent
	{
		float mX, mY;
		Window::MouseButton mButton;
		Window::MouseEventType mEvent;
	public:
		MouseInputEvent(Window* wnd, float x, float y, Window::MouseButton button, Window::MouseEventType event) : InputEvent(wnd)
		{
			mX = x; mY = y; mButton = button; mEvent = event;
		}
		
		void execute()
		{
			mWindow->handleMouseEvent(mEvent, mX, mY, mButton);
		}
	};
	
	class TouchInputEvent : public InputEvent
	{
		harray<gvec2> mTouches;
	public:
		TouchInputEvent(Window* wnd, harray<gvec2>& touches) : InputEvent(wnd)
		{
			mTouches = touches;
		}
		
		void execute()
		{
			mWindow->handleTouchEvent(mTouches);
		}
	};
	
	
    iOSWindow::iOSWindow(int w, int h, bool fullscreen, chstr title)
    {
		mKeyboardRequest = 0;
		mRetainLoadingOverlay = false;
		mFocused = true;
		mInputEventsMutex = false;
		mMultiTouchActive = false;
		appDelegate = ((ApriliOSAppDelegate*)[[UIApplication sharedApplication] delegate]);
		viewcontroller = [appDelegate viewController];
		uiwnd = [appDelegate uiwnd];
		[UIApplication sharedApplication].statusBarHidden = fullscreen ? YES : NO;
		mFullscreen = true; // iOS apps are always fullscreen

		mFirstFrameDrawn = false; // show window after drawing first frame

		mRunning = true;
    }
	
	Window::DeviceType iOSWindow::getDeviceType()
	{
		if ([UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPhone)
			return DEVICE_IPHONE;
		else
			return DEVICE_IPAD;
	}
    
    void iOSWindow::enterMainLoop()
    {
        NSLog(@"Fatal error: Using enterMainLoop on iOS!");
        exit(-1);
    }
	
	bool iOSWindow::updateOneFrame()
	{
		// call input events
		InputEvent* e;
		while ((e = popInputEvent()) != 0)
		{
			e->execute();
			delete e;
		}	
		if (mKeyboardRequest != 0 && mTouches.size() == 0) // only process keyboard when there is no interaction with the screen
		{
			bool visible = isKeyboardVisible();
			if      (visible && mKeyboardRequest == -1) [glview terminateKeyboardHandling];
			else if (!visible && mKeyboardRequest == 1) [glview beginKeyboardHandling];
			mKeyboardRequest = 0;
		}
		
		float k = mTimer.diff(true);
		return performUpdate(k);	
	}

    void iOSWindow::terminateMainLoop()
	{
        NSLog(@"Fatal error: Using terminateMainLoop on iOS!");
        exit(-2);
    }
	
	void iOSWindow::destroyWindow()
	{
		// just stopping the animation on iOS
		[glview stopAnimation];
	}
	
    void iOSWindow::showSystemCursor(bool visible)
    {
        // no effect on iOS
    }
	
	void iOSWindow::addInputEvent(InputEvent* event)
	{
		while (mInputEventsMutex); // wait it out
		mInputEventsMutex = true;
		mInputEvents += event;
		mInputEventsMutex = false;
	}

	InputEvent* iOSWindow::popInputEvent()
	{
		while (mInputEventsMutex); // wait it out
		if (mInputEvents.size() == 0) return 0;
		mInputEventsMutex = true;
		InputEvent* e = mInputEvents.front();
		mInputEvents.pop_front();
		mInputEventsMutex = false;
		return e;
	}

    bool iOSWindow::isSystemCursorShown()
    {
        return false; // iOS never shows system cursor
    }
	
    int iOSWindow::getWidth()
    {
		// TODO dont swap width and height in case display is in portrait mode
#if __IPHONE_3_2 //__IPHONE_OS_VERSION_MIN_REQUIRED >= 30200
		CAEAGLLayer *caeagllayer = ((CAEAGLLayer*)glview.layer);
		if ([caeagllayer respondsToSelector:@selector(contentsScale)])
		{
			return uiwnd.bounds.size.height * caeagllayer.contentsScale;
		}
#endif
		
        return uiwnd.bounds.size.height;
    }
    int iOSWindow::getHeight()
    {
		// TODO dont swap width and height in case display is in portrait mode
#if __IPHONE_3_2 //__IPHONE_OS_VERSION_MIN_REQUIRED >= 30200
		CAEAGLLayer *caeagllayer = ((CAEAGLLayer*)glview.layer);
		if ([caeagllayer respondsToSelector:@selector(contentsScale)])
		{
			return uiwnd.bounds.size.width * caeagllayer.contentsScale;
		}
#endif
        return uiwnd.bounds.size.width;
    }

    void iOSWindow::setWindowTitle(chstr title)
    {
        // no effect on iOS
    }
	
    gtypes::Vector2 iOSWindow::getCursorPosition()
    {
        return gtypes::Vector2(mCursorX,mCursorY);
    }

    void iOSWindow::presentFrame()
    {
		if (mFirstFrameDrawn)
			[glview swapBuffers];
		else
		{
			doEvents();
			if (!mRetainLoadingOverlay)
				[viewcontroller removeImageView];
			mFirstFrameDrawn = true;
		}
    }

	void* iOSWindow::getIDFromBackend()
	{
		return viewcontroller;
	}

	void iOSWindow::doEvents()
	{
		SInt32 result;
		do
		{
			result = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, TRUE);
		} while(result == kCFRunLoopRunHandledSource);
	}
	
	void iOSWindow::callTouchCallback()
	{
		if (!mTouchCallback) return;
		harray<gvec2> lst;
		gvec2 vec;
		CGPoint pt;
		float scale = _getTouchScale();
		
		foreach(UITouch*, it, mTouches)
		{
			pt = [*it locationInView:glview];
			vec.x = pt.x * scale;
			vec.y = pt.y * scale;
			lst += vec;
		}
		mInputEvents += new TouchInputEvent(this, lst);
	}
	
	bool iOSWindow::isRotating()
	{
		return g_wnd_rotating;
	}
	
	hstr iOSWindow::getParam(chstr param)
	{
		if (param == "retain_loading_overlay")
			return mRetainLoadingOverlay ? "1" : "0";
		else return "";
	}
	
	void iOSWindow::setParam(chstr param, chstr value)
	{
		if (param == "retain_loading_overlay")
		{
			bool prev = mRetainLoadingOverlay;
			mRetainLoadingOverlay = (value == "1");
			if (mRetainLoadingOverlay == false && prev == true && mFirstFrameDrawn)
			{
				[viewcontroller removeImageView];
			}
		}
	}

	float iOSWindow::_getTouchScale()
	{
#if __IPHONE_3_2 //__IPHONE_OS_VERSION_MIN_REQUIRED >= 30200
		static float scale = -1;
		if (scale == -1)
		{
			CAEAGLLayer* caeagllayer = (CAEAGLLayer*)[glview layer];
			if ([caeagllayer respondsToSelector:@selector(contentsScale)]) scale = [caeagllayer contentsScale];
			else scale = 1; // prior to ios 3.2
		}
		return scale;
#else
		return 1;
#endif
	}

	harray<UITouch*> iOSWindow::_convertTouchesToCoordinates(void* nssetTouches)
	{
		float scale = _getTouchScale();
		// return value stored in mCursorX and mCursorY
		harray<UITouch*> lst;
		NSSet* touches = (NSSet*)nssetTouches;

		UITouch* touch;
		int len = [touches count];

		if (len == 1)
		{
			touch = touches.anyObject;
			CGPoint location = [touch locationInView:glview];
			//For "primary" landscape orientation, this is how we calc it
			mCursorX = location.x * scale;
			mCursorY = location.y * scale;
			lst += touch;
		}
		else
		{
			for (touch in touches)
			{
				lst += touch;
			}
		}
		
		return lst;
	}
	
	void iOSWindow::touchesBegan_withEvent_(void* nssetTouches, void* uieventEvent)
	{
		harray<UITouch*> touches = _convertTouchesToCoordinates(nssetTouches);
		
		mTouches += touches;
		if (mTouches.size() > 1)
		{
			if (!mMultiTouchActive && mTouches.size() == 1)
			{
				// cancel (notify the app) the previously called mousedown event so we can begin the multi touch event properly
				addInputEvent(new MouseInputEvent(this, -10000, -10000, AMOUSEBTN_LEFT, AMOUSEEVT_UP));
			}
			mMultiTouchActive = true;
		}
		else
		{
			Window::MouseEventType mouseevt = AMOUSEEVT_DOWN;
			Window::MouseButton mousebtn = AMOUSEBTN_LEFT;
			
			addInputEvent(new MouseInputEvent(this, mCursorX, mCursorY, mousebtn, mouseevt));
		}
		callTouchCallback();
	}

	void iOSWindow::touchesEnded_withEvent_(void* nssetTouches, void* uieventEvent)
	{
		harray<UITouch*> touches = _convertTouchesToCoordinates(nssetTouches);
		int num_touches = mTouches.size();
		foreach(UITouch*, it, touches)
		{
			if (mTouches.contains(*it)) // this safeguard is necesarry because of older iOS version bugs
				mTouches.remove(*it);
		}
		if (mMultiTouchActive)
		{
			if (num_touches == touches.size()) mMultiTouchActive = false;
		}
		else
		{
			Window::MouseEventType mouseevt = AMOUSEEVT_UP;
			Window::MouseButton mousebtn = AMOUSEBTN_LEFT;
			addInputEvent(new MouseInputEvent(this, mCursorX, mCursorY, mousebtn, mouseevt));
		}
		callTouchCallback();
	}
	
	
	void iOSWindow::touchesCancelled_withEvent_(void* nssetTouches, void* uieventEvent)
	{
		// FIXME needs to cancel touches, not treat them as "release"
		touchesEnded_withEvent_(nssetTouches, uieventEvent);
		
	}
	
	
	void iOSWindow::touchesMoved_withEvent_(void* nssetTouches, void* uieventEvent)
	{
		_convertTouchesToCoordinates(nssetTouches);
		
		Window::MouseEventType mouseevt = AMOUSEEVT_MOVE;
		Window::MouseButton mousebtn = AMOUSEBTN_NONE;
		
		addInputEvent(new MouseInputEvent(this, mCursorX, mCursorY, mousebtn, mouseevt));
		
		callTouchCallback();
	}
	
	bool iOSWindow::isKeyboardVisible()
	{
		return [glview isKeyboardActive];
	}
	
	void iOSWindow::beginKeyboardHandling()
	{
		mKeyboardRequest = 1;
	}
	void iOSWindow::terminateKeyboardHandling()
	{
		mKeyboardRequest = -1;
	}
	float iOSWindow::prefixRotationAngle()
	{
		// this function needs updating if we want
		// support for truly portrait-rendered apps
		
		// currently, in the same way as is the case
		// in rest of april for iOS, we only provide
		// support for landscape orientations
		
		//NSLog(@"Orientation: %d", viewcontroller.interfaceOrientation);
		switch (viewcontroller.interfaceOrientation)
		{
			case UIInterfaceOrientationPortrait:
				return 90.0f; // simulate left landscape orientation (needed only until we transform into a landscape orientation on earlier iOS)
			case UIInterfaceOrientationLandscapeLeft:
			case UIInterfaceOrientationLandscapeRight:
				return 0.0f; // any landscape orientation will be well supported elsewhere
				
			case UIInterfaceOrientationPortraitUpsideDown:
				return -90.0f; // this shouldn't occur except if someone plays with shouldAutorotateToInterfaceOrientation in AprilViewController
				
			default:
				break;
		}
		return 0;
	}
	
	void iOSWindow::injectiOSChar(unsigned int inputChar)
	{
		
		if (inputChar == 0)
		{
			// deploy backspace
			handleKeyEvent(AKEYEVT_DOWN, AK_BACK, 8);
			handleKeyEvent(AKEYEVT_UP, AK_BACK, 8);
		}
		if(inputChar >= 32)
		{
			// deploy keypress
			april::KeySym keycode = AK_NONE; // FIXME incorrect, might cause a nasty bug. 
											 // however, writing a translation table atm 
											 // isn't the priority.
		
			handleKeyEvent(AKEYEVT_DOWN, keycode, inputChar);
			handleKeyEvent(AKEYEVT_UP, keycode, inputChar);
		}
	}
	
	void iOSWindow::keyboardWasShown()
	{
		if(mVKeyboardCallback)
			mVKeyboardCallback(true);
	}
	void iOSWindow::keyboardWasHidden()
	{
		if(mVKeyboardCallback)
			mVKeyboardCallback(false);
	}
	
	void iOSWindow::setDeviceOrientationCallback(void (*do_callback)(DeviceOrientation))
	{
			
		if(do_callback)
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
	void iOSWindow::handleDisplayAndUpdate()
	{
		updateOneFrame();
		rendersys->presentFrame();
	}
	
	void iOSWindow::deviceOrientationDidChange()
	{
		if(mDeviceOrientationCallback)
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
			mDeviceOrientationCallback(newOrientation);
		}
	}
	void iOSWindow::applicationWillResignActive()
	{
		if (!mFirstFrameDrawn)
		{
			log("April iOS Window: received app suspend request before first frame was drawn, quitting app.");
			destroy();
			exit(0);
		}
		if (mFocused)
		{
			mFocused = false;
			if (mFocusCallback)
			{
				mFocusCallback(false);
			}
			[glview stopAnimation];
		}
	}
	void iOSWindow::applicationDidBecomeActive()
	{
		if (!mFocused)
		{
			mFocused = true;
			[glview startAnimation];
			if (mFocusCallback)
			{
				mFocusCallback(true);
			}
			
		}
	}

	SystemInfo& getSystemInfo()
	{
		static SystemInfo info;
		if (info.name == "")
		{
			info.locale = [[[NSLocale preferredLanguages] objectAtIndex:0] UTF8String];
			
			size_t size=255;
			char cname[256];
			sysctlbyname("hw.machine", cname, &size, NULL, 0);
			hstr name = cname;
			
			info.name = name; // defaults for unknown devices
			info.ram = 1024; // defaults
			info.cpu_cores = sysconf(_SC_NPROCESSORS_ONLN);
			info.max_texture_size = 0;
						
			if (name.starts_with("iPad"))
			{
				if (name.starts_with("iPad1"))
				{
					info.name = "iPad1";
					info.ram = 256;
				}
				else if (name.starts_with("iPad2"))
				{
					info.name = "iPad2";
					info.ram = 512;
				}
				else if (name.starts_with("iPad3"))
				{
					info.name = "iPad3";
					info.ram = 1024;
				}
			}
			else if (name.starts_with("iPhone"))
			{
				if (name == "iPhone1,1")
				{
					info.name = "iPhone2G";
					info.ram = 128;
				}
				else if (name == "iPhone1,2")
				{
					info.name = "iPhone3G";
					info.ram = 128;
				}
				else if (name == "iPhone2,1")
				{
					info.name = "iPhone3GS";
					info.ram = 256;
				}
				else if (name.starts_with("iPhone3"))
				{
					info.name = "iPhone4";
					info.ram = 512;
				}
				else if (name.starts_with("iPhone4"))
				{
					info.name = "iPhone4S";
					info.ram = 512;
				}
				else if (name.starts_with("iPhone5"))
				{
					info.name = "iPhone5";
					info.ram = 1024;
				}
			}
			else if (name.starts_with("iPod"))
			{
				if (name == "iPod1,1")
				{
					info.name = "iPod1";
					info.ram = 128;
				}
				else if (name == "iPod2,1")
				{
					info.name = "iPod2";
					info.ram = 128;
				}
				else if (name == "iPod3,1")
				{
					info.name = "iPod3";
					info.ram = 256;
				}
				else if (name == "iPod4,1")
				{
					info.name = "iPod4";
					info.ram = 256;
				}
			}
			//else: i386 (iphone simulator) and possible future device types
		}
		if (info.max_texture_size == 0 && april::rendersys != NULL)
			glGetIntegerv(GL_MAX_TEXTURE_SIZE, &info.max_texture_size);

		return info;
	}

}