/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    Internal.h

Abstract:

    This module contains the local type definitions for the UMDF OSR USB Sample Filter
    driver.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#pragma once

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

//
// Include the WUDF headers
//

#include "wudfddi.h"

//
// Use specstrings for in/out annotation of function parameters.
//

#include "specstrings.h"

//
// Forward definitions of classes in the other header files.
//

typedef class CMyDriver *PCMyDriver;
typedef class CMyDevice *PCMyDevice;
typedef class CMyQueue  *PCMyQueue;

//
// Define the tracing flags.
//

#define WPP_CONTROL_GUIDS                                                   \
    WPP_DEFINE_CONTROL_GUID(                                                \
        MyDriverTraceControl, (73cdcaa5,ce52,43f2,aa2d,5f5a84e22213),       \
                                                                            \
        WPP_DEFINE_BIT(MYDRIVER_ALL_INFO)                                   \
        )

#define WPP_FLAG_LEVEL_LOGGER(flag, level)                                  \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level)                                 \
    (WPP_LEVEL_ENABLED(flag) &&                                             \
     WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

//
// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
// begin_wpp config
// FUNC Trace{FLAG=MYDRIVER_ALL_INFO}(LEVEL, MSG, ...);
// end_wpp
//

//
// Driver specific #defines
//

#define MYDRIVER_TRACING_ID      L"Microsoft\\UMDF\\OsrUsbFilter"
#define MYDRIVER_COM_DESCRIPTION L"UMDF OSR USB Sample Filter Driver"
#define MYDRIVER_CLASS_ID        {0x422d8dbc, 0x520d, 0x4d7e, {0x8f, 0x53, 0x92, 0x0e, 0x5c, 0x86, 0x7e, 0x6c}}

//
// Include the type specific headers.
//

#include "comsup.h"
#include "driver.h"
#include "device.h"
#include "queue.h"

__forceinline 
#ifdef _PREFAST_
__declspec(noreturn)
#endif
VOID
WdfTestNoReturn(
    VOID
    )
{
    // do nothing.
}

#define WUDF_SAMPLE_DRIVER_ASSERT(p)  \
{                                   \
    if ( !(p) )                     \
    {                               \
        DebugBreak();               \
        WdfTestNoReturn();          \
    }                               \
}

#define SAFE_RELEASE(p)     {if ((p)) { (p)->Release(); (p) = NULL; }}
