/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    acpinotify.c

Abstract:

    This module implements PEP device notifications.


Environment:

    Kernel mode

--*/

//
//-------------------------------------------------------------------- Includes
//

#include "pch.h"

#if defined(EVENT_TRACING)
#include "acpinotify.tmh"
#endif

//
//--------------------------------------------------------------------- Pragmas
//

#pragma alloc_text(PAGE, PepAcpiAbandonDevice)

//
//------------------------------------------------------------------- Functions
//

VOID
PepAcpiPrepareDevice (
    _Inout_updates_bytes_(sizeof(PEP_ACPI_PREPARE_DEVICE)) PVOID Data
    )

/*++

Routine Description:

    This routine is called by default to prepare a device to be created.

Arguments:

    Data - Supplies a pointer to a PEP_ACPI_PREPARE_DEVICE structure.

Return Value:

    None.

--*/

{

    PPEP_ACPI_PREPARE_DEVICE AcpiPrepareDevice;
    PPEP_DEVICE_DEFINITION DeviceDefinition;

    DeviceDefinition = NULL;
    AcpiPrepareDevice = (PPEP_ACPI_PREPARE_DEVICE)Data;
    AcpiPrepareDevice->OutputFlags = PEP_ACPI_PREPARE_DEVICE_OUTPUT_FLAG_NONE;
    AcpiPrepareDevice->DeviceAccepted =
        PepIsDeviceAccepted(PEP_NOTIFICATION_CLASS_ACPI,
                            AcpiPrepareDevice->AcpiDeviceName,
                            &DeviceDefinition);

    TraceEvents(INFO,
                DBG_PEP,
                "%s: %s: Device = %S, Accepted = %d.\n",
                __FUNCTION__,
                PepAcpiNotificationHandlers[
                    PEP_NOTIFY_ACPI_PREPARE_DEVICE].Name,
                AcpiPrepareDevice->AcpiDeviceName->Buffer,
                (ULONG)AcpiPrepareDevice->DeviceAccepted);

    return;
}

VOID
PepAcpiAbandonDevice (
    _Inout_updates_bytes_(sizeof(PEP_ACPI_ABANDON_DEVICE)) PVOID Data
    )

/*++

Routine Description:

    This routine is called to abandon a device once it is being removed.

Arguments:

    Data - Supplies a pointer to a PEP_ACPI_ABANDON_DEVICE structure.

Return Value:

    None.

--*/

{

    PPEP_ACPI_ABANDON_DEVICE AcpiAbandonDevice;
    PPEP_DEVICE_DEFINITION DeviceDefinition;

    PAGED_CODE();

    AcpiAbandonDevice = (PPEP_ACPI_ABANDON_DEVICE)Data;
    AcpiAbandonDevice->DeviceAccepted =
        PepIsDeviceAccepted(PEP_NOTIFICATION_CLASS_ACPI,
                            AcpiAbandonDevice->AcpiDeviceName,
                            &DeviceDefinition);

    TraceEvents(INFO,
                DBG_PEP,
                "%s: %s: Device = %S, Accepted = %d.\n",
                __FUNCTION__,
                PepAcpiNotificationHandlers[
                    PEP_NOTIFY_ACPI_ABANDON_DEVICE].Name,
                AcpiAbandonDevice->AcpiDeviceName->Buffer,
                AcpiAbandonDevice->DeviceAccepted);

    return;
}

VOID
PepAcpiRegisterDevice (
    _Inout_updates_bytes_(sizeof(PEP_ACPI_REGISTER_DEVICE)) PVOID Data
    )

/*++

Routine Description:

    This routine is called to claim responsibility for a device. As part of
    registration, it will invoke the device-specific registered routine if
    one is supplied.

Arguments:

    Data - Supplies a pointer to a PEP_ACPI_REGISTER_DEVICE structure.

Return Value:

    None.

--*/

