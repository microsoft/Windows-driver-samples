/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   globals.cpp

Abstract:

   Stores function definitions, variable declerations and pre-processor macros
   global to the filter module. This is shared between all filters.

--*/

#pragma once

//
// Module's Instance handle from DLLEntry of process.
//
extern HINSTANCE g_hInstance;

//
// Server lock count
//
extern LONG g_cServerLocks;

//
// Global defines
//
#define CB_COPY_BUFFER   0x10000
#define MAX_UISTRING_LEN 256
#define OEM_SIGNATURE    'XDSM'
#define OEM_VERSION      0x00000001L

//
// Conversion functions for Microns to 1/100th of Inch (and visa versa)
//
#define HUNDREDTH_OFINCH_TO_MICRON(x) MulDiv(x, 25400, 100)
#define MICRON_TO_HUNDREDTH_OFINCH(x) MulDiv(x, 100, 25400)

//
// Macros for checking pointers and handles.
//
#define CHECK_POINTER(p, hr) ((p) == NULL ? hr : S_OK)
#define CHECK_HANDLE(h, hr) ((h) == NULL ? hr : S_OK)

static const FLOAT kMaxByteAsFloat = 255.0f;
static const FLOAT kMaxWordAsFloat = 65535.0f;

static const WORD  kS2Dot13Neg = 0x8000;
static const WORD  kS2Dot13One = 0x2000;
static const WORD  kS2Dot13Min = 0xFFFF;
static const WORD  kS2Dot13Max = 0x7FFF;

static const FLOAT k96thInchAsMicrons = 264.58f;

//
// countof macro
//
#ifndef countof
#define countof(ary) (sizeof(ary) / sizeof((ary)[0]))
#endif

//
// Converts the Win32 last error to an HRESULT
//
HRESULT
GetLastErrorAsHResult(
    void
    );

//
// Converts a GDI status error to an HRESULT
//
HRESULT
GetGDIStatusErrorAsHResult(
    _In_ Status gdiPStatus
    );

//
// Check if we are running under Vista
//
BOOL
IsVista(
    VOID
    );

//
// Returns a unique number
//
DWORD
GetUniqueNumber(
    VOID
    );
