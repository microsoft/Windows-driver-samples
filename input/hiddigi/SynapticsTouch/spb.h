/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved. 
    Sample code. Dealpoint ID #843729.

    Module Name: 

        spb.h

    Abstract:

        This module contains the touch driver I2C helper definitions.

    Environment:
 
        Kernel Mode

    Revision History:

--*/

#pragma once

#include <wdm.h>
#include <wdf.h>

#define DEFAULT_SPB_BUFFER_SIZE 64

//
// SPB (I2C) context
//

typedef struct _SPB_CONTEXT
{
    WDFIOTARGET SpbIoTarget;
    LARGE_INTEGER I2cResHubId;
    WDFMEMORY WriteMemory;
    WDFMEMORY ReadMemory;
    WDFWAITLOCK SpbLock;
} SPB_CONTEXT;

NTSTATUS 
SpbReadDataSynchronously(
    _In_ SPB_CONTEXT *SpbContext,
    _In_ UCHAR Address,
    _In_reads_bytes_(Length) PVOID Data,
    _In_ ULONG Length
    );

VOID
SpbTargetDeinitialize(
    IN WDFDEVICE FxDevice,
    IN SPB_CONTEXT *SpbContext
    );

NTSTATUS
SpbTargetInitialize(
    IN WDFDEVICE FxDevice,
    IN SPB_CONTEXT *SpbContext
    );

NTSTATUS
SpbWriteDataSynchronously(
    IN SPB_CONTEXT *SpbContext,
    IN UCHAR Address,
    IN PVOID Data,
    IN ULONG Length
    );
