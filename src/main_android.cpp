/// @file
/// @author  Boris Mikic
/// @version 1.4
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php

#ifdef _ANDROID
#include <jni.h>

#include <hltypes/harray.h>
#include <hltypes/hstring.h>

#include "main.h"
#include "RenderSystem.h"
#include "Window.h"

namespace april
{
	void JNICALL _JNI_init(JNIEnv* env, jclass classe, jobjectArray _args)
	{
		harray<hstr> args;
		jstring arg;
		const char* str;
		int length = env->GetArrayLength(_args);
		for (int i = 0; i < length; i++)
		{
			arg = (jstring)env->GetObjectArrayElement(_args, i);
			str = env->GetStringUTFChars(arg, NULL);
			args += hstr(str);
			env->ReleaseStringUTFChars(arg, str);
		}
		april_init(args);
	}

	void JNICALL _JNI_destroy(JNIEnv* env, jclass classe)
	{
		april_destroy();
	}

	void JNICALL _JNI_render(JNIEnv* env, jclass classe)
	{
		// TODO
		april::rendersys->getWindow()->enterMainLoop(); // this does not really enter a loop in AndroidJNIWindow
	}

#define METHOD_COUNT 3 // make sure this fits
	static JNINativeMethod methods[METHOD_COUNT] =
	{
		{"init",	"([Ljava/lang/String;)V",	(void*)&april::_JNI_init	},
		{"destroy",	"()V",						(void*)&april::_JNI_destroy	},
		{"render",	"()V",						(void*)&april::_JNI_render	}
	};

	jint JNI_OnLoad(JavaVM* vm, void* reserved, chstr packageName)
	{
		JNIEnv* env;
		if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
		{
			return -1;
		}
		jclass classe = env->FindClass((packageName + "/AprilJNI").c_str());
		if (env->RegisterNatives(classe, methods, METHOD_COUNT) != 0)
		{
			return -1;
		}
		return JNI_VERSION_1_6;
	}
}

#endif