{

    BOOLEAN DeviceAccepted;
    PPEP_DEVICE_DEFINITION DeviceDefinition;
    ULONG InstancePathOffset;
    KIRQL Irql;
    PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice;
    PPEP_ACPI_REGISTER_DEVICE RegisterDevice;
    ULONG SizeNeeded;
    NTSTATUS Status;

    PepInternalDevice = NULL;
    RegisterDevice = (PPEP_ACPI_REGISTER_DEVICE)Data;
    DeviceAccepted = PepIsDeviceAccepted(PEP_NOTIFICATION_CLASS_ACPI,
                                         RegisterDevice->AcpiDeviceName,
                                         &DeviceDefinition);

    if (DeviceAccepted == FALSE) {
        TraceEvents(ERROR,
                    DBG_PEP,
                    "%s: %s: Device registration routine "
                    "failed. Device = %S.\n",
                    __FUNCTION__,
                    PepAcpiNotificationHandlers[
                        PEP_NOTIFY_ACPI_REGISTER_DEVICE].Name,
                    RegisterDevice->AcpiDeviceName->Buffer);

        RegisterDevice->DeviceHandle = NULL;
        goto RegisterDeviceEnd;
    }

    InstancePathOffset = ALIGN_UP_BY(DeviceDefinition->ContextSize,
                                     sizeof(WCHAR));

    SizeNeeded = InstancePathOffset +
                 RegisterDevice->AcpiDeviceName->Length +
                 sizeof(WCHAR);

    PepInternalDevice = ExAllocatePoolWithTag(NonPagedPool,
                                              SizeNeeded,
                                              PEP_POOL_TAG);

    if (PepInternalDevice == NULL) {
        TraceEvents(ERROR,
                    DBG_PEP,
                    "%s: %s: Failed to allocate internal "
                    "device context.\n",
                    __FUNCTION__,
                    PepAcpiNotificationHandlers[
                        PEP_NOTIFY_ACPI_REGISTER_DEVICE].Name);

        RegisterDevice->DeviceHandle = NULL;
        goto RegisterDeviceEnd;
    }

    RtlZeroMemory(PepInternalDevice, SizeNeeded);
    KeInitializeSpinLock(&PepInternalDevice->Lock);
    PepInternalDevice->KernelHandle = RegisterDevice->KernelHandle;
    PepInternalDevice->DeviceType = DeviceDefinition->Type;
    PepInternalDevice->DeviceDefinition = DeviceDefinition;
    PepInternalDevice->InstancePath = OffsetToPtr(PepInternalDevice,
                                                  InstancePathOffset);

    RtlCopyMemory(PepInternalDevice->InstancePath,
                  RegisterDevice->AcpiDeviceName->Buffer,
                  RegisterDevice->AcpiDeviceName->Length);

    //
    // Invoke the device initialization routine if one is supplied.
    //

    if (DeviceDefinition->Initialize != NULL) {
        Status = DeviceDefinition->Initialize(PepInternalDevice);
        if(!NT_SUCCESS(Status)) {
            TraceEvents(ERROR,
                        DBG_PEP,
                        "%s: %s: Device initialization "
                        "routine failed. Status = %!STATUS!.\n",
                        __FUNCTION__,
                        PepAcpiNotificationHandlers[
                            PEP_NOTIFY_ACPI_REGISTER_DEVICE].Name,
                        Status);

            RegisterDevice->DeviceHandle = NULL;
            goto RegisterDeviceEnd;
        }
    }

    //
    // Invoke the device-specific registered routine if one is supplied.
    //

    PepInvokeNotificationHandler(PEP_NOTIFICATION_CLASS_ACPI,
                                 NULL,
                                 PepHandlerTypeSyncCritical,
                                 PEP_NOTIFY_ACPI_REGISTER_DEVICE,
                                 PepInternalDevice,
                                 Data,
                                 sizeof(PEP_ACPI_REGISTER_DEVICE),
                                 NULL);

    //
    // Store the device inside the internal list.
    //

    KeAcquireSpinLock(&PepGlobalSpinLock, &Irql);
    InsertTailList(&PepDeviceList, &PepInternalDevice->ListEntry);
    KeReleaseSpinLock(&PepGlobalSpinLock,Irql);

    //
    // Return the ACPI handle back to the PoFx.
    //

    RegisterDevice->DeviceHandle = (PEPHANDLE)PepInternalDevice;
    RegisterDevice->OutputFlags = PEP_ACPI_REGISTER_DEVICE_OUTPUT_FLAG_NONE;
    TraceEvents(INFO,
                DBG_PEP,
                "%s: %s: SUCCESS! Device = %S, "
                "PEPHANDLE = %p\n.",
                __FUNCTION__,
                PepAcpiNotificationHandlers[
                    PEP_NOTIFY_ACPI_REGISTER_DEVICE].Name,
                RegisterDevice->AcpiDeviceName->Buffer,
                PepInternalDevice);

    PepInternalDevice = NULL;

RegisterDeviceEnd:
    if (PepInternalDevice != NULL) {
        ExFreePoolWithTag(PepInternalDevice, PEP_POOL_TAG);
    }

    return;
}

