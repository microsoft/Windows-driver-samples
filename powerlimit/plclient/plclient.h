/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    plclient.h

Abstract:

    This is the header file for the simulated power limit client driver.

--*/

#pragma once

//-------------------------------------------------------------------- Includes

#include <ntddk.h>
#include <wdf.h>
#include <ntstrsafe.h>
#include <initguid.h>
#include <wdmguid.h>
#include <poclass.h>
#include <limits.h>
#include "powerlimitclient_drvinterface.h"

//----------------------------------------------------------------------- Types

typedef struct {
    ULONG LimitCount;
    PPOWER_LIMIT_ATTRIBUTES LimitAttributes;
    PPOWER_LIMIT_VALUE LimitValues;
} FDO_DATA, *PFDO_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DATA, GetDeviceExtension);

//----------------------------------------------------------------------- Debug

#define DebugPrint(l, m, ...) DbgPrintEx(DPFLTR_POWER_ID, l, "[plclient]: "m, __VA_ARGS__)
#define DebugEnter() DebugPrint(PLCLIENT_PRINT_TRACE, "Entering %s", __FUNCTION__)
#define DebugExit() DebugPrint(PLCLIENT_PRINT_TRACE, "Leaving " __FUNCTION__ "\n")
#define DebugExitStatus(_status_) DebugPrint(PLCLIENT_PRINT_TRACE, "Leaving " __FUNCTION__ ": Status=0x%08x\n", _status_)

#define PLCLIENT_PRINT_ERROR        DPFLTR_ERROR_LEVEL
#define PLCLIENT_PRINT_TRACE        DPFLTR_TRACE_LEVEL
#define PLCLIENT_PRINT_INFO         DPFLTR_INFO_LEVEL

#define PLCLIENT_TAG 'PLCL'

//--------------------------------------------------------------------- Globals

extern WDFWAITLOCK GlobalMutex;

//------------------------------------------------------------------ Prototypes

FORCEINLINE
VOID
AcquireGlobalMutex (
    VOID
    )
{

    WdfWaitLockAcquire(GlobalMutex, 0);
    return;
}

FORCEINLINE
VOID
ReleaseGlobalMutex (
    VOID
    )
{

    WdfWaitLockRelease(GlobalMutex);
    return;
}

//
// plclient.c
//

QUERY_POWER_LIMIT_ATTRIBUTES PLCQueryAttributes;
SET_POWER_LIMIT PLCSetLimits;
QUERY_POWER_LIMIT PLCQueryLimitValues;

NTSTATUS
InitPowerLimitValues (
    _Inout_ PFDO_DATA DevExt
    );

VOID
CleanupPowerLimitValues (
    _Inout_opt_ PFDO_DATA DevExt
    );
