/*++

Copyright (c) Microsoft Corporation

Module Name:

    plpolicy.c

Abstract:

    This module implements power limit related operations for the simulated power
    limit policy driver.

--*/

//-------------------------------------------------------------------- Includes

#include "plpolicy.h"

//------------------------------------------------------------------ Prototypes

NTSTATUS
GetDeviceObjectFromInterfaceName (
    _In_ PUNICODE_STRING Name,
    _Outptr_ PFILE_OBJECT *File,
    _Outptr_ PDEVICE_OBJECT *Device
    );

PDEVICE_REGISTRATION
FindRegistrationByName (
    _In_ PUNICODE_STRING TargetDeviceName,
    _In_ PFDO_DATA DevExt
    );

PDEVICE_REGISTRATION
FindRegistrationById (
    _In_ PFDO_DATA DevExt,
    _In_ ULONG RequestId
    );

//--------------------------------------------------------------------- Globals

WDFWAITLOCK GlobalMutex;

//--------------------------------------------------------------------- Pragmas

#pragma alloc_text(PAGE, RegisterRequest)
#pragma alloc_text(PAGE, UnregisterRequest)
#pragma alloc_text(PAGE, QueryAttributes)
#pragma alloc_text(PAGE, QueryLimitValues)
#pragma alloc_text(PAGE, SetLimitValues)
#pragma alloc_text(PAGE, GetDeviceObjectFromInterfaceName)
#pragma alloc_text(PAGE, FindRegistrationByName)
#pragma alloc_text(PAGE, FindRegistrationById)

//------------------------------------------------------------------- Functions

_Use_decl_annotations_
NTSTATUS
RegisterRequest (
    PFDO_DATA DevExt,
    PUNICODE_STRING TargetDeviceName,
    PDEVICE_OBJECT PolicyDeviceObject,
    PULONG RequestId
    )

/*++

Routine Description:

    This routine registers a power limit request for the specified device.

Arguments:

    DevExt - Supplies a pointer to the device extension.

    TargetDeviceName - Supplies the name of the target device.

    PolicyDeviceObject - Supplies a pointer to the policy driver's device object.

    RequestId - Supplies a pointer to the registered request.

Return Value:

    NTSTATUS.

--*/

{

    PFILE_OBJECT FileObject;
    COUNTED_REASON_CONTEXT ReasContext;
    PDEVICE_REGISTRATION Registration;
    ULONG SizeNeeded;
    NTSTATUS Status;
    PDEVICE_OBJECT TargetDeviceObject;

    PAGED_CODE();

    FileObject = NULL;
    TargetDeviceObject = NULL;
    Registration = NULL;
    AcquireGlobalMutex();
    if (FindRegistrationByName(TargetDeviceName, DevExt) != NULL) {
        Status = STATUS_SUCCESS;
        goto RegisterEnd;
    }

    Status = GetDeviceObjectFromInterfaceName(TargetDeviceName,
                                              &FileObject,
                                              &TargetDeviceObject);

    if (!NT_SUCCESS(Status)) {
        goto RegisterEnd;
    }

    SizeNeeded = sizeof(DEVICE_REGISTRATION) + TargetDeviceName->MaximumLength;
    Registration = ExAllocatePool2(POOL_FLAG_PAGED, SizeNeeded, PLPOLICY_TAG);
    if (Registration == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto RegisterEnd;
    }

    RtlZeroMemory(Registration, SizeNeeded);
    Registration->TargetDeviceName.Length = TargetDeviceName->Length;
    Registration->TargetDeviceName.MaximumLength = TargetDeviceName->MaximumLength;
    Registration->TargetDeviceName.Buffer = OffsetToPtr(Registration,
                                                        sizeof(DEVICE_REGISTRATION));

    RtlCopyMemory(Registration->TargetDeviceName.Buffer,
                  TargetDeviceName->Buffer,
                  TargetDeviceName->MaximumLength);

    ReasContext.Version = DIAGNOSTIC_REASON_VERSION;
    ReasContext.Flags = DIAGNOSTIC_REASON_SIMPLE_STRING;
    RtlInitUnicodeString(&ReasContext.SimpleString, L"Simulated Power Limit Policy Device");
    Status = PoCreatePowerLimitRequest(&Registration->PowerLimitRequest,
                                       TargetDeviceObject,
                                       PolicyDeviceObject,
                                       &ReasContext);

    if (!NT_SUCCESS(Status)) {
        goto RegisterEnd;
    }

    Registration->Initialized = TRUE;
    Registration->RequestId = DevExt->RequestCount;
    *RequestId = DevExt->RequestCount;
    InsertTailList(&DevExt->RequestHeader, &Registration->Link);
    DevExt->RequestCount += 1;
    Registration = NULL;
    Status = STATUS_SUCCESS;

RegisterEnd:
    ReleaseGlobalMutex();
    if (FileObject != NULL) {
        ObDereferenceObject(FileObject);
    }

    if (TargetDeviceObject != NULL) {
        ObDereferenceObject(TargetDeviceObject);
    }

    if (Registration != NULL) {
        ExFreePoolWithTag(Registration, PLPOLICY_TAG);
    }

    return Status;
}

