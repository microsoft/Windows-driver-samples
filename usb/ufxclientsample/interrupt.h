/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    interrupt.h

Abstract:

    Defines the structures and functions needed to manage wdf interrupt object.

Environment:

    Kernel mode

--*/
#pragma once

#include "registers.h"

//
// Context space for device WDFINTERRUPT object 
//
typedef struct _DEVICE_INTERRUPT_CONTEXT {

    WDFCOMMONBUFFER Buffer;

} DEVICE_INTERRUPT_CONTEXT, *PDEVICE_INTERRUPT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_INTERRUPT_CONTEXT, DeviceInterruptGetContext)

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
InterruptCreate (
    _In_ WDFDEVICE Device,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptResourceRaw,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptResourceTranslated
    );


_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
InterruptProgramEventBuffers (
    _In_ WDFINTERRUPT DeviceInterrupt
    );