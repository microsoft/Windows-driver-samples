
/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    spb_i2c.c

Abstract:

    This sample implements a GPIO client driver for simulated GPIO (SimGpio)
    controller.

    Note: DIRQL in the comments below refers to device IRQL, which is any
        IRQL > DISPATCH_LEVEL (and less than some IRQL reserved for OS use).


Environment:

    Kernel mode

--*/

//
// ------------------------------------------------------------------- Includes
//

#include <ntddk.h>
#include <wdf.h>
#include <gpioclx.h>
#define RESHUB_USE_HELPER_ROUTINES
#include "reshub.h"  // Resource and descriptor definitions
#include "spb.h"    // SPB definitions
#include "simgpio_i2c.h"
#include "trace.h"
#include "spb_i2c.tmh"

//
// -------------------------------------------------------------------- Defines
//

NTSTATUS
SimGpioSpbRead (
    _In_ PSIM_GPIO_CONTEXT GpioContext,
    _In_ SIM_GPIO_REGISTER_ADDRESS RegisterAddress,
    _Out_writes_(DataLength) PUCHAR Data,
    _In_ USHORT  DataLength
    );

NTSTATUS
SimGpioSpbSequence (
    _In_ PSIM_GPIO_CONTEXT GpioContext,
    _In_reads_(SequenceLength) PVOID Sequence,
    _In_ SIZE_T SequenceLength
    );

NTSTATUS
SimGpioSpbWrite (
    _In_ PSIM_GPIO_CONTEXT GpioContext,
    _In_ USHORT RegisterAddress,
    _In_reads_(DataLength) PUCHAR Data,
    _In_ ULONG DataLength
    );

//
// ------------------------------------------------------------------ Functions
//

NTSTATUS
SimGpioSetupSpbConnection (
    _In_ PSIM_GPIO_CONTEXT GpioContext
    )

/*++

Routine Description:

    This routine opens an I/O target to the controller driver using the
    connection ID received during PrepareController callback.

    N.B. This function is not marked pageable because this function is in
         the device power up path.

Arguments:

    GpioContext - Supplies a pionter to the client driver's device context.

Return Value:

    NTSTATUS code.

--*/

{

    WDF_OBJECT_ATTRIBUTES Attributes;
    WDF_IO_TARGET_OPEN_PARAMS Parameters;
    WDF_OBJECT_ATTRIBUTES RequestAttributes;
    PSIM_GPIO_REQUEST_CONTEXT RequestContext;
    NTSTATUS Status;
    DECLARE_UNICODE_STRING_SIZE(SpbDevicePath, RESOURCE_HUB_PATH_SIZE);

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Status = WdfIoTargetCreate(GpioContext->Device,
                               &Attributes,
                               &GpioContext->SpbIoTarget);

    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: WdfIoTargetCreate failed to create SPB IoTarget! "
                "Device = %p, Status:%#x\n",
                __FUNCTION__,
                GpioContext->Device,
                Status);

        if (GpioContext->SpbIoTarget != NULL) {
            WdfObjectDelete(GpioContext->SpbIoTarget);
        }

        goto SetupSpbConnectionEnd;
    }

    //
    // Use the connection ID supplied to create the full device path. This
    // device path (indirectly) represents the path for the I2C controller.
    //

    Status = RESOURCE_HUB_CREATE_PATH_FROM_ID(
                                &SpbDevicePath,
                                GpioContext->SpbConnectionId.LowPart,
                                GpioContext->SpbConnectionId.HighPart);

    if (!NT_SUCCESS(Status)) {
        TraceEvents(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPB,
            "ResourceHub create device path (%wZ) failed Status:%#x",
            &SpbDevicePath,
            Status);

        goto SetupSpbConnectionEnd;
    }

    //
    // Initialize the parameters for the SPB IO target.
    //

    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&Parameters,
                                                &SpbDevicePath,
                                                (GENERIC_READ | GENERIC_WRITE));

    Parameters.ShareAccess = 0;
    Parameters.CreateDisposition = FILE_OPEN;
    Parameters.FileAttributes = FILE_ATTRIBUTE_NORMAL;

    //
    // Open the SPB IO target. This creates a handle to the I2C controller
    // behind which SimGPO resides.
    //

    Status = WdfIoTargetOpen(GpioContext->SpbIoTarget, &Parameters);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPB,
            "WdfIoTargetOpen failed to open SPB target Status:%#x",
            Status);

        goto SetupSpbConnectionEnd;
    }

    //
    // Create a SPB request for writes and reads to the IO target.
    //

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&RequestAttributes,               \
                                            SIM_GPIO_REQUEST_CONTEXT);

    Status = WdfRequestCreate(&RequestAttributes,
                              NULL,
                              &GpioContext->SpbRequest);

    if (!NT_SUCCESS(Status)) {
        TraceEvents(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPB,
            "%s: WdfRequestCreate failed creating SpbRequest Status:%#x",
            __FUNCTION__,
            Status);

        goto SetupSpbConnectionEnd;
    }

    //
    // Initialize the request context with default values
    //

    RequestContext = GetRequestContext(GpioContext->SpbRequest);
    RequestContext->Device = GpioContext->Device;
    RequestContext->SequenceRequest = FALSE;

