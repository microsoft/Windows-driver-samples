/*++

Module Name:

    I2C.c

Abstract:

    This file contains the definitions for I2C functions and callbacks.

    TODO: If your port controller hardware is not compliant with the Type-C Port Controller
    Interface Specification in the respect that it does not use I2C,
    you will need to modify this file accordingly.

Environment:

    Kernel-mode Driver Framework

--*/

#include "Driver.h"
#include "I2C.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, I2CInitialize)
#pragma alloc_text (PAGE, I2COpen)
#pragma alloc_text (PAGE, I2CClose)
#pragma alloc_text (PAGE, I2CReadSynchronously)
#pragma alloc_text (PAGE, I2CWriteSynchronously)
#pragma alloc_text (PAGE, I2CPerformDeviceReset)
#pragma alloc_text (PAGE, I2CReadSynchronouslyMultiple)
#endif

NTSTATUS
I2CInitialize(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated
)
/*++

Routine Description:

    Initialize the I2C resource that provides a communications channel to the
    port controller hardware.

Arguments:

    DeviceContext - Context for a framework device.

    ResourcesRaw - A handle to a framework resource-list object that identifies the raw hardware
    resources that the Plug and Play manager has assigned to the device.

    ResourcesTranslated - A handle to a framework resource-list object that identifies the
    translated hardware resources that the Plug and Play manager has assigned to the device.

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNC_ENTRY(TRACE_I2C);

    UNREFERENCED_PARAMETER(ResourcesRaw);
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;
    WDF_INTERRUPT_CONFIG interruptConfig;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor = nullptr;
    ULONG interruptIndex = 0;
    BOOLEAN connFound = FALSE;
    BOOLEAN interruptFound = FALSE;
    ULONG resourceCount;

    // Check for I2C and Interrupt resources from the resources that PnP manager has
    // allocated to our device.
    resourceCount = WdfCmResourceListGetCount(ResourcesTranslated);

    for (ULONG i = 0; ((connFound == FALSE) || (interruptFound == FALSE)) && (i < resourceCount); i++)
    {
        descriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);

        switch (descriptor->Type)
        {
        case CmResourceTypeConnection:
            // Check for I2C resource
            if (descriptor->u.Connection.Class == CM_RESOURCE_CONNECTION_CLASS_SERIAL &&
                descriptor->u.Connection.Type == CM_RESOURCE_CONNECTION_TYPE_SERIAL_I2C)
            {
                DeviceContext->I2CConnectionId.LowPart = descriptor->u.Connection.IdLowPart;
                DeviceContext->I2CConnectionId.HighPart = descriptor->u.Connection.IdHighPart;

                connFound = TRUE;

                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_I2C,
                    "I2C resource found with connection id: 0x%llx",
                    DeviceContext->I2CConnectionId.QuadPart);
            }
            break;

        case CmResourceTypeInterrupt:
            // We've found an interrupt resource.
            interruptFound = TRUE;
            interruptIndex = i;
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_I2C,
                "Interrupt resource found at index: %lu", interruptIndex);
            break;

        default:
            // We don't care about other descriptors.
            break;
        }
    }

    // Fail if either connection or interrupt resource was not found.
    if (!connFound)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "Failed finding required I2C resource. Status: %!STATUS!", status);

        goto Exit;
    }

    if (!interruptFound)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "Failed finding required interrupt resource. Status: %!STATUS!", status);

        goto Exit;
    }

    // The alerts from the port controller hardware will be handled in a passive ISR.
    // The ISR performs hardware read and write operations which block until the hardware access is complete.
    // Waiting is unacceptable at DIRQL, so we perform our ISR at PASSIVE_LEVEL.
    WDF_INTERRUPT_CONFIG_INIT(&interruptConfig, OnInterruptPassiveIsr, NULL);

    interruptConfig.PassiveHandling = TRUE;
    interruptConfig.InterruptTranslated = WdfCmResourceListGetDescriptor(ResourcesTranslated, interruptIndex);
    interruptConfig.InterruptRaw = WdfCmResourceListGetDescriptor(ResourcesRaw, interruptIndex);

    status = WdfInterruptCreate(
        DeviceContext->Device,
        &interruptConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &DeviceContext->AlertInterrupt);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "WdfInterruptCreate failed. status: %!STATUS!", status);
        goto Exit;
    }

Exit:
    TRACE_FUNC_EXIT(TRACE_I2C);
    return status;
}

NTSTATUS
I2COpen(
    _In_ PDEVICE_CONTEXT DeviceContext
)
/*++

Routine Description:

    This routine opens a handle to the I2C controller.

Arguments:

    DeviceContext - a pointer to the device context

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNC_ENTRY(TRACE_I2C);

    PAGED_CODE();

    NTSTATUS status;
    WDF_IO_TARGET_OPEN_PARAMS openParams;
    WDF_OBJECT_ATTRIBUTES requestAttributes;
    WDF_OBJECT_ATTRIBUTES workitemAttributes;
    WDF_WORKITEM_CONFIG workitemConfig;

    // Create the device path using the connection ID.
    DECLARE_UNICODE_STRING_SIZE(DevicePath, RESOURCE_HUB_PATH_SIZE);

    RESOURCE_HUB_CREATE_PATH_FROM_ID(
        &DevicePath,
        DeviceContext->I2CConnectionId.LowPart,
        DeviceContext->I2CConnectionId.HighPart);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_I2C,
        "Opening handle to I2C target via %wZ", &DevicePath);

    status = WdfIoTargetCreate(DeviceContext->Device, WDF_NO_OBJECT_ATTRIBUTES, &DeviceContext->I2CIoTarget);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "WdfIoTargetCreate failed - %!STATUS!", status);
        goto Exit;
    }

    // Open a handle to the I2C controller.
    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
        &openParams,
        &DevicePath,
        (GENERIC_READ | GENERIC_WRITE));

    openParams.ShareAccess = 0;
    openParams.CreateDisposition = FILE_OPEN;
    openParams.FileAttributes = FILE_ATTRIBUTE_NORMAL;

    status = WdfIoTargetOpen(DeviceContext->I2CIoTarget, &openParams);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "Failed to open I2C I/O target - %!STATUS!", status);
        goto Exit;
    }

    // Create a WDFMEMORY object. Do call WdfMemoryAssignBuffer before use it,
    status = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        static_cast<PVOID>(&status), // initial value does not matter
        sizeof(status),
        &DeviceContext->I2CMemory);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "WdfMemoryCreatePreallocated failed with status %!STATUS!", status);
        goto Exit;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&requestAttributes);
    requestAttributes.ParentObject = DeviceContext->I2CIoTarget;

    for (ULONG i = 0; i < I2CRequestSourceMax; i++)
    {
        status = WdfRequestCreate(&requestAttributes, DeviceContext->I2CIoTarget, &DeviceContext->OutgoingRequests[i]);
        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
                "WdfRequestCreate failed with status %!STATUS!", status);
            goto Exit;
        }
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&workitemAttributes, WORKITEM_CONTEXT);
    workitemAttributes.ParentObject = DeviceContext->I2CIoTarget;

    WDF_WORKITEM_CONFIG_INIT(&workitemConfig, EvtWorkItemGetStatus);
    status = WdfWorkItemCreate(&workitemConfig, &workitemAttributes, &DeviceContext->I2CWorkItemGetStatus);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "WdfWorkItemCreate failed with status %!STATUS!", status);
        goto Exit;
    }

    WDF_WORKITEM_CONFIG_INIT(&workitemConfig, EvtWorkItemGetControl);
    status = WdfWorkItemCreate(&workitemConfig, &workitemAttributes, &DeviceContext->I2CWorkItemGetControl);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "WdfWorkItemCreate failed with status %!STATUS!", status);
        goto Exit;
    }

Exit:
    TRACE_FUNC_EXIT(TRACE_I2C);
    return status;
}

void
I2CClose(
    _In_ PDEVICE_CONTEXT DeviceContext
)
/*++

Routine Description:

    This routine closes a handle to the I2C controller.

Arguments:

    DeviceContext - a pointer to the device context.

--*/
{
    TRACE_FUNC_ENTRY(TRACE_I2C);

    PAGED_CODE();

    if (DeviceContext->I2CIoTarget != WDF_NO_HANDLE)
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_I2C, "Closing handle to I2C target");
        WdfIoTargetClose(DeviceContext->I2CIoTarget);
    }

    TRACE_FUNC_EXIT(TRACE_I2C);
}

