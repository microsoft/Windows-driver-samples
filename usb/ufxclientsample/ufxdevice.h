/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ufxdevice.h

Abstract:

    Defines the structures needed for UFXDEVICE object

Environment:

    Kernel mode

--*/
#pragma once

#define MAX_PHYSICAL_ENDPOINTS 32

//
// Context space for Sample UFXDEVICE object
//
typedef struct _UFXDEVICE_CONTEXT {
    WDFDEVICE FdoWdfDevice;

    //
    // This is an out-of-order collection of endpoints, since endpoints can be
    // re-ordered as UFX brings interfaces up and down. However, endpoint 0
    // is always at index 0.
    //
    WDFCOLLECTION Endpoints;
    UFXENDPOINT PhysicalEndpointToUfxEndpoint[MAX_PHYSICAL_ENDPOINTS];
    USBFN_DEVICE_STATE UsbState;
    USBFN_PORT_TYPE UsbPort;
    BOOLEAN IsIdle;
} UFXDEVICE_CONTEXT, *PUFXDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(UFXDEVICE_CONTEXT, UfxDeviceGetContext);
    
_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
UfxDevice_DeviceCreate(
    _In_ WDFDEVICE WdfDevice
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
UfxDevice_Reset (
    _In_ UFXDEVICE Device
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
UfxDeviceSetRunStop (
    _In_ UFXDEVICE UfxDevice,
    _In_ BOOLEAN Set
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
UfxDevice_StartConfiguration (
    _In_ UFXDEVICE Device,
    _In_ BOOLEAN SoftReset
    );
