/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Acpi.cpp

Abstract:

    ACPI method evaluation helper routines.

Environment:

    Kernel-mode only.

--*/

#include "Pch.h"
#include "Acpi.tmh"


#define ACPI_METHOD_OUTPUT_BUFFER_SIZE  1024

// {6F8398C2-7CA4-11E4-AD36-631042B5008F}
DEFINE_GUID(GUID_UCSI_DSM,
0x6f8398c2, 0x7ca4, 0x11e4, 0xad, 0x36, 0x63, 0x10, 0x42, 0xb5, 0x00, 0x8f);

#define UCSI_DSM_REVISION 0
#define UCSI_DSM_FUNCTION_SUPPORTED_FUNCTIONS_INDEX 0x0
#define UCSI_DSM_FUNCTION_SEND_DATA_INDEX 0x1
#define UCSI_DSM_FUNCTION_RECEIVE_DATA_INDEX 0x2
#define UCSI_DSM_FUNCTION_USB_DEVICE_CONTROLLER_STATUS_INDEX 0x3

#define UCSI_DSM_EXECUTION_TIMEOUT_IN_MS 3000

EXTERN_C_START

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Acpi_EvaluateUcsiDsm (
    _In_ PACPI_CONTEXT AcpiCtx,
    _In_ ULONG FunctionIndex,
    _Outptr_opt_ PACPI_EVAL_OUTPUT_BUFFER* Output
    );

EXTERN_C_END

