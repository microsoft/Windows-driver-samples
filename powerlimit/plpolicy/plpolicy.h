/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    plpolicy.h

Abstract:

    This is the header file for the simulated power limit policy driver.

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
#include "powerlimitpolicy_drvinterface.h"

//----------------------------------------------------------------------- Types

typedef struct _DEVICE_REGISTRATION {
    LIST_ENTRY Link;
    BOOLEAN Initialized;
    ULONG RequestId;
    PVOID PowerLimitRequest;
    UNICODE_STRING TargetDeviceName;
} DEVICE_REGISTRATION, *PDEVICE_REGISTRATION;

typedef struct _FDO_DATA {
    ULONG RequestCount;
    LIST_ENTRY RequestHeader;
} FDO_DATA, *PFDO_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DATA, GetDeviceExtension);

//----------------------------------------------------------------------- Debug

#define DebugPrint(l, m, ...) DbgPrintEx(DPFLTR_POWER_ID, l, "[plpolicy]: "m, __VA_ARGS__)
#define DebugEnter() DebugPrint(PLPOLICY_PRINT_TRACE, "Entering " __FUNCTION__ "\n")
#define DebugExit() DebugPrint(PLPOLICY_PRINT_TRACE, "Leaving " __FUNCTION__ "\n")
#define DebugExitStatus(_status_) DebugPrint(PLPOLICY_PRINT_TRACE, "Leaving " __FUNCTION__ ": Status=0x%x\n", _status_)

#define PLPOLICY_PRINT_ERROR        DPFLTR_ERROR_LEVEL
#define PLPOLICY_PRINT_TRACE        DPFLTR_TRACE_LEVEL
#define PLPOLICY_PRINT_INFO         DPFLTR_INFO_LEVEL

#define PLPOLICY_TAG 'PLPO'

#define OffsetToPtr(Base, Offset) ((PVOID)((PUCHAR)(Base) + (Offset)))

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
// plpolicy.c
//

NTSTATUS
RegisterRequest (
    _Inout_ PFDO_DATA DevExt,
    _In_ PUNICODE_STRING TargetDeviceName,
    _In_ PDEVICE_OBJECT PolicyDeviceObject,
    _Out_ PULONG RequestId
    );

NTSTATUS
UnregisterRequest (
    _Inout_ PFDO_DATA DevExt,
    _In_ ULONG RequestId
    );

NTSTATUS
QueryAttributes (
    _Out_ PPOWER_LIMIT_ATTRIBUTES Buffer,
    _Inout_ PFDO_DATA DevExt,
    _In_ ULONG RequestId,
    _In_ ULONG BufferCount
    );

NTSTATUS
QueryLimitValues (
    _Out_ PPOWER_LIMIT_VALUE Buffer,
    _Inout_ PFDO_DATA DevExt,
    _In_ ULONG RequestId,
    _In_ ULONG BufferCount
    );

NTSTATUS
SetLimitValues (
    _Inout_ PFDO_DATA DevExt,
    _In_ ULONG RequestId,
    _In_ ULONG BufferCount,
    _In_ PPOWER_LIMIT_VALUE Buffer
    );
