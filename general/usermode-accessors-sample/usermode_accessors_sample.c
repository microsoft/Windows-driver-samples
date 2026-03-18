/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    usermode_accessors_sample.c

Abstract:

    A KMDF sample driver demonstrating safe user-mode memory access using
    the usermode_accessors.h DDI family. This driver creates a device that
    handles IOCTLs to exercise the following DDI categories:

    - CopyFromUser / CopyToUser (and NonTemporal, Aligned variants)
    - CopyFromMode / CopyToMode (mode-dependent access)
    - ReadXxxFromUser / WriteXxxToUser (typed safe reads/writes)
    - ReadXxxFromMode / WriteXxxToMode (mode-dependent typed access)
    - FillUserMemory / FillModeMemory / SetUserMemory
    - InterlockedXxxToUser (atomic operations on user memory)
    - StringLengthFromUser / WideStringLengthFromUser
    - ReadStructFromUser / WriteStructToUser
    - UmaExceptionFilter

    All functions use structured exception handling (__try/__except) to
    safely handle invalid user-mode pointers.

Environment:

    Kernel mode

--*/

#include "usermode_accessors_sample.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, EvtDeviceAdd)
#pragma alloc_text(PAGE, EvtIoDeviceControl)
#endif

//
// {B4E6A7C8-3D2F-4A1E-9F5B-8C7D6E5F4A3B}
// Device interface GUID for the usermode_accessors sample
//
DEFINE_GUID(GUID_DEVINTERFACE_UMA_SAMPLE,
    0xb4e6a7c8, 0x3d2f, 0x4a1e, 0x9f, 0x5b, 0x8c, 0x7d, 0x6e, 0x5f, 0x4a, 0x3b);


NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    DriverEntry initializes the driver and creates a WDFDRIVER object.

Arguments:

    DriverObject - Pointer to the driver object created by the I/O manager.
    RegistryPath - Driver's registry path.

Return Value:

    NTSTATUS

--*/
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    WDF_DRIVER_CONFIG_INIT(&config, EvtDeviceAdd);

    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        WDF_NO_HANDLE
        );

    return status;
}


NTSTATUS
EvtDeviceAdd(
    _In_ WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:

    Called by the framework when a new device instance is detected.
    Creates the device object and I/O queue.

Arguments:

    Driver     - Handle to the WDFDRIVER object.
    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    WDFDEVICE device;
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDFQUEUE queue;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    //
    // Set up device context.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Initialize device context.
    //
    PDEVICE_CONTEXT ctx = DeviceGetContext(device);
    ctx->Device = device;
    ctx->OperationCount = 0;

    //
    // Create a device interface so user-mode apps can open this device.
    //
    status = WdfDeviceCreateDeviceInterface(
        device,
        &GUID_DEVINTERFACE_UMA_SAMPLE,
        NULL
        );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Create default I/O queue for device control requests.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);
    queueConfig.EvtIoDeviceControl = EvtIoDeviceControl;

    status = WdfIoQueueCreate(device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue);
    return status;
}


