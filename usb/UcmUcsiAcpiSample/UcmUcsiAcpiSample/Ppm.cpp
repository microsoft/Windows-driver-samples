/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Ppm.cpp

Abstract:

    Platform Policy Manager functions

Environment:

    Kernel-mode.

--*/

#include "Pch.h"
#ifndef __INTELLISENSE__
#include "Ppm.tmh"
#endif

namespace UcmUcsiAcpiClient
{

const ULONG UCSI_EXPECTED_NOTIFY_CODE = 0x80;

PAGED_CODE_SEG
NTSTATUS
Ppm::CreateAndInitialize(
    Fdo *Device,
    Ppm **PpmObject
    )
{
    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);
    PAGED_CODE();

    NTSTATUS status;
    UCMUCSI_PPM_CONFIG ucsiPpmConfig;
    WDFDEVICE wdfDevice = Device->GetObjectHandle();
    UCMUCSIPPM ppmObject = WDF_NO_HANDLE;
    WDF_OBJECT_ATTRIBUTES attrib;
    Ppm* ppm;

    status = PreparePpmConfig(Device, &ucsiPpmConfig);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] prepare PPM config failed with %!STATUS!",
            Device->GetObjectHandle(), status);
        goto Exit;
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attrib, Ppm);
    Ppm::ObjectAttributesInit(&attrib);
    status = UcmUcsiPpmCreate(wdfDevice, &ucsiPpmConfig, &attrib, &ppmObject);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[WDFDEVICE: 0x%p] UcmUCsiPpmCreate failed %!STATUS!", 
            wdfDevice, status);
        goto Exit;
    }

    ppm = new (GetAcpiPpmFromUcmUcsiPpm(ppmObject)) Ppm(ppmObject);
    
    status = ppm->Initialize(Device);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    *PpmObject = ppm;
    status = STATUS_SUCCESS;
 
Exit:
    return status;
}

Ppm::Ppm(
    UCMUCSIPPM PpmObject
    ) :ObjectContext(PpmObject)
{
}

Ppm*
Ppm::GetContextFromObject (
    UCMUCSIPPM PpmObject
    )
{
    return GetAcpiPpmFromUcmUcsiPpm(PpmObject);
}

PAGED_CODE_SEG
void
Ppm::PpmNotificationCallbackThunk(
    Ppm* PpmObject,
    ULONG NotifyValue
)
{
    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);
    PAGED_CODE();

    PpmObject->PpmNotificationCallback(NotifyValue);

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}


PAGED_CODE_SEG
NTSTATUS
Ppm::Initialize(
    Fdo* FdoObject
    )
{
    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);
    PAGED_CODE();

    NTSTATUS status;
    WDFDEVICE device = FdoObject->GetObjectHandle();
    WDF_OBJECT_ATTRIBUTES attrib;
    WDF_IO_QUEUE_CONFIG queueConfig;

    m_Device = FdoObject;
    m_Acpi = FdoObject->GetAcpiObject();

    WDF_OBJECT_ATTRIBUTES_INIT(&attrib);
    attrib.ParentObject = GetObjectHandle();

    // @@BEGIN_NOTPUBLICCODE
    // Making the queue parallel dispatch should not make a difference since the Cx guarantees that
    // it will not send another request until the previous one has been completed.
    // @@END_NOTPUBLICCODE

    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchParallel);

    // Has to be power managed as per Cx requirements.
    queueConfig.PowerManaged = WdfTrue;
    queueConfig.EvtIoDeviceControl = Ppm::EvtIoDeviceControlThunk;
    status = WdfIoQueueCreate(device, &queueConfig, &attrib, &m_UcsiCommandRequestQueue);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[WDFDEVICE: 0x%p] WdfIoQueueCreate "
            "for UCSI Command request queue failed - %!STATUS!", m_Device->GetObjectHandle(), status);
        goto Exit;
    }

    UcmUcsiPpmSetUcsiCommandRequestQueue(GetObjectHandle(), m_UcsiCommandRequestQueue);

    status = STATUS_SUCCESS;

Exit:
    return status;
}

