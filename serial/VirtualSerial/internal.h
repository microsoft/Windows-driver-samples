/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    Internal.h

Abstract:

    This module contains the local type definitions for the UMDF VirtualSerial
    driver sample.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#pragma once

#include <windows.h>
#include <devioctl.h>
#include <ntddser.h>
#include <setupapi.h>
#include <strsafe.h>
#include <intsafe.h>

//
// Include the WUDF DDI
//

#include "wudfddi.h"

//
// Use specstrings for in/out annotation of function parameters.
//

#include "specstrings.h"

//
// Forward definitions of classes in the other header files.
//

typedef class CMyDriver    *PCMyDriver;
typedef class CMyDevice    *PCMyDevice;
typedef class CMyQueue     *PCMyQueue;
typedef class CRingBuffer  *PCMyRingBuffer;

//
// Define useful macros
//

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#define SAFE_RELEASE(p)     {if ((p)) { (p)->Release(); (p) = NULL; }}

//
// Define the tracing flags.
//

#define WPP_CONTROL_GUIDS                                                   \
    WPP_DEFINE_CONTROL_GUID(                                                \
        MyDriverTraceControl, (C31877A2,C8C8,4c58,B5B8,7D6311D5E343),       \
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

#define MYDRIVER_TRACING_ID L"Microsoft\\UMDF\\VirtualSerial"
#define MYDRIVER_CLASS_ID   { 0xc8ecc087, 0x6b79, 0x4de5, { 0x8f, 0xb4, 0xc0, 0x33, 0x58, 0xa2, 0x96, 0x17 } }

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

#define MAXULONG    0xffffffff

//
// Device states
//
#define COMMAND_MATCH_STATE_IDLE   0
#define COMMAND_MATCH_STATE_GOT_A  1
#define COMMAND_MATCH_STATE_GOT_T  2

//
// Include the type specific headers.
//

#include "comsup.h"
#include "driver.h"
#include "device.h"
#include "ringbuffer.h"
#include "queue.h"
#include "serial.h"
