/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    CLISRV.h

Abstract:

    Header file for functions common to bth echo server and client

Environment:

    Kernel mode

--*/

#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <initguid.h> 
#include <ntstrsafe.h>
#include <bthdef.h>
#include <ntintsafe.h>
#include <bthguid.h>
#include <bthioctl.h>
#include <sdpnode.h>
#include <bthddi.h>
#include <bthsdpddi.h>
#include <bthsdpdef.h>

#include "trace.h"
#include "public.h"

#define POOLTAG_BTHECHOSAMPLE 'htbw'


enum _BTHECHOSAMPLE_MAX_DATA_LENGTH
{
    BthEchoSampleMaxDataLength = 256
};

//
// Device context common to both client and the server
//

typedef struct _BTHECHOSAMPLE_DEVICE_CONTEXT_HEADER
{
    //
    // Framework device this context is associated with
    //
    WDFDEVICE Device;

    //
    // Default I/O target
    //
    WDFIOTARGET IoTarget;

    //
    // Profile driver interface which contains profile driver DDI
    //
    BTH_PROFILE_DRIVER_INTERFACE ProfileDrvInterface;

    //
    // Local Bluetooth Address
    //
    BTH_ADDR LocalBthAddr;

#if (NTDDI_VERSION >= NTDDI_WIN8)

    //
    // Features supported by the local stack
    //
    BTH_HOST_FEATURE_MASK LocalFeatures;

#endif

    //
    // Preallocated request to be reused during initialization/deinitialzation phase
    // Access to this reqeust is not synchronized
    //
    WDFREQUEST Request;
} BTHECHOSAMPLE_DEVICE_CONTEXT_HEADER, *PBTHECHOSAMPLE_DEVICE_CONTEXT_HEADER;

#include "connection.h"

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
BthEchoSharedDeviceContextHeaderInit(
    PBTHECHOSAMPLE_DEVICE_CONTEXT_HEADER header,
    WDFDEVICE Device
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
BthEchoSharedRetrieveLocalInfo(
    _In_ PBTHECHOSAMPLE_DEVICE_CONTEXT_HEADER DevCtxHdr
    );

#if (NTDDI_VERSION >= NTDDI_WIN8)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
BthEchoSharedGetHostSupportedFeatures(
    _In_ PBTHECHOSAMPLE_DEVICE_CONTEXT_HEADER DevCtxHdr
    );

#endif


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
BthEchoSharedSendBrbSynchronously(
    _In_ WDFIOTARGET IoTarget,
    _In_ WDFREQUEST Request,
    _In_ PBRB Brb,
    _In_ ULONG BrbSize
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
BthEchoSharedSendBrbAsync(
    _In_ WDFIOTARGET IoTarget,
    _In_ WDFREQUEST Request,
    _In_ PBRB Brb,
    _In_ size_t BrbSize,
    _In_ PFN_WDF_REQUEST_COMPLETION_ROUTINE ComplRoutine,
    _In_opt_ WDFCONTEXT Context
    );