PAGED_CODE_SEG
NTSTATUS
Ppm::PreparePpmConfig(
    Fdo* FdoObject,
    PUCMUCSI_PPM_CONFIG UcsiPpmConfig
    )
{
    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);
    PAGED_CODE();

    NTSTATUS status;
    bool IsUsbDeviceControllerEnabled;
    Acpi* acpiObject;
    UCMUCSI_CONNECTOR_COLLECTION connectorInfoCollection;

    acpiObject = FdoObject->GetAcpiObject();
    // @@BEGIN_NOTPUBLICCODE
    // TODO: This should probably be done by the Cx, because this is a standard mechanism
    //       that any type of UCM would use.
    // @@END_NOTPUBLICCODE

    status = QueryConnectorsAndInitCollection(FdoObject, &connectorInfoCollection);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    UCMUCSI_PPM_CONFIG_INIT(UcsiPpmConfig, connectorInfoCollection);

    status = acpiObject->CheckIfUsbDeviceControllerIsEnabled(IsUsbDeviceControllerEnabled);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }
    UcsiPpmConfig->UsbDeviceControllerEnabled = IsUsbDeviceControllerEnabled;

    if (IsUsbDeviceControllerEnabled)
    {
        TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] USB Device Controller is enabled", FdoObject->GetObjectHandle());
    }
    else
    {
        TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] USB Device Controller is disabled", FdoObject->GetObjectHandle());
    }

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
    return status;
}

PAGED_CODE_SEG
NTSTATUS
Ppm::PrepareHardware()
{
    NTSTATUS status;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    status = UcmUcsiPpmStart(GetObjectHandle());
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] UcmUcsiPpmStart returned failure %!STATUS!", m_Device->GetObjectHandle(), status);
    }

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
    return status;
}

PAGED_CODE_SEG
VOID
Ppm::ReleaseHardware()
{
    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    UcmUcsiPpmStop(GetObjectHandle());

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}

PAGED_CODE_SEG
NTSTATUS
Ppm::QueryConnectorsAndInitCollection (
    _In_ Fdo* FdoObject,
    _Out_ UCMUCSI_CONNECTOR_COLLECTION* ConnectorCollectionHandle
    )
{
    NTSTATUS status;
    WDFDEVICE device;
    WDFMEMORY enumChildrenMem;
    PACPI_ENUM_CHILDREN_OUTPUT_BUFFER enumChildrenBuf;
    PACPI_ENUM_CHILD enumChild;
    ACPI_PLD_BUFFER pldBuffer;
    ULONG64 connectorId;
    ULONG i;
    ULONG numConnectors;
    Acpi* acpiObject;
    UCMUCSI_CONNECTOR_INFO connectorInfo;

    PAGED_CODE();
    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    numConnectors = 0;
    enumChildrenMem = WDF_NO_HANDLE;
    device = FdoObject->GetObjectHandle();
    acpiObject = FdoObject->GetAcpiObject();

    // Cx would parent the collection object to the device object anyway. So no need
    // to parent the collection object here.
    status = UcmUcsiConnectorCollectionCreate(FdoObject->GetObjectHandle(),
                WDF_NO_OBJECT_ATTRIBUTES,
                ConnectorCollectionHandle);
    if (!NT_SUCCESS(status))
    {
        TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] UcmUcsiConnectorCollectionCreate failed with status %!STATUS!",
            FdoObject->GetAcpiObject(), status);
        goto Exit;
    }

    status = acpiObject->EnumChildren(&enumChildrenMem);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    enumChildrenBuf = (PACPI_ENUM_CHILDREN_OUTPUT_BUFFER) WdfMemoryGetBuffer (enumChildrenMem, nullptr);

    // If there is only one child, which is this device itself, then there is no connector
    // information available. Assume there is only one connector.
    if (enumChildrenBuf->NumberOfChildren == 1)
    {
        TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Connector information not found. Assuming single-connector system", device);

        UCMUCSI_CONNECTOR_INFO_INIT(&connectorInfo);
        connectorInfo.ConnectorId = 0;

        status = UcmUcsiConnectorCollectionAddConnector(*ConnectorCollectionHandle, &connectorInfo);
        if (!NT_SUCCESS(status))
        {
            TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] UcmUcsiConnectorCollectionAddConnector Failed with status %!STATUS!",
                FdoObject->GetObjectHandle(), status);
            goto Exit;
        }
    }
    else
    {
        enumChild = enumChildrenBuf->Children;
        for (i = 1; i < enumChildrenBuf->NumberOfChildren; ++i)
        {
            enumChild = ACPI_ENUM_CHILD_NEXT(enumChild);

            //
            // Connectors are child devices that have a _PLD method. Skip any children
            // that don't have any children, and attempt to evaluate _PLD on the ones that do.
            //

            if ((enumChild->Flags & ACPI_OBJECT_HAS_CHILDREN) == 0)
            {
                continue;
            }

            status = acpiObject->EvaluatePld(enumChild->Name, &pldBuffer);
            if (!NT_SUCCESS(status))
            {
                continue;
            }

            connectorId = UCM_CONNECTOR_ID_FROM_ACPI_PLD(&pldBuffer);

            UCMUCSI_CONNECTOR_INFO_INIT(&connectorInfo);
            connectorInfo.ConnectorId = connectorId;

            status = UcmUcsiConnectorCollectionAddConnector(*ConnectorCollectionHandle, &connectorInfo);
            if (!NT_SUCCESS(status))
            {
                TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] UcmUcsiConnectorCollectionAddConnector Failed with status %!STATUS!",
                    FdoObject->GetObjectHandle(), status);
                goto Exit;
            }

            TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Found connector %s with ID 0x%I64x", device, enumChild->Name, connectorId);

            numConnectors++;
        }
    }

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] Found %lu connectors", device, numConnectors);

