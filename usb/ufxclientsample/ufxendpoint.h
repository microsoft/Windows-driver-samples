/*++

Copyright (c) 1990-2013 Microsoft Corporation. All rights reserved.

Module Name:

    ufxendpoint.h

Abstract:

    Defines the structures and functions needed for UFXENDPOINT object

Environment:

    Kernel mode

--*/

#pragma once

#include <ufxclient.h>
#include "transfer.h"

#define CONTROL_ENDPOINT(Endpoint) \
    ((UfxEndpointGetContext(Endpoint)->Descriptor.bmAttributes & USB_ENDPOINT_TYPE_MASK) == \
        USB_ENDPOINT_TYPE_CONTROL)

#define DIRECTION_IN(Endpoint) \
    (CONTROL_ENDPOINT(Endpoint) || \
     USB_ENDPOINT_DIRECTION_IN(UfxEndpointGetContext(Endpoint)->Descriptor.bEndpointAddress))

#define DIRECTION_OUT(Endpoint) \
    (CONTROL_ENDPOINT(Endpoint) || \
     USB_ENDPOINT_DIRECTION_OUT(UfxEndpointGetContext(Endpoint)->Descriptor.bEndpointAddress))
  
typedef struct _UFXENDPOINT_CONTEXT {
    WDFDEVICE WdfDevice;
    UFXDEVICE UfxDevice;
    USB_ENDPOINT_DESCRIPTOR Descriptor;
    ULONG PhysicalEndpoint;
    WDFREQUEST StallRequest;
    WDFREQUEST ClearRequest;
} UFXENDPOINT_CONTEXT, *PUFXENDPOINT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(UFXENDPOINT_CONTEXT, UfxEndpointGetContext);

typedef struct _ENDPOINT_QUEUE_CONTEXT {
    UFXENDPOINT Endpoint;
} ENDPOINT_QUEUE_CONTEXT, *PENDPOINT_QUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(ENDPOINT_QUEUE_CONTEXT, EndpointQueueGetContext);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
UfxEndpointAdd (
    _In_ UFXDEVICE Device,
    _In_ PUSB_ENDPOINT_DESCRIPTOR Descriptor,
    _Inout_ PUFXENDPOINT_INIT EndpointInit
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
UfxEndpointConfigure (
    _In_ UFXENDPOINT Endpoint
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
UfxEndpointConfigureHardware (
    _In_ UFXENDPOINT Endpoint,
    _In_ BOOLEAN Modify
    );