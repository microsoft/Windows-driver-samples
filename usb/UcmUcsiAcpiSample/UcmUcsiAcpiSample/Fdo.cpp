/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Fdo.cpp

Abstract:

    FDO callbacks, functions, and types.

Environment:

    Kernel-mode.

--*/

#include "Pch.h"
#include "Fdo.tmh"

namespace UcmUcsiAcpiClient
{

PAGED_CODE_SEG
NTSTATUS
Fdo::CreateAndInitialize (
    PWDFDEVICE_INIT DeviceInit
    )
{
    TRACE_FUNC_ENTRY(TRACE_FLAG_FDO);
    PAGED_CODE();

    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attrib;
    WDFDEVICE device;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    Fdo* fdo;

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = Fdo::EvtDevicePrepareHardwareThunk;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = Fdo::EvtDeviceReleaseHardwareThunk;
    pnpPowerCallbacks.EvtDeviceD0Entry = Fdo::EvtDeviceD0EntryThunk;
    pnpPowerCallbacks.EvtDeviceD0Exit = Fdo::EvtDeviceD0ExitThunk;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    status = UcmUcsiDeviceInitInitialize(DeviceInit);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_FDO, "[PWDFDEVICE_INIT: 0x%p] UcmUcsiDeviceInitInitialize failed - "
            "%!STATUS!", DeviceInit, status);
        goto Exit;
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attrib, Fdo);
    Fdo::ObjectAttributesInit(&attrib);

    status = WdfDeviceCreate(&DeviceInit, &attrib, &device);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_FDO, "[PWDFDEVICE_INIT: 0x%p] WdfDeviceCreate failed - %!STATUS!",
            DeviceInit, status);
        goto Exit;
    }

    fdo = new (GetFdoFromWdfDevice(device)) Fdo(device);

    status = fdo->Initialize();
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_FDO);
    return status;
}

Fdo*
Fdo::GetContextFromObject (
    WDFDEVICE WdfDevice
    )
{
    return GetFdoFromWdfDevice(WdfDevice);
}

Fdo::Fdo (
    WDFDEVICE WdfDevice
    ):
    ObjectContext(WdfDevice)
{
    TRACE_FUNC_ENTRY(TRACE_FLAG_FDO);
    TRACE_INFO(TRACE_FLAG_FDO, "[WDFDEVICE: 0x%p] Fdo constructed", GetObjectHandle());
    TRACE_FUNC_EXIT(TRACE_FLAG_FDO);
}

Fdo::~Fdo ()
{
    TRACE_FUNC_ENTRY(TRACE_FLAG_FDO);
    TRACE_INFO(TRACE_FLAG_FDO, "[WDFDEVICE: 0x%p] Fdo destroy", GetObjectHandle());
    TRACE_FUNC_EXIT(TRACE_FLAG_FDO);
}

void
Fdo::EvtObjectContextCleanup ()
{
    TRACE_FUNC_ENTRY(TRACE_FLAG_FDO);
    TRACE_INFO(TRACE_FLAG_FDO, "[WDFDEVICE: 0x%p] Fdo cleanup", GetObjectHandle());
    TRACE_FUNC_EXIT(TRACE_FLAG_FDO);
}

PAGED_CODE_SEG
NTSTATUS
Fdo::Initialize ()
{
    TRACE_FUNC_ENTRY(TRACE_FLAG_FDO);
    PAGED_CODE();

    NTSTATUS status;
    WDF_DEVICE_STATE deviceState;
    WDFDEVICE device;
    UCMUCSI_DEVICE_CONFIG config;

    device = GetObjectHandle();

    UCMUCSI_DEVICE_CONFIG_INIT(&config);
    status = UcmUcsiDeviceInitialize(device, &config);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_FDO, "[WDFDEVICE: 0x%p] UcmUcsiDeviceInitialize failed - %!STATUS!",
            device, status);
        goto Exit;
    }

    WDF_DEVICE_STATE_INIT(&deviceState);
    deviceState.NotDisableable = WdfFalse;
    WdfDeviceSetDeviceState(device, &deviceState);

    status = m_Acpi.Initialize(GetObjectHandle());
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_FDO, "[WDFDEVICE: 0x%p] ACPI initialize failed - %!STATUS!",
            device, status);
        goto Exit;
    }

    TRACE_INFO(TRACE_FLAG_FDO, "[WDFDEVICE: 0x%p] Fdo initialized", device);
    status = STATUS_SUCCESS;

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_FDO);
    return status;
}

Acpi*
Fdo::GetAcpiObject()
{
    return &m_Acpi;
}

PAGED_CODE_SEG
NTSTATUS
Fdo::EvtDevicePrepareHardwareThunk (
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated
    )
{
    PAGED_CODE();

    return Fdo::GetContextFromObject(Device)->EvtDevicePrepareHardware(ResourcesRaw,
        ResourcesTranslated);
}

