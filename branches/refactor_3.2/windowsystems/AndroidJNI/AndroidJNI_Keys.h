/// @file
/// @author  Kresimir Spes
/// @version 3.2
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
/// 
/// @section DESCRIPTION
/// 

#ifdef _ANDROID
#ifndef APRIL_ANDROIDJNI_KEYS_H
#define APRIL_ANDROIDJNI_KEYS_H

#include "Keys.h"

namespace april
{
	Key android2april(int androidKeyCode);
	void initAndroidKeyMap();
}

#endif
#endif
