/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    device.c

Abstract:

   This file contains the device entry points and callbacks.
    
Environment:

    Windows User-Mode Driver Framework

--*/

#include "precomp.h"
#include "Trace.h"
#include "Defaults.h"
#include "Device.h"
#include "FixSession.h"
#include "FixHandler.h"
#include "Queue.h"

#include "Device.tmh"


CDevice::CDevice(
    _In_ WDFDEVICE Device
) :
    _Device(Device),
    _Queue(nullptr)
{
}

CDevice::~CDevice()
{
    if (_Queue != nullptr)
    {
        _Queue = nullptr;
    }
}

CQueue*
CDevice::GetQueue()
{
    return _Queue;
}

NTSTATUS
CDevice::OnDeviceAdd(
    _In_ WDFDRIVER /*Driver*/,
    _Inout_ PWDFDEVICE_INIT DeviceInit
)
{
    NTSTATUS status = STATUS_SUCCESS;

    WDF_PNPPOWER_EVENT_CALLBACKS powerCallbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&powerCallbacks);

    powerCallbacks.EvtDeviceD0Entry = CDevice::OnD0Entry;
    powerCallbacks.EvtDeviceD0Exit = CDevice::OnD0Exit;

    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &powerCallbacks);

    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes,
                                            CDevice);
        
    deviceAttributes.EvtCleanupCallback = CDevice::OnCleanup;
    deviceAttributes.SynchronizationScope = WdfSynchronizationScopeNone;
    deviceAttributes.ExecutionLevel = WdfExecutionLevelPassive;

    WDFDEVICE device;
    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "WdfDeviceCreate failed with Status %!STATUS!", status);
        goto Exit;
    }

    // Construct device object on the preallocated buffer using placement 'new'
    CDevice* pDevice = new (GetDeviceObject(device)) CDevice(device);

    status = pDevice->Initialize();

Exit:
    return status;
}

NTSTATUS
CDevice::Initialize()
{
    NTSTATUS status = STATUS_SUCCESS;

    status = CQueue::AddQueueToDevice(_Device, &_Queue);

    // Register GNSS Device interface. Note that this sample is not publishing a symbolic link.
    status = WdfDeviceCreateDeviceInterface(_Device,
                                            (LPGUID)&GUID_DEVINTERFACE_GNSS,
                                            nullptr);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "WdfDeviceCreateDeviceInterface failed with %!STATUS!", status);
        goto Exit;
    }

Exit:
    return status;
}

void
CDevice::OnCleanup(
    _In_ WDFOBJECT Object
)
{
    CDevice *pDevice = GetDeviceObject(Object);

    // Device object constructed using placement 'new' so explicitly invoke destructor
    pDevice->~CDevice();
}

NTSTATUS
CDevice::OnD0Entry(
    _In_ WDFDEVICE /*Device*/,
    _In_ WDF_POWER_DEVICE_STATE /*PreviousState*/
)
{
    NTSTATUS status;

    //
    // FIX ME:
    // Add further functionality.
    // Refer to power consideration from MSDN document: https://docs.microsoft.com/en-us/windows-hardware/drivers/gnss/gnss-driver-requirements
    //

    status = STATUS_SUCCESS;

    return status;
}

NTSTATUS
CDevice::OnD0Exit(
    _In_ WDFDEVICE /*Device*/,
    _In_ WDF_POWER_DEVICE_STATE /*TargetState*/
)
{
    NTSTATUS status;
    
    //
    // FIX ME:
    // Add further functionality.
    // Refer to power consideration from MSDN document: https://docs.microsoft.com/en-us/windows-hardware/drivers/gnss/gnss-driver-requirements
    //

    status = STATUS_SUCCESS;

    return status;
}
