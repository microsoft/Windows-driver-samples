/**************************************************************************
*
*  Copyright © Microsoft Corporation
*
*  File Title:  stdafx.h
*
*  Project:     Production Scanner Driver Sample
*
*  Description: precompiled header file
*
***************************************************************************/

#pragma once

//
// This is the size of the buffer the driver uses to write data to the
// WIA image download stream and also to declare for legacy applications
// through WIA_IPA_BUFFER_SIZE. The buffer size chosen is 64KB (65536 bytes):
//
#define DEFAULT_BUFFER_SIZE 65536

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

//
// WIA_DEBUG is needed in order to enable WIA tracing in free
// builds (see WIAS_TRACE and WIAEX_ERROR in wiamdef.h):
//
#ifndef WIA_DEBUG
#define WIA_DEBUG
#endif

//
// Windows system headers:
//

#include <windows.h>             // Windows
#include <stdlib.h>              // C standard library
#include <stdio.h>               // std out
#include <coguid.h>              // COM
#include <objbase.h>             // COM
#include <shobjidl.h>            // Shell UI Extension
#include <shlobj.h>              // Shell UI Extension
#include <shlwapi.h>             // Shell light weight API
#include <strsafe.h>             // Safe character string APIs
#include <gdiplus.h>             // GDI+
#include <limits.h>
#include <initguid.h>

//
// WIA driver core headers:
//
#include <sti.h>                 // STI defines
#include <stiusd.h>              // IStiUsd interface
#include <wiamindr.h>            // IWiaMinidrv interface
#include <wiadevd.h>             // IWiaUIExtension interface
#include <wiamdef.h>

//
// WIA driver headers:
//
#include "constants.h"           // Constant declarations
#include "basicarray.h"          // CSimpleDynamicArray class
#include "propman.h"             // WIA driver property manager class
#include "capman.h"              // WIA driver capability manager class
#include "wiautil.h"             // WIA driver helper functions
#include "resource.h"            // WIA driver resource definitions
#include "minidrv.h"             // WIA driver header

//
// Helpers for image file format translations (using GDI+):
//
using namespace Gdiplus;
#include "fileconv.h"

//
// WIA tracing macro helpers:
//
#define WIAEX_ERROR(args) { WIAS_ERROR((g_hInst, "Error in %s (%u):", __FUNCTION__, __LINE__)); WIAS_ERROR(args); }
#define WIAEX_TRACE(args) { FAILED(hr) ? WIAS_ERROR(args) : WIAS_TRACE(args);  }
#define WIAEX_TRACE_FUNC_HR { FAILED(hr) ? WIAS_ERROR((g_hInst, "%s failed, hr = 0x%08X", __FUNCTION__, hr)) : WIAS_TRACE((g_hInst, "%s succeeded, hr = 0x%08X", __FUNCTION__, hr)); }
#define WIAEX_TRACE_BEGIN { WIAS_TRACE((g_hInst, "%s..", __FUNCTION__)); }
