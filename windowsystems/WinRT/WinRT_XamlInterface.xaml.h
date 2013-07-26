﻿/// @file
/// @author  Boris Mikic
/// @version 3.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
/// 
/// @section DESCRIPTION
/// 
/// Defines a WinRT View.

#ifdef _WINRT_WINDOW
#ifndef APRIL_WINRT_XAML_INTERFACE_H
#define APRIL_WINRT_XAML_INTERFACE_H

#include "windowsystems/WinRT/WinRT_XamlInterface.g.h" // auto-generated file

using namespace Windows::UI::Xaml::Controls;

namespace april
{
    [Windows::Foundation::Metadata::WebHostHidden]
    public ref class WinRT_XamlInterface sealed
    {
    public:
        WinRT_XamlInterface();

		void showKeyboard();
		void hideKeyboard();

    };

}
#endif
#endif
