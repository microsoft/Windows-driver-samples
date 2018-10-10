/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Fdo.h

Abstract:

    The class defines the context attached to the FDO. It manages lifetime for
    the PPM transport (ACPI in this case) and PPM object that it creates with
    the class entension.

Environment:

    Kernel-mode.

--*/

#pragma once

#include "ProjectCommon.h"


namespace UcmUcsiAcpiClient
{

class Ppm;

class Fdo: public WdfCpp::ObjectContext<WDFDEVICE, Fdo>
{
public:

    static
    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    CreateAndInitialize (
        _Inout_ PWDFDEVICE_INIT DeviceInit
        );

    static
    GET_CONTEXT_FROM_OBJECT
    GetContextFromObject;

    // Since FDO owns the transport, we need a getter to get the object.
    Acpi*
    GetAcpiObject();

private:

    friend class WdfCpp::ObjectContext<WDFDEVICE, Fdo>;

    Fdo (
        _In_ WDFDEVICE WdfDevice
        );

    ~Fdo ();

    Fdo&
    operator= (
        _In_ const Fdo&
        ) = delete;

    Fdo (
        _In_ const Fdo&
        ) = delete;

    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    Initialize ();
    
    EVT_OBJECT_CONTEXT_CLEANUP
    EvtObjectContextCleanup;

    static
    EVT_WDF_DEVICE_PREPARE_HARDWARE
    PAGED_CODE_SEG
    EvtDevicePrepareHardwareThunk;

    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    EvtDevicePrepareHardware (
        _In_ WDFCMRESLIST ResourcesRaw,
        _In_ WDFCMRESLIST ResourcesTranslated
        );

    static
    EVT_WDF_DEVICE_RELEASE_HARDWARE
    PAGED_CODE_SEG
    EvtDeviceReleaseHardwareThunk;

    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    EvtDeviceReleaseHardware (
        _In_ WDFCMRESLIST ResourcesTranslated
        );

    static
    EVT_WDF_DEVICE_D0_ENTRY
    EvtDeviceD0EntryThunk;

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS
    EvtDeviceD0Entry (
        _In_ WDF_POWER_DEVICE_STATE PreviousState
        );

    static
    EVT_WDF_DEVICE_D0_EXIT
    EvtDeviceD0ExitThunk;

    _IRQL_requires_max_(PASSIVE_LEVEL)
    NTSTATUS
    EvtDeviceD0Exit (
        _In_ WDF_POWER_DEVICE_STATE TargetState
        );

    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    VOID
    DestroyPpmObject();

    // FDO own the transport because it needs to talk to PPM firmware even before
    // UCMUCSIPPM object is created with UCSI Cx.
    Acpi m_Acpi;

    Ppm* m_Ppm;
  
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(Fdo, GetFdoFromWdfDevice);

}