NTSTATUS
I2CWriteAsynchronously(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ UINT8 RegisterAddress,
    _In_reads_bytes_(Length) PVOID Data,
    _In_ ULONG Length
)
/*++

Routine Description:

    Sends data to the I2C controller to write to the specified register
    on the port controller hardware.

    Before calling, DeviceContext->IncomingRequest should be set to a valid value. The IncomingRequest
    will be completed automatically when writing is finished.

Arguments:

    DeviceContext - Context information about the device.

    RegisterAddress - The I2C register address to write data to.

    Data - Pointer to the data to write.

    Length - Length of the data to write.

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNC_ENTRY(TRACE_I2C);

    NTSTATUS status;
    WDF_REQUEST_REUSE_PARAMS reuseParams;

    if (Length == 0)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C, "Parameter 'Length' cannot be 0.");
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (REGISTER_ADDR_SIZE + Length > sizeof(DeviceContext->I2CAsyncBuffer))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C, "Unexpected value of data length. Length: %lu. Size of buffer: %lu", Length, sizeof(DeviceContext->I2CAsyncBuffer));
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    // Reuse the preallocated WDFREQUEST for I2C.
    WDF_REQUEST_REUSE_PARAMS_INIT(&reuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);
    status = WdfRequestReuse(DeviceContext->I2CAsyncRequest, &reuseParams);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "WdfRequestReuse failed with status %!STATUS!", status);
        goto Exit;
    }

    WdfRequestSetCompletionRoutine(DeviceContext->I2CAsyncRequest, I2COnWriteCompletion, DeviceContext);

    // Combine register address and user data to write into a single buffer.
    DeviceContext->I2CAsyncBuffer[0] = RegisterAddress;

    RtlCopyMemory(&DeviceContext->I2CAsyncBuffer[REGISTER_ADDR_SIZE], Data, Length);

    status = WdfMemoryAssignBuffer(
        DeviceContext->I2CMemory,
        static_cast<PVOID>(DeviceContext->I2CAsyncBuffer),
        REGISTER_ADDR_SIZE + Length);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "WdfMemoryAssignBuffer failed with status %!STATUS!", status);
    }

    status = WdfIoTargetFormatRequestForWrite(
        DeviceContext->I2CIoTarget,
        DeviceContext->I2CAsyncRequest,
        DeviceContext->I2CMemory,
        NULL,
        NULL);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "WdfIoTargetFormatRequestForWrite failed with status %!STATUS!", status);
        goto Exit;
    }

    // Send the request to the I2C I/O Target.
    if (WdfRequestSend(DeviceContext->I2CAsyncRequest, DeviceContext->I2CIoTarget, NULL) == FALSE)
    {
        status = WdfRequestGetStatus(DeviceContext->I2CAsyncRequest);
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "[WDFREQUEST: 0x%p] WdfRequestSend for I2C write failed with status %!STATUS!",
            DeviceContext->I2CAsyncRequest, status);
        goto Exit;
    }

Exit:
    TRACE_FUNC_EXIT(TRACE_I2C);
    return status;
}

NTSTATUS
I2CReadAsynchronously(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ UINT8 RegisterAddress,
    _In_ ULONG Length
)
/*++

Routine Description:

    Asynchronously reads data from the port controller's registers over the I2C controller.

    Before calling, DeviceContext->IncomingRequest should be set to a valid value. The IncomingRequest
    will be completed automatically when reading is finished, and any data read out from the
    controller will be stored in the output buffer of IncomingRequest.

    Note: This function is not used in the sample yet. It is left here for future reference.

Arguments:

    DeviceContext - Context information for the port controller device.

    RegisterAddress - The I2C register address from which to read data.

    Length - Length of the data to read.

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNC_ENTRY(TRACE_I2C);

    NTSTATUS status;
    WDF_REQUEST_REUSE_PARAMS reuseParams;

    // Static analysis cannot figure out the SPB_TRANSFER_LIST_ENTRY
    // size but using an index variable quiets the warning.
    ULONG index = 0;

    // Store the address.
    DeviceContext->I2CRegisterAddress = RegisterAddress;

    if (Length == 0)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C, "Parameter 'Length' cannot be 0.");
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    // Reuse the specified WDFREQUEST
    WDF_REQUEST_REUSE_PARAMS_INIT(&reuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);
    status = WdfRequestReuse(DeviceContext->I2CAsyncRequest, &reuseParams);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "WdfRequestReuse failed with status %!STATUS!", status);
        goto Exit;
    }

    WdfRequestSetCompletionRoutine(DeviceContext->I2CAsyncRequest, I2COnReadCompletion, DeviceContext);

    // Prepare the I2C transfer.

    if (Length > sizeof(DeviceContext->I2CAsyncBuffer))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C, "Unexpected value of data length. Length: %lu. Size of buffer: %lu", Length, sizeof(DeviceContext->I2CAsyncBuffer));
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    SPB_TRANSFER_LIST_AND_ENTRIES(I2C_TRANSFER_COUNT) transferList;
    SPB_TRANSFER_LIST_INIT(&(transferList.List), I2C_TRANSFER_COUNT);

    // Static analysis can't figure out the relationship between the transfer array size
    // and the transfer count.
    _Analysis_assume_(ARRAYSIZE(transferList.List.Transfers) == I2C_TRANSFER_COUNT);

    transferList.List.Transfers[index] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(
        SpbTransferDirectionToDevice,
        0,
        &DeviceContext->I2CRegisterAddress,
        REGISTER_ADDR_SIZE);

    transferList.List.Transfers[index + 1] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(
        SpbTransferDirectionFromDevice,
        0,
        DeviceContext->I2CAsyncBuffer,
        Length);

    // The IOCTL is METHOD_BUFFERED, so the memory (transferList) doesn't
    // have to persist until the request is completed.
    status = WdfMemoryAssignBuffer(
        DeviceContext->I2CMemory,
        static_cast<PVOID>(&transferList),
        sizeof(transferList));
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "WdfMemoryAssignBuffer failed with status %!STATUS!", status);
    }

    status = WdfIoTargetFormatRequestForIoctl(
        DeviceContext->I2CIoTarget,
        DeviceContext->I2CAsyncRequest,
        IOCTL_SPB_EXECUTE_SEQUENCE,
        DeviceContext->I2CMemory,
        NULL,
        NULL,
        NULL);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "WdfIoTargetFormatRequestForIoctl failed with status %!STATUS!", status);
        goto Exit;
    }

    // Send the request to the I2C I/O Target.
    if (WdfRequestSend(DeviceContext->I2CAsyncRequest, DeviceContext->I2CIoTarget, NULL) == FALSE)
    {
        status = WdfRequestGetStatus(DeviceContext->I2CAsyncRequest);
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "[WDFREQUEST: 0x%p] WdfRequestSend for I2C read failed with status %!STATUS!",
            DeviceContext->I2CAsyncRequest, status);
        goto Exit;
    }

Exit:
    TRACE_FUNC_EXIT(TRACE_I2C);
    return status;
}

NTSTATUS
I2CReadSynchronously(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ I2C_REQUEST_SOURCE RequestSource,
    _In_ UINT8 RegisterAddress,
    _Out_writes_bytes_(Length) PVOID Data,
    _In_ ULONG Length
)
/*++

Routine Description:

    Synchronously reads data from the port controller's registers over the I2C controller
    for a request originating from the client driver.

Arguments:

    DeviceContext - Context information for the port controller device.

    RequestSource - Identify the caller so the correct WDFREQUEST can be re-used

    RegisterAddress - The I2C register address from which to read data.

    Length - Length of the data to read.

    Data - The data read from the registers.

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNC_ENTRY(TRACE_I2C);

    PAGED_CODE();

    NTSTATUS status;
    WDF_REQUEST_SEND_OPTIONS requestOptions;
    WDF_MEMORY_DESCRIPTOR memoryDescriptor;
    WDF_REQUEST_REUSE_PARAMS reuseParams;
    ULONG_PTR bytesTransferred = 0;
    UINT8 transferBuffer[I2C_BUFFER_SIZE];

    // Static analysis cannot figure out the SPB_TRANSFER_LIST_ENTRY
    // size but using an index variable quiets the warning.
    ULONG index = 0;

    WDFREQUEST request = DeviceContext->OutgoingRequests[RequestSource];

    // Reuse the preallocated WDFREQUEST for internal requests.
    WDF_REQUEST_REUSE_PARAMS_INIT(&reuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);
    status = WdfRequestReuse(request, &reuseParams);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "[WDFREQUEST: 0x%p] WdfRequestReuse for I2CSyncRequest failed with status %!STATUS!",
            request, status);
        goto Exit;
    }

    if (Length == 0)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C, "Parameter 'Length' cannot be 0.");
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (Length > sizeof(transferBuffer))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C, "Unexpected value of data length. Length: %lu. Size of buffer: %lu", Length, sizeof(transferBuffer));
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    // Prepare the I2C transfer.
    SPB_TRANSFER_LIST_AND_ENTRIES(I2C_TRANSFER_COUNT) transferList;
    SPB_TRANSFER_LIST_INIT(&(transferList.List), I2C_TRANSFER_COUNT);

    // Static analysis can't figure out the relationship between the transfer array size
    // and the transfer count.
    _Analysis_assume_(ARRAYSIZE(transferList.List.Transfers) == I2C_TRANSFER_COUNT);

    transferList.List.Transfers[index] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(
        SpbTransferDirectionToDevice,
        0,
        &RegisterAddress,
        REGISTER_ADDR_SIZE);

    transferList.List.Transfers[index + 1] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(
        SpbTransferDirectionFromDevice,
        0,
        transferBuffer,
        Length);

    // Initialize the memory descriptor with the transfer list.
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memoryDescriptor, &transferList, sizeof(transferList));

    WDF_REQUEST_SEND_OPTIONS_INIT(&requestOptions, WDF_REQUEST_SEND_OPTION_TIMEOUT);
    requestOptions.Timeout = WDF_REL_TIMEOUT_IN_MS(I2C_SYNCHRONOUS_TIMEOUT);

    // Send the request to the I2C I/O Target.
    status = WdfIoTargetSendIoctlSynchronously(DeviceContext->I2CIoTarget,
        request,
        IOCTL_SPB_EXECUTE_SEQUENCE,
        &memoryDescriptor,
        NULL,
        &requestOptions,
        &bytesTransferred);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "[WDFREQUEST: 0x%p] WdfIoTargetSendIoctlSynchronously failed with status %!STATUS!",
            request, status);

        // A synchronous I2C request failing is a good indicator that I2C is unresponsive.
        // Attempt to reset the device.
        I2CPerformDeviceReset(DeviceContext);

        goto Exit;
    }

    if (bytesTransferred != REGISTER_ADDR_SIZE + Length)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C, "Unexpected number of bytes transferred. Expected %lu, transferred %Iu.", REGISTER_ADDR_SIZE + Length, bytesTransferred);
        status = STATUS_INFO_LENGTH_MISMATCH;
        goto Exit;
    }

    // Get the returned data out of the buffer.
    RtlCopyMemory(Data, transferBuffer, Length);
Exit:
    TRACE_FUNC_EXIT(TRACE_I2C);
    return status;
}

NTSTATUS
I2CReadSynchronouslyMultiple(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ I2C_REQUEST_SOURCE requestSource,
    _Inout_updates_(Count) REGISTER_ITEM* Items,
    _In_ ULONG Count
)
/*++

Routine Description:

    Synchronously reads data from the port controller's registers over the I2C controller
    for a request originating from the client driver.

Arguments:

    DeviceContext - Context information for the port controller device.

    RequestSource - Identify the caller so the correct WDFREQUEST can be re-used

    Items - Array of (register, data, length) triplets

    Count - Count of elements from the Items array above

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNC_ENTRY(TRACE_I2C);

    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;
    PREGISTER_ITEM item;

    for (ULONG i = 0; i < Count; i++)
    {
        item = &Items[i];
        status = I2CReadSynchronously(DeviceContext,
            requestSource,
            item->RegisterAddress,
            item->Data,
            item->Length);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

Exit:
    TRACE_FUNC_EXIT(TRACE_I2C);
    return status;
}

NTSTATUS
I2CWriteSynchronously(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ I2C_REQUEST_SOURCE RequestSource,
    _In_ UINT8 RegisterAddress,
    _In_reads_(Length) PVOID Data,
    _In_ ULONG Length
)
/*++

Routine Description:

    Synchronously writes data from the port controller's registers over the I2C controller
    for a request originating from the client driver.

Arguments:

    DeviceContext - Context information for the port controller device.

    RequestSource - Identify the caller so the correct WDFREQUEST can be re-used

    RegisterAddress - The I2C register address from which to write data.

    Data - The data to write.

    Length - Length of the data to write.

Return Value:

    NTSTATUS

--*/
{
    TRACE_FUNC_ENTRY(TRACE_I2C);

    PAGED_CODE();

    NTSTATUS status;
    WDF_REQUEST_SEND_OPTIONS requestOptions;
    WDF_MEMORY_DESCRIPTOR memoryDescriptor;
    WDF_REQUEST_REUSE_PARAMS reuseParams;
    ULONG_PTR bytesTransferred = 0;
    UINT8 transferBuffer[I2C_BUFFER_SIZE];

    WDFREQUEST request = DeviceContext->OutgoingRequests[RequestSource];

    if (Length == 0)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C, "Parameter 'Length' cannot be 0.");
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    // Reuse the preallocated WDFREQUEST for internal requests.
    WDF_REQUEST_REUSE_PARAMS_INIT(&reuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);
    status = WdfRequestReuse(request, &reuseParams);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "[WDFREQUEST: 0x%p] WdfRequestReuse failed with status %!STATUS!",
            request, status);
        goto Exit;
    }

    transferBuffer[0] = RegisterAddress;

    if (REGISTER_ADDR_SIZE + Length > sizeof(transferBuffer))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C, "Unexpected value of length. Length: %lu. Size of buffer: %lu", Length, sizeof(transferBuffer));
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    RtlCopyMemory(&transferBuffer[REGISTER_ADDR_SIZE], Data, Length);

    // Initialize the memory descriptor with the write buffer.
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memoryDescriptor, &transferBuffer, REGISTER_ADDR_SIZE + Length);

    WDF_REQUEST_SEND_OPTIONS_INIT(&requestOptions, WDF_REQUEST_SEND_OPTION_TIMEOUT);
    requestOptions.Timeout = WDF_REL_TIMEOUT_IN_MS(I2C_SYNCHRONOUS_TIMEOUT);

    // Send the write request to the I2C I/O Target.
    status = WdfIoTargetSendWriteSynchronously(
        DeviceContext->I2CIoTarget,
        request,
        &memoryDescriptor,
        NULL,
        &requestOptions,
        &bytesTransferred);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "[WDFREQUEST: 0x%p] WdfIoTargetSendWriteSynchronously failed with status %!STATUS!",
            request, status);

        // A synchronous I2C request failing is a good indicator that I2C is unresponsive.
        // Attempt to reset the device.
        I2CPerformDeviceReset(DeviceContext);

        goto Exit;
    }

    if (bytesTransferred != REGISTER_ADDR_SIZE + Length)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C, "Unexpected number of bytes transferred. Expected %lu, transferred %Iu.", REGISTER_ADDR_SIZE + Length, bytesTransferred);
        status = STATUS_INFO_LENGTH_MISMATCH;
        goto Exit;
    }