#pragma alloc_text(PAGE, Acpi_PrepareHardware)
#pragma alloc_text(PAGE, Acpi_ReleaseHardware)
#pragma alloc_text(PAGE, Acpi_UcsiDsmSendData)
#pragma alloc_text(PAGE, Acpi_UcsiDsmReceiveData)
#pragma alloc_text(PAGE, Acpi_EnumChildren)
#pragma alloc_text(PAGE, Acpi_EvaluatePld)
#pragma alloc_text(PAGE, Acpi_EvaluateUcsiDsm)
#pragma alloc_text(PAGE, Acpi_UcsiDsmIsUsbDeviceControllerEnabled)


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Acpi_PrepareHardware (
    _In_ PACPI_CONTEXT AcpiCtx
    )
{
    NTSTATUS status;
    WDFDEVICE device;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_ACPI);

    device = Context_GetWdfDevice(AcpiCtx);

    if (AcpiCtx->Initialized != FALSE)
    {
        status = STATUS_INVALID_DEVICE_STATE;
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] ACPI already initialized", device);
        goto Exit;
    }

    status = WdfFdoQueryForInterface(device,
                                     &GUID_ACPI_INTERFACE_STANDARD2,
                                     (PINTERFACE) &AcpiCtx->AcpiInterface,
                                     sizeof(ACPI_INTERFACE_STANDARD2),
                                     1,
                                     NULL);

    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] WdfFdoQueryForInterface for ACPI_INTERFACE_STANDARD2 failed - %!STATUS!", device, status);
        goto Exit;
    }

    AcpiCtx->Initialized = TRUE;

    TRACE_INFO(TRACE_FLAG_ACPI, "[Device: 0x%p] ACPI prepare hardware completed", device);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_ACPI);

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
Acpi_ReleaseHardware (
    _In_ PACPI_CONTEXT AcpiCtx
    )
{
    WDFDEVICE device;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_ACPI);

    if (AcpiCtx->Initialized == FALSE)
    {
        goto Exit;
    }

    device = Context_GetWdfDevice(AcpiCtx);

    AcpiCtx->AcpiInterface.InterfaceDereference(AcpiCtx->AcpiInterface.Context);
    RtlZeroMemory(&AcpiCtx->AcpiInterface, sizeof(AcpiCtx->AcpiInterface));

    AcpiCtx->Initialized = FALSE;

    TRACE_INFO(TRACE_FLAG_ACPI, "[Device: 0x%p] ACPI release hardware completed", device);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_ACPI);
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Acpi_UcsiDsmSendData (
    _In_ PACPI_CONTEXT AcpiCtx
    )
{
    NTSTATUS status;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_ACPI);

    status = Acpi_EvaluateUcsiDsm(AcpiCtx,
                                  UCSI_DSM_FUNCTION_SEND_DATA_INDEX,
                                  nullptr);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_ACPI);

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Acpi_UcsiDsmReceiveData (
    _In_ PACPI_CONTEXT AcpiCtx
    )
{
    NTSTATUS status;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_ACPI);

    status = Acpi_EvaluateUcsiDsm(AcpiCtx,
                                  UCSI_DSM_FUNCTION_RECEIVE_DATA_INDEX,
                                  nullptr);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_ACPI);

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Acpi_EnumChildren (
    _In_ PACPI_CONTEXT AcpiCtx,
    _Out_ WDFMEMORY* EnumChildrenOutput
    )
{
    NTSTATUS status;
    WDFDEVICE device;
    WDFMEMORY inputMem;
    PACPI_ENUM_CHILDREN_INPUT_BUFFER inputBuf;
    size_t inputBufSize;
    WDF_MEMORY_DESCRIPTOR inputMemDesc;
    WDFMEMORY outputMem;
    PACPI_ENUM_CHILDREN_OUTPUT_BUFFER outputBuf;
    size_t outputBufSize;
    WDF_MEMORY_DESCRIPTOR outputMemDesc;
    WDF_OBJECT_ATTRIBUTES attributes;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_ACPI);

    device = Context_GetWdfDevice(AcpiCtx);
    inputMem = WDF_NO_HANDLE;
    outputMem = WDF_NO_HANDLE;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;

    inputBufSize = sizeof(*inputBuf);
    status = WdfMemoryCreate(&attributes,
                             NonPagedPoolNx,
                             0,
                             inputBufSize,
                             &inputMem,
                             (PVOID*) &inputBuf);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] WdfMemoryCreate for %Iu bytes failed", device, inputBufSize);
        goto Exit;
    }

    RtlZeroMemory(inputBuf, inputBufSize);
    inputBuf->Signature = ACPI_ENUM_CHILDREN_INPUT_BUFFER_SIGNATURE;
    inputBuf->Flags = ENUM_CHILDREN_IMMEDIATE_ONLY;

    WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&inputMemDesc, inputMem, nullptr);

    outputBufSize = sizeof(*outputBuf);

    do
    {
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = device;

        status = WdfMemoryCreate(&attributes,
                                 NonPagedPoolNx,
                                 0,
                                 outputBufSize,
                                 &outputMem,
                                 (PVOID*) &outputBuf);
        if (!NT_SUCCESS(status))
        {
            TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] WdfMemoryCreate for %Iu bytes failed", device, outputBufSize);
            goto Exit;
        }

        WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&outputMemDesc, outputMem, nullptr);

        status = WdfIoTargetSendIoctlSynchronously(WdfDeviceGetIoTarget(device),
                                                   NULL,
                                                   IOCTL_ACPI_ENUM_CHILDREN,
                                                   &inputMemDesc,
                                                   &outputMemDesc,
                                                   nullptr,
                                                   nullptr);

        if (NT_SUCCESS(status))
        {
            if (outputBuf->Signature != ACPI_ENUM_CHILDREN_OUTPUT_BUFFER_SIGNATURE)
            {
                status = STATUS_ACPI_INVALID_DATA;
                TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] Invalid data in ACPI_ENUM_CHILDREN_OUTPUT_BUFFER", device);
                goto Exit;
            }

            //
            // There must be atleast one, because this device is included in the list.
            //

            if (outputBuf->NumberOfChildren < 1)
            {
                status = STATUS_ACPI_INVALID_DATA;
                TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] No child devices in ACPI_ENUM_CHILDREN_OUTPUT_BUFFER", device);
                goto Exit;
            }

            break;
        }

        if (status != STATUS_BUFFER_OVERFLOW)
        {
            TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] IOCTL_ACPI_ENUM_CHILDREN failed - %!STATUS!", device, status);
            goto Exit;
        }

        if (outputBuf->Signature != ACPI_ENUM_CHILDREN_OUTPUT_BUFFER_SIGNATURE)
        {
            status = STATUS_ACPI_INVALID_DATA;
            TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] Invalid data in ACPI_ENUM_CHILDREN_OUTPUT_BUFFER", device);
            goto Exit;
        }

        //
        // when IOCTL_ACPI_ENUM_CHILDREN returns STATUS_BUFFER_OVERFLOW, the OutputBuffer->NumberOfChildren is set
        // to the size, in bytes, to hold the whole ACPI_ENUM_CHILDREN_OUTPUT_BUFFER, including the fixed header
        // and the variable array for child objects
        //
        _Analysis_assume_(outputBuf->NumberOfChildren > sizeof(ACPI_ENUM_CHILDREN_OUTPUT_BUFFER));

        outputBufSize = outputBuf->NumberOfChildren;
        WdfObjectDelete(outputMem);
        outputMem = WDF_NO_HANDLE;

#pragma warning(suppress:4127)
    } while (true);

    *EnumChildrenOutput = outputMem;

