/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    wdf.c

Abstract:

    The module implements WDF boilerplate for the simulate power limit policy.

--*/

//-------------------------------------------------------------------- Includes

#include "plpolicy.h"

//------------------------------------------------------------------ Prototypes

DRIVER_INITIALIZE                   DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD           EvtDriverDeviceAdd;
EVT_WDF_DRIVER_UNLOAD               EvtDriverUnload;
EVT_WDF_OBJECT_CONTEXT_DESTROY      EvtDeviceDestroy;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL  EvtIoDeviceControl;

NTSTATUS
GetDeviceName (
    _Out_ PUNICODE_STRING DeviceName,
    _In_reads_bytes_(BufferLength) PWCHAR Buffer,
    _In_ SIZE_T BufferLength
    );

//--------------------------------------------------------------------- Pragmas

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, EvtDriverDeviceAdd)
#pragma alloc_text(PAGE, EvtDriverUnload)
#pragma alloc_text(PAGE, EvtIoDeviceControl)
#pragma alloc_text(PAGE, EvtDeviceDestroy)
#pragma alloc_text(PAGE, GetDeviceName)

//------------------------------------------------------------------- Functions

_Use_decl_annotations_
NTSTATUS
DriverEntry (
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry configures and creates a WDF
    driver object.

Parameters Description:

    DriverObject - Supplies a pointer to the driver object.

    RegistryPath - Supplies a pointer to a unicode string representing the
        path to the driver-specific key in the registry.

Return Value:

    NTSTATUS.

--*/

{

    WDF_DRIVER_CONFIG DriverConfig;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(RegistryPath);

    //
    // Initiialize the DriverConfig data that controls the attributes that are
    // global to this driver.
    //

    DebugEnter();
    WDF_DRIVER_CONFIG_INIT(&DriverConfig, EvtDriverDeviceAdd);
    DriverConfig.EvtDriverUnload = EvtDriverUnload;

    //
    // Create a framework driver object to represent this driver.
    //

    Status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &DriverConfig,
                             WDF_NO_HANDLE);

    if (!NT_SUCCESS(Status)) {
        DebugPrint(PLPOLICY_PRINT_ERROR,
                   "WdfDriverCreate() Failed! 0x%x\n",
                   Status);

        goto DriverEntryEnd;
    }

    //
    // Initialize global mutex.
    //

    Status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &GlobalMutex);
    if (!NT_SUCCESS(Status)) {
        DebugPrint(PLPOLICY_PRINT_ERROR,
                   "WdfSpinLockCreate() Failed! 0x%x\n",
                   Status);

        goto DriverEntryEnd;
    }

    Status = STATUS_SUCCESS;

DriverEntryEnd:
    DebugExitStatus(Status);
    return Status;
}

_Use_decl_annotations_
NTSTATUS
EvtDriverDeviceAdd (
    WDFDRIVER Driver,
    PWDFDEVICE_INIT DeviceInit
    )

/*++

Routine Description:

    EvtDriverDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. A WDF device object is created and initailized to
    represent the simulation control interface.

Arguments:

    Driver - Supplies a handle to a framework driver object created in
        DriverEntry

    DeviceInit - Supplies a pointer to a framework-allocated WDFDEVICE_INIT
        structure.

Return Value:

    NTSTATUS

--*/