VOID
PepAcpiUnregisterDevice (
    _Inout_updates_bytes_(sizeof(PEP_ACPI_UNREGISTER_DEVICE)) PVOID Data
    )

/*++

Routine Description:

    This routine is called to release responsibility for a device. As part of
    unregistration, it will invoke the device-specific registered routine if
    one is supplied.

Arguments:

    Data - Supplies a pointer to a PEP_ACPI_UNREGISTER_DEVICE structure.

Return Value:

    None.

--*/

{

    PWSTR DeviceName;
    KIRQL Irql;
    PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice;
    PPEP_ACPI_UNREGISTER_DEVICE UnregisterDevice;

    UnregisterDevice = (PPEP_ACPI_UNREGISTER_DEVICE)Data;
    PepInternalDevice =
        (PPEP_INTERNAL_DEVICE_HEADER)UnregisterDevice->DeviceHandle;

    KeAcquireSpinLock(&PepGlobalSpinLock,&Irql);
    RemoveEntryList(&PepInternalDevice->ListEntry);
    KeReleaseSpinLock(&PepGlobalSpinLock,Irql);

    DeviceName = PepGetDeviceName(PepInternalDevice->DeviceType);
    if (DeviceName != NULL) {
        TraceEvents(INFO,
                    DBG_PEP,
                    "%s: %s: Device = %S.\n",
                    __FUNCTION__,
                    PepAcpiNotificationHandlers[
                        PEP_NOTIFY_ACPI_UNREGISTER_DEVICE].Name,
                    DeviceName);
    }

    ExFreePoolWithTag(PepInternalDevice, PEP_POOL_TAG);
    return;
}

VOID
PepAcpiEnumerateDeviceNamespace (
    _Inout_updates_bytes_(sizeof(PEP_ACPI_ENUMERATE_DEVICE_NAMESPACE))
        PVOID Data
    )

/*++

Routine Description:

    This routine handles PEP_NOTIFY_ACPI_ENUMERATE_DEVICE_NAMESPACE
    notification. As part of enumeration, it will invoke the device-specific
    registered routine if one is supplied.

Arguments:

    Data - Supplies a pointer to a PEP_ACPI_ENUMERATE_DEVICE_NAMESPACE
        structure.

Return Value:

    None.

--*/

{

    ULONG Count;
    PPEP_DEVICE_DEFINITION DeviceDefinition;
    PPEP_ACPI_ENUMERATE_DEVICE_NAMESPACE EdnBuffer;
    ULONG Index;
    SIZE_T ObjectBufferSize;
    PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice;
    SIZE_T RequiredSize;

    EdnBuffer = (PPEP_ACPI_ENUMERATE_DEVICE_NAMESPACE)Data;
    PepInternalDevice = (PPEP_INTERNAL_DEVICE_HEADER)EdnBuffer->DeviceHandle;
    DeviceDefinition = PepInternalDevice->DeviceDefinition;

    //
    // Always return method count regardless of success or failure.
    //

    EdnBuffer->ObjectCount = DeviceDefinition->ObjectCount;

    //
    // Check if the output buffer size is sufficient or not.
    //

    ObjectBufferSize = EdnBuffer->ObjectBufferSize;
    Count = DeviceDefinition->ObjectCount;
    RequiredSize = Count * sizeof(PEP_ACPI_OBJECT_NAME_WITH_TYPE);
    if (ObjectBufferSize < RequiredSize) {
        EdnBuffer->Status = STATUS_BUFFER_TOO_SMALL;
        TraceEvents(ERROR,
                    DBG_PEP,
                    "%s: %s: "
                    "Insufficient buffer size. "
                    "Required = %d, Provided = %d.\n",
                    __FUNCTION__,
                    PepAcpiNotificationHandlers[
                        PEP_NOTIFY_ACPI_ENUMERATE_DEVICE_NAMESPACE].Name,
                    (ULONG)RequiredSize,
                    (ULONG)ObjectBufferSize);

        goto EnumerateDeviceNamespaceEnd;
    }

    for (Index = 0; Index < Count; Index += 1) {
        TraceEvents(INFO,
                    DBG_PEP,
                    "%s: %s: Enumerate method %d.\n",
                    __FUNCTION__,
                    PepAcpiNotificationHandlers[
                        PEP_NOTIFY_ACPI_ENUMERATE_DEVICE_NAMESPACE].Name,
                    (ULONG)DeviceDefinition->Objects[Index].ObjectName);

        EdnBuffer->Objects[Index].Name.NameAsUlong =
            DeviceDefinition->Objects[Index].ObjectName;

        EdnBuffer->Objects[Index].Type =
            DeviceDefinition->Objects[Index].ObjectType;
    }

    EdnBuffer->Status = STATUS_SUCCESS;

    //
    // Invoke the device-specific registered routine if one is supplied.
    //

    PepInvokeNotificationHandler(PEP_NOTIFICATION_CLASS_ACPI,
                                 NULL,
                                 PepHandlerTypeSyncCritical,
                                 PEP_NOTIFY_ACPI_ENUMERATE_DEVICE_NAMESPACE,
                                 PepInternalDevice,
                                 Data,
                                 sizeof(PEP_ACPI_ENUMERATE_DEVICE_NAMESPACE),
                                 NULL);

EnumerateDeviceNamespaceEnd:
    return;
}

