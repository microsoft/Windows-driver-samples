/*++

Copyright (C) Microsoft Corporation, All Rights Reserved.

Module Name:

    Device.cpp

Abstract:

    This module contains the implementation of the UMDF
    driver's device callback object.

Environment:

   Windows User-Mode Driver Framework (WUDF)

--*/

#include "Internal.h"
#include "Device.tmh"

NTSTATUS
CDevice::OnDeviceAdd(
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
{
    FunctionEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    WDFDEVICE Device;
    WDF_OBJECT_ATTRIBUTES DeviceAttributes;
    WDF_OBJECT_ATTRIBUTES FileAttributes;
    WDF_FILEOBJECT_CONFIG FileConfig;
    WDF_PNPPOWER_EVENT_CALLBACKS PowerCallbacks;
    CDevice* pDevice = nullptr;

    UNREFERENCED_PARAMETER(Driver);

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PowerCallbacks);

    PowerCallbacks.EvtDeviceD0Entry = CDevice::OnD0Entry;
    PowerCallbacks.EvtDeviceD0Exit = CDevice::OnD0Exit;

    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &PowerCallbacks);

    WDF_FILEOBJECT_CONFIG_INIT(
                &FileConfig,
                CDevice::OnFileCreate,
                CDevice::OnFileClose,
                WDF_NO_EVENT_CALLBACK);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&FileAttributes, CFileObject);
    FileAttributes.EvtDestroyCallback = CFileObject::OnDestroy;

    WdfDeviceInitSetFileObjectConfig(DeviceInit, &FileConfig, &FileAttributes);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&DeviceAttributes, CDevice);

    DeviceAttributes.EvtCleanupCallback = CDevice::OnCleanup;
    DeviceAttributes.SynchronizationScope = WdfSynchronizationScopeNone;
    DeviceAttributes.ExecutionLevel = WdfExecutionLevelPassive;

    Status = WdfDeviceCreate(&DeviceInit, &DeviceAttributes, &Device);

    if (!NT_SUCCESS(Status)) {
        TraceInfo("WdfDeviceCreate failed with Status %!STATUS!", Status);
        goto Exit;
    }

    // Construct device object on the preallocated buffer using placement 'new'
    pDevice = new (GetDeviceObject(Device)) CDevice(Device);

    if (pDevice == nullptr) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    Status = pDevice->Initialize();

Exit:
    FunctionReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS CDevice::Initialize()
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    WDF_IO_QUEUE_CONFIG QueueConfig;
    WDF_OBJECT_ATTRIBUTES QueueAttributes;
    WDFQUEUE Queue = nullptr;

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&QueueConfig, WdfIoQueueDispatchParallel);

    QueueConfig.PowerManaged = WdfFalse;
    QueueConfig.EvtIoDeviceControl = CQueue::OnIoDeviceControl;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&QueueAttributes, CQueue);

    Status = WdfIoQueueCreate(
                    m_Device,
                    &QueueConfig,
                    &QueueAttributes,
                    &Queue);

    if (!NT_SUCCESS(Status)) {
        TraceInfo("WdfIoQueueCreate failed with Status %!STATUS!", Status);
        goto Exit;
    }

    // Construct a queue object on the preallocated buffer using placement new operation
    m_pQueue = new (GetQueueObject(Queue)) CQueue(Queue);

    if (m_pQueue == nullptr) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    Status = m_pQueue->Initialize();

Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

VOID CDevice::OnCleanup(_In_ WDFOBJECT Object)
{
    FunctionEntry("...");

    CDevice *pDevice = GetDeviceObject(Object);

    NT_ASSERT(pDevice != nullptr);

    if (pDevice->m_Device == Object) {
        // Device object constructed using placement 'new' so explicitly invoke destructor
        pDevice->Deinitialize();
        pDevice->~CDevice();
    }

    FunctionReturnVoid();
}

NTSTATUS CDevice::Deinitialize()
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;

    if (m_pQueue != nullptr) {
        // Queue object constructed using placement new so explicitly invoke destructor
        m_pQueue->Deinitialize();
        m_pQueue->~CQueue();

        m_pQueue = nullptr;
    }

    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CDevice::OnD0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
    )
{
    FunctionEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    CDevice *pDevice = GetDeviceObject(Device);

    NT_ASSERT(pDevice != nullptr);
    Status = pDevice->OnD0Entry(PreviousState);

    FunctionReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CDevice::OnD0Entry(
    _In_ WDF_POWER_DEVICE_STATE PreviousState
    )
{
    MethodEntry("PreviousState = %d", PreviousState);
    NTSTATUS Status = STATUS_SUCCESS;
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CDevice::OnD0Exit(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState
    )
{
    FunctionEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    CDevice *pDevice = GetDeviceObject(Device);

    NT_ASSERT(pDevice != nullptr);
    Status = pDevice->OnD0Exit(TargetState);

    FunctionReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CDevice::OnD0Exit(
    _In_ WDF_POWER_DEVICE_STATE TargetState
    )
{
    MethodEntry("TargetState = %d", TargetState);
    NTSTATUS Status = STATUS_SUCCESS;
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

VOID
CDevice::OnFileCreate(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_ WDFFILEOBJECT FileObject
    )
{
    FunctionEntry("...");

    CDevice *pDevice = GetDeviceObject(Device);

    NT_ASSERT(pDevice != nullptr);
    pDevice->GetQueue()->OnFileCreate(Device, Request, FileObject);

    FunctionReturnVoid();
}

VOID
CDevice::OnFileClose(
    _In_ WDFFILEOBJECT FileObject
    )
{
    FunctionEntry("...");

    CDevice *pDevice = GetDeviceObject(WdfFileObjectGetDevice(FileObject));

    NT_ASSERT(pDevice != nullptr);
    pDevice->GetQueue()->OnFileClose(FileObject);

    FunctionReturnVoid();
}