{

    PFDO_DATA DevExt;
    WDF_OBJECT_ATTRIBUTES DeviceAttributes;
    WDFDEVICE DeviceHandle;
    WDF_IO_QUEUE_CONFIG IoQueueConfig;
    WDFQUEUE Queue;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    DebugEnter();

    //
    // Initialize attributes and a context area for the device object.
    //

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&DeviceAttributes, FDO_DATA);
    DeviceAttributes.EvtDestroyCallback = &EvtDeviceDestroy;


    //
    // Create a framework device object.  This call will in turn create
    // a WDM device object, attach to the lower stack, and set the
    // appropriate flags and attributes.
    //

    Status = WdfDeviceCreate(&DeviceInit, &DeviceAttributes, &DeviceHandle);
    if (!NT_SUCCESS(Status)) {
        DebugPrint(PLPOLICY_PRINT_ERROR,
                   "WdfDeviceCreate() Failed! 0x%x\n",
                   Status);

        goto EvtDriverDeviceAddEnd;
    }

    //
    // Configure a default queue for IO requests. This queue processes requests
    // to read/write the simulated state.
    //

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&IoQueueConfig,
                                           WdfIoQueueDispatchSequential);

    IoQueueConfig.EvtIoDeviceControl = EvtIoDeviceControl;
    Status = WdfIoQueueCreate(DeviceHandle,
                              &IoQueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &Queue);

    if (!NT_SUCCESS(Status)) {
        DebugPrint(PLPOLICY_PRINT_ERROR,
                   "WdfIoQueueCreate() Failed! 0x%x\n",
                   Status);

        goto EvtDriverDeviceAddEnd;
    }

    //
    // Create device interface for this device. The interface will be
    // enabled by the framework when StartDevice returns successfully.
    // Clients of this driver will open this interface and send ioctls.
    //

    Status = WdfDeviceCreateDeviceInterface(DeviceHandle,
                                            &GUID_DEVINTERFACE_POWERLIMIT_POLICY,
                                            NULL);

    if (!NT_SUCCESS(Status)) {
        DebugPrint(PLPOLICY_PRINT_ERROR,
                   "WdfDeviceCreateDeviceInterface() Failed! 0x%x\n",
                   Status);

        goto EvtDriverDeviceAddEnd;
    }

    //
    // Ignore failures to initialize the device; the test cases will fail in
    // this case.
    //

    DevExt = GetDeviceExtension(DeviceHandle);
    InitializeListHead(&DevExt->RequestHeader);
    DevExt->RequestCount = 0;
    Status = STATUS_SUCCESS;

EvtDriverDeviceAddEnd:
    DebugExitStatus(Status);
    return Status;
}

_Use_decl_annotations_
VOID
EvtDeviceDestroy (
    WDFOBJECT Object
    )

/*++

Routine Description:

    This routine destroys the device's data.

Arguments:

    Object - Supplies the WDF reference to the device that is being removed.

Return Value:

    None.

--*/

{

    PFDO_DATA DevExt;
    PLIST_ENTRY Link;
    PDEVICE_REGISTRATION Registration;

    PAGED_CODE();

    DebugEnter();
    DevExt = GetDeviceExtension(Object);
    while (IsListEmpty(&DevExt->RequestHeader) == FALSE) {
        Link = DevExt->RequestHeader.Flink;
        Registration = CONTAINING_RECORD(Link, DEVICE_REGISTRATION, Link);
        RemoveEntryList(&Registration->Link);
        PoDeletePowerLimitRequest(Registration->PowerLimitRequest);
        ExFreePoolWithTag(Registration, PLPOLICY_TAG);
    }

    DebugExit();
    return;
}

_Use_decl_annotations_
VOID
EvtIoDeviceControl (
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength,
    ULONG IoControlCode
    )

/*++

Routine Description:

    This routine processes an IOCTL sent to the PEP.

Arguments:

    Queue - Supplies a handle to the WDF queue object.

    Request - Supplies a handle to the WDF request object for this request.

    OuputBufferLength - Supplies the length of the output buffer, in bytes.

    InputBufferLength - Supplies the length of the input buffer, in bytes.

    IoControlCode - Supplies the IOCTL code being processes.

Return Value:

    None.

--*/