PAGED_CODE_SEG
NTSTATUS
Fdo::EvtDevicePrepareHardware (
    WDFCMRESLIST  ResourcesRaw,
    WDFCMRESLIST ResourcesTranslated
    )
{
    TRACE_FUNC_ENTRY(TRACE_FLAG_FDO);
    PAGED_CODE();

    NTSTATUS status;
    WDFDEVICE device;
    Acpi::CONFIG ucsiAcpiConfig;
    ULONG index;
    ULONG resourceCount;
    bool allResourcesFound;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR res;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR rawRes;

    device = GetObjectHandle();

    resourceCount = WdfCmResourceListGetCount(ResourcesTranslated);
    allResourcesFound = false;

    for (index = 0; !allResourcesFound && (index < resourceCount); ++index)
    {
        res = WdfCmResourceListGetDescriptor(ResourcesTranslated, index);
        if (res->Type != CmResourceTypeMemory)
        {
            continue;
        }

        rawRes = WdfCmResourceListGetDescriptor(ResourcesRaw, index);

        //
        // Verify if address is below 4GB and not straddling the 4GB boundary.
        //

        if ((rawRes->u.Memory.Start.HighPart != 0) ||
            ((rawRes->u.Memory.Start.LowPart + rawRes->u.Memory.Length) <
                rawRes->u.Memory.Start.LowPart))
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            TRACE_ERROR(TRACE_FLAG_FDO, "[Device: 0x%p] Memory resource address (%I64x) not below 4GB", 
                device, rawRes->u.Memory.Start.QuadPart);
            goto Exit;
        }

        ucsiAcpiConfig.DataBlockAddress = res->u.Memory.Start;
        ucsiAcpiConfig.DataBlockLength = res->u.Memory.Length;

        // prepare hardware for ACPI transport using the resources.
        status = m_Acpi.PrepareHardware(&ucsiAcpiConfig);
        if (!NT_SUCCESS(status))
        {
            TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] ACPI Prepare hardware failed", device);
            goto Exit;
        }

        // This is a good time to create the PPM object with the Cx.
        status = Ppm::CreateAndInitialize(this, &m_Ppm);
        if (!NT_SUCCESS(status))
        {
            TRACE_INFO(TRACE_FLAG_FDO, "[WDFDEVICE: 0x%p] PPM CreateAndInitialize Failed with %!STATUS!",
                device, status);
            goto Exit;
        }

        allResourcesFound = true;
    }

    if (allResourcesFound == false)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TRACE_ERROR(TRACE_FLAG_FDO, "[Device: 0x%p] Could not find required resources", device);
        goto Exit;
    }

    status = m_Ppm->PrepareHardware();
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] PPM prepare hardware failed with %!STATUS!",
            device, status);
        goto Exit;
    }

    status = STATUS_SUCCESS;
    TRACE_INFO(TRACE_FLAG_FDO, "[WDFDEVICE: 0x%p] Fdo Prepare Hardware completed", device);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_FDO);
    return status;
}


PAGED_CODE_SEG
NTSTATUS
Fdo::EvtDeviceReleaseHardwareThunk (
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesTranslated
    )
{
    PAGED_CODE();

    return Fdo::GetContextFromObject(Device)->EvtDeviceReleaseHardware(ResourcesTranslated);
}


PAGED_CODE_SEG
NTSTATUS
Fdo::EvtDeviceReleaseHardware (
    WDFCMRESLIST /* ResourcesTranslated */
    )
{
    TRACE_FUNC_ENTRY(TRACE_FLAG_FDO);
    PAGED_CODE();

    WDFDEVICE device = GetObjectHandle();

    if (m_Ppm)
    {
        m_Ppm->ReleaseHardware();
        DestroyPpmObject();
    }

    m_Acpi.ReleaseHardware();

    TRACE_INFO(TRACE_FLAG_FDO, "[WDFDEVICE: 0x%p] Fdo Release Hardware completed", device);

    TRACE_FUNC_EXIT(TRACE_FLAG_FDO);
    return STATUS_SUCCESS;
}


NTSTATUS
Fdo::EvtDeviceD0EntryThunk (
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
    )
{
    Fdo* fdo = Fdo::GetContextFromObject(Device);
    return fdo->EvtDeviceD0Entry(PreviousState);
}


NTSTATUS
Fdo::EvtDeviceD0Entry (
    WDF_POWER_DEVICE_STATE PreviousState
    )
{
    TRACE_FUNC_ENTRY(TRACE_FLAG_FDO);

    WDFDEVICE device = GetObjectHandle();

    TRACE_INFO(TRACE_FLAG_FDO, "[WDFDEVICE: 0x%p] Entering D0 from %!WDF_POWER_DEVICE_STATE!",
        device, PreviousState);

    (VOID)m_Ppm->TurnOnPpmNotification();

    TRACE_FUNC_EXIT(TRACE_FLAG_FDO);
    return STATUS_SUCCESS;
}

NTSTATUS
Fdo::EvtDeviceD0ExitThunk (
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState
    )
{
    Fdo* fdo = Fdo::GetContextFromObject(Device);
    return fdo->EvtDeviceD0Exit(TargetState);
}

NTSTATUS
Fdo::EvtDeviceD0Exit (
    WDF_POWER_DEVICE_STATE TargetState
    )
{
    TRACE_FUNC_ENTRY(TRACE_FLAG_FDO);

    WDFDEVICE device = GetObjectHandle();

    TRACE_INFO(TRACE_FLAG_FDO, "[WDFDEVICE: 0x%p] Exiting D0 to %!WDF_POWER_DEVICE_STATE!",
        device, TargetState);

    m_Ppm->TurnOffPpmNotification();

    TRACE_FUNC_EXIT(TRACE_FLAG_FDO);

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
VOID
Fdo::DestroyPpmObject()
{
    TRACE_FUNC_ENTRY(TRACE_FLAG_FDO);
    PAGED_CODE();
    
    WDFDEVICE device = GetObjectHandle();

    TRACE_INFO(TRACE_FLAG_FDO, "[WDFDEVICE: 0x%p] Destroying UCMUCSIPPM object: 0x%p",
        device, m_Ppm->GetObjectHandle());

    WdfObjectDelete(m_Ppm->GetObjectHandle());
    m_Ppm = nullptr;

    TRACE_FUNC_EXIT(TRACE_FLAG_FDO);
}

}
