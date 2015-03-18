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
// Define the tracing GUID for this driver
//

#define TRACE_CONTROL_GUID (12579E92,1B46,40A6,9CFC,C718A677830B)


//
// Driver specific #defines
//

#define MYDRIVER_TRACING_ID L"Microsoft\\UMDF\\NetNfpProvider"

/* 278F44F0-FF5C-4FE3-BF20-F8AA158EA7BC */
#define MYDRIVER_CLASS_ID   { 0x278F44F0, 0xFF5C, 0x4FE3, {0xBF, 0x20, 0xF8, 0xAA, 0x15, 0x8E, 0xA7, 0xBC} }
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
// define the maximum size of the message queue
//

#define MAX_MESSAGE_QUEUE_SIZE 50

//
// MessageId: STATUS_CANCELLED
//
// MessageText:
//
// The I/O request was canceled.
//
#define STATUS_CANCELLED                 (0xC0000120L)

//
// MessageId: STATUS_INVALID_DEVICE_STATE
//
// MessageText:
//
// The device is not in a valid state to perform this request.
//
#define STATUS_INVALID_DEVICE_STATE      (0xC0000184L)

//
// MessageId: STATUS_INVALID_BUFFER_SIZE
//
// MessageText:
//
// The size of the buffer is invalid for the specified operation.
//
#define STATUS_INVALID_BUFFER_SIZE       (0xC0000206L)

//
// MessageId: STATUS_OBJECT_PATH_NOT_FOUND
//
// MessageText:
//
// {Path Not Found}
// The path %hs does not exist.
//
#define STATUS_OBJECT_PATH_NOT_FOUND     ((NTSTATUS)0xC000003AL)

//
// Include the type specific headers.
//
#include <atlbase.h>
#include <atlcom.h>

// Windows Headers
#include <initguid.h>
#include <Winsock2.h>
#include <Mswsock.h>
#include <Strsafe.h>
#include <devioctl.h>
#include <nfpdev.h>

// Sample headers
#include "NetNfp.h"
#include "WppDefs.h"
#include "list.h"
#include "connection.h"
#include "filecontext.h"
#include "driver.h"
#include "device.h"
#include "queue.h"

_Analysis_mode_(_Analysis_operator_new_null_)