Exit:

    if (inputMem != WDF_NO_HANDLE)
    {
        WdfObjectDelete(inputMem);
    }

    if (!NT_SUCCESS(status) && (outputMem != WDF_NO_HANDLE))
    {
        WdfObjectDelete(outputMem);
    }

    TRACE_FUNC_EXIT(TRACE_FLAG_ACPI);

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Acpi_EvaluatePld (
    _In_ PACPI_CONTEXT AcpiCtx,
    _In_ LPCSTR DeviceName,
    _Out_ PACPI_PLD_BUFFER PldBuffer
    )
{
    NTSTATUS status;
    WDFDEVICE device;
    WDF_MEMORY_DESCRIPTOR inputMemDesc;
    ACPI_EVAL_INPUT_BUFFER_EX inputBuffer;
    size_t inputBufferSize;
    WDFMEMORY outputMemory;
    WDF_MEMORY_DESCRIPTOR outputMemDesc;
    PACPI_EVAL_OUTPUT_BUFFER outputBuffer;
    size_t outputBufferSize;
    size_t outputArgumentBufferSize;
    WDF_OBJECT_ATTRIBUTES attributes;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_ACPI);

    device = Context_GetWdfDevice(AcpiCtx);
    outputMemory = WDF_NO_HANDLE;

    inputBufferSize = sizeof(inputBuffer);
    RtlZeroMemory(&inputBuffer, inputBufferSize);

    inputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE_EX;

    status = RtlStringCchPrintfA(inputBuffer.MethodName,
                                 sizeof(inputBuffer.MethodName),
                                 "%s._PLD",
                                 DeviceName);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] RtlStringCchPrintfA for creating method name failed - %!STATUS!", device, status);
        goto Exit;
    }

    outputArgumentBufferSize = 1024;
    outputBufferSize =
        FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) +
        outputArgumentBufferSize;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;

    status = WdfMemoryCreate(&attributes,
                             NonPagedPoolNx,
                             0,
                             outputBufferSize,
                             &outputMemory,
                             (PVOID*) &outputBuffer);

    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] WdfMemoryCreate failed for %Iu bytes - %!STATUS!", device, outputBufferSize, status);
        goto Exit;
    }

    RtlZeroMemory(outputBuffer, outputBufferSize);

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputMemDesc, &inputBuffer, (ULONG) inputBufferSize);
    WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&outputMemDesc, outputMemory, NULL);

    status = WdfIoTargetSendInternalIoctlSynchronously(
                 WdfDeviceGetIoTarget(device),
                 NULL,
                 IOCTL_ACPI_EVAL_METHOD_EX,
                 &inputMemDesc,
                 &outputMemDesc,
                 NULL,
                 NULL);

    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] IOCTL_ACPI_EVAL_METHOD_EX for %s failed - %!STATUS!", device, inputBuffer.MethodName, status);
        goto Exit;
    }

    if (outputBuffer->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE)
    {
        status = STATUS_ACPI_INVALID_DATA;
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] ACPI_EVAL_OUTPUT_BUFFER signature is incorrect", device);
        goto Exit;
    }

    if (outputBuffer->Count < 1)
    {
        status = STATUS_ACPI_INVALID_DATA;
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] _PLD for %s didn't return anything", device, inputBuffer.MethodName);
        goto Exit;
    }

    if (outputBuffer->Argument[0].Type != ACPI_METHOD_ARGUMENT_BUFFER)
    {
        status = STATUS_ACPI_INVALID_DATA;
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] _PLD for %s returned an unexpected argument of type %d", device, inputBuffer.MethodName, outputBuffer->Argument[0].Type);
        goto Exit;
    }

    if (outputBuffer->Argument[0].DataLength < sizeof(*PldBuffer))
    {
        status = STATUS_ACPI_INVALID_DATA;
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] Unexpected _PLD buffer size for %s. Expected %Iu bytes, got %Iu bytes", device, inputBuffer.MethodName, sizeof(*PldBuffer), outputBuffer->Argument[0].DataLength);
        goto Exit;
    }

    *PldBuffer = *((PACPI_PLD_BUFFER) outputBuffer->Argument[0].Data);

Exit:

    if (outputMemory != WDF_NO_HANDLE)
    {
        WdfObjectDelete(outputMemory);
    }

    TRACE_FUNC_EXIT(TRACE_FLAG_ACPI);

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Acpi_EvaluateUcsiDsm (
    _In_ PACPI_CONTEXT AcpiCtx,
    _In_ ULONG FunctionIndex,
    _Outptr_opt_ PACPI_EVAL_OUTPUT_BUFFER* Output
    )
