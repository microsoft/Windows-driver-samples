/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Ppm.h

Abstract:

    This class contaisn the contexts and methods necessary to interact with UCMUCSIPPM object
    which is created with the class extension.

Environment:

    Kernel-mode.

--*/

#pragma once

#include "ProjectCommon.h"

namespace UcmUcsiAcpiClient
{

class Ppm :public WdfCpp::ObjectContext<UCMUCSIPPM, Ppm>
{
public:

    static
    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    CreateAndInitialize (
        _In_ Fdo* FdoObject,
        _Deref_out_ Ppm** PpmObject
        );

    static
    GET_CONTEXT_FROM_OBJECT
    GetContextFromObject;

    static
    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    EVT_ACPI_NOTIFICATION_CALLBACK
    PpmNotificationCallbackThunk;


    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    PrepareHardware ();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    VOID
    ReleaseHardware ();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    TurnOnPpmNotification();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    VOID
    TurnOffPpmNotification();

    _IRQL_requires_max_(DISPATCH_LEVEL)
    PAGED_CODE_SEG
    VOID
    PpmNotificationCallback(
        _In_ ULONG NotifyValue
        );

    static
    EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL
    EvtIoDeviceControlThunk;

    void
    EvtIoDeviceControl(
        _In_ WDFREQUEST Request,
        _In_ ULONG IoControlCode
        );

    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    VOID
    EvtSendData(
        _In_ WDFREQUEST Request
        );

    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    VOID
    EvtReceiveData(
        _In_ WDFREQUEST Request
        );

private:
    friend class WdfCpp::ObjectContext<UCMUCSIPPM, Ppm>;

    typedef struct _PPM_CONFIG
    {
        WDFMEMORY WdfObject;
        UCMUCSI_PPM_CONFIG UcsiPpmConfig;
    } PPM_CONFIG, *PPPM_CONFIG;

    Ppm (_In_ UCMUCSIPPM UcmUcsiPpm);

    ~Ppm() = default;

    Ppm(_In_ const Ppm&) = delete;
    Ppm& operator =(_In_ const Ppm&) = delete;

    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    Initialize (
        _In_ Fdo* FdoObject
        );

    // This method needs to be static because we need to query connectors
    // even before we have initialized PPM object with the Cx.
    static
    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    QueryConnectorsAndInitCollection(
    _In_ Fdo* FdoObject,
    _Out_ UCMUCSI_CONNECTOR_COLLECTION* ConnectorCollectionHandle
    );

    static
    PAGED_CODE_SEG
    NTSTATUS
    PreparePpmConfig(
        _In_ Fdo* FdoObject,
        _Out_ PUCMUCSI_PPM_CONFIG UcsiPpmConfig
        );

    FORCEINLINE
    WDFDEVICE
    GetOwningDevice()
    {
        UCMUCSI_ASSERT(m_Device);
        return m_Device->GetObjectHandle();
    }

    // Pointer to the owning WDFDEVICE.
    Fdo* m_Device = nullptr;

    // Queue that is registered with the Cx for receiving UCSI Commands.
    WDFQUEUE m_UcsiCommandRequestQueue = WDF_NO_HANDLE;

    // PPM Transport
    Acpi* m_Acpi;

    bool m_IsUsbDeviceControllerEnabled = false;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(Ppm, GetAcpiPpmFromUcmUcsiPpm);

}