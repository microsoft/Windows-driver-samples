/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    Internal.h

Abstract:

    This module contains the local type definitions for the UMDF OSR Fx2
    driver sample.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#pragma once

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

//
// Include the WUDF Headers
//

#include "wudfddi.h"

//
// Include SetupDi functions.
//

#include "setupapi.h"

//
// Use specstrings for in/out annotation of function parameters.
//

#include "specstrings.h"

//
// Include the safestring functions.
//

#include "strsafe.h"

//
// Get limits on common data types (ULONG_MAX for example)
//

#include "limits.h"

//
// We need usb I/O targets to talk to the OSR device.
//

#include "wudfusb.h"

//
// Include the header shared between the drivers and the test applications.
//

#include "public.h"

//
// Include the header shared between the drivers and the test applications.
//

#include "WUDFOsrUsbPublic.h"

//
// Forward definitions of classes in the other header files.
//

typedef class CMyDriver *PCMyDriver;
typedef class CMyDevice *PCMyDevice;
typedef class CMyQueue  *PCMyQueue;

typedef class CMyControlQueue   *PCMyControlQueue;
typedef class CMyReadWriteQueue *PCMyReadWriteQueue;

typedef class CCancelCallback *PCCancelCallback;

//
// Define the tracing flags.
//
// TODO: Choose a different trace control GUID
//

#define WPP_CONTROL_GUIDS                                                   \
    WPP_DEFINE_CONTROL_GUID(                                                \
        WudfOsrUsbFx2TraceGuid, (da5fbdfd,1eae,4ecf,b426,a3818f325ddb),   \
                                                                            \
        WPP_DEFINE_BIT(MYDRIVER_ALL_INFO)                                   \
        WPP_DEFINE_BIT(TEST_TRACE_DRIVER)                                   \
        WPP_DEFINE_BIT(TEST_TRACE_DEVICE)                                   \
        WPP_DEFINE_BIT(TEST_TRACE_QUEUE)                                    \
        )                             

#define WPP_FLAG_LEVEL_LOGGER(flag, level)                                  \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level)                                 \
    (WPP_LEVEL_ENABLED(flag) &&                                             \
     WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
           WPP_LEVEL_LOGGER(flags)
               
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
           (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//
// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
// begin_wpp config
// FUNC Trace{FLAG=MYDRIVER_ALL_INFO}(LEVEL, MSG, ...);
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// end_wpp
//

//
// Driver specific #defines
//
// TODO: Change these values to be appropriate for your driver.
//

#define MYDRIVER_TRACING_ID      L"Microsoft\\UMDF\\OsrUsb"
#define MYDRIVER_CLASS_ID        {0x0865b2b0, 0x6b73, 0x428f, {0xa3, 0xea, 0x21, 0x72, 0x83, 0x2d, 0x6b, 0xfc}}

//
// Include the type specific headers.
//

#include "comsup.h"
#include "driver.h"
#include "device.h"
#include "queue.h"
#include "ControlQueue.h"
#include "ReadWriteQueue.h"
#include "list.h"

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

#define WUDF_TEST_DRIVER_ASSERT(p)  \
{                                   \
    if ( !(p) )                     \
    {                               \
        DebugBreak();               \
        WdfTestNoReturn();          \
    }                               \
}

#define SAFE_RELEASE(p)     {if ((p)) { (p)->Release(); (p) = NULL; }}

#define IDLE_TIMEOUT_IN_MSEC 10*1000