SetupSpbConnectionEnd:
    return Status;
}

VOID
SimGpioDestroySpbConnection (
    _In_ PSIM_GPIO_CONTEXT GpioContext
    )

/*++

Routine Description:

    This routine closes the SPB I/O target and releases SPB-related resources.

    N.B. This function is not marked pageable because this function is in
         the device power down path.

Arguments:

    GpioContext - Supplies a pionter to the client driver's device context.

Return Value:

    None.

--*/

{

    //
    // Delete the SPB request.
    //

    if (GpioContext->SpbRequest != NULL) {
        WdfObjectDelete(GpioContext->SpbRequest);
    }

    //
    // Delete the IO target. Note this will also close if it is opened.
    //

    if (GpioContext->SpbIoTarget != NULL) {
        WdfObjectDelete(GpioContext->SpbIoTarget);
    }

    return;
}

NTSTATUS
SimGpioSpbReadByte (
    _In_ PSIM_GPIO_BANK GpioBank,
    _In_ SIM_GPIO_REGISTER_ADDRESS RegisterAddress,
    _Out_writes_(sizeof(UCHAR)) PUCHAR Data
    )

/*++

Routine Description:

    This routine performs a single-byte read from the SPB I/O target.

    N.B. This routine is called at PASSIVE_LEVEL for off-SoC GPIOs but is not
         marked as PAGED_CODE as it could be executed late in the hibernate or
         early in resume sequence (or the deep-idle sequence).

Arguments:

    GpioBank - Supplies a pionter to the GPIO bank to be read from.

    RegisterAddress - Supplies the bank-relative register address to be read.

    Data - Supplies the byte buffer to read the data into.

Return Value:

    NTSTATUS code.

--*/

{

    USHORT ActualAddress;
    PSIM_GPIO_CONTEXT GpioContext;
    NTSTATUS Status;

    if (RegisterAddress >= MaximumSimGpioAddress) {
        Status = STATUS_NOT_SUPPORTED;
        goto SpbReadByteEnd;
    }

    ActualAddress = GpioBank->AddressBase + (USHORT)RegisterAddress;
    GpioContext = GpioBank->GpioContext;
    Status = SimGpioSpbRead(GpioContext, RegisterAddress, Data, sizeof(UCHAR));

SpbReadByteEnd:
    return Status;
}

NTSTATUS
SimGpioSpbRead (
    _In_ PSIM_GPIO_CONTEXT GpioContext,
    _In_ SIM_GPIO_REGISTER_ADDRESS RegisterAddress,
    _Out_writes_(DataLength) PUCHAR Data,
    _In_ USHORT  DataLength
    )

/*++

Routine Description:

    This routine performs a read from the SPB I/O target.

    N.B. This routine is called at PASSIVE_LEVEL for off-SoC GPIOs but is not
         marked as PAGED_CODE as it could be executed late in the hibernate or
         early in resume sequence (or the deep-idle sequence).

Arguments:

    GpioContext - Supplies a pionter to the client driver's device context.

    RegisterAddress - Supplies the absolute register address to read from.

    Data - Supplies the output byte buffer containing the data read.

    DataLength - Supplies the length of the byte buffer data.

Return Value:

    NTSTATUS code.

--*/

