//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef STRICT
#define STRICT
#endif

#include "targetver.h"
#if (NTDDI_VERSION >= NTDDI_WIN10_VB)
#pragma message("MFT0 is deprecated for this target Windows version and beyond- Change project settings to target an older version of Windows")
#endif

#if (NTDDI_VERSION <= NTDDI_WIN7)
// TODO: disable the MFT0 conditional to the target system version
#pragma message("MFT0 is in not supported in this target Windows version - Change project settings to target a newer version of Windows")
#endif

#include <wrl.h>
#include <initguid.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <ks.h>
#include <ksmedia.h>
#include <inspectable.h>
#include <vector>


using namespace Microsoft::WRL;