Exit:
    TRACE_FUNC_EXIT(TRACE_I2C);
    return status;
}

VOID
I2COnReadCompletion(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context
)
/*++

Routine Description:

    Completion routine for hardware access after reading. Completes the WDFREQUEST from UcmTcpciCx.

Arguments:

    Request - A handle to a framework request object that represents the completed I/O request.

    Target - A handle to an I/O target object that represents the I/O target that completed the request.

    Params - A pointer to a WDF_REQUEST_COMPLETION_PARAMS structure that contains
    information about the completed request.

    Context - Driver-supplied context information,
    which the driver specified in a previous call to WdfRequestSetCompletionRoutine. In this case,
    it is of type PDEVICE_CONTEXT.

--*/
{
    TRACE_FUNC_ENTRY(TRACE_I2C);

    UNREFERENCED_PARAMETER(Target);

    NTSTATUS status;
    PDEVICE_CONTEXT deviceContext;
    PVOID outputBuffer;
    size_t readLength = 0;

    deviceContext = (PDEVICE_CONTEXT)Context;

    status = Params->IoStatus.Status;
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "[WDFREQUEST: 0x%p] I2CTcpciRequestOnReadCompletion - WDFREQUEST completed with failure status: %!STATUS!",
            Request, status);
        goto Exit;
    }

    readLength = Params->IoStatus.Information;
    if (readLength <= REGISTER_ADDR_SIZE)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "[WDFREQUEST: 0x%p] Invalid output buffer length: %Ix",
            Request, readLength);
        readLength = 0;
        goto Exit;
    }

    // Subtract REGISTER_ADDR_SIZE from readLength to account for only the read bytes.
    readLength -= REGISTER_ADDR_SIZE;

    // Retrieve the output buffer of the pending UcmTcpciCx request
    status = WdfRequestRetrieveOutputBuffer(deviceContext->IncomingRequest, readLength, &outputBuffer, NULL);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "WdfRequestRetrieveOutputBuffer failed. Status: %!STATUS!", status);
        goto Exit;
    }

    // Copy the read bytes into the output buffer of the WDFREQUEST from UcmTcpciCx.
    RtlCopyMemory(outputBuffer, deviceContext->I2CAsyncBuffer, readLength);

    WdfRequestSetInformation(deviceContext->IncomingRequest, readLength);

