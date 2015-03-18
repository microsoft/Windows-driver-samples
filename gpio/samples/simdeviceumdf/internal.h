/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    Internal.h

Abstract:

    This module contains the local type definitions for the UMDF Echo
    driver sample.


Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/
#ifndef _INTERNAL_H_
#define _INTERNAL_H_

#pragma once

#define UMDF_USING_NTSTATUS

#include <windows.h>
#include <winternl.h>
#include <ntstatus.h>
#include <strsafe.h>
#include <gpio.h>

_Analysis_mode_(_Analysis_code_type_user_driver_);  // Macro letting the compiler know this is not a kernel driver (this will help surpress needless warnings)

// Common WPD, UMDF, and WDM headers
#include <devioctl.h>
#include <initguid.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

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

typedef class CSimdeviceDriver *PCSimdeviceDriver;
typedef class CSimdevice *PCSimdevice;
typedef class CSimdeviceQueue  *PCSimdeviceQueue;

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

#define WUDF_TEST_DRIVER_ASSERT(p)  \
{                                   \
    if ( !(p) )                     \
    {                               \
        DebugBreak();               \
        WdfTestNoReturn();          \
    }                               \
}

#define MYDRIVER_CLASS_ID   {0x7ab7dcf5, 0xd1d4, 0x4085, {0x95, 0x47, 0x1d, 0xb9, 0x68, 0xcc, 0xa7, 0x20}}


#define SAFE_RELEASE(p)     {if ((p)) { (p)->Release(); (p) = NULL; }}
#endif
