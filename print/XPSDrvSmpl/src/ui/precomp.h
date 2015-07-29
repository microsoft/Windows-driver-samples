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

   Precompiled header for UI plugin module.

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
// Prefast warning suppression macros
//
#include <suppress.h>

//
// Windows includes
//
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <limits.h>

//
// COM includes
//
#include <objbase.h>
#include <oleauto.h>

//
// ATL Includes
//
#include <atlbase.h>

#pragma warning (push)
#pragma warning (disable:4458)
//
// GDIPlus includes
//
#include <gdiplus.h>
#pragma warning (pop)

//
// MSXML includes
//
#include <msxml6.h>

//
// Standard library includes
//
#include <new>
#include <vector>
#include <map>

#include <prsht.h>
#include <initguid.h>
#include <winddiui.h>

#include <printoem.h>
#include <prdrvcom.h>
#include <prcomoem.h>

//
// Commonly used namespaces
//
using namespace std;
using namespace Gdiplus;

//
// StrSafe.h needs to be included last to disallow bad string functions.
//
#include <strsafe.h>

#define E_ELEMENT_NOT_FOUND  HRESULT_FROM_WIN32(ERROR_NOT_FOUND)


