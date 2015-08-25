/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   precomp.h

Abstract:

   Precompiled header.

--*/

#pragma once

//
// Annotate this as a usermode driver for static analysis
//
#include <DriverSpecs.h>
_Analysis_mode_(_Analysis_code_type_user_driver_)

//
// Standard Annotation Language include
//
#include <sal.h>

//
// Windows includes
//
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <windows.h>

//
// COM includes
//
#include <objbase.h>
#include <oleauto.h>

//
// ATL Includes
//
#include <atlbase.h>

//
// STL Includes
//
#include <new>

//
// MSXML includes
//
#include <msxml6.h>

#include <StrSafe.h>

#include "common.ver"