Exit:

    if (enumChildrenMem != WDF_NO_HANDLE)
    {
        WdfObjectDelete(enumChildrenMem);
    }

    if (!NT_SUCCESS(status) && *ConnectorCollectionHandle != WDF_NO_HANDLE)
    {
        WdfObjectDelete(*ConnectorCollectionHandle);
        *ConnectorCollectionHandle = WDF_NO_HANDLE;
    }

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
    return status;
}

PAGED_CODE_SEG
NTSTATUS
Ppm::TurnOnPpmNotification()
{
    NTSTATUS status;
    ACPI_NOTIFICATION_CALLBACK_CONTEXT callbackConext;

    PAGED_CODE();
    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    callbackConext.m_PpmObject = this;
    callbackConext.m_CallbackFunction = PpmNotificationCallbackThunk;
    status = m_Acpi->RegisterNotificationCallback(&callbackConext);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] Registering for ACPI notifications failed - %!STATUS!",
           GetOwningDevice(), status);
    }

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
    return status;
}


PAGED_CODE_SEG
VOID
Ppm::TurnOffPpmNotification()
{
    PAGED_CODE();
    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    m_Acpi->UnRegisterNotificationCallback();

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}

PAGED_CODE_SEG
VOID
Ppm::PpmNotificationCallback(
    ULONG NotifyValue
    )
{
    PAGED_CODE();
    if (NotifyValue != UCSI_EXPECTED_NOTIFY_CODE)
    {
        TRACE_WARN(TRACE_FLAG_PPM, "[Device: 0x%p] Unexpected notify code %lu", 
            GetOwningDevice(), NotifyValue);
        goto Exit;
    }

    UcmUcsiPpmNotification(GetObjectHandle(), m_Acpi->GetUcsiDataBlock());

Exit:
    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}

VOID
Ppm::EvtIoDeviceControlThunk (
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t /* OutputBufferLength */,
    size_t /* InputBufferLength */,
    ULONG IoControlCode
    )
{
    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);
    NTSTATUS status;

    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    UCMUCSIPPM* ucmUcsiPpmPtr;
    Ppm* ppm;

    status = WdfRequestRetrieveInputBuffer(Request, sizeof(*ucmUcsiPpmPtr),
        reinterpret_cast<PVOID*>(&ucmUcsiPpmPtr), nullptr);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[WDFDEVICE: 0x%p] IOCTL with code "
            "%!UCMUCSI_PPM_IOCTL! did not have input buffer of minimum expected size - "
            "%!STATUS!", device, IoControlCode, status);
        goto Exit;
    }

    ppm = Ppm::GetContextFromObject(*ucmUcsiPpmPtr);

    ppm->EvtIoDeviceControl(Request, IoControlCode);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}