Exit:

    WdfRequestComplete(deviceContext->IncomingRequest, status);
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_I2C,
        "[WDFREQUEST: 0x%p] WDFREQUEST from UcmTcpciCx completed with status: %!STATUS!", deviceContext->IncomingRequest, status);

    TRACE_FUNC_EXIT(TRACE_I2C);
}

VOID
I2COnWriteCompletion(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context
)
/*++

Routine Description:

    Completion routine for hardware access after a write. Completes the WDFREQUEST from UcmTcpciCx.

Arguments:

    Request - A handle to a framework request object that represents the completed I/O request.

    Target - A handle to an I/O target object that represents the I/O target that completed the request.

    Params - A pointer to a WDF_REQUEST_COMPLETION_PARAMS structure that contains
    information about the completed request.

    Context - Driver-supplied context information,
    which the driver specified in a previous call to WdfRequestSetCompletionRoutine. In this case,
    it is of type PDEVICE_CONTEXT.

--*/
{
    TRACE_FUNC_ENTRY(TRACE_I2C);

    UNREFERENCED_PARAMETER(Target);

    NTSTATUS status;
    PDEVICE_CONTEXT deviceContext;

    deviceContext = (PDEVICE_CONTEXT)Context;

    status = Params->IoStatus.Status;
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
            "[WDFREQUEST: 0x%p] I2COnWriteCompletion - WDFREQUEST completed with failure status: %!STATUS!",
            Request, status);
        goto Exit;
    }

