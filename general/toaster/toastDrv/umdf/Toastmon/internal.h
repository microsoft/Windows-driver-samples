/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    Internal.h

Abstract:

    This module contains the local type definitions for the 
    driver sample.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#pragma once

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
// Include ATL to provide basic COM support.
//

#define _ATL_FREE_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

#include <atlbase.h>
#include <atlcom.h>

using namespace ATL;

extern CComModule _Module;
//
// Forward definitions of classes in the other header files.
//

typedef class CMyDriver       *PCMyDriver;
typedef class CMyDevice       *PCMyDevice;
typedef class CMyRemoteTarget *PCMyRemoteTarget;

//
// Define the tracing flags.
//
// TODO: Choose a different trace control GUID
//

#define WPP_CONTROL_GUIDS                                                   \
    WPP_DEFINE_CONTROL_GUID(                                                \
        MyDriverTraceControl, (dee2c67c,6328,48d9,876b,64682c4e9e9b),       \
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
// TODO: Change these values to be appropriate for your driver.
//

#define MYDRIVER_TRACING_ID L"Microsoft\\UMDF\\ToastMon"
#define MYDRIVER_CLASS_ID   { 0x8d4ec202, 0x1076, 0x4895, {0xa0, 0x72, 0x29, 0x7d, 0xa8, 0x8e, 0x60, 0x05} }

//
// Include simple doubly-linked list macros
//
#include "list.h"

//
// Include the type specific headers.
//

#include "comsup.h"
#include "driver.h"
#include "remotetarget.h"
#include "device.h"
