/*++

Copyright (c) Microsoft Corporation

Module Name:

    sampleservice.c

Abstract:

    This file demonstrates a simple service plugin for the TREE class extension driver.

Environment:

    Kernel mode

--*/

#include <ntddk.h>
#include <wdf.h>
#include <ntstrsafe.h>
#include <initguid.h>
#include <wdmguid.h>
#include <TrustedRuntimeClx.h>
#include <SampleOSService.h>

typedef enum _SAMPLE_SERVICE_TYPE {
    SampleServiceEcho,
    SampleServiceKernelMemory
} SAMPLE_SERVICE_TYPE;

typedef struct _SAMPLE_SERVICE_FILE_CONTEXT {
    SAMPLE_SERVICE_TYPE ServiceType;
} SAMPLE_SERVICE_FILE_CONTEXT, *PSAMPLE_SERVICE_FILE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(SAMPLE_SERVICE_FILE_CONTEXT);

typedef
NTSTATUS
SAMPLE_SERVICE_REQUEST_HANDLER(
    _In_ ULONG FunctionCode,
    _In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_to_(OutputBufferLength, *BytesWritten) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength,
    _Out_ size_t* BytesWritten
    );

//
// Driver entry point
//

DRIVER_INITIALIZE DriverEntry;

//
// Device callbacks
//

EVT_WDF_DRIVER_UNLOAD DriverUnload;
EVT_WDF_DRIVER_DEVICE_ADD SampleServiceEvtAddDevice;
EVT_WDF_DEVICE_D0_ENTRY SampleServiceEvtD0Entry;
EVT_WDF_DEVICE_D0_EXIT SampleServiceEvtD0Exit;
EVT_WDF_DEVICE_FILE_CREATE SampleServiceEvtCreateFile;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL SampleServiceEvtInternalIoctl;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL SampleServiceEvtQuery;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL SampleServiceEvtExecute;
SAMPLE_SERVICE_REQUEST_HANDLER SampleServiceEchoHandler;
SAMPLE_SERVICE_REQUEST_HANDLER SampleServiceKernelMemoryHandler;

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)
#pragma alloc_text(PAGE, SampleServiceEvtAddDevice)
#pragma alloc_text(PAGE, SampleServiceEvtCreateFile)
#pragma alloc_text(PAGE, SampleServiceEvtInternalIoctl)
#pragma alloc_text(PAGE, SampleServiceEvtQuery)
#pragma alloc_text(PAGE, SampleServiceEvtExecute)
#pragma alloc_text(PAGE, SampleServiceEchoHandler)
#pragma alloc_text(PAGE, SampleServiceKernelMemoryHandler)

#pragma data_seg("PAGED")

DECLARE_CONST_UNICODE_STRING(GUID_ECHO_SERVICE_STRING,
                             L"{33C7FF13-50B0-454A-8BEB-F73EED6C0AF9}");

DECLARE_CONST_UNICODE_STRING(GUID_KERNEL_MEMORY_SERVICE_STRING,
                             L"{D28698A4-3B07-4F34-B65E-AE3DA6ACF2AC}");

#pragma data_seg()

volatile ULONG RandomSeed;

_Use_decl_annotations_
NTSTATUS
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
    )

/*++

    Routine Description:

        This is the initialization routine for the device driver. This routine
        creates the driver object for the sample OS service.

    Arguments:

        DriverObject - Pointer to driver object created by the system.

    Return Value:

        NTSTATUS code.

--*/

{

    WDFDRIVER Driver;
    WDF_DRIVER_CONFIG DriverConfig;
    NTSTATUS Status;

    Driver = NULL;

    //
    // Create the WDF driver object
    //

    WDF_DRIVER_CONFIG_INIT(&DriverConfig, SampleServiceEvtAddDevice);
    DriverConfig.EvtDriverUnload = DriverUnload;
    DriverConfig.DriverPoolTag = 'SOMS';

    Status = WdfDriverCreate(DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &DriverConfig,
        &Driver);

    if (!NT_SUCCESS(Status)) {
        goto DriverEntryEnd;
    }

DriverEntryEnd:
    return Status;
}

_Use_decl_annotations_
VOID
DriverUnload(
    WDFDRIVER Driver
    )