void
Ppm::EvtIoDeviceControl(
    _In_ WDFREQUEST Request,
    _In_ ULONG IoControlCode
    )
{
    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);

    NTSTATUS status;

    TRACE_INFO(TRACE_FLAG_PPM, "[WDFDEVICE: 0x%p] Received "
        "%!UCMUCSI_PPM_IOCTL!", m_Device->GetObjectHandle(), IoControlCode);

    switch (IoControlCode)
    {
    case IOCTL_UCMUCSI_PPM_SEND_UCSI_DATA_BLOCK:
        EvtSendData(Request);
        break;

    case IOCTL_UCMUCSI_PPM_GET_UCSI_DATA_BLOCK:
        EvtReceiveData(Request);
        break;

    default:
        TRACE_INFO(TRACE_FLAG_PPM, "[WDFDEVICE: 0x%p] Unrecognized IOCTL "
            "%!UCMUCSI_PPM_IOCTL!", m_Device->GetObjectHandle(), IoControlCode);
        status = STATUS_NOT_SUPPORTED;
        goto Exit;
    }

    status = STATUS_SUCCESS;

Exit:

    if (!NT_SUCCESS(status))
    {
        WdfRequestComplete(Request, status);
    }

    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}

PAGED_CODE_SEG
VOID
Ppm::EvtSendData(
    WDFREQUEST Request
    )
{
    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);
    PAGED_CODE();

    NTSTATUS status;
    PUCMUCSI_PPM_SEND_UCSI_DATA_BLOCK_IN_PARAMS inParams;

    status = WdfRequestRetrieveInputBuffer(Request, sizeof(*inParams),
        reinterpret_cast<PVOID*>(&inParams), nullptr);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[WDFDEVICE: 0x%p] WdfRequestRetrieveInputBuffer failed - %!STATUS!",
            m_Device->GetObjectHandle(), status);
        goto Exit;
    }

    status = m_Acpi->SendData(&inParams->UcmUcsiDataBlock);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[WDFDEVICE: 0x%p] ACPI Send failed- %!STATUS!",
            m_Device->GetObjectHandle(), status);
        goto Exit;
    }

Exit:
    WdfRequestComplete(Request, status);
    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}

PAGED_CODE_SEG
VOID
Ppm::EvtReceiveData(
    WDFREQUEST Request
    )
{
    TRACE_FUNC_ENTRY(TRACE_FLAG_PPM);
    PAGED_CODE();

    NTSTATUS status;

    PUCMUCSI_PPM_GET_UCSI_DATA_BLOCK_IN_PARAMS inParams;
    PUCMUCSI_PPM_GET_UCSI_DATA_BLOCK_OUT_PARAMS outParams;

    status = WdfRequestRetrieveInputBuffer(Request, sizeof(*inParams),
        reinterpret_cast<PVOID*>(&inParams), nullptr);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[WDFDEVICE: 0x%p] WdfRequestRetrieveInputBuffer failed - %!STATUS!",
            m_Device->GetObjectHandle(), status);
        goto Exit;
    }

    status = WdfRequestRetrieveOutputBuffer(Request, sizeof(*outParams),
        reinterpret_cast<PVOID*>(&outParams), nullptr);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[WDFDEVICE: 0x%p] WdfRequestRetrieveOutputBuffer failed - %!STATUS!",
            m_Device->GetObjectHandle(), status);
        goto Exit;
    }
    
    status = m_Acpi->ReceiveData(&outParams->UcmUcsiDataBlock);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_PPM, "[WDFDEVICE: 0x%p] ACPI Send failed- %!STATUS!",
            m_Device->GetObjectHandle(), status);
        goto Exit;
    }
    WdfRequestSetInformation(Request, sizeof(*outParams));

Exit:
    WdfRequestComplete(Request, status);
    TRACE_FUNC_EXIT(TRACE_FLAG_PPM);
}

}