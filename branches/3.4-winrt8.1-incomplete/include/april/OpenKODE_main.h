/// @file
/// @version 3.4
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// 
/// @section DESCRIPTION
/// 
/// Defines main for OpenKODE.

#ifdef _OPENKODE
#ifndef APRIL_OPENKODE_MAIN_H
#define APRIL_OPENKODE_MAIN_H

#include <hltypes/harray.h>
#include <hltypes/hstring.h>

#include "main_base.h"
#include "aprilExport.h"

KDint KD_APIENTRY kdMain(KDint argc, const KDchar* const* argv)
{
	return april_main(april_init, april_destroy, (int)argc, (char**)argv);
}
#endif
#endif
