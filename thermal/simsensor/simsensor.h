/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    simsensor.h

Abstract:

    This is the header file for the simulated sensor driver.

@@BEGIN_DDKSPLIT
Author:

    Nicholas Brekhus (NiBrekhu) 25-Jul-2011

Revision History:

@@END_DDKSPLIT
--*/

//--------------------------------------------------------------------- Pragmas

#pragma once

//-------------------------------------------------------------------- Includes

#include <ntddk.h>
#include <wdf.h>
#include <ntstrsafe.h>
#include <initguid.h>
#include <wdmguid.h>
#include <poclass.h>

//----------------------------------------------------------------------- Debug

#if DBG
    extern ULONG SimSensorDebug;
    #define DebugPrint(l, m, ...) \
        if(l & SimSensorDebug)  DbgPrint(m, __VA_ARGS__)

    #define DebugEnter() \
        DebugPrint(SIMSENSOR_TRACE, "Entering " __FUNCTION__ "\n")

    #define DebugExit() \
        DebugPrint(SIMSENSOR_TRACE, "Leaving " __FUNCTION__ "\n")

    #define DebugExitStatus(_status_) \
        DebugPrint(SIMSENSOR_TRACE, "Leaving " __FUNCTION__ ": Status=0x%x\n", _status_)

#else

    #define DebugPrint(l, m, ...)
    #define DebugEnter()
    #define DebugExit()
    #define DebugExitStatus(_status_)

#endif

#define SIMSENSOR_NOTE         0x00000001
#define SIMSENSOR_WARN         0x00000002
#define SIMSENSOR_ERROR_ONLY   0x00000004
#define SIMSENSOR_ERROR        (SIMSENSOR_ERROR_ONLY | SIMSENSOR_WARN)
#define SIMSENSOR_TRACE        0x00000008
#define SIMSENSOR_DEBUG        0x80000000
#define SIMSENSOR_PRINT_ALWAYS 0xffffffff
#define SIMSENSOR_PRINT_NEVER  0x0

//----------------------------------------------------------------- Definitions

typedef struct {
    WDFQUEUE    PendingRequestQueue;
    WDFWAITLOCK QueueLock;
    WDFWORKITEM InterruptWorker;

    //
    // Virtual temperature sensor internal state. This portion of the context
    // should be opaque to most of the driver, except the portion implementing
    // the virtual sensor hardware.
    //

    struct {
        PVOID       PolicyHandle;
        BOOLEAN     Enabled;
        ULONG       LowerBound;
        ULONG       UpperBound;
        ULONG       Temperature;
        WDFWAITLOCK Lock;
    } Sensor;
} FDO_DATA, *PFDO_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DATA, GetDeviceExtension);

typedef struct {
    LARGE_INTEGER ExpirationTime;
    ULONG HighTemperature;
    ULONG LowTemperature;
} READ_REQUEST_CONTEXT, *PREAD_REQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(READ_REQUEST_CONTEXT);