{

    PPOWERLIMIT_POLICY_ATTRIBUTES Attributes;
    ULONG BytesWritten;
    PFDO_DATA DevExt;
    WDFDEVICE Device;
    UNICODE_STRING DeviceName;
    PVOID InputBuffer;
    PVOID OutputBuffer;
    PPOWERLIMIT_POLICY_ATTRIBUTES QueryAttributeInput;
    PPOWERLIMIT_POLICY_VALUES QueryValueInput;
    ULONG RequestId;
    PPOWERLIMIT_POLICY_VALUES SetInput;
    ULONG SizeNeeded;
    NTSTATUS Status;
    PPOWERLIMIT_POLICY_VALUES Values;

    DebugPrint(PLPOLICY_PRINT_TRACE,
               "EvtIoDeviceControl: 0x%08x\n",
               IoControlCode);

    BytesWritten = 0;
    OutputBuffer = NULL;
    InputBuffer = NULL;
    if (InputBufferLength > 0) {
        Status = WdfRequestRetrieveInputBuffer(Request,
                                               InputBufferLength,
                                               &InputBuffer,
                                               NULL);

        if (!NT_SUCCESS(Status)) {
            goto DeviceControlEnd;
        }
    }

    if (OutputBufferLength > 0) {
        Status = WdfRequestRetrieveOutputBuffer(Request,
                                                OutputBufferLength,
                                                &OutputBuffer,
                                                NULL);

        if (!NT_SUCCESS(Status)) {
            goto DeviceControlEnd;
        }
    }

    Device = WdfIoQueueGetDevice(Queue);
    DevExt = GetDeviceExtension(Device);
    switch (IoControlCode) {
    case IOCTL_POWERLIMIT_POLICY_REGISTER:
        if ((InputBufferLength == 0) || (InputBuffer == NULL)) {
            Status = STATUS_INVALID_PARAMETER;
            goto DeviceControlEnd;
        }

        if ((OutputBufferLength != sizeof(ULONG)) || (OutputBuffer == NULL)) {
            Status = STATUS_INVALID_PARAMETER;
            goto DeviceControlEnd;
        }

        Status = GetDeviceName(&DeviceName,
                               (PWCHAR)InputBuffer,
                               InputBufferLength);

        if (!NT_SUCCESS(Status)) {
            goto DeviceControlEnd;
        }

        Status = RegisterRequest(DevExt,
                                 &DeviceName,
                                 WdfDeviceWdmGetDeviceObject(Device),
                                 &RequestId);

        if (NT_SUCCESS(Status)) {
            *(PULONG)OutputBuffer = RequestId;
            BytesWritten = sizeof(ULONG);
        }

        break;

    case IOCTL_POWERLIMIT_POLICY_UNREGISTER:
        if (InputBufferLength < sizeof(ULONG)) {
            Status = STATUS_INVALID_PARAMETER;
            goto DeviceControlEnd;
        }

        RequestId = *(PULONG)InputBuffer;
        Status = UnregisterRequest(DevExt, RequestId);

        break;

    case IOCTL_POWERLIMIT_POLICY_QUERY_ATTRIBUTES:
        if (InputBufferLength < sizeof(POWERLIMIT_POLICY_ATTRIBUTES)) {
            Status = STATUS_INVALID_PARAMETER;
            goto DeviceControlEnd;
        }

        QueryAttributeInput = (PPOWERLIMIT_POLICY_ATTRIBUTES)InputBuffer;
        SizeNeeded = FIELD_OFFSET(POWERLIMIT_POLICY_ATTRIBUTES,
                                  Buffer[QueryAttributeInput->BufferCount]);

        if ((OutputBufferLength < SizeNeeded) || (InputBufferLength < SizeNeeded)) {
            Status = STATUS_INVALID_PARAMETER;
            goto DeviceControlEnd;
        }

        Attributes = ExAllocatePool2(POOL_FLAG_PAGED, SizeNeeded, PLPOLICY_TAG);
        if (Attributes == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto DeviceControlEnd;
        }

        Attributes->RequestId = QueryAttributeInput->RequestId;
        Attributes->BufferCount = QueryAttributeInput->BufferCount;
        Status = QueryAttributes(Attributes->Buffer,
                                 DevExt,
                                 QueryAttributeInput->RequestId,
                                 QueryAttributeInput->BufferCount);

        if (NT_SUCCESS(Status)) {
            RtlCopyMemory(OutputBuffer, Attributes, SizeNeeded);
            BytesWritten = SizeNeeded;
        }

        ExFreePoolWithTag(Attributes, PLPOLICY_TAG);

        break;

    case IOCTL_POWERLIMIT_POLICY_QUERY_VALUES:
        if (InputBufferLength < sizeof(POWERLIMIT_POLICY_VALUES)) {
            Status = STATUS_INVALID_PARAMETER;
            goto DeviceControlEnd;
        }

        QueryValueInput = (PPOWERLIMIT_POLICY_VALUES)InputBuffer;
        SizeNeeded = FIELD_OFFSET(POWERLIMIT_POLICY_VALUES,
                                  Buffer[QueryValueInput->BufferCount]);

        if ((OutputBufferLength < SizeNeeded) || (InputBufferLength < SizeNeeded)) {
            Status = STATUS_INVALID_PARAMETER;
            goto DeviceControlEnd;
        }

        Values = ExAllocatePool2(POOL_FLAG_PAGED, SizeNeeded, PLPOLICY_TAG);
        if (Values == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto DeviceControlEnd;
        }

        //
        // The input buffer contains which limits to read, copy them over before
        // calling kernel API.
        //

        RtlCopyMemory(Values, QueryValueInput, SizeNeeded);
        Status = QueryLimitValues(Values->Buffer,
                                  DevExt,
                                  QueryValueInput->RequestId,
                                  QueryValueInput->BufferCount);

        if (NT_SUCCESS(Status)) {
            RtlCopyMemory(OutputBuffer, Values, SizeNeeded);
            BytesWritten = SizeNeeded;
        }

        ExFreePoolWithTag(Values, PLPOLICY_TAG);

        break;

    case IOCTL_POWERLIMIT_POLICY_SET_VALUES:
        if (InputBufferLength < sizeof(POWERLIMIT_POLICY_VALUES)) {
            Status = STATUS_INVALID_PARAMETER;
            goto DeviceControlEnd;
        }

        SetInput = (PPOWERLIMIT_POLICY_VALUES)InputBuffer;
        SizeNeeded = FIELD_OFFSET(POWERLIMIT_POLICY_VALUES,
                                  Buffer[SetInput->BufferCount]);

        if (InputBufferLength < SizeNeeded) {
            Status = STATUS_INVALID_PARAMETER;
            goto DeviceControlEnd;
        }

        Status = SetLimitValues(DevExt,
                                SetInput->RequestId,
                                SetInput->BufferCount,
                                SetInput->Buffer);

        break;

    default:
        Status = STATUS_NOT_SUPPORTED;
        break;
    }

DeviceControlEnd:
    WdfRequestCompleteWithInformation(Request, Status, BytesWritten);
    DebugExitStatus(Status);
    return;
}