_Use_decl_annotations_
NTSTATUS
UnregisterRequest (
    PFDO_DATA DevExt,
    ULONG RequestId
    )

/*++

Routine Description:

    This routine unregisters a power limit request for the specified device.

Arguments:

    DevExt - Supplies a pointer to the device extension.

    RequestId - Supplies the Id of the request.

Return Value:

    NTSTATUS.

--*/

{

    PDEVICE_REGISTRATION Registration;
    NTSTATUS Status;

    PAGED_CODE();

    AcquireGlobalMutex();
    Registration = FindRegistrationById(DevExt, RequestId);
    if (Registration == NULL) {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto UnregisterEnd;
    }

    RemoveEntryList(&Registration->Link);
    PoDeletePowerLimitRequest(Registration->PowerLimitRequest);
    ExFreePoolWithTag(Registration, PLPOLICY_TAG);
    Status = STATUS_SUCCESS;

UnregisterEnd:
    ReleaseGlobalMutex();
    return Status;
}

_Use_decl_annotations_
NTSTATUS
QueryAttributes (
    PPOWER_LIMIT_ATTRIBUTES Buffer,
    PFDO_DATA DevExt,
    ULONG RequestId,
    ULONG BufferCount
    )

/*++

Routine Description:

    This routine returns supported power limit parameter's attributes.

Arguments:

    Buffer - Supplies a pointer to save attributes.

    DevExt - Supplies a pointer to the device extension.

    RequestId - Supplies the Id of the request.

    BufferCount - Supplies the count of buffer.

Return Value:

    NTSTATUS.

--*/

{

    ULONG AttributeCount;
    PDEVICE_REGISTRATION Registration;
    NTSTATUS Status;

    PAGED_CODE();

    AcquireGlobalMutex();
    Registration = FindRegistrationById(DevExt, RequestId);
    if (Registration == NULL) {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto QueryAttributesEnd;
    }

    Status = PoQueryPowerLimitAttributes(Registration->PowerLimitRequest,
                                         BufferCount,
                                         Buffer,
                                         &AttributeCount);

QueryAttributesEnd:
    ReleaseGlobalMutex();
    return Status;
}

_Use_decl_annotations_
NTSTATUS
QueryLimitValues (
    PPOWER_LIMIT_VALUE Buffer,
    PFDO_DATA DevExt,
    ULONG RequestId,
    ULONG BufferCount
    )

/*++

Routine Description:

    This routine checks values of power limits.

Arguments:

    Buffer - Supplies a pointer to save values.

    DevExt - Supplies a pointer to the device extension.

    RequestId - Supplies the Id of the request.

    BufferCount - Supplies the count of buffer.

Return Value:

    NTSTATUS.

--*/

{

    PDEVICE_REGISTRATION Registration;
    NTSTATUS Status;

    PAGED_CODE();

    AcquireGlobalMutex();
    Registration = FindRegistrationById(DevExt, RequestId);
    if (Registration == NULL) {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto QueryLimitValuesEnd;
    }

    Status = PoQueryPowerLimitValue(Registration->PowerLimitRequest,
                                    BufferCount,
                                    Buffer);

QueryLimitValuesEnd:
    ReleaseGlobalMutex();
    return Status;
}

_Use_decl_annotations_
NTSTATUS
SetLimitValues (
    PFDO_DATA DevExt,
    ULONG RequestId,
    ULONG BufferCount,
    PPOWER_LIMIT_VALUE Buffer
    )

/*++

Routine Description:

    This routine sets values of power limits.

Arguments:

    DevExt - Supplies a pointer to the device extension.

    RequestId - Supplies the Id of the request.

    BufferCount - Supplies the count of buffer.

    Buffer - Supplies a pointer to proposed control values.

Return Value:

    NTSTATUS.

--*/

{

    COUNTED_REASON_CONTEXT ReasContext;
    PDEVICE_REGISTRATION Registration;
    NTSTATUS Status;

    PAGED_CODE();

    AcquireGlobalMutex();
    Registration = FindRegistrationById(DevExt, RequestId);
    if (Registration == NULL) {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto QueryLimitValuesEnd;
    }

    ReasContext.Version = DIAGNOSTIC_REASON_VERSION;
    ReasContext.Flags = DIAGNOSTIC_REASON_SIMPLE_STRING;
    RtlInitUnicodeString(&ReasContext.SimpleString, L"Sim PL Policy");
    Status = PoSetPowerLimitValue(Registration->PowerLimitRequest,
                                  &ReasContext,
                                  BufferCount,
                                  Buffer);

QueryLimitValuesEnd:
    ReleaseGlobalMutex();
    return Status;
}