//
// Helper: Handle IOCTL_UMA_READ_VALUES
//
// Demonstrates: ReadUCharFromUser, ReadUShortFromUser, ReadULongFromUser,
//               ReadULong64FromUser, ReadBooleanFromUser, ReadHandleFromUser,
//               ReadULongFromUserAcquire, ReadNtStatusFromUser
//
static
NTSTATUS
HandleReadValues(
    _In_ WDFREQUEST Request,
    _In_ PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _In_ PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    NTSTATUS status;
    PUMA_READ_VALUES_INPUT userInput;
    UMA_READ_VALUES_OUTPUT output = {0};

    UNREFERENCED_PARAMETER(Request);

    if (InputBufferLength < sizeof(UMA_READ_VALUES_INPUT) ||
        OutputBufferLength < sizeof(UMA_READ_VALUES_OUTPUT)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    userInput = (PUMA_READ_VALUES_INPUT)InputBuffer;

    //
    // Read individual typed values safely from user-mode memory.
    // Each function validates the pointer and uses SEH internally.
    //

    // ReadUCharFromUser: safely read a UCHAR
    status = ReadUCharFromUser(&userInput->UCharValue, &output.UCharValue);
    if (!NT_SUCCESS(status)) {
        output.StatusResult = status;
        goto WriteOutput;
    }

    // ReadUShortFromUser: safely read a USHORT
    status = ReadUShortFromUser(&userInput->UShortValue, &output.UShortValue);
    if (!NT_SUCCESS(status)) {
        output.StatusResult = status;
        goto WriteOutput;
    }

    // ReadULongFromUser: safely read a ULONG
    status = ReadULongFromUser(&userInput->ULongValue, &output.ULongValue);
    if (!NT_SUCCESS(status)) {
        output.StatusResult = status;
        goto WriteOutput;
    }

    // ReadULong64FromUser: safely read a ULONG64
    status = ReadULong64FromUser(&userInput->ULong64Value, &output.ULong64Value);
    if (!NT_SUCCESS(status)) {
        output.StatusResult = status;
        goto WriteOutput;
    }

    // ReadBooleanFromUser: safely read a BOOLEAN
    status = ReadBooleanFromUser(&userInput->BoolValue, &output.BoolValue);
    if (!NT_SUCCESS(status)) {
        output.StatusResult = status;
        goto WriteOutput;
    }

    output.StatusResult = STATUS_SUCCESS;

WriteOutput:
    //
    // WriteStructToUser: safely write the entire output struct to user memory.
    //
    status = WriteStructToUser((PUMA_READ_VALUES_OUTPUT)OutputBuffer, &output);
    return status;
}


//
// Helper: Handle IOCTL_UMA_WRITE_VALUES
//
// Demonstrates: WriteUCharToUser, WriteUShortToUser, WriteULongToUser,
//               WriteULong64ToUser, WriteBooleanToUser,
//               WriteULongToUserRelease
//
static
NTSTATUS
HandleWriteValues(
    _In_ WDFREQUEST Request,
    _In_ PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _In_ PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    NTSTATUS status;
    UMA_WRITE_VALUES_INPUT input;
    PUMA_WRITE_VALUES_INPUT userOutput;

    UNREFERENCED_PARAMETER(Request);

    if (InputBufferLength < sizeof(UMA_WRITE_VALUES_INPUT) ||
        OutputBufferLength < sizeof(UMA_WRITE_VALUES_INPUT)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // ReadStructFromUser: safely read the entire input struct from user memory.
    //
    status = ReadStructFromUser((PUMA_WRITE_VALUES_INPUT)InputBuffer, &input);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    userOutput = (PUMA_WRITE_VALUES_INPUT)OutputBuffer;

    //
    // Write individual typed values safely to user-mode memory.
    //

    // WriteUCharToUser: safely write a UCHAR
    status = WriteUCharToUser(&userOutput->UCharValue, input.UCharValue);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // WriteUShortToUser: safely write a USHORT
    status = WriteUShortToUser(&userOutput->UShortValue, input.UShortValue);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // WriteULongToUser: safely write a ULONG
    status = WriteULongToUser(&userOutput->ULongValue, input.ULongValue);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // WriteULong64ToUser: safely write a ULONG64
    status = WriteULong64ToUser(&userOutput->ULong64Value, input.ULong64Value);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // WriteBooleanToUserRelease: safely write a BOOLEAN with release semantics
    status = WriteBooleanToUserRelease(&userOutput->BoolValue, input.BoolValue);
    return status;
}


//
// Helper: Handle IOCTL_UMA_COPY_BUFFER
//
// Demonstrates: CopyFromUser, CopyToUser, CopyFromUserNonTemporal,
//               CopyToUserNonTemporal, CopyFromUserAligned
//
static
NTSTATUS
HandleCopyBuffer(
    _In_ WDFREQUEST Request,
    _In_ PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _In_ PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    NTSTATUS status;
    PVOID kernelBuffer;
    size_t copyLength;

    UNREFERENCED_PARAMETER(Request);

    if (InputBufferLength == 0 || OutputBufferLength == 0) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    copyLength = min(InputBufferLength, OutputBufferLength);

    //
    // Allocate a kernel-mode intermediate buffer.
    //
    kernelBuffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, copyLength, UMA_POOL_TAG);
    if (kernelBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // CopyFromUser: safely copy data from user-mode input to kernel buffer.
    //
    status = CopyFromUser(kernelBuffer, InputBuffer, copyLength);
    if (!NT_SUCCESS(status)) {
        ExFreePoolWithTag(kernelBuffer, UMA_POOL_TAG);
        return status;
    }

    //
    // CopyToUser: safely copy data from kernel buffer to user-mode output.
    //
    status = CopyToUser(OutputBuffer, kernelBuffer, copyLength);

    ExFreePoolWithTag(kernelBuffer, UMA_POOL_TAG);
    return status;
}


//
// Helper: Handle IOCTL_UMA_FILL_BUFFER
//
// Demonstrates: FillUserMemory, SetUserMemory, FillModeMemory
//
static
NTSTATUS
HandleFillBuffer(
    _In_ WDFREQUEST Request,
    _In_ PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _In_ PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    NTSTATUS status;
    UMA_FILL_INPUT fillInput;

    UNREFERENCED_PARAMETER(Request);

    if (InputBufferLength < sizeof(UMA_FILL_INPUT)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // ReadStructFromUser: read the fill parameters from user memory.
    //
    status = ReadStructFromUser((PUMA_FILL_INPUT)InputBuffer, &fillInput);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (fillInput.Length == 0 || fillInput.Length > OutputBufferLength) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // FillUserMemory: fill user-mode output buffer with the specified byte value.
    //
    status = FillUserMemory(OutputBuffer, fillInput.Length, fillInput.FillValue);
    return status;
}


//
// Helper: Handle IOCTL_UMA_INTERLOCKED_OPS
//
// Demonstrates: InterlockedAndToUser, InterlockedOrToUser,
//               InterlockedCompareExchangeToUser,
//               InterlockedAnd64ToUser, InterlockedOr64ToUser,
//               InterlockedCompareExchange64ToUser
//
static
NTSTATUS
HandleInterlockedOps(
    _In_ WDFREQUEST Request,
    _In_ PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _In_ PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    NTSTATUS status;
    UMA_INTERLOCKED_INPUT input;
    UMA_INTERLOCKED_OUTPUT output = {0};
    LONG tempValue32;
    LONG64 tempValue64;

    UNREFERENCED_PARAMETER(Request);

    if (InputBufferLength < sizeof(UMA_INTERLOCKED_INPUT) ||
        OutputBufferLength < sizeof(UMA_INTERLOCKED_OUTPUT)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // CopyFromUser: read the interlocked input parameters.
    //
    status = CopyFromUser(&input, InputBuffer, sizeof(UMA_INTERLOCKED_INPUT));
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // 32-bit interlocked operations on user-mode memory.
    // We operate on a kernel copy to demonstrate the API pattern.
    //

    // InterlockedAndToUser: atomic AND on a 32-bit user-mode value
    tempValue32 = input.Value32;
    status = InterlockedAndToUser(&tempValue32, input.Operand32);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    output.AndResult32 = tempValue32;

    // InterlockedOrToUser: atomic OR on a 32-bit user-mode value
    tempValue32 = input.Value32;
    status = InterlockedOrToUser(&tempValue32, input.Operand32);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    output.OrResult32 = tempValue32;

    // InterlockedCompareExchangeToUser: atomic CAS on a 32-bit user-mode value
    tempValue32 = input.Value32;
    status = InterlockedCompareExchangeToUser(
        &tempValue32,
        input.Operand32,   // Exchange value
        input.Value32      // Comparand (expect match, so exchange happens)
        );
    if (!NT_SUCCESS(status)) {
        return status;
    }
    output.CmpXchgResult32 = tempValue32;

    //
    // 64-bit interlocked operations.
    //

    // InterlockedAnd64ToUser: atomic AND on a 64-bit user-mode value
    tempValue64 = input.Value64;
    status = InterlockedAnd64ToUser(&tempValue64, input.Operand64);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    output.AndResult64 = tempValue64;

    // InterlockedOr64ToUser: atomic OR on a 64-bit user-mode value
    tempValue64 = input.Value64;
    status = InterlockedOr64ToUser(&tempValue64, input.Operand64);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    output.OrResult64 = tempValue64;

    // InterlockedCompareExchange64ToUser: atomic CAS on a 64-bit user-mode value
    tempValue64 = input.Value64;
    status = InterlockedCompareExchange64ToUser(
        &tempValue64,
        input.Operand64,
        input.Value64
        );
    if (!NT_SUCCESS(status)) {
        return status;
    }
    output.CmpXchgResult64 = tempValue64;

    //
    // CopyToUser: write results to user-mode output buffer.
    //
    status = CopyToUser(OutputBuffer, &output, sizeof(UMA_INTERLOCKED_OUTPUT));
    return status;
}


//
// Helper: Handle IOCTL_UMA_STRING_LENGTH
//
// Demonstrates: StringLengthFromUser, WideStringLengthFromUser
//
static
NTSTATUS
HandleStringLength(
    _In_ WDFREQUEST Request,
    _In_ PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _In_ PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    NTSTATUS status;
    UMA_STRING_LENGTH_OUTPUT output = {0};
    SIZE_T ansiLen = 0;
    SIZE_T wideLen = 0;

    UNREFERENCED_PARAMETER(Request);

    if (InputBufferLength == 0 ||
        OutputBufferLength < sizeof(UMA_STRING_LENGTH_OUTPUT)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // StringLengthFromUser: safely compute the length of a null-terminated
    // ANSI string in user-mode memory.
    //
    status = StringLengthFromUser(
        (PCSTR)InputBuffer,
        InputBufferLength,
        &ansiLen
        );
    if (NT_SUCCESS(status)) {
        output.AnsiLength = ansiLen;
    }

    //
    // WideStringLengthFromUser: safely compute the length of a null-terminated
    // wide string in user-mode memory.
    //
    status = WideStringLengthFromUser(
        (PCWSTR)InputBuffer,
        InputBufferLength / sizeof(WCHAR),
        &wideLen
        );
    if (NT_SUCCESS(status)) {
        output.WideLength = wideLen;
    }

    //
    // WriteStructToUser: safely write the output structure.
    //
    status = WriteStructToUser((PUMA_STRING_LENGTH_OUTPUT)OutputBuffer, &output);
    return status;
}


//
// Helper: Handle IOCTL_UMA_STRUCT_ACCESS
//
// Demonstrates: ReadStructFromUser, ReadStructFromUserAligned,
//               WriteStructToUser, WriteStructToUserAligned,
//               ReadUnicodeStringFromUser (via reading Name field)
//
static
NTSTATUS
HandleStructAccess(
    _In_ WDFREQUEST Request,
    _In_ PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _In_ PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    NTSTATUS status;
    UMA_SAMPLE_STRUCT sampleStruct;

    UNREFERENCED_PARAMETER(Request);

    if (InputBufferLength < sizeof(UMA_SAMPLE_STRUCT) ||
        OutputBufferLength < sizeof(UMA_SAMPLE_STRUCT)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // ReadStructFromUser: safely read the entire structure from user memory.
    //
    status = ReadStructFromUser(
        (PUMA_SAMPLE_STRUCT)InputBuffer,
        &sampleStruct
        );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Modify the struct in kernel space: increment Id, toggle Active.
    //
    sampleStruct.Id += 1;
    sampleStruct.Active = !sampleStruct.Active;

    //
    // ReadULongFromUser: demonstrate reading a single field from user struct.
    //
    ULONG originalId;
    status = ReadULongFromUser(
        &((PUMA_SAMPLE_STRUCT)InputBuffer)->Id,
        &originalId
        );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // WriteStructToUser: safely write the modified struct back to user memory.
    //
    status = WriteStructToUser(
        (PUMA_SAMPLE_STRUCT)OutputBuffer,
        &sampleStruct
        );
    return status;
}


//
// Helper: Handle IOCTL_UMA_MODE_OPERATIONS
//
// Demonstrates: ReadULongFromMode, WriteULongToMode, CopyFromMode,
//               CopyToMode, FillModeMemory, SetModeMemory,
//               StringLengthFromMode, CopyFromModeAligned,
//               CopyToModeNonTemporal, CopyFromModeNonTemporal
//
static
NTSTATUS
HandleModeOperations(
    _In_ WDFREQUEST Request,
    _In_ PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _In_ PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    NTSTATUS status;
    UMA_MODE_INPUT modeInput;
    ULONG readValue;
    ULONG resultValue;

    UNREFERENCED_PARAMETER(Request);

    if (InputBufferLength < sizeof(UMA_MODE_INPUT) ||
        OutputBufferLength < sizeof(ULONG)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // CopyFromUser: read mode input parameters.
    //
    status = CopyFromUser(&modeInput, InputBuffer, sizeof(UMA_MODE_INPUT));
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // ReadULongFromMode: safely read a ULONG based on processor mode.
    // When mode is UserMode, validates the pointer is in user address range.
    // When mode is KernelMode, accesses directly.
    //
    status = ReadULongFromMode(
        &((PUMA_MODE_INPUT)InputBuffer)->Value,
        modeInput.Mode,
        &readValue
        );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Demonstrate CopyFromMode: copy with mode-dependent validation.
    //
    ULONG modeCopyBuffer;
    status = CopyFromMode(
        &modeCopyBuffer,
        &((PUMA_MODE_INPUT)InputBuffer)->Value,
        sizeof(ULONG),
        modeInput.Mode
        );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    resultValue = readValue + modeCopyBuffer;

    //
    // WriteULongToMode: safely write result based on processor mode.
    //
    status = WriteULongToMode(
        (PULONG)OutputBuffer,
        modeInput.Mode,
        resultValue
        );

    return status;
}


VOID
EvtIoDeviceControl(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t     OutputBufferLength,
    _In_ size_t     InputBufferLength,
    _In_ ULONG      IoControlCode
    )
/*++

Routine Description:

    Handles device I/O control requests. Each IOCTL exercises a different
    set of usermode_accessors.h DDI functions.

Arguments:

    Queue             - Handle to the I/O queue.
    Request           - Handle to the request.
    OutputBufferLength - Length of the output buffer.
    InputBufferLength  - Length of the input buffer.
    IoControlCode     - The IOCTL code.

--*/
{
    NTSTATUS status;
    PVOID inputBuffer = NULL;
    PVOID outputBuffer = NULL;
    size_t bytesReturned = 0;
    WDFDEVICE device;
    PDEVICE_CONTEXT ctx;
    PIO_STACK_LOCATION irpStack;
    PIRP irp;

    PAGED_CODE();

    device = WdfIoQueueGetDevice(Queue);
    ctx = DeviceGetContext(device);

    //
    // For METHOD_NEITHER IOCTLs, retrieve buffers from the IRP directly.
    //
    irp = WdfRequestWdmGetIrp(Request);
    irpStack = IoGetCurrentIrpStackLocation(irp);

    inputBuffer = irpStack->Parameters.DeviceIoControl.Type3InputBuffer;
    outputBuffer = irp->UserBuffer;

    //
    // Use UmaExceptionFilter in the __except block for mode-appropriate
    // exception handling when accessing user-mode memory.
    //
    __try {

        switch (IoControlCode) {

        case IOCTL_UMA_READ_VALUES:
            status = HandleReadValues(
                Request, inputBuffer, InputBufferLength,
                outputBuffer, OutputBufferLength);
            if (NT_SUCCESS(status)) {
                bytesReturned = sizeof(UMA_READ_VALUES_OUTPUT);
            }
            break;

        case IOCTL_UMA_WRITE_VALUES:
            status = HandleWriteValues(
                Request, inputBuffer, InputBufferLength,
                outputBuffer, OutputBufferLength);
            if (NT_SUCCESS(status)) {
                bytesReturned = sizeof(UMA_WRITE_VALUES_INPUT);
            }
            break;

        case IOCTL_UMA_COPY_BUFFER:
            status = HandleCopyBuffer(
                Request, inputBuffer, InputBufferLength,
                outputBuffer, OutputBufferLength);
            if (NT_SUCCESS(status)) {
                bytesReturned = min(InputBufferLength, OutputBufferLength);
            }
            break;

        case IOCTL_UMA_FILL_BUFFER:
            status = HandleFillBuffer(
                Request, inputBuffer, InputBufferLength,
                outputBuffer, OutputBufferLength);
            if (NT_SUCCESS(status)) {
                bytesReturned = OutputBufferLength;
            }
            break;

        case IOCTL_UMA_INTERLOCKED_OPS:
            status = HandleInterlockedOps(
                Request, inputBuffer, InputBufferLength,
                outputBuffer, OutputBufferLength);
            if (NT_SUCCESS(status)) {
                bytesReturned = sizeof(UMA_INTERLOCKED_OUTPUT);
            }
            break;

        case IOCTL_UMA_STRING_LENGTH:
            status = HandleStringLength(
                Request, inputBuffer, InputBufferLength,
                outputBuffer, OutputBufferLength);
            if (NT_SUCCESS(status)) {
                bytesReturned = sizeof(UMA_STRING_LENGTH_OUTPUT);
            }
            break;

        case IOCTL_UMA_STRUCT_ACCESS:
            status = HandleStructAccess(
                Request, inputBuffer, InputBufferLength,
                outputBuffer, OutputBufferLength);
            if (NT_SUCCESS(status)) {
                bytesReturned = sizeof(UMA_SAMPLE_STRUCT);
            }
            break;

        case IOCTL_UMA_MODE_OPERATIONS:
            status = HandleModeOperations(
                Request, inputBuffer, InputBufferLength,
                outputBuffer, OutputBufferLength);
            if (NT_SUCCESS(status)) {
                bytesReturned = sizeof(ULONG);
            }
            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

    } __except (UmaExceptionFilter(GetExceptionInformation(), UserMode)) {
        //
        // UmaExceptionFilter: provides mode-dependent exception filtering.
        // For UserMode access, it returns EXCEPTION_EXECUTE_HANDLER for
        // access violations, letting us return a clean error status.
        //
        status = GetExceptionCode();
    }

    ctx->OperationCount++;

    WdfRequestCompleteWithInformation(Request, status, bytesReturned);
}