VOID
EvtDriverUnload (
    WDFDRIVER Driver
    )

/*++

Routine Description:

    This routine is called at driver unload to clean up any lingering thermal
    requests.

Arguments:

    Driver - Supplies a pointer to the WDF driver object.

Return Value:

    None.

--*/

{

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    DebugEnter();

    //
    // N.B. Does nothing since we don't have anything to clean up, just print
    //      some debug info.
    //

    DebugExit();
    return;
}

_Use_decl_annotations_
NTSTATUS
GetDeviceName (
    PUNICODE_STRING DeviceName,
    PWCHAR Buffer,
    SIZE_T BufferLength
    )

/*++

Routine Description:

    This routine extracts a device name from an IOCTL input buffer, being
    careful to validate the name is well formed.

Arguments:

    DeviceName - Supplies a UNICODE_STRING to initialize with the device name.

    Buffer - Supplies the buffer containing the device name.

    BufferLength - Supplies the length of the buffer containing the device name,
        in bytes.

Return Value:

    NTSTATUS.

--*/

{

    size_t Length;
    NTSTATUS Status;

    Status = RtlStringCchLengthW(Buffer, BufferLength / sizeof(WCHAR), &Length);
    if (!NT_SUCCESS(Status)) {
        goto GetDeviceNameEnd;
    }

    if (Length > NTSTRSAFE_UNICODE_STRING_MAX_CCH) {
        Status = STATUS_INVALID_PARAMETER;
        goto GetDeviceNameEnd;
    }

    DeviceName->Buffer = Buffer;
    DeviceName->Length = (USHORT)(Length * sizeof(WCHAR));
    DeviceName->MaximumLength = (USHORT)(Length * sizeof(WCHAR));
    Status = STATUS_SUCCESS;

GetDeviceNameEnd:
    return Status;
}