VOID
PepAcpiQueryObjectInformation (
    _Inout_updates_bytes_(sizeof(PEP_ACPI_QUERY_OBJECT_INFORMATION)) PVOID Data
    )

/*++

Routine Description:

    This routine handles PEP_NOTIFY_ACPI_QUERY_OBJECT_INFORMATION
    notification. As part of query, it will invoke the device-specific
    registered routine if one is supplied.

Arguments:

    Data - Supplies a pointer to a PEP_NOTIFY_ACPI_QUERY_OBJECT_INFORMATION
        structure.

Return Value:

    None.

--*/

{

    ULONG Count;
    PPEP_DEVICE_DEFINITION DeviceDefinition;
    ULONG Index;
    PPEP_ACPI_QUERY_OBJECT_INFORMATION QoiBuffer;
    PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice;

    QoiBuffer = (PPEP_ACPI_QUERY_OBJECT_INFORMATION)Data;
    PepInternalDevice = (PPEP_INTERNAL_DEVICE_HEADER)QoiBuffer->DeviceHandle;
    DeviceDefinition = PepInternalDevice->DeviceDefinition;
    Count = DeviceDefinition->ObjectCount;
    for (Index = 0; Index < Count; Index += 1) {
        if (QoiBuffer->Name.NameAsUlong ==
                DeviceDefinition->Objects[Index].ObjectName) {

            QoiBuffer->MethodObject.InputArgumentCount =
                DeviceDefinition->Objects[Index].InputArgumentCount;

            QoiBuffer->MethodObject.OutputArgumentCount =
                DeviceDefinition->Objects[Index].OutputArgumentCount;
        }
    }

    //
    // Invoke the device-specific registered routine if one is supplied.
    //

    PepInvokeNotificationHandler(PEP_NOTIFICATION_CLASS_ACPI,
                                 NULL,
                                 PepHandlerTypeSyncCritical,
                                 PEP_NOTIFY_ACPI_QUERY_OBJECT_INFORMATION,
                                 PepInternalDevice,
                                 Data,
                                 sizeof(PEP_ACPI_QUERY_OBJECT_INFORMATION),
                                 NULL);

    return;
}

VOID
PepAcpiEvaluateControlMethod (
    _Inout_updates_bytes_(sizeof(PEP_ACPI_EVALUATE_CONTROL_METHOD)) PVOID Data
    )

/*++

Routine Description:

    This routine handles PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD
    notification. By default, this routine will fail the evaluation. If
    device-specific registered routine is applied, it will be called instead.

Arguments:

    Data - Supplies a pointer to a PEP_ACPI_EVALUATE_CONTROL_METHOD structure.

Return Value:

    None.

--*/

{

    PPEP_ACPI_EVALUATE_CONTROL_METHOD EcmBuffer;
    PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice;

    EcmBuffer = (PPEP_ACPI_EVALUATE_CONTROL_METHOD)Data;
    PepInternalDevice = (PPEP_INTERNAL_DEVICE_HEADER)EcmBuffer->DeviceHandle;

    //
    // By default, assume the method evaluation will fail.
    //

    EcmBuffer->MethodStatus = STATUS_NOT_IMPLEMENTED;

    //
    // Invoke the device-specific registered routine if one is supplied.
    //

    PepInvokeNotificationHandler(PEP_NOTIFICATION_CLASS_ACPI,
                                 NULL,
                                 PepHandlerTypeSyncCritical,
                                 PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD,
                                 PepInternalDevice,
                                 Data,
                                 sizeof(PEP_ACPI_EVALUATE_CONTROL_METHOD),
                                 &EcmBuffer->MethodStatus);

    return;
}

VOID
PepAcpiQueryDeviceControlResources (
    _Inout_updates_bytes_(sizeof(PEP_ACPI_QUERY_DEVICE_CONTROL_RESOURCES))
        PVOID Data
    )