/*++

    Routine Description:

        This is the cleanup function called when the driver is unloaded.

    Arguments:

        Driver - Supplies a handle to WDFDRIVER object created in DriverEntry.

    Return Value:

        None.

--*/

{

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Driver);
}

_Use_decl_annotations_
NTSTATUS
SampleServiceEvtAddDevice(
    WDFDRIVER Driver,
    PWDFDEVICE_INIT DeviceInit
    )

/*++

    Routine Description:

        This routine is called when the driver is being attached to a specific
        device.

    Arguments:

        Driver - Supplies a handle to the framework driver object.

        DeviceInit - Supplies a pointer to the device initialization parameters.

    Return Value:

        NTSTATUS code.

--*/

{

    WDFDEVICE Device;
    WDF_FILEOBJECT_CONFIG FileConfig;
    WDF_OBJECT_ATTRIBUTES ObjectAttributes;
    WDF_PNPPOWER_EVENT_CALLBACKS PowerCallbacks;
    WDFQUEUE Queue;
    WDF_IO_QUEUE_CONFIG QueueConfig;
    NTSTATUS Status;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Driver);

    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);
    WdfDeviceInitSetDeviceType(DeviceInit, 32999);
    WdfDeviceInitSetPowerNotPageable(DeviceInit);

    //
    // Initialize file context and register file creation handler
    //

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ObjectAttributes,
                                            SAMPLE_SERVICE_FILE_CONTEXT);

    ObjectAttributes.ExecutionLevel = WdfExecutionLevelPassive;
    WDF_FILEOBJECT_CONFIG_INIT(&FileConfig,
                               &SampleServiceEvtCreateFile,
                               WDF_NO_EVENT_CALLBACK,
                               WDF_NO_EVENT_CALLBACK);

    FileConfig.FileObjectClass = WdfFileObjectWdfCanUseFsContext;
    WdfDeviceInitSetFileObjectConfig(DeviceInit,
                                     &FileConfig,
                                     &ObjectAttributes);

    //
    // Device interface will be enabled/disabled in these callbacks
    //

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PowerCallbacks);
    PowerCallbacks.EvtDeviceD0Entry = SampleServiceEvtD0Entry;
    PowerCallbacks.EvtDeviceD0Exit = SampleServiceEvtD0Exit;

    Status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &Device);
    if (!NT_SUCCESS(Status)) {
        goto SampleServiceEvtAddDeviceEnd;
    }

    //
    // Create a default queue to process requests
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&ObjectAttributes);
    ObjectAttributes.ExecutionLevel = WdfExecutionLevelPassive;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&QueueConfig,
                                           WdfIoQueueDispatchParallel);

    QueueConfig.EvtIoInternalDeviceControl = SampleServiceEvtInternalIoctl;
    Status = WdfIoQueueCreate(Device,
                              &QueueConfig,
                              &ObjectAttributes,
                              &Queue);

    if (!NT_SUCCESS(Status)) {
        goto SampleServiceEvtAddDeviceEnd;
    }

    //
    // This tells TrEE driver that this OS service is available. Two services
    // are served by this device, and they are distinguished by filename.
    //
    Status = WdfDeviceCreateDeviceInterface(Device,
                                            &GUID_ECHO_SERVICE,
                                            &GUID_ECHO_SERVICE_STRING);

    if (!NT_SUCCESS(Status)) {
        goto SampleServiceEvtAddDeviceEnd;
    }

    Status = WdfDeviceCreateDeviceInterface(Device,
                                            &GUID_KERNEL_MEMORY_SERVICE,
                                            &GUID_KERNEL_MEMORY_SERVICE_STRING);

    if (!NT_SUCCESS(Status)) {
        goto SampleServiceEvtAddDeviceEnd;
    }

SampleServiceEvtAddDeviceEnd:
    return Status;
}

_Use_decl_annotations_
NTSTATUS
SampleServiceEvtD0Entry(
    WDFDEVICE Device,
    WDF_POWER_DEVICE_STATE PreviousState
    )

