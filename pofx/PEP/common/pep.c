/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    pep.c

Abstract:

    This module implements PEP registration.


Environment:

    Kernel Mode

--*/

//
//-------------------------------------------------------------------- Includes
//

#include <initguid.h>
#include "pch.h"

#if defined(EVENT_TRACING)
#include "pep.tmh"
#endif

//
//------------------------------------------------------------------- Globals
//

PEP_KERNEL_INFORMATION PepKernelInformation;
LIST_ENTRY PepCompletedWorkList;
LIST_ENTRY PepDeviceList;
KSPIN_LOCK PepGlobalSpinLock;
LIST_ENTRY PepPendingWorkList;
BOOLEAN PepRegistered = FALSE;
WDFSPINLOCK PepWorkListLock;

//
// Define the default ACPI notification handlers.
//

PEP_GENERAL_NOTIFICATION_HANDLER PepAcpiNotificationHandlers[] = {
    {0, NULL, "UNKNOWN"},
    {PEP_NOTIFY_ACPI_PREPARE_DEVICE,
     PepAcpiPrepareDevice,
     "PEP_ACPI_PREPARE_DEVICE"},

    {PEP_NOTIFY_ACPI_ABANDON_DEVICE,
     PepAcpiAbandonDevice,
     "PEP_ACPI_ABANDON_DEVICE"},

    {PEP_NOTIFY_ACPI_REGISTER_DEVICE,
     PepAcpiRegisterDevice,
     "PEP_ACPI_REGISTER_DEVICE"},

    {PEP_NOTIFY_ACPI_UNREGISTER_DEVICE,
     PepAcpiUnregisterDevice,
     "PEP_ACPI_UNREGISTER_DEVICE"},

    {PEP_NOTIFY_ACPI_ENUMERATE_DEVICE_NAMESPACE,
     PepAcpiEnumerateDeviceNamespace,
     "PEP_ACPI_ENUMERATE_DEVICE_NAMESPACE"},

    {PEP_NOTIFY_ACPI_QUERY_OBJECT_INFORMATION,
     PepAcpiQueryObjectInformation,
     "PEP_ACPI_QUERY_OBJECT_INFORMATION"},

    {PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD,
     PepAcpiEvaluateControlMethod,
     "PEP_ACPI_EVALUATE_CONTROL_METHOD"},

    {PEP_NOTIFY_ACPI_QUERY_DEVICE_CONTROL_RESOURCES,
     PepAcpiQueryDeviceControlResources,
     "PEP_ACPI_QUERY_DEVICE_CONTROL_RESOURCES"},

    {PEP_NOTIFY_ACPI_TRANSLATED_DEVICE_CONTROL_RESOURCES,
     PepAcpiTranslatedDeviceControlResources,
     "PEP_ACPI_TRANSLATED_DEVICE_CONTROL_RESOURCES"},

    {PEP_NOTIFY_ACPI_WORK,
     PepAcpiWorkNotification,
     "PEP_ACPI_WORK"}
};

PEP_GENERAL_NOTIFICATION_HANDLER PepDpmNotificationHandlers[] = {
    {0, NULL, "UNKNOWN"},

};

//
//--------------------------------------------------------------------- Pragmas
//

#pragma alloc_text(INIT, PepRegister)
#pragma alloc_text(INIT, PepInitialize)

//
//------------------------------------------------------------------- Functions
//

NTSTATUS
PepRegister (
    _In_ BOOLEAN AcpiHandlerOptIn,
    _In_ BOOLEAN DpmHandlerOptIn,
    _In_ BOOLEAN PpmHandlerOptIn
    )

/*++

Routine Description:

    This routine registers as a power engine plugin with the OS.

Arguments:

    AcpiHandlerOptIn - Indicates whether the PEP handles ACPI notification.

    DpmHandlerOptIn  - Indicates whether the DPM handles ACPI notification.

    PpmHandlerOptIn  - Indicates whether the PPM handles ACPI notification.

Return Value:

    NTSTATUS.

--*/

