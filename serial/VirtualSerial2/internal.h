/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    Internal.h

Abstract:

    This module contains the local type definitions for the VirtualSerial
    driver sample.

Environment:

    Windows Driver Framework

--*/

#pragma once

#ifdef _KERNEL_MODE
#include <ntddk.h>
#else
#include <windows.h>
#endif

#include <wdf.h>

#define _NTDEF_

//
// Include the type specific headers.
//
#include "serial.h"
#include "driver.h"
#include "device.h"
#include "ringbuffer.h"
#include "queue.h"

//
// Tracing and Assert
//

#define Trace(level, _fmt_, ...)                    \
    DbgPrintEx(DPFLTR_DEFAULT_ID, level,            \
                _fmt_ "\n", __VA_ARGS__)

#define TRACE_LEVEL_ERROR   DPFLTR_ERROR_LEVEL
#define TRACE_LEVEL_INFO    DPFLTR_INFO_LEVEL

#ifndef ASSERT
#define ASSERT(exp) {                               \
    if (!(exp)) {                                   \
        RtlAssert(#exp, __FILE__, __LINE__, NULL);  \
    }                                               \
}
#endif