{

    USHORT Address;
    UCHAR Index;
    SPB_TRANSFER_LIST_AND_ENTRIES(2) Sequence;
    NTSTATUS Status;

    //
    // Build the SPB sequence (send address, read data).
    //

    Address = RegisterAddress;
    SPB_TRANSFER_LIST_INIT(&(Sequence.List), 2);
    Index = 0; // silence prefast
    Sequence.List.Transfers[Index] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(
                                         SpbTransferDirectionToDevice,
                                         0,
                                         &Address,
                                         SIM_GPIO_REGISTER_ADDRESS_SIZE);

    Index += 1; // silence prefast
    Sequence.List.Transfers[Index] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(
                                         SpbTransferDirectionFromDevice,
                                         0x0,
                                         Data,
                                         DataLength);

    //
    // Send the read as a sequence request to the SPB target.
    //

    Status = SimGpioSpbSequence(GpioContext, &Sequence, sizeof(Sequence));
    if (!NT_SUCCESS(Status)) {
        TraceEvents(TRACE_LEVEL_ERROR,
                    TRACE_FLAG_SPB,
                    "%s: SpbSequence failed sending a read sequence! "
                    "Status:%#x\n",
                    __FUNCTION__,
                    Status);
    }

    return Status;
}

NTSTATUS
SimGpioSpbWriteByte (
    _In_ PSIM_GPIO_BANK GpioBank,
    _In_ SIM_GPIO_REGISTER_ADDRESS RegisterAddress,
    _In_ UCHAR Data
    )

/*++

Routine Description:

    This routine performs a single-byte write to the SPB I/O target.

    N.B. This routine is called at PASSIVE_LEVEL for off-SoC GPIOs but is not
         marked as PAGED_CODE as it could be executed late in the hibernate or
         early in resume sequence (or the deep-idle sequence).

Arguments:

    GpioBank - Supplies a pionter to the GPIO bank to be written to.

    RegisterAddress - Supplies the bank-relative register address to write to.

    Data - Supplies the data to be written.

Return Value:

    NTSTATUS code.

--*/

{

    USHORT ActualAddress;
    PSIM_GPIO_CONTEXT GpioContext;
    NTSTATUS Status;

    if (RegisterAddress >= MaximumSimGpioAddress) {
        Status = STATUS_NOT_SUPPORTED;
        goto SpbWriteByteEnd;
    }

    ActualAddress = GpioBank->AddressBase + (USHORT)RegisterAddress;
    GpioContext = GpioBank->GpioContext;
    Status = SimGpioSpbWrite(GpioContext, RegisterAddress, &Data, sizeof(UCHAR));

SpbWriteByteEnd:
    return Status;
}

NTSTATUS
SimGpioSpbWrite (
    _In_ PSIM_GPIO_CONTEXT GpioContext,
    _In_ USHORT RegisterAddress,
    _In_reads_(DataLength) PUCHAR Data,
    _In_ ULONG DataLength
    )

/*++

Routine Description:

    This routine performs a write to the SPB I/O target.

    N.B. This routine is called at PASSIVE_LEVEL for off-SoC GPIOs but is not
         marked as PAGED_CODE as it could be executed late in the hibernate or
         early in resume sequence (or the deep-idle sequence).

Arguments:

    GpioContext - Supplies a pionter to the client driver's device context.

    RegisterAddress - Supplies the absolute register address to write to.

    Data - Supplies the byte buffer containing the data to be written.

    DataLength - Supplies the length of the write buffer.

Return Value:

    NTSTATUS code.

--*/

