/// @file
/// @author  Kresimir Spes
/// @version 1.31
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php

#ifdef _WIN32
#include "RenderSystem.h"
#include "Timer.h"

namespace april
{
	Timer::Timer()
	{
		mDt = 0;
		mTd2 = 0;
		mTd = 0;
		mFrequency = 0;
		mPerformanceTimerStart = 0;
		mResolution = 0;
		mMmTimerStart = 0;
		mMmTimerElapsed = 0;
		mPerformanceTimerElapsed = 0;
		mPerformanceTimer = false;
		
		if (!QueryPerformanceFrequency((LARGE_INTEGER*)&mFrequency))
		{
			april::log("performance timer not available, multimedia timer will be used instead!");
			mPerformanceTimer = false;
			mMmTimerStart = timeGetTime();
			mResolution = 1.0f / 1000.0f;
			mFrequency = 1000;
			mMmTimerElapsed = (unsigned long)mMmTimerStart;
		}
		else
		{
			QueryPerformanceCounter((LARGE_INTEGER*)&mPerformanceTimerStart);
			mPerformanceTimer = true;
			mResolution = (float)(1.0 / mFrequency);
			mPerformanceTimerElapsed = mPerformanceTimerStart;
		}
	}
	
	Timer::~Timer()
	{
		
	}
	
	float Timer::getTime()
	{
		__int64 time;
		if (mPerformanceTimer)
		{
			QueryPerformanceCounter((LARGE_INTEGER *) &time);
			return ((float)(time - mPerformanceTimerStart) * mResolution * 1000.0f);
		}
		else
		{
			return ((float)(timeGetTime() - mMmTimerStart) * mResolution * 1000.0f);
		}
	}
	
	float Timer::diff(bool update)
	{
		if (update)
		{
			this->update();
		}
		return mDt;
	}
	
	void Timer::update()
	{
		mTd2 = getTime();
		mDt = (mTd2 - mTd) * 0.1f;
		mTd = mTd2;
	}
	
}
#endif