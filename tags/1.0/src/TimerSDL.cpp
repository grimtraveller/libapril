/************************************************************************************\
This source file is part of the Awesome Portable Rendering Interface Library         *
For latest info, see http://libapril.sourceforge.net/                                *
**************************************************************************************
Copyright (c) 2010 Kresimir Spes (kreso@cateia.com)                                  *
Copyright (c) 2010 Ivan Vucica (ivan@vucica.net)                                     *
*                                                                                    *
* This program is free software; you can redistribute it and/or modify it under      *
* the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php   *
\************************************************************************************/
#include <SDL/SDL.h>
#include "Timer.h"
#include "RenderSystem.h"

namespace April
{
    Timer::Timer()
    {
        mDt = 0;
        mTd2 = 0;
        mTd = 0;
        mFrequency = 0;
        mPerformanceTimerStart = 0;
        mResolution = 0; // unused in SDL timer
        mMmTimerStart = 0;
        mMmTimerElapsed = 0;
        mPerformanceTimerElapsed = 0;
        mPerformanceTimer = 0;
        
		// for sdl:
		mPerformanceTimer	= 0; // was: "false"
		mMmTimerStart	    = SDL_GetTicks();
		mFrequency			= 1000;
		mMmTimerElapsed	    = mMmTimerStart;
            
        
    }
    
    Timer::~Timer()
    {
        
    }
    
    float Timer::getTime()
    {        
		return( (float) ( SDL_GetTicks() - mMmTimerStart));
        
    }
    
    float Timer::diff(bool doUpdate)
    {
        if(doUpdate)
        {
            this->update();
            return mDt;
        }
        else
            return mDt;
        
    }
    
    void Timer::update()
    {
        mTd2 = getTime();
        mDt = (mTd2-mTd) * 0.1f;
        mTd = mTd2;
    }
}