{

    PEP_INFORMATION PepInformation;
    NTSTATUS Status;

    RtlZeroMemory(&PepInformation, sizeof(PepInformation));
    PepInformation.Version = PEP_INFORMATION_VERSION;
    PepInformation.Size = sizeof(PepInformation);
    if (AcpiHandlerOptIn != FALSE) {
        PepInformation.AcceptAcpiNotification = PepAcpiNotify;
    }

    if (DpmHandlerOptIn != FALSE) {
        PepInformation.AcceptDeviceNotification = PepDpmNotify;
    }

    if (PpmHandlerOptIn != FALSE) {
        PepInformation.AcceptProcessorNotification = PepPpmNotify;
    }

    RtlZeroMemory(&PepKernelInformation, sizeof(PepKernelInformation));
    PepKernelInformation.Version = PEP_KERNEL_INFORMATION_V3;
    PepKernelInformation.Size = sizeof(PepKernelInformation);
    Status = PoFxRegisterPlugin(&PepInformation, &PepKernelInformation);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(ERROR,
                    DBG_INIT,
                    "%s: PoFxRegisterPlugin() Failed! Status = %!STATUS!.\n",
                    __FUNCTION__,
                    Status);

        goto RegisterEnd;
    }

    NT_ASSERT(PepKernelInformation.Plugin != NULL);
    NT_ASSERT(PepKernelInformation.RequestWorker != NULL);

RegisterEnd:
    return Status;
}

NTSTATUS
PepInitialize (
    _In_ PDRIVER_OBJECT WdmDriverObject,
    _In_ WDFDRIVER WdfDriver,
    _In_ PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine performs initialization operations.

Arguments:

    WdmDriverObject - Supplies a pointer to the WDM driver object.

    WdfDriver - Supplies a handle to the WDF driver object.

    RegistryPath - Supplies a pointer to the driver specific registry key.

Return Value:

    NTSTATUS.

--*/

{

    BOOLEAN AcpiOptin;
    BOOLEAN DpmOptin;
    WDF_OBJECT_ATTRIBUTES ObjectAttributes;
    BOOLEAN PpmOptin;
    NTSTATUS Status;

    KeInitializeSpinLock(&PepGlobalSpinLock);
    InitializeListHead(&PepDeviceList);
    InitializeListHead(&PepPendingWorkList);
    InitializeListHead(&PepCompletedWorkList);

    //
    // Create the spin-lock that is used to synchronize state during device
    // powering down.
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&ObjectAttributes);

    Status = WdfSpinLockCreate(&ObjectAttributes, &PepWorkListLock);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(ERROR,
                    DBG_INIT,
                    "%s: WdfSpinLockCreate() Failed! Status = %!STATUS!.\n",
                    __FUNCTION__,
                    Status);

        goto InitializeEnd;
    }

    //
    // Query the registry for flags. This registry flag may not be present
    // and thus a failure is not treated as fatal.
    //

    AcpiOptin = FALSE;
    DpmOptin = FALSE;
    PpmOptin = FALSE;
    PepQueryRegisterOptions(WdmDriverObject,
                            WdfDriver,
                            RegistryPath,
                            &AcpiOptin,
                            &DpmOptin,
                            &PpmOptin);

    //
    // The default behavior is to handle Acpi.
    //

    if ((AcpiOptin == FALSE) &&
        (DpmOptin == FALSE) &&
        (PpmOptin ==  FALSE)) {

        AcpiOptin = TRUE;
    }

    Status = PepRegister(AcpiOptin, DpmOptin, PpmOptin);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(ERROR,
                    DBG_INIT,
                    "%s: PepRegister() Failed! Status = %!STATUS!.\n",
                    __FUNCTION__,
                    Status);

    } else {
        PepRegistered = TRUE;
    }

InitializeEnd:
    return Status;
}

__drv_requiresIRQL(PASSIVE_LEVEL)
VOID
PepQueryRegisterOptions (
    _In_ PDRIVER_OBJECT WdmDriverObject,
    _In_ WDFDRIVER WdfDriver,
    _In_ PUNICODE_STRING RegistryPath,
    _Out_ PBOOLEAN PepAcpiOptin,
    _Out_ PBOOLEAN PepDpmOptin,
    _Out_ PBOOLEAN PepPpmOptin
    )