/*++

    N.B. Caller is expected to free the Output buffer.

--*/
{
    NTSTATUS status;
    WDFDEVICE device;
    WDFMEMORY inputMemory;
    WDF_MEMORY_DESCRIPTOR inputMemDesc;
    PACPI_EVAL_INPUT_BUFFER_COMPLEX inputBuffer;
    size_t inputBufferSize;
    size_t inputArgumentBufferSize;
    PACPI_METHOD_ARGUMENT argument;
    WDF_MEMORY_DESCRIPTOR outputMemDesc;
    PACPI_EVAL_OUTPUT_BUFFER outputBuffer;
    size_t outputBufferSize;
    size_t outputArgumentBufferSize;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_REQUEST_SEND_OPTIONS sendOptions;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_ACPI);

    device = Context_GetWdfDevice(AcpiCtx);
    inputMemory = WDF_NO_HANDLE;
    outputBuffer = nullptr;

    inputArgumentBufferSize =
        ACPI_METHOD_ARGUMENT_LENGTH(sizeof(GUID)) +
        ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG)) +
        ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG)) +
        ACPI_METHOD_ARGUMENT_LENGTH(0);

    inputBufferSize =
        FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX, Argument) +
        inputArgumentBufferSize;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;

    status = WdfMemoryCreate(&attributes,
                             NonPagedPoolNx,
                             0,
                             inputBufferSize,
                             &inputMemory,
                             (PVOID*) &inputBuffer);

    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] WdfMemoryCreate failed for %Iu bytes - %!STATUS!", device, inputBufferSize, status);
        goto Exit;
    }

    RtlZeroMemory(inputBuffer, inputBufferSize);

    inputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
    inputBuffer->Size = (ULONG) inputArgumentBufferSize;
    inputBuffer->ArgumentCount = 4;
    inputBuffer->MethodNameAsUlong = (ULONG) 'MSD_';

    argument = &(inputBuffer->Argument[0]);
    ACPI_METHOD_SET_ARGUMENT_BUFFER(argument,
                                    &GUID_UCSI_DSM,
                                    sizeof(GUID_UCSI_DSM));

    argument = ACPI_METHOD_NEXT_ARGUMENT(argument);
    ACPI_METHOD_SET_ARGUMENT_INTEGER(argument, UCSI_DSM_REVISION);

    argument = ACPI_METHOD_NEXT_ARGUMENT(argument);
    ACPI_METHOD_SET_ARGUMENT_INTEGER(argument, FunctionIndex);

    argument = ACPI_METHOD_NEXT_ARGUMENT(argument);
    argument->Type = ACPI_METHOD_ARGUMENT_PACKAGE_EX;
    argument->DataLength = 0;

    outputArgumentBufferSize = ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG));
    outputBufferSize =
        FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) +
        outputArgumentBufferSize;

    outputBuffer = (PACPI_EVAL_OUTPUT_BUFFER) ExAllocatePool2(POOL_FLAG_NON_PAGED,
                                                              outputBufferSize,
                                                              TAG_UCSI);

    if (outputBuffer == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] ExAllocatePoolWithTag failed for %Iu bytes", device, outputBufferSize);
        goto Exit;
    }

    WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&inputMemDesc, inputMemory, NULL);
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputMemDesc, outputBuffer, (ULONG) outputBufferSize);

    WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);
    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
                                         WDF_REL_TIMEOUT_IN_MS(UCSI_DSM_EXECUTION_TIMEOUT_IN_MS));

    status = WdfIoTargetSendInternalIoctlSynchronously(
                 WdfDeviceGetIoTarget(device),
                 NULL,
                 IOCTL_ACPI_EVAL_METHOD,
                 &inputMemDesc,
                 &outputMemDesc,
                 &sendOptions,
                 NULL);

    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] IOCTL_ACPI_EVAL_METHOD for _DSM failed - %!STATUS!", device, status);
        goto Exit;
    }

    if (outputBuffer->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE)
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] ACPI_EVAL_OUTPUT_BUFFER signature is incorrect", device);
        status = STATUS_ACPI_INVALID_DATA;
        goto Exit;
    }

Exit:

    if (inputMemory != WDF_NO_HANDLE)
    {
        WdfObjectDelete(inputMemory);
    }

    if (!NT_SUCCESS(status) || (Output == nullptr))
    {
        if (outputBuffer)
        {
            ExFreePoolWithTag(outputBuffer, TAG_UCSI);
        }
    }
    else
    {
        *Output = outputBuffer;
    }

    TRACE_FUNC_EXIT(TRACE_FLAG_ACPI);

    return status;
}


