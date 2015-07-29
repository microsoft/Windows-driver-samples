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

   Precompiled header for all filters

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
#include <windowsx.h>

//
// COM includes
//
#include <objbase.h>
#include <oleauto.h>

//
// Standard library includes
//
#include <new>
#include <math.h>
#pragma warning(push)
#pragma warning(disable : 4018)
#include <vector>
#pragma warning(pop)
#include <deque>
#include <map>

//
// ATL Includes
//
#include <atlbase.h>

#pragma warning (push)
#pragma warning (disable:4458)
//
// GDIPlus includes
//
#include <GDIPlus.h>
#pragma warning (pop)

//
// MSXML includes
//
#include <msxml6.h>

//
// Filter pipeline includes
//
#include <winspool.h>
#include <filterpipeline.h>
#include <filterpipelineutil.h>
#include <prntvpt.h>

//
// WCS Includes
//
#include <icm.h>

//
// Windows Imaging Component
//
#include <wincodec.h>

//
// Commonly used namespaces
//
using namespace std;
using namespace Gdiplus;

//
// String safe includes - included last to prevent build warnings
//
#include <strsafe.h>

#include "common.ver"