Exit:
    // Complete the WDFREQUEST from UcmTcpciCx.
    WdfRequestComplete(deviceContext->IncomingRequest, status);
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_I2C,
        "[WDFREQUEST: 0x%p] WDFREQUEST from UcmTcpciCx completed with status: %!STATUS!",
        deviceContext->IncomingRequest, status);

    TRACE_FUNC_EXIT(TRACE_I2C);
}

void
I2CPerformDeviceReset(
    _In_ PDEVICE_CONTEXT DeviceContext
)
/*++

Routine Description:

    Recovery mechanism for a malfunctioning I2C bus.
    Attempt a platform-level device reset.
    If unsuccessful or we have exceeded the maximum number of reset attempts, call WdfDeviceSetFailed.

Arguments:

    DeviceContext - Context information for the port controller device.

--*/
{
    TRACE_FUNC_ENTRY(TRACE_I2C);

    PAGED_CODE();

    NTSTATUS status;

    if (DeviceContext->ResetInterface.DeviceReset != NULL && DeviceContext->ResetAttempts <= MAX_DEVICE_RESET_ATTEMPTS)
    {
        // Attempt a platform-level device reset (PLDR) to recover from an I2C error.
        // This will disconnect the device from the power rail and reconnect it.

        ++DeviceContext->ResetAttempts;

        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_I2C,
            "[WDFDEVICE: 0x%p] Performing PlatformLevelDeviceReset", DeviceContext->Device);
        status = DeviceContext->ResetInterface.DeviceReset(DeviceContext->ResetInterface.Context, PlatformLevelDeviceReset, 0, NULL);
        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_I2C,
                "[WDFDEVICE: 0x%p] PlatformLevelDeviceReset failed with status: %!STATUS!", DeviceContext->Device, status);

            // If PLDR fails, perform WdfDeviceSetFailed.

            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_I2C,
                "[WDFDEVICE: 0x%p] Performing WdfDeviceSetFailed with WdfDeviceFailedAttemptRestart", DeviceContext->Device);

            WdfDeviceSetFailed(DeviceContext->Device, WdfDeviceFailedAttemptRestart);

            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_I2C,
                "[WDFDEVICE: 0x%p] WdfDeviceSetFailed complete", DeviceContext->Device);
        }
    }
    else
    {
        // Either platform-level device reset failed or DEVICE_RESET_INTERFACE_STANDARD was not
        // supported by the bus driver. Use WdfDeviceSetFailed and attempt to restart the device.
        // When the driver is reloaded, it will reinitialize I2C.
        // If several consecutive restart attempts fail (because the restarted driver again reports an error),
        // the framework stops trying to restart the device.

        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_I2C,
            "[WDFDEVICE: 0x%p] Performing WdfDeviceSetFailed with WdfDeviceFailedAttemptRestart",DeviceContext->Device);

        WdfDeviceSetFailed(DeviceContext->Device, WdfDeviceFailedAttemptRestart);

        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_I2C,
            "[WDFDEVICE: 0x%p] WdfDeviceSetFailed complete", DeviceContext->Device);
    }

    TRACE_FUNC_EXIT(TRACE_I2C);
}