/*++

Routine Description:

    This routine queries options/flags set in the registry and intializes
    respective variables.

Arguments:

    WdmDriverObject - Supplies a pointer to the WDM driver object.

    WdfDriver - Supplies a handle to the WDF driver object.

    RegistryPath - Supplies a pointer to the driver specific registry key.

    PepAcpiOptin -  Supplies a pointer that receives whether ACPI opt-in
        is enabled.

    PepDpmOptin - Supplies a pointer that receives whether DPM opt-in
        is enabled.

    PepPpmOptin - Supplies a pointer that receives whether PPM opt-in
        is enabled.

Return Value:

    None.

--*/

{

    DECLARE_CONST_UNICODE_STRING(Key, PEP_REGISTRY_OPTIN_KEY);
    ULONG Flag;
    NTSTATUS Status;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(WdmDriverObject);

    *PepAcpiOptin = FALSE;
    *PepDpmOptin = FALSE;
    *PepPpmOptin = FALSE;

    //
    // Check whether ACPI/DPM/PPM opt-in is enabled.
    //

    Flag = 0x0;
    Status = PepQueryRegistry(NULL, WdfDriver, RegistryPath, &Key, &Flag);
    if (NT_SUCCESS(Status)) {
        if (CHECK_FLAG(Flag, PEP_REGISTRY_OPTIN_KEY_ACPI_MASK)) {
            *PepAcpiOptin = TRUE;
        }

        if (CHECK_FLAG(Flag, PEP_REGISTRY_OPTIN_KEY_DPM_MASK)) {
            *PepDpmOptin = TRUE;
        }

        if (CHECK_FLAG(Flag, PEP_REGISTRY_OPTIN_KEY_PPM_MASK)) {
            *PepPpmOptin = TRUE;
        }
    }

    return;
}

BOOLEAN
PepDpmNotify (
    _In_ ULONG Notification,
    _In_ PVOID Data
    )

/*++

Routine Description:

    Handles all incoming DPM notifications from the OS.

Arguments:

    Notification - Supplies the notification type.

    Data - Supplies a pointer to a data structure specific to the
        notification type.

Return Value:

    TRUE if the notification type was recognized;
    FALSE otherwise.

--*/

{

    BOOLEAN Recognized;

    Recognized = FALSE;
    if (Notification >= ARRAYSIZE(PepDpmNotificationHandlers)) {
        goto DpmNotifyEnd;
    }

    if ((PepDpmNotificationHandlers[Notification].Notification == 0) ||
        (PepDpmNotificationHandlers[Notification].Handler == NULL)) {

        goto DpmNotifyEnd;
    }

    PepDpmNotificationHandlers[Notification].Handler(Data);
    Recognized = TRUE;

DpmNotifyEnd:
    return Recognized;
}

BOOLEAN
PepAcpiNotify (
    _In_ ULONG Notification,
    _In_ PVOID Data
    )

/*++

Routine Description:

    Handles all incoming ACPI notifications from the OS.

Arguments:

    Notification - Supplies the notification type.

    Data - Supplies a pointer to a data structure specific to the
        notification type.

Return Value:

    TRUE if the notification type was recognized;
    FALSE otherwise.

--*/

{

    BOOLEAN Recognized;

    Recognized = FALSE;
    if (Notification >= ARRAYSIZE(PepAcpiNotificationHandlers)) {
        goto AcpiNotifyEnd;
    }

    if ((PepAcpiNotificationHandlers[Notification].Notification == 0) ||
        (PepAcpiNotificationHandlers[Notification].Handler == NULL)) {

        goto AcpiNotifyEnd;
    }

    PepAcpiNotificationHandlers[Notification].Handler(Data);
    Recognized = TRUE;

AcpiNotifyEnd:
    return Recognized;
}

BOOLEAN
PepPpmNotify (
    _In_ PEPHANDLE Handle,
    _In_ ULONG Notification,
    _Inout_opt_ PVOID Data
    )

/*++

Routine Description:

    Handles all incoming PPM notifications from the OS. Will be implemented
    in the future.

Arguments:

    Notification - Supplies the notification type.

    Data - Supplies a pointer to a data structure specific to the
        notification type.

Return Value:

    Returns TRUE if the notification type was recognized;
    FALSE otherwise.

--*/

{

    UNREFERENCED_PARAMETER(Handle);
    UNREFERENCED_PARAMETER(Notification);
    UNREFERENCED_PARAMETER(Data);

    //
    // Not implemented.
    //

    NT_ASSERT(FALSE);

    return FALSE;
}
