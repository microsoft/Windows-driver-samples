/*++
Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    regctrl.h

Environment:

    User mode only

--*/

#pragma once

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <devioctl.h>
#include <tchar.h>
#include <strsafe.h>

#include "common.h"

//
// Utility macro
//

#define ARRAY_LENGTH(array)    (sizeof (array) / sizeof (array[0]))

//
// Logging macros
//

#define InfoPrint(str, ...)                 \
    printf(##str"\n",                       \
            __VA_ARGS__)

#define ErrorPrint(str, ...)                \
    printf("ERROR: %u: "##str"\n",          \
            __LINE__,                       \
            __VA_ARGS__)

//
// Global variables
//

//
// Handle to the driver
//
extern HANDLE g_Driver;

//
// Handle to the root test key
//
extern HKEY g_RootKey;

//
// Version number for the registry callback
//
extern ULONG g_MajorVersion;
extern ULONG g_MinorVersion;


//
// The user mode samples
//

VOID 
PreNotificationBlockSample();

VOID 
PreNotificationBypassSample();

VOID
PostNotificationOverrideSuccessSample();

VOID
PostNotificationOverrideErrorSample();

VOID
CaptureSample();

//
// Utility routines to load and unload the driver
//

BOOL 
UtilLoadDriver(
    _In_ LPTSTR szDriverNameNoExt,
    _In_ LPTSTR szDriverNameWithExt,
    _In_ LPTSTR szWin32DeviceName,
    _Out_ HANDLE *pDriver
    );

BOOL 
UtilUnloadDriver(
    _In_ HANDLE hDriver, 
    _In_opt_ SC_HANDLE hSCM, 
    _In_ LPTSTR szDriverNameNoExt
    );