/*++

    Routine Description:

        This routine is called when the device is coming back from a lower
        power state to D0.

    Arguments:

        Device - Supplies a handle to device object.

        PreviousState - Supplies the power state the device was in.

    Return Value:

        NTSTATUS code.

--*/

{

    UNREFERENCED_PARAMETER(PreviousState);

    RandomSeed = (ULONG)(KeQueryInterruptTime() >> 8);

    WdfDeviceSetDeviceInterfaceState(Device,
                                     &GUID_ECHO_SERVICE,
                                     &GUID_ECHO_SERVICE_STRING,
                                     TRUE);

    WdfDeviceSetDeviceInterfaceState(Device,
                                     &GUID_KERNEL_MEMORY_SERVICE,
                                     &GUID_KERNEL_MEMORY_SERVICE_STRING,
                                     TRUE);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
SampleServiceEvtD0Exit(
    WDFDEVICE Device,
    WDF_POWER_DEVICE_STATE TargetState
    )

/*++

    Routine Description:

        This routine is called when the device is going down to a lower
        power state.

    Arguments:

        Device - Supplies a handle to device object.

        PreviousState - Supplies the power state the device is going to.

    Return Value:

        NTSTATUS code.

--*/

{

    UNREFERENCED_PARAMETER(TargetState);

    WdfDeviceSetDeviceInterfaceState(Device,
                                     &GUID_ECHO_SERVICE,
                                     &GUID_ECHO_SERVICE_STRING,
                                     FALSE);

    WdfDeviceSetDeviceInterfaceState(Device,
                                     &GUID_KERNEL_MEMORY_SERVICE,
                                     &GUID_KERNEL_MEMORY_SERVICE_STRING,
                                     FALSE);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
VOID
SampleServiceEvtCreateFile(
    WDFDEVICE Device,
    WDFREQUEST Request,
    WDFFILEOBJECT FileObject
    )

/*++

    Routine Description:

        This routine is called when the device is coming back from a lower
        power state to D0.

    Arguments:

        Device - Supplies a handle to device object.

        PreviousState - Supplies the power state the device was in.

    Return Value:

        NTSTATUS code.

--*/

{
    PSAMPLE_SERVICE_FILE_CONTEXT FileContext;
    UNICODE_STRING Filename;
    NTSTATUS Status;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Device);

    FileContext = WdfObjectGet_SAMPLE_SERVICE_FILE_CONTEXT(FileObject);
    Filename = *WdfFileObjectGetFileName(FileObject);
    if (Filename.Buffer[0] == L'\\') {
        ++Filename.Buffer;
        Filename.Length -= sizeof(WCHAR);
        Filename.MaximumLength -= sizeof(WCHAR);
    }

    //
    // Record what service is this create request is for
    //
    if (RtlCompareUnicodeString(&Filename,
                                &GUID_ECHO_SERVICE_STRING,
                                TRUE) == 0) {

        FileContext->ServiceType = SampleServiceEcho;
        Status = STATUS_SUCCESS;

    } else if (RtlCompareUnicodeString(&Filename,
                                       &GUID_KERNEL_MEMORY_SERVICE_STRING,
                                       TRUE) == 0) {

        FileContext->ServiceType = SampleServiceKernelMemory;
        Status = STATUS_SUCCESS;

    } else {

        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }

    WdfRequestComplete(Request, Status);
}

_Use_decl_annotations_
VOID
SampleServiceEvtInternalIoctl(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength,
    ULONG IoControlCode
    )

/*++

    Routine Description:

        This routine is called when the device receives an OS service request
        from TrEE class extension, which is sent as an internal device control
        request.

    Arguments:

        Queue -  Supplies a handle to the framework queue object that is
                 associated with the I/O request.

        Request - Supplies a handle to a framework request object.

        OutputBufferLength - Supplies the length of the request's output
                             buffer, if an output buffer is available.

        InputBufferLength - Supplies the length of the request's input buffer,
                            if an input buffer is available.

        IoControlCode - Supplies the driver-defined or system-defined I/O
                        control code (IOCTL) that is associated with the
                        request.

    Return Value:

        NTSTATUS code.

--*/

{
    PAGED_CODE();

    switch (IoControlCode) {
    case IOCTL_TR_SERVICE_QUERY:
        SampleServiceEvtQuery(Queue,
                              Request,
                              OutputBufferLength,
                              InputBufferLength,
                              IoControlCode);

        return;

    case IOCTL_TR_EXECUTE_FUNCTION:
        SampleServiceEvtExecute(Queue,
                                Request,
                                OutputBufferLength,
                                InputBufferLength,
                                IoControlCode);

        return;

    default:
        WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
    }
}

_Use_decl_annotations_
VOID
SampleServiceEvtQuery(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength,
    ULONG IoControlCode
    )

/*++

    Routine Description:

        This routine is called when the device receives a query request from
        TrEE class extension. OS service provider fills up information about
        the service.

    Arguments:

        Queue -  Supplies a handle to the framework queue object that is
                 associated with the I/O request.

        Request - Supplies a handle to a framework request object.

        OutputBufferLength - Supplies the length of the request's output
                             buffer, if an output buffer is available.

        InputBufferLength - Supplies the length of the request's input buffer,
                            if an input buffer is available.

        IoControlCode - Supplies the driver-defined or system-defined I/O
                        control code (IOCTL) that is associated with the
                        request.

    Return Value:

        NTSTATUS code.

--*/

{

    PSAMPLE_SERVICE_FILE_CONTEXT FileContext;
    WDFFILEOBJECT FileObject;
    PTR_SERVICE_INFORMATION ServiceInformation;
    NTSTATUS Status;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(IoControlCode);

    FileObject = WdfRequestGetFileObject(Request);
    FileContext = WdfObjectGet_SAMPLE_SERVICE_FILE_CONTEXT(FileObject);
    Status = WdfRequestRetrieveOutputBuffer(Request,
                                            sizeof(TR_SERVICE_INFORMATION),
                                            (PVOID*)&ServiceInformation,
                                            NULL);

    if (!NT_SUCCESS(Status)) {
        goto SampleServiceEvtInternalQueryFail;
    }

    ServiceInformation->InterfaceVersion = 1;
    ServiceInformation->ServiceMajorVersion = 1;
    ServiceInformation->ServiceMinorVersion = 1;
    WdfRequestCompleteWithInformation(Request,
                                      STATUS_SUCCESS,
                                      sizeof(TR_SERVICE_INFORMATION));

    return;

SampleServiceEvtInternalQueryFail:
    WdfRequestComplete(Request, Status);
}

_Use_decl_annotations_
VOID
SampleServiceEvtExecute(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength,
    ULONG IoControlCode
    )

/*++

    Routine Description:

        This routine is called when the device receives an OS service execute
        request from TrEE miniport.

    Arguments:

        Queue -  Supplies a handle to the framework queue object that is
                 associated with the I/O request.

        Request - Supplies a handle to a framework request object.

        OutputBufferLength - Supplies the length of the request's output
                             buffer, if an output buffer is available.

        InputBufferLength - Supplies the length of the request's input buffer,
                            if an input buffer is available.

        IoControlCode - Supplies the driver-defined or system-defined I/O
                        control code (IOCTL) that is associated with the
                        request.

    Return Value:

        NTSTATUS code.

--*/

{
    size_t BytesWritten;
    PSAMPLE_SERVICE_FILE_CONTEXT FileContext;
    WDFFILEOBJECT FileObject;
    PTR_SERVICE_REQUEST ServiceRequest;
    PTR_SERVICE_REQUEST_RESPONSE ServiceResponse;
    NTSTATUS Status;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(IoControlCode);

    FileObject = WdfRequestGetFileObject(Request);
    FileContext = WdfObjectGet_SAMPLE_SERVICE_FILE_CONTEXT(FileObject);
    Status = WdfRequestRetrieveInputBuffer(Request,
                                           sizeof(TR_SERVICE_REQUEST),
                                           (PVOID*)&ServiceRequest,
                                           NULL);

    if (!NT_SUCCESS(Status)) {
        goto SampleServiceEvtInternalIoctlFail;
    }

    Status = WdfRequestRetrieveOutputBuffer(Request,
                                            sizeof(TR_SERVICE_REQUEST_RESPONSE),
                                            (PVOID*)&ServiceResponse,
                                            NULL);

    if (!NT_SUCCESS(Status)) {
        goto SampleServiceEvtInternalIoctlFail;
    }

    switch (FileContext->ServiceType) {
    case SampleServiceEcho:
        Status = SampleServiceEchoHandler(ServiceRequest->FunctionCode,
                                          ServiceRequest->InputBuffer,
                                          (size_t)ServiceRequest->InputBufferSize,
                                          ServiceRequest->OutputBuffer,
                                          (size_t)ServiceRequest->OutputBufferSize,
                                          &BytesWritten);

        break;

    case SampleServiceKernelMemory:
        Status = SampleServiceKernelMemoryHandler(ServiceRequest->FunctionCode,
                                                  ServiceRequest->InputBuffer,
                                                  (size_t)ServiceRequest->InputBufferSize,
                                                  ServiceRequest->OutputBuffer,
                                                  (size_t)ServiceRequest->OutputBufferSize,
                                                  &BytesWritten);
        break;

    default:
        //
        // Shouldn't reach here
        //
        NT_ASSERT(FALSE);

        Status = STATUS_INVALID_PARAMETER;
        goto SampleServiceEvtInternalIoctlFail;
    }

    //
    // Bytes written to the output buffer of the service request goes into here
    //
    ServiceResponse->BytesWritten = BytesWritten;

    //
    // Number of bytes written to WDF request's output buffer is always
    // sizeof(TR_SERVICE_REQUEST_RESPONSE)
    //
    WdfRequestCompleteWithInformation(Request,
                                      Status,
                                      sizeof(TR_SERVICE_REQUEST_RESPONSE));

    return;

SampleServiceEvtInternalIoctlFail:
    WdfRequestComplete(Request, Status);
}

_Use_decl_annotations_
NTSTATUS
SampleServiceEchoHandler(
    ULONG FunctionCode,
    PVOID InputBuffer,
    size_t InputBufferLength,
    PVOID OutputBuffer,
    size_t OutputBufferLength,
    size_t* BytesWritten
    )

/*++

    Routine Description:

        This routine handles OS service requests sent to echo service.

    Arguments:

        FunctionCode - Supplies the function code of the request.

        InputBuffer - Supplies a pointer to the request's input buffer,
                      if an input buffer is available.

        InputBufferLength - Supplies the length of the request's input buffer,
                            if an input buffer is available.

        OutputBuffer - Supplies a pointer to the request's output buffer,
                       if an output buffer is available.

        OutputBufferLength - Supplies the length of the request's output
                             buffer, if an output buffer is available.

        BytesWritten - Supplies a pointer to the variable where number of bytes
                       actually written to the output buffer. In case the
                       output buffer is too small to contain all the data, the
                       required size will be written.

    Return Value:

        NTSTATUS code.

--*/

{
    ULONG Index;
    NTSTATUS Status;

    PAGED_CODE();

    switch (FunctionCode) {
    case ECHO_SERVICE_ECHO:
        if (OutputBufferLength < InputBufferLength) {
            *BytesWritten = InputBufferLength;
            Status = STATUS_BUFFER_OVERFLOW;
            goto SampleServiceEchoHandlerEnd;
        }

        RtlCopyMemory(OutputBuffer, InputBuffer, InputBufferLength);
        *BytesWritten = InputBufferLength;
        Status = STATUS_SUCCESS;
        break;

    case ECHO_SERVICE_REPEAT:
        *BytesWritten = 0;
        while (OutputBufferLength >= InputBufferLength) {
            RtlCopyMemory(OutputBuffer, InputBuffer, InputBufferLength);

            OutputBuffer = (PVOID)(((ULONG_PTR)OutputBuffer) + InputBufferLength);
            OutputBufferLength -= InputBufferLength;
            *BytesWritten += InputBufferLength;
        }

        Status = STATUS_SUCCESS;
        break;

    case ECHO_SERVICE_REVERSE:
        if (OutputBufferLength < InputBufferLength) {
            Status = STATUS_BUFFER_OVERFLOW;
            goto SampleServiceEchoHandlerEnd;
        }

        for (Index = 0; Index < InputBufferLength; ++Index) {
            ((PUCHAR)OutputBuffer)[Index] = ((PUCHAR)InputBuffer)[InputBufferLength - Index - 1];
        }

        *BytesWritten = InputBufferLength;
        Status = STATUS_SUCCESS;
        break;

    default:
        return STATUS_INVALID_PARAMETER;
    }

SampleServiceEchoHandlerEnd:
    return Status;
}

UCHAR ScratchMemory[1024];

NTSTATUS
SampleServiceKernelMemoryHandler(
    _In_ ULONG FunctionCode,
    _In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_to_(OuputBufferLength, *BytesWritten) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength,
    _Out_ size_t* BytesWritten
    )

/*++

    Routine Description:

        This routine handles OS service requests sent to kernel memory service.

    Arguments:

        FunctionCode - Supplies the function code of the request.

        InputBuffer - Supplies a pointer to the request's input buffer,
                      if an input buffer is available.

        InputBufferLength - Supplies the length of the request's input buffer,
                            if an input buffer is available.

        OutputBuffer - Supplies a pointer to the request's output buffer,
                       if an output buffer is available.

        OutputBufferLength - Supplies the length of the request's output
                             buffer, if an output buffer is available.

        BytesWritten - Supplies a pointer to the variable where number of bytes
                       actually written to the output buffer. In case the
                       output buffer is too small to contain all the data, the
                       required size will be written.

    Return Value:

        NTSTATUS code.

--*/

{

    UCHAR PreviousValue;
    NTSTATUS Status;
    PKERNEL_MEMORY_SAFE_RANGE SafeRange;
    PKERNEL_MEMORY_WRITE_BYTE WriteByte;

    PAGED_CODE();

    switch (FunctionCode) {
    case KERNEL_MEMORY_SERVICE_GET_SAFE_RANGE:
        if (OutputBufferLength < sizeof(KERNEL_MEMORY_SAFE_RANGE)) {
            *BytesWritten = sizeof(KERNEL_MEMORY_SAFE_RANGE);
            Status = STATUS_BUFFER_OVERFLOW;
            goto SampleServiceKernelMemoryHandlerEnd;
        }

        SafeRange = (PKERNEL_MEMORY_SAFE_RANGE)OutputBuffer;
        SafeRange->Base = (ULONG64)(ULONG_PTR)&ScratchMemory;
        SafeRange->Length = sizeof(ScratchMemory);
        *BytesWritten = sizeof(KERNEL_MEMORY_SAFE_RANGE);
        Status = STATUS_SUCCESS;
        break;

    case KERNEL_MEMORY_SERVICE_WRITE_BYTE:
        if (InputBufferLength < sizeof(KERNEL_MEMORY_WRITE_BYTE)) {
            Status = STATUS_INVALID_PARAMETER;
            goto SampleServiceKernelMemoryHandlerEnd;
        }

        if (OutputBufferLength < sizeof(UCHAR)) {
            *BytesWritten = sizeof(UCHAR);
            Status = STATUS_BUFFER_OVERFLOW;
            goto SampleServiceKernelMemoryHandlerEnd;
        }

        WriteByte = (PKERNEL_MEMORY_WRITE_BYTE)InputBuffer;
        PreviousValue = *(PUCHAR)(ULONG_PTR)WriteByte->Address;
        *(PUCHAR)(ULONG_PTR)WriteByte->Address = WriteByte->Value;
        Status = STATUS_SUCCESS;
        break;

    case KERNEL_MEMORY_SERVICE_READ_BYTE:
        if (InputBufferLength < sizeof(ULONG64)) {
            Status = STATUS_INVALID_PARAMETER;
            goto SampleServiceKernelMemoryHandlerEnd;
        }

        if (OutputBufferLength < sizeof(UCHAR)) {
            *BytesWritten = sizeof(UCHAR);
            Status = STATUS_BUFFER_OVERFLOW;
            goto SampleServiceKernelMemoryHandlerEnd;
        }

        *(PUCHAR)OutputBuffer = *(PUCHAR)(ULONG_PTR)*(PULONG64)InputBuffer;
        Status = STATUS_SUCCESS;
        break;

    default:
        return STATUS_INVALID_PARAMETER;
    }

SampleServiceKernelMemoryHandlerEnd:
    return Status;
}