_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
Acpi_RegisterNotificationCallback (
    _In_ PACPI_CONTEXT AcpiCtx,
    _In_ PFN_ACPI_NOTIFY_CALLBACK Callback,
    _In_ PVOID Context
    )
{
    NTSTATUS status;
    WDFDEVICE device;

    TRACE_FUNC_ENTRY(TRACE_FLAG_ACPI);

    device = Context_GetWdfDevice(AcpiCtx);

    if (AcpiCtx->RegisteredForNotifications != FALSE)
    {
        status = STATUS_INVALID_DEVICE_STATE;
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] Already registered for ACPI notifications", device);
        goto Exit;
    }

    status = AcpiCtx->AcpiInterface.RegisterForDeviceNotifications(AcpiCtx->AcpiInterface.Context,
                                                                   Callback,
                                                                   Context);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] Registering for ACPI notifications failed - %!STATUS!", device, status);
        goto Exit;
    }

    AcpiCtx->RegisteredForNotifications = TRUE;

    TRACE_INFO(TRACE_FLAG_ACPI, "[Device: 0x%p] Registered for ACPI notifications", device);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_ACPI);

    return status;
}


_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
Acpi_UnregisterNotificationCallback (
    _In_ PACPI_CONTEXT AcpiCtx
    )
{
    WDFDEVICE device;

    TRACE_FUNC_ENTRY(TRACE_FLAG_ACPI);

    if (AcpiCtx->RegisteredForNotifications == FALSE)
    {
        goto Exit;
    }

    device = Context_GetWdfDevice(AcpiCtx);

    AcpiCtx->AcpiInterface.UnregisterForDeviceNotifications(AcpiCtx->AcpiInterface.Context);
    AcpiCtx->RegisteredForNotifications = FALSE;

    TRACE_INFO(TRACE_FLAG_ACPI, "[Device: 0x%p] Unregistered ACPI notifications", device);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_ACPI);
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
Acpi_UcsiDsmIsUsbDeviceControllerEnabled (
    _In_ PACPI_CONTEXT AcpiCtx,
    _Out_ PBOOLEAN IsUsbDeviceControllerEnabled
    )
{
    NTSTATUS status;
    WDFDEVICE device;
    PACPI_EVAL_OUTPUT_BUFFER output;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_ACPI);

    output = nullptr;
    device = Context_GetWdfDevice(AcpiCtx);

    status = Acpi_EvaluateUcsiDsm(AcpiCtx, UCSI_DSM_FUNCTION_SUPPORTED_FUNCTIONS_INDEX, &output);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] _DSM query for supported functions failed - %!STATUS!", device, status);
        goto Exit;
    }

    if (output->Count != 1)
    {
        status = STATUS_ACPI_INVALID_DATA;
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] _DSM query for supported functions returned unexpected number of arguments %lu", device, output->Count);
        goto Exit;
    }

    NT_ASSERT_ASSUME(output->Length == sizeof(ACPI_EVAL_OUTPUT_BUFFER));

    if (output->Argument[0].Type != ACPI_METHOD_ARGUMENT_BUFFER)
    {
        status = STATUS_ACPI_INVALID_DATA;
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] _DSM query for supported functions returned an unexpected argument of type %d", device, output->Argument[0].Type);
        goto Exit;
    }

    if (!TEST_BIT(output->Argument[0].Data[0], UCSI_DSM_FUNCTION_USB_DEVICE_CONTROLLER_STATUS_INDEX))
    {
        //
        // The function to query whether the device controller is enabled is not supported. Assume
        // that the controller is enabled.
        //

        status = STATUS_SUCCESS;
        *IsUsbDeviceControllerEnabled = TRUE;
        goto Exit;
    }

    ExFreePoolWithTag(output, TAG_UCSI);
    output = nullptr;

    status = Acpi_EvaluateUcsiDsm(AcpiCtx,
                                  UCSI_DSM_FUNCTION_USB_DEVICE_CONTROLLER_STATUS_INDEX,
                                  &output);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] _DSM query for USB device controller status failed - %!STATUS!", device, status);
        goto Exit;
    }

    if (output->Count != 1)
    {
        status = STATUS_ACPI_INVALID_DATA;
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] _DSM query for USB device controller status returned unexpected number of arguments %lu", device, output->Count);
        goto Exit;
    }

    NT_ASSERT_ASSUME(output->Length == sizeof(ACPI_EVAL_OUTPUT_BUFFER));

    *IsUsbDeviceControllerEnabled = (output->Argument[0].Argument) ? TRUE : FALSE;
    status = STATUS_SUCCESS;

Exit:

    if (output)
    {
        ExFreePoolWithTag(output, TAG_UCSI);
    }

    TRACE_FUNC_EXIT(TRACE_FLAG_ACPI);

    return status;
}
