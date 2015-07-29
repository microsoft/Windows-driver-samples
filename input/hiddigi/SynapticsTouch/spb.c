/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved. 
    Sample code. Dealpoint ID #843729.

    Module Name:

        spb.c

    Abstract:

        Contains all I2C-specific functionality

    Environment:

        Kernel mode

    Revision History:

--*/

#include "internal.h"
#include "controller.h"
#include "spb.tmh"

NTSTATUS
SpbDoWriteDataSynchronously(
    IN SPB_CONTEXT *SpbContext,
    IN UCHAR Address,
    IN PVOID Data,
    IN ULONG Length
    )
/*++
 
  Routine Description:

    This helper routine abstracts creating and sending an I/O
    request (I2C Write) to the Spb I/O target.

  Arguments:

    SpbContext - Pointer to the current device context 
    Address    - The I2C register address to write to
    Data       - A buffer to receive the data at at the above address
    Length     - The amount of data to be read from the above address

  Return Value:

    NTSTATUS Status indicating success or failure

--*/
{
    PUCHAR buffer;
    ULONG length;
    WDFMEMORY memory;
    WDF_MEMORY_DESCRIPTOR memoryDescriptor;
    NTSTATUS status;

    //
    // The address pointer and data buffer must be combined
    // into one contiguous buffer representing the write transaction.
    //
    length = Length + 1;
    memory = NULL;

    if (length > DEFAULT_SPB_BUFFER_SIZE)
    {
        status = WdfMemoryCreate(
            WDF_NO_OBJECT_ATTRIBUTES,
            NonPagedPool,
            TOUCH_POOL_TAG,
            length,
            &memory,
            &buffer);

        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_SPB,
                "Error allocating memory for Spb write - %!STATUS!",
                status);
            goto exit;
        }

        WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(
            &memoryDescriptor,
            memory,
            NULL);
    }
    else
    {
        buffer = (PUCHAR) WdfMemoryGetBuffer(SpbContext->WriteMemory, NULL);

        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
            &memoryDescriptor,
            (PVOID) buffer,
            length);
    }

    //
    // Transaction starts by specifying the address bytes
    //
    RtlCopyMemory(buffer, &Address, sizeof(Address));

    //
    // Address is followed by the data payload
    //
    RtlCopyMemory((buffer+sizeof(Address)), Data, length-sizeof(Address));

    status = WdfIoTargetSendWriteSynchronously(
        SpbContext->SpbIoTarget,
        NULL,
        &memoryDescriptor,
        NULL,
        NULL,
        NULL);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPB,
            "Error writing to Spb - %!STATUS!",
            status);
        goto exit;
    }

exit:

    if (NULL != memory)
    {
        WdfObjectDelete(memory);
    }

    return status;
}

NTSTATUS
SpbWriteDataSynchronously(
    IN SPB_CONTEXT *SpbContext,
    IN UCHAR Address,
    IN PVOID Data,
    IN ULONG Length
    )
/*++
 
  Routine Description:

    This routine abstracts creating and sending an I/O
    request (I2C Write) to the Spb I/O target and utilizes
    a helper routine to do work inside of locked code.

  Arguments:

    SpbContext - Pointer to the current device context 
    Address    - The I2C register address to write to
    Data       - A buffer to receive the data at at the above address
    Length     - The amount of data to be read from the above address

  Return Value:

    NTSTATUS Status indicating success or failure

--*/
{
    NTSTATUS status;

    WdfWaitLockAcquire(SpbContext->SpbLock, NULL);

    status = SpbDoWriteDataSynchronously(
        SpbContext, 
        Address, 
        Data, 
        Length);

    WdfWaitLockRelease(SpbContext->SpbLock);

    return status;
}

NTSTATUS 
SpbReadDataSynchronously(
    _In_ SPB_CONTEXT *SpbContext,
    _In_ UCHAR Address,
    _In_reads_bytes_(Length) PVOID Data,
    _In_ ULONG Length
    )
/*++
 
  Routine Description:

    This helper routine abstracts creating and sending an I/O
    request (I2C Read) to the Spb I/O target.

  Arguments:

    SpbContext - Pointer to the current device context 
    Address    - The I2C register address to read from
    Data       - A buffer to receive the data at at the above address
    Length     - The amount of data to be read from the above address

  Return Value:

    NTSTATUS Status indicating success or failure

--*/
{
    PUCHAR buffer;
    WDFMEMORY memory;
    WDF_MEMORY_DESCRIPTOR memoryDescriptor;
    NTSTATUS status;
    ULONG_PTR bytesRead;

    WdfWaitLockAcquire(SpbContext->SpbLock, NULL);

    memory = NULL;
    status = STATUS_INVALID_PARAMETER;
    bytesRead = 0;

    //
    // Read transactions start by writing an address pointer
    //
    status = SpbDoWriteDataSynchronously(
        SpbContext,
        Address,
        NULL,
        0);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPB,
            "Error setting address pointer for Spb read - %!STATUS!",
            status);
        goto exit;
    }

    if (Length > DEFAULT_SPB_BUFFER_SIZE)
    {
        status = WdfMemoryCreate(
            WDF_NO_OBJECT_ATTRIBUTES,
            NonPagedPool,
            TOUCH_POOL_TAG,
            Length,
            &memory,
            &buffer);

        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_SPB,
                "Error allocating memory for Spb read - %!STATUS!",
                status);
            goto exit;
        }

        WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(
            &memoryDescriptor,
            memory,
            NULL);
    }
    else
    {
        buffer = (PUCHAR) WdfMemoryGetBuffer(SpbContext->ReadMemory, NULL);

        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
            &memoryDescriptor,
            (PVOID) buffer,
            Length);
    }


    status = WdfIoTargetSendReadSynchronously(
        SpbContext->SpbIoTarget,
        NULL,
        &memoryDescriptor,
        NULL,
        NULL,
        &bytesRead);

    if (!NT_SUCCESS(status) || 
        bytesRead != Length)
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPB,
            "Error reading from Spb - %!STATUS!",
            status);
        goto exit;
    }

    //
    // Copy back to the caller's buffer
    //
    RtlCopyMemory(Data, buffer, Length);

