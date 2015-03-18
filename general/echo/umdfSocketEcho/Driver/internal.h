/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    Internal.h

Abstract:

    This module contains the local type definitions for the UMDF Socketecho sample
    driver sample.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#pragma once

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

//
// Include the winsock headers before any other windows headers.
//
#include <winsock2.h>
#include <ws2tcpip.h>

//
// Include the WUDF DDI 
//

#include "wudfddi.h"

//
// Use specstrings for in/out annotation of function parameters.
//

#include "specstrings.h"

//
// Define the tracing flags.
//

#define WPP_CONTROL_GUIDS                                                   \
    WPP_DEFINE_CONTROL_GUID(                                                \
        MyDriverTraceControl, (64316518,DFE2,42B6,8786,4995E5EC435),        \
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

#define MYDRIVER_TRACING_ID L"Microsoft\\UMDF\\SocketEcho"
#define MYDRIVER_CLASS_ID   { 0x83B87D35, 0x76B8, 0x4920, {0xB4, 0x3C, 0x3B, 0xDE, 0x6B, 0x0E, 0xC5, 0xB8} }

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) {if ((p)) { (p)->Release(); (p) = NULL; }}
#endif

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
{                                     \
    if ( !(p) )                       \
    {                                 \
        DebugBreak();                 \
        WdfTestNoReturn();            \
    }                                 \
}

//
// Include the type specific headers.
//
#include <atlbase.h>
#include <atlcom.h>

#include "connection.h"
#include "filecontext.h"
#include "devicecontext.h"
#include "driver.h"
#include "device.h"
#include "queue.h"

_Analysis_mode_(_Analysis_operator_new_null_)