{

    PUCHAR Buffer;
    ULONG BufferLength;
    ULONG_PTR BytesWritten;
    WDF_MEMORY_DESCRIPTOR MemoryDescriptor;
    WDFMEMORY MemoryWrite;
    NTSTATUS Status;

    MemoryWrite = NULL;
    if ((Data == NULL) || (DataLength <= 0)) {
        Status = STATUS_INVALID_PARAMETER;
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_SPB,
                "%s: Invalid write request! Data:%p Length:%lu Status:%#x\n",
                __FUNCTION__,
                Data,
                DataLength,
                Status);

        goto SpbWriteEnd;
    }

    //
    // A SPB write-write is a single write request with the register
    // and data combined in one buffer. So we need to allocate memory
    // for the size of a register + data length

    BufferLength = DataLength + SIM_GPIO_REGISTER_ADDRESS_SIZE;
    Status = WdfMemoryCreate(WDF_NO_OBJECT_ATTRIBUTES,
                             NonPagedPoolNx,
                             SIM_GPIO_POOL_TAG,
                             BufferLength,
                             &MemoryWrite,
                             (PVOID*)&Buffer);

    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_SPB,
                "%s: WdfMemoryCreate failed allocating memory buffer for write!"
                "Status:%#x\n",
                __FUNCTION__,
                Status);

        goto SpbWriteEnd;
    }

    //
    // Setup the write buffer. The buffer should contain address followed by
    // data.
    //

    RtlCopyMemory(Buffer, &RegisterAddress, SIM_GPIO_REGISTER_ADDRESS_SIZE);
    RtlCopyMemory((Buffer + SIM_GPIO_REGISTER_ADDRESS_SIZE), Data, DataLength);

    //
    // Send the request synchronously.
    //

    WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&MemoryDescriptor, MemoryWrite, NULL);
    Status = WdfIoTargetSendWriteSynchronously(
                            GpioContext->SpbIoTarget,
                            NULL,
                            &MemoryDescriptor,
                            NULL,
                            NULL,
                            &BytesWritten);

    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_SPB,
                "%s: WdfIoTargetSendWriteSynchronously failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto SpbWriteEnd;
    }

SpbWriteEnd:
    if (MemoryWrite != NULL) {
        WdfObjectDelete(MemoryWrite);
    }

    return Status;
}

NTSTATUS
SimGpioSpbSequence (
    _In_ PSIM_GPIO_CONTEXT GpioContext,
    _In_reads_(SequenceLength) PVOID Sequence,
    _In_ SIZE_T SequenceLength
    )

/*++

Routine Description:

    This routine issues a sequence read-write request to the SPB I/O target.

    N.B. This routine is called at PASSIVE_LEVEL for off-SoC GPIOs but is not
         marked as PAGED_CODE as it could be executed late in the hibernate or
         early in resume sequence (or the deep-idle sequence).

Arguments:

    GpioContext - Supplies a pionter to the client driver's device context.

    Sequence - Supplies a pointer to a list of sequence transfers.

    SequenceLength - Supplies the length of sequence transfers.

Return Value:

    NTSTATUS code.

--*/

{

    WDF_OBJECT_ATTRIBUTES Attributes;
    ULONG_PTR BytesReturned;
    WDF_MEMORY_DESCRIPTOR MemoryDescriptor;
    WDFMEMORY MemorySequence;
    NTSTATUS Status;

    if ((Sequence == NULL) || (SequenceLength == 0)) {
        Status = STATUS_INVALID_PARAMETER;
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_SPB,
                "%s: Invalid sequence request! Sequence:%p Length:%lu Status:%#x\n",
                __FUNCTION__,
                Sequence,
                (ULONG)SequenceLength,
                Status);

        goto SpbSequenceEnd;
    }

    //
    // Create preallocated WDFMEMORY.
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Status = WdfMemoryCreatePreallocated(
                            &Attributes,
                            Sequence,
                            SequenceLength,
                            &MemorySequence);

    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_SPB,
                "%s: WdfMemoryCreatePreallocated failed! Status:%#x\n",
                __FUNCTION__,
                Status);

        goto SpbSequenceEnd;
    }

    WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&MemoryDescriptor, MemorySequence, NULL);

    //
    // Send the SPB sequence IOCTL.
    //

    Status = WdfIoTargetSendIoctlSynchronously(
                            GpioContext->SpbIoTarget,
                            NULL,
                            IOCTL_SPB_EXECUTE_SEQUENCE,
                            &MemoryDescriptor,
                            NULL,
                            NULL,
                            &BytesReturned);

    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_SPB,
                "%s: Failed sending SPB sequence request! Bytes:%lu Status:%#x",
                __FUNCTION__,
                (ULONG)BytesReturned,
                Status);

        goto SpbSequenceEnd;
    }

SpbSequenceEnd:
    return Status;
}