/*++

Routine Description:

    This routine handles PEP_NOTIFY_ACPI_QUERY_DEVICE_CONTROL_RESOURCES
    notification. By default, this routine assumes no resource needed. If
    device-specific registered routine is applied, it will be called instead.
    If no handler is implemented, mark the request as success to indicate no
    resource is needed.

Arguments:

    Data - Supplies a pointer to a
        PEP_NOTIFY_ACPI_QUERY_DEVICE_CONTROL_RESOURCES structure.

Return Value:

    None.

--*/

{

    PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice;
    PPEP_ACPI_QUERY_DEVICE_CONTROL_RESOURCES ResourceBuffer;

    ResourceBuffer = (PPEP_ACPI_QUERY_DEVICE_CONTROL_RESOURCES)Data;
    PepInternalDevice =
        (PPEP_INTERNAL_DEVICE_HEADER)ResourceBuffer->DeviceHandle;

    //
    // By default, assume the device doesn't need any BIOS control resources.
    // If they are required, a device-specific handler will be installed to
    // fill in the appropriate values.
    //

    ResourceBuffer->Status = STATUS_NOT_IMPLEMENTED;

    //
    // Invoke the device-specific registered routine if one is supplied.
    //

    PepInvokeNotificationHandler(PEP_NOTIFICATION_CLASS_ACPI,
                                 NULL,
                                 PepHandlerTypeSyncCritical,
                                 PEP_NOTIFY_ACPI_QUERY_DEVICE_CONTROL_RESOURCES,
                                 PepInternalDevice,
                                 Data,
                                 sizeof(PEP_ACPI_QUERY_DEVICE_CONTROL_RESOURCES),
                                 &ResourceBuffer->Status);

    //
    // If no handler was implemented, then succeed the request to indicate
    // no resources are needed.
    //

    if (ResourceBuffer->Status == STATUS_NOT_IMPLEMENTED) {
        TraceEvents(INFO,
                    DBG_PEP,
                    "%s: %s: No resource required.\n",
                    __FUNCTION__,
                    PepAcpiNotificationHandlers[
                        PEP_NOTIFY_ACPI_QUERY_DEVICE_CONTROL_RESOURCES].Name);

        ResourceBuffer->BiosResourcesSize = 0;
        ResourceBuffer->Status = STATUS_SUCCESS;
    }

    return;
}

VOID
PepAcpiTranslatedDeviceControlResources (
    _Inout_updates_bytes_(sizeof(PEP_ACPI_TRANSLATED_DEVICE_CONTROL_RESOURCES))
        PVOID Data
    )

/*++

Routine Description:

    This routine handles PEP_NOTIFY_ACPI_TRANSLATED_DEVICE_CONTROL_RESOURCES
    notification. By default, this routine does nothing. If device-specific
    registered routine is applied, it will be called instead.

Arguments:

    Data - Supplies a pointer a
        PEP_NOTIFY_ACPI_TRANSLATED_DEVICE_CONTROL_RESOURCES structure.

Return Value:

    None.

--*/

{

    PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice;
    PPEP_ACPI_TRANSLATED_DEVICE_CONTROL_RESOURCES ResourceBuffer;

    ResourceBuffer = (PPEP_ACPI_TRANSLATED_DEVICE_CONTROL_RESOURCES)Data;
    PepInternalDevice =
        (PPEP_INTERNAL_DEVICE_HEADER)ResourceBuffer->DeviceHandle;

    //
    // Invoke the device-specific registered routine if one is supplied.
    //

    PepInvokeNotificationHandler(PEP_NOTIFICATION_CLASS_ACPI,
                                 NULL,
                                 PepHandlerTypeSyncCritical,
                                 PEP_NOTIFY_ACPI_TRANSLATED_DEVICE_CONTROL_RESOURCES,
                                 PepInternalDevice,
                                 Data,
                                 sizeof(PEP_ACPI_TRANSLATED_DEVICE_CONTROL_RESOURCES),
                                 NULL);

    return;
}

VOID
PepAcpiWorkNotification (
    _Inout_updates_bytes_(sizeof(PEP_WORK)) PVOID Data
    )

/*++

Routine Description:

    This routine is called to handle PEP_NOTIFY_ACPI_WORK notification. It calls
    the worker routine to drain the completed work queue.

Arguments:

    Data - Supplies a pointer to a PEP_WORK structure.

Return Value:

    None.

--*/

{

    PepProcessCompleteWorkRequests(Data);
    return;
}
