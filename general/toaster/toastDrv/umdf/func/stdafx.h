/*++

  Copyright (c) Microsoft Corporation, All Rights Reserved

  Module Name:

    stdafx.h

  Abstract:

    Include file for standard system include filesor project specific 
    include files that are used frequently, but are changed infrequently

  Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/


#pragma once

#ifndef STRICT
#define STRICT
#endif


#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // Windows XP and newer.
#endif

#define _ATL_FREE_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

// turns off ATL's hiding of some common and often safely ignored warning messages
#define _ATL_ALL_WARNINGS

#define SAFE_RELEASE(p)     {if ((p)) { (p)->Release(); (p) = NULL; }}

#include "resource.h"
#include <atlbase.h>
#include <atlcom.h>

extern const GUID GUID_DEVINTERFACE_TOASTER;

using namespace ATL;