exit:
    if (NULL != memory)
    {
       WdfObjectDelete(memory);
    }

    WdfWaitLockRelease(SpbContext->SpbLock);
   
    return status;
}

VOID
SpbTargetDeinitialize(
    IN WDFDEVICE FxDevice,
    IN SPB_CONTEXT *SpbContext
    )
/*++
 
  Routine Description:

    This helper routine is used to free any members added to the SPB_CONTEXT,
    note the SPB I/O target is parented to the device and will be
    closed and free'd when the device is removed.
 
  Arguments:

    FxDevice   - Handle to the framework device object
    SpbContext - Pointer to the current device context 

  Return Value:

    NTSTATUS Status indicating success or failure

--*/
{
    UNREFERENCED_PARAMETER(FxDevice);
    UNREFERENCED_PARAMETER(SpbContext);
    
    //
    // Free any SPB_CONTEXT allocations here
    //
    if (SpbContext->SpbLock != NULL)
    {
        WdfObjectDelete(SpbContext->SpbLock);
    }

    if (SpbContext->ReadMemory != NULL)
    {
        WdfObjectDelete(SpbContext->ReadMemory);
    }

    if (SpbContext->WriteMemory != NULL)
    {
        WdfObjectDelete(SpbContext->WriteMemory);
    }
}

NTSTATUS
SpbTargetInitialize(
    IN WDFDEVICE FxDevice,
    IN SPB_CONTEXT *SpbContext
    )
/*++
 
  Routine Description:

    This helper routine opens the Spb I/O target and 
    initializes a request object used for the lifetime
    of communication between this driver and Spb.

  Arguments:

    FxDevice   - Handle to the framework device object
    SpbContext - Pointer to the current device context 

  Return Value:

    NTSTATUS Status indicating success or failure

--*/
{
    WDF_OBJECT_ATTRIBUTES objectAttributes;
    WDF_IO_TARGET_OPEN_PARAMS openParams;
    UNICODE_STRING spbDeviceName;
    WCHAR spbDeviceNameBuffer[RESOURCE_HUB_PATH_SIZE];
    NTSTATUS status;
    
    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
    objectAttributes.ParentObject = FxDevice;

    status = WdfIoTargetCreate(
        FxDevice,
        &objectAttributes,
        &SpbContext->SpbIoTarget);
    
    if (!NT_SUCCESS(status)) 
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPB,
            "Error creating IoTarget object - %!STATUS!", 
            status);

        WdfObjectDelete(SpbContext->SpbIoTarget);
        goto exit;
    }

    RtlInitEmptyUnicodeString(
        &spbDeviceName,
        spbDeviceNameBuffer,
        sizeof(spbDeviceNameBuffer));

    status = RESOURCE_HUB_CREATE_PATH_FROM_ID(
        &spbDeviceName,
        SpbContext->I2cResHubId.LowPart,
        SpbContext->I2cResHubId.HighPart);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPB,
            "Error creating Spb resource hub path string - %!STATUS!",
            status);
        goto exit;
    }

    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
        &openParams,
        &spbDeviceName,
        (GENERIC_READ | GENERIC_WRITE));

    openParams.ShareAccess = 0;
    openParams.CreateDisposition = FILE_OPEN;
    openParams.FileAttributes = FILE_ATTRIBUTE_NORMAL;

    status = WdfIoTargetOpen(SpbContext->SpbIoTarget, &openParams);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPB,
            "Error opening Spb target for communication - %!STATUS!", 
            status);
        goto exit;
    }

    //
    // Allocate some fixed-size buffers from NonPagedPool for typical
    // Spb transaction sizes to avoid pool fragmentation in most cases
    //
    status = WdfMemoryCreate(
        WDF_NO_OBJECT_ATTRIBUTES,
        NonPagedPool,
        TOUCH_POOL_TAG,
        DEFAULT_SPB_BUFFER_SIZE,
        &SpbContext->WriteMemory,
        NULL);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPB,
            "Error allocating default memory for Spb write - %!STATUS!",
            status);
        goto exit;
    }

    status = WdfMemoryCreate(
        WDF_NO_OBJECT_ATTRIBUTES,
        NonPagedPool,
        TOUCH_POOL_TAG,
        DEFAULT_SPB_BUFFER_SIZE,
        &SpbContext->ReadMemory,
        NULL);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPB,
            "Error allocating default memory for Spb read - %!STATUS!",
            status);
        goto exit;
    }

    //
    // Allocate a waitlock to guard access to the default buffers
    //
    status = WdfWaitLockCreate(
        WDF_NO_OBJECT_ATTRIBUTES,
        &SpbContext->SpbLock);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPB,
            "Error creating Spb Waitlock - %!STATUS!",
            status);
        goto exit;
    }

exit:

    if (!NT_SUCCESS(status))
    {
        SpbTargetDeinitialize(FxDevice, SpbContext);
    }

    return status;
}