NTSTATUS
GetDeviceObjectFromInterfaceName (
    _In_ PUNICODE_STRING Name,
    _Outptr_ PFILE_OBJECT *File,
    _Outptr_ PDEVICE_OBJECT *Device
    )

/*++

Routine Description:

    This routine retrieves the device object for the named interface.

Arguments:

    Name - Supplies a pointer to the name of the device interface.

    File - Supplies a pointer to a location to receive the file object.

    Device - Supplies a pointer to a location to receive the device object.

Return Value:

    NTSTATUS.

--*/

{

    OBJECT_ATTRIBUTES Attributes;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;

    PAGED_CODE();

    DeviceObject = NULL;
    FileObject = NULL;
    Handle = NULL;
    InitializeObjectAttributes(&Attributes,
                               Name,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    //
    // Open a handle to the device.
    //

    Status = ZwCreateFile(&Handle,
                          FILE_ALL_ACCESS,
                          &Attributes,
                          &IoStatus,
                          NULL,
                          0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN,
                          0,
                          NULL,
                          0);

    if (!NT_SUCCESS(Status)) {
        Handle = NULL;
        goto GetInterfaceDeviceObjectEnd;
    }

    //
    // Retrieve the file object associated with the device handle.
    //

    Status = ObReferenceObjectByHandle(Handle,
                                       0,
                                       *IoFileObjectType,
                                       KernelMode,
                                       &FileObject,
                                       NULL);

    if (!NT_SUCCESS(Status)) {
        FileObject = NULL;
        goto GetInterfaceDeviceObjectEnd;
    }

    //
    // Get the device object associated with the file object.
    //
    // N.B. The device object must be referenced before dereferencing the file
    //      object.
    //

    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    if (DeviceObject == NULL) {
        Status = STATUS_UNSUCCESSFUL;
        goto GetInterfaceDeviceObjectEnd;
    }

    ObReferenceObject(DeviceObject);
    *File = FileObject;
    *Device = DeviceObject;
    FileObject = NULL;
    Status = STATUS_SUCCESS;

GetInterfaceDeviceObjectEnd:
    if (FileObject != NULL) {
        ObDereferenceObject(FileObject);
    }

    if (Handle != NULL) {
        ZwClose(Handle);
    }

    return Status;
}

_Use_decl_annotations_
PDEVICE_REGISTRATION
FindRegistrationByName (
    PUNICODE_STRING TargetDeviceName,
    PFDO_DATA DevExt
    )

/*++

Routine Description:

    This routine searches for an existing registration matching the given
    device name.

Arguments:

    TargetDeviceName - Supplies the PDO name of the device.

    DevExt - Supplies a pointer to the device extension.

Return Value:

    A pointer to the device registration, if it is found. Otherwise, NULL.

--*/

{

    LONG Comparison;
    PLIST_ENTRY Link;
    PDEVICE_REGISTRATION Registration;

    Registration = NULL;
    if (DevExt->RequestCount == 0) {
        goto FindRegistrationByNameEnd;
    }

    for (Link = DevExt->RequestHeader.Flink;
         Link != &DevExt->RequestHeader;
         Link = Link->Flink) {

        Registration = CONTAINING_RECORD(Link, DEVICE_REGISTRATION, Link);
        Comparison = RtlCompareUnicodeString(TargetDeviceName,
                                             &Registration->TargetDeviceName,
                                             FALSE);

        if (Comparison == 0) {
            break;
        }

        Registration = NULL;
    }

FindRegistrationByNameEnd:
    return Registration;
}

_Use_decl_annotations_
PDEVICE_REGISTRATION
FindRegistrationById (
    PFDO_DATA DevExt,
    ULONG RequestId
    )

/*++

Routine Description:

    This routine searches for an existing registration matching the given
    device name.

Arguments:

    TargetDeviceName - Supplies the PDO name of the device.

    RequestId - Supplies the ID of the power limit request.

Return Value:

    A pointer to the device registration, if it is found. Otherwise, NULL.

--*/

{

    PLIST_ENTRY Link;
    PDEVICE_REGISTRATION Registration;

    Registration = NULL;
    if (DevExt->RequestCount == 0) {
        goto FindRegistrationByIdEnd;
    }

    for (Link = DevExt->RequestHeader.Flink;
         Link != &DevExt->RequestHeader;
         Link = Link->Flink) {

        Registration = CONTAINING_RECORD(Link, DEVICE_REGISTRATION, Link);
        if (Registration->RequestId == RequestId) {
            break;
        }

        Registration = NULL;
    }

FindRegistrationByIdEnd:
    return Registration;
}