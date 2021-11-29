/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Acpi.cpp

Abstract:

    UCSI Acpi transport functions.

Environment:

    Kernel-mode.

--*/

#include "Pch.h"
#include "Acpi.tmh"


namespace UcmUcsiAcpiClient
{

//
// ACPI _DSM method GUID value as described in the document "BIOS Implemetation of UCSI" at
// https://www.intel.com/content/dam/www/public/us/en/documents/white-papers/bios-implementation-of-ucsi.pdf
// {6F8398C2-7CA4-11E4-AD36-631042B5008F}
//
DEFINE_GUID(GUID_UCSI_DSM,
0x6f8398c2, 0x7ca4, 0x11e4, 0xad, 0x36, 0x63, 0x10, 0x42, 0xb5, 0x00, 0x8f);

const ULONG UCSI_DSM_REVISION = 0;
const ULONG UCSI_DSM_FUNCTION_SUPPORTED_FUNCTIONS_INDEX = 0x0;
const ULONG UCSI_DSM_FUNCTION_SEND_DATA_INDEX = 0x1;
const ULONG UCSI_DSM_FUNCTION_RECEIVE_DATA_INDEX = 0x2;
const ULONG UCSI_DSM_FUNCTION_USB_DEVICE_CONTROLLER_STATUS_INDEX = 0x3;
const ULONG UCSI_DSM_FUNCTION_DISABLE_DATA_ROLE_CORRECTION_POLICY = 0x4;

//
// There is no formal requirement of timeout for _DMS method. 3 second timeout
// was inherited from the previous driver ucmucsi.sys.
//
const ULONG UCSI_DSM_EXECUTION_TIMEOUT_IN_MS = 3000;

PAGED_CODE_SEG
NTSTATUS
Acpi::Initialize(
    WDFDEVICE Device
    )
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_ACPI);

    m_OwningDevice = Device;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = Device;

    status = WdfWaitLockCreate(&attributes, &m_CallbackLock);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[WDFDEVICE: 0x%p] WdfWaitLockCreate failed - %!STATUS!",
            Device, status);
        goto Exit;
    }

    status = STATUS_SUCCESS;

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_ACPI);

    return status;
}

PAGED_CODE_SEG
NTSTATUS
Acpi::PrepareHardware(
    PCONFIG Config
    )
{
    NTSTATUS status;
    PVOID mappedMemory;
    UCSI_VERSION version;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_ACPI);

    //
    // Map the shared memory to the virtual space so that the driver can access it.
    //
    mappedMemory = MmMapIoSpaceEx(Config->DataBlockAddress,
                                  Config->DataBlockLength,
                                  PAGE_NOCACHE | PAGE_READWRITE);

#pragma prefast(suppress: __WARNING_REDUNDANT_POINTER_TEST, "API indicates failure by returning NULL")
    if (mappedMemory == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TRACE_ERROR(TRACE_FLAG_PPM, "[Device: 0x%p] MmMapIoSpaceEx failed", m_OwningDevice);
        goto Exit;
    }

    m_UcsiDataBlock = (PUCSI_DATA_BLOCK) mappedMemory;
    m_MappedMemoryLength = Config->DataBlockLength;

    version.AsUInt16 = READ_REGISTER_USHORT(reinterpret_cast<volatile USHORT*> (&m_UcsiDataBlock->UcsiVersion));

    TRACE_INFO(TRACE_FLAG_PPM, "[Device: 0x%p] UCSI version Major: %hx Minor: %hx SubMinor: %hx",
        m_OwningDevice, version.MajorVersion, version.MinorVersion, version.SubMinorVersion);

    //
    // Query ACPI interface so that we can register notifications with ACPI. 
    //
    if (m_InterfaceQueried != false)
    {
        status = STATUS_INVALID_DEVICE_STATE;
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] ACPI interface already queried", m_OwningDevice);
        goto Exit;
    }

    status = WdfFdoQueryForInterface(m_OwningDevice,
                                     &GUID_ACPI_INTERFACE_STANDARD2,
                                     (PINTERFACE) &m_AcpiInterface,
                                     sizeof(ACPI_INTERFACE_STANDARD2),
                                     1,
                                     NULL);

    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] WdfFdoQueryForInterface for ACPI_INTERFACE_STANDARD2 failed - %!STATUS!", 
            m_OwningDevice, status);
        goto Exit;
    }

    m_InterfaceQueried = true;
    TRACE_INFO(TRACE_FLAG_ACPI, "[Device: 0x%p] ACPI prepare hardware completed", m_OwningDevice);

Exit:
    TRACE_FUNC_EXIT(TRACE_FLAG_ACPI);
    return status;
}

PAGED_CODE_SEG
VOID
Acpi::ReleaseHardware()
{

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_ACPI);

    if (m_UcsiDataBlock != nullptr)
    {
        MmUnmapIoSpace(m_UcsiDataBlock, m_MappedMemoryLength);
        m_UcsiDataBlock = nullptr;
        m_MappedMemoryLength = 0;
    }

    if (m_InterfaceQueried == false)
    {
        goto Exit;
    }

    m_AcpiInterface.InterfaceDereference(m_AcpiInterface.Context);
    RtlZeroMemory(&m_AcpiInterface, sizeof(m_AcpiInterface));

    m_InterfaceQueried = false;

    TRACE_INFO(TRACE_FLAG_ACPI, "[Device: 0x%p] ACPI release hardware completed", m_OwningDevice);

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_ACPI);
}

PAGED_CODE_SEG
NTSTATUS
Acpi::EnumChildren(
    WDFMEMORY* EnumChildrenOutput
    )
{
    NTSTATUS status;
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

    inputMem = WDF_NO_HANDLE;
    outputMem = WDF_NO_HANDLE;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = m_OwningDevice;

    inputBufSize = sizeof(*inputBuf);
    status = WdfMemoryCreate(&attributes,
                             NonPagedPoolNx,
                             0,
                             inputBufSize,
                             &inputMem,
                             (PVOID*) &inputBuf);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] WdfMemoryCreate for %Iu bytes failed", 
            m_OwningDevice, inputBufSize);
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
        attributes.ParentObject = m_OwningDevice;

        status = WdfMemoryCreate(&attributes,
                                 NonPagedPoolNx,
                                 0,
                                 outputBufSize,
                                 &outputMem,
                                 (PVOID*) &outputBuf);
        if (!NT_SUCCESS(status))
        {
            TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] WdfMemoryCreate for %Iu bytes failed",
                m_OwningDevice, outputBufSize);
            goto Exit;
        }

        WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&outputMemDesc, outputMem, nullptr);

        status = WdfIoTargetSendIoctlSynchronously(WdfDeviceGetIoTarget(m_OwningDevice),
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
                TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] Invalid data in ACPI_ENUM_CHILDREN_OUTPUT_BUFFER", m_OwningDevice);
                goto Exit;
            }

            // There must be atleast one, because this device is included in the list.
            if (outputBuf->NumberOfChildren < 1)
            {
                status = STATUS_ACPI_INVALID_DATA;
                TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] No child devices in ACPI_ENUM_CHILDREN_OUTPUT_BUFFER", m_OwningDevice);
                goto Exit;
            }

            break;
        }

        if (status != STATUS_BUFFER_OVERFLOW)
        {
            TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] IOCTL_ACPI_ENUM_CHILDREN failed - %!STATUS!", m_OwningDevice, status);
            goto Exit;
        }

        if (outputBuf->Signature != ACPI_ENUM_CHILDREN_OUTPUT_BUFFER_SIGNATURE)
        {
            status = STATUS_ACPI_INVALID_DATA;
            TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] Invalid data in ACPI_ENUM_CHILDREN_OUTPUT_BUFFER", m_OwningDevice);
            goto Exit;
        }

        // NOTE: The NumberOfChildren returned by ACPI actually contains the required size
        // when the status returned is STATUS_BUFFER_OVERFLOW 
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

PAGED_CODE_SEG
NTSTATUS 
Acpi::EvaluatePld(
    LPCSTR DeviceName,
    PACPI_PLD_BUFFER PldBuffer
    )
{
    NTSTATUS status;
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

    // @@BEGIN_NOTPUBLICCODE
    // TODO: Refactor so that _DSM evaluation and _PLD evaluation use a common helper routine.
    //
    // TODO: _PLD evaluation of multiple connectors can be optimized to reduce number of memory
    //       allocations. Right now, output memory will be allocated for each request. We can
    //       reuse a single allocation for all evaluations.
    //
    // TODO: When refactoring code, must keep in mind that it would be efficient to allow the
    //       caller to specify a good initial output buffer size, which would depend on the method
    //       being evaluated. Caller may have a good estimate of the buffer size required. Otherwise
    //       we'll end up making two calls (one to figure out the size, another for actual
    //       evaluation) everytime.
    // @@END_NOTPUBLICCODE

    TRACE_FUNC_ENTRY(TRACE_FLAG_ACPI);

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
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] RtlStringCchPrintfA for creating method name failed - %!STATUS!", m_OwningDevice, status);
        goto Exit;
    }

    // @@BEGIN_NOTPUBLICCODE
    // TODO: Pick an intelligent initial size for the buffer, and then do try-and-try-again.
    // @@END_NOTPUBLICCODE

    outputArgumentBufferSize = 1024;
    outputBufferSize =
        FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) +
        outputArgumentBufferSize;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = m_OwningDevice;

    status = WdfMemoryCreate(&attributes,
                             NonPagedPoolNx,
                             0,
                             outputBufferSize,
                             &outputMemory,
                             (PVOID*) &outputBuffer);

    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] WdfMemoryCreate failed for %Iu bytes - %!STATUS!", m_OwningDevice, outputBufferSize, status);
        goto Exit;
    }

    RtlZeroMemory(outputBuffer, outputBufferSize);

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputMemDesc, &inputBuffer, (ULONG) inputBufferSize);
    WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&outputMemDesc, outputMemory, NULL);

    status = WdfIoTargetSendInternalIoctlSynchronously(
                 WdfDeviceGetIoTarget(m_OwningDevice),
                 NULL,
                 IOCTL_ACPI_EVAL_METHOD_EX,
                 &inputMemDesc,
                 &outputMemDesc,
                 NULL,
                 NULL);

    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] IOCTL_ACPI_EVAL_METHOD_EX for %s failed - %!STATUS!", m_OwningDevice, inputBuffer.MethodName, status);
        goto Exit;
    }

    if (outputBuffer->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE)
    {
        status = STATUS_ACPI_INVALID_DATA;
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] ACPI_EVAL_OUTPUT_BUFFER signature is incorrect", m_OwningDevice);
        goto Exit;
    }

    if (outputBuffer->Count < 1)
    {
        status = STATUS_ACPI_INVALID_DATA;
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] _PLD for %s didn't return anything", m_OwningDevice, inputBuffer.MethodName);
        goto Exit;
    }

    if (outputBuffer->Argument[0].Type != ACPI_METHOD_ARGUMENT_BUFFER)
    {
        status = STATUS_ACPI_INVALID_DATA;
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] _PLD for %s returned an unexpected argument of type %d", m_OwningDevice, inputBuffer.MethodName, outputBuffer->Argument[0].Type);
        goto Exit;
    }

    if (outputBuffer->Argument[0].DataLength < sizeof(*PldBuffer))
    {
        status = STATUS_ACPI_INVALID_DATA;
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] Unexpected _PLD buffer size for %s. Expected %Iu bytes, got %Iu bytes", m_OwningDevice, inputBuffer.MethodName, sizeof(*PldBuffer), outputBuffer->Argument[0].DataLength);
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

PAGED_CODE_SEG
NTSTATUS
Acpi::CheckIfUsbDeviceControllerIsEnabled(
    bool& IsUsbDeviceControllerEnabled
    )
{
    NTSTATUS status;
    PACPI_EVAL_OUTPUT_BUFFER output;
    UCHAR supportFunctions;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_ACPI);

    output = nullptr;

    status = EvaluateUcsiDsm(UCSI_DSM_FUNCTION_SUPPORTED_FUNCTIONS_INDEX, &output);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] _DSM query for supported functions failed - %!STATUS!", m_OwningDevice, status);
        goto Exit;
    }

    if (output->Count != 1)
    {
        status = STATUS_ACPI_INVALID_DATA;
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] _DSM query for supported functions returned unexpected number of arguments %lu", m_OwningDevice, output->Count);
        goto Exit;
    }

    NT_ASSERT_ASSUME(output->Length == sizeof(ACPI_EVAL_OUTPUT_BUFFER));

    if (output->Argument[0].Type != ACPI_METHOD_ARGUMENT_BUFFER)
    {
        status = STATUS_ACPI_INVALID_DATA;
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] _DSM query for supported functions returned an unexpected argument of type %d", m_OwningDevice, output->Argument[0].Type);
        goto Exit;
    }

    supportFunctions = output->Argument[0].Data[0];

    //
    // Free the buffer since we will reuse "output" to evaluate other UCSI DSM functions.
    //

    delete[] output;
    output = nullptr;


    //
    // First, check if the platform disables the OS policy of data role correction.
    // If it's disabled, report UsbDeviceControllerEnabled==true to UcmUcsiCx, so that
    // it will not initiate DR_SWAP when being UFP.
    //

    if (TEST_BIT(supportFunctions, UCSI_DSM_FUNCTION_DISABLE_DATA_ROLE_CORRECTION_POLICY))
    {
        status = EvaluateUcsiDsm(UCSI_DSM_FUNCTION_DISABLE_DATA_ROLE_CORRECTION_POLICY, &output);
        if (!NT_SUCCESS(status))
        {
            TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] _DSM query for UCSI_DSM_FUNCTION_DISABLE_DATA_ROLE_CORRECTION_POLICY failed - %!STATUS!", m_OwningDevice, status);
            goto Exit;
        }

        if (output->Count != 1)
        {
            status = STATUS_ACPI_INVALID_DATA;
            TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] _DSM query for UCSI_DSM_FUNCTION_DISABLE_DATA_ROLE_CORRECTION_POLICY returned unexpected number of arguments %lu", m_OwningDevice, output->Count);
            goto Exit;
        }

        NT_ASSERT_ASSUME(output->Length == sizeof(ACPI_EVAL_OUTPUT_BUFFER));

        if (output->Argument[0].Argument)
        {
            status = STATUS_SUCCESS;
            IsUsbDeviceControllerEnabled = true;
            TRACE_INFO(TRACE_FLAG_ACPI, "[Device: 0x%p] Data role correction policy disabled.", m_OwningDevice);
            goto Exit;
        }

        //
        // Free the buffer since we may reuse "output" to evaluate another UCSI DSM function.
        //

        delete[] output;
        output = nullptr;
    }

    //
    // The platform hasn't disabled the OS policy for data role correction. Now, check
    // if the platform has a USB device controller, and return the corresponding 
    // UsbDeviceControllerEnabled value for UcmUcsiCx to switch on/off the OS policy 
    // for data role correction.
    //

    if (!TEST_BIT(supportFunctions, UCSI_DSM_FUNCTION_USB_DEVICE_CONTROLLER_STATUS_INDEX))
    {
        //
        // The function to query whether the device controller is enabled is not supported. Assume
        // that the controller is enabled.
        //

        TRACE_INFO(TRACE_FLAG_ACPI, "[Device: 0x%p] _DSM to query if USB device controller is enabled is not supported. Assuming that device controller is supported",
            m_OwningDevice);

        status = STATUS_SUCCESS;
        IsUsbDeviceControllerEnabled = true;
        goto Exit;
    }

    status = EvaluateUcsiDsm(UCSI_DSM_FUNCTION_USB_DEVICE_CONTROLLER_STATUS_INDEX, &output);
    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] _DSM query for USB device controller status failed - %!STATUS!", m_OwningDevice, status);
        goto Exit;
    }

    if (output->Count != 1)
    {
        status = STATUS_ACPI_INVALID_DATA;
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] _DSM query for USB device controller status returned unexpected number of arguments %lu", m_OwningDevice, output->Count);
        goto Exit;
    }

    NT_ASSERT_ASSUME(output->Length == sizeof(ACPI_EVAL_OUTPUT_BUFFER));

    IsUsbDeviceControllerEnabled = (output->Argument[0].Argument) ? true : false;
    status = STATUS_SUCCESS;

Exit:

    if (output)
    {
        delete[] output;
        output = nullptr;
    }

    TRACE_FUNC_EXIT(TRACE_FLAG_ACPI);

    return status;
}

PAGED_CODE_SEG
NTSTATUS
Acpi::EvaluateUcsiDsm(
    ULONG FunctionIndex,
    PACPI_EVAL_OUTPUT_BUFFER* Output
    )
{
    NTSTATUS status;
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
    attributes.ParentObject = m_OwningDevice;

    status = WdfMemoryCreate(&attributes,
                             NonPagedPoolNx,
                             0,
                             inputBufferSize,
                             &inputMemory,
                             (PVOID*) &inputBuffer);

    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] WdfMemoryCreate failed for %Iu bytes - %!STATUS!",
            m_OwningDevice, inputBufferSize, status);
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

    // @@BEGIN_NOTPUBLICCODE
    // TODO: Try-and-try-again approach so that it can fetch any size output buffer.
    // @@END_NOTPUBLICCODE

    outputArgumentBufferSize = ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG));
    outputBufferSize =
        FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) +
        outputArgumentBufferSize;

    outputBuffer = reinterpret_cast<PACPI_EVAL_OUTPUT_BUFFER>(new BYTE[outputBufferSize]);

    if (outputBuffer == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] Memory allocation failed for %Iu bytes", m_OwningDevice, outputBufferSize);
        goto Exit;
    }

    WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&inputMemDesc, inputMemory, NULL);
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputMemDesc, outputBuffer, (ULONG) outputBufferSize);

    WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);
    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
                                         WDF_REL_TIMEOUT_IN_MS(UCSI_DSM_EXECUTION_TIMEOUT_IN_MS));

    status = WdfIoTargetSendInternalIoctlSynchronously(
                 WdfDeviceGetIoTarget(m_OwningDevice),
                 NULL,
                 IOCTL_ACPI_EVAL_METHOD,
                 &inputMemDesc,
                 &outputMemDesc,
                 &sendOptions,
                 NULL);

    if (!NT_SUCCESS(status))
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] IOCTL_ACPI_EVAL_METHOD for _DSM failed - %!STATUS!", m_OwningDevice, status);
        goto Exit;
    }

    if (outputBuffer->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE)
    {
        TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] ACPI_EVAL_OUTPUT_BUFFER signature is incorrect", m_OwningDevice);
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
            delete[] outputBuffer;
            outputBuffer = nullptr;
        }
    }
    else
    {
        *Output = outputBuffer;
    }

    TRACE_FUNC_EXIT(TRACE_FLAG_ACPI);
    return status;
}

PAGED_CODE_SEG
NTSTATUS
Acpi::SendData(
    const PUCSI_DATA_BLOCK DataBlock
    )
{
    NTSTATUS status;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_ACPI);

#if defined (_WIN64)

    WRITE_REGISTER_ULONG64(reinterpret_cast<volatile ULONG64*> (&m_UcsiDataBlock->Control),
        DataBlock->Control.AsUInt64);

#else

    WRITE_REGISTER_BUFFER_ULONG(reinterpret_cast<volatile ULONG*> (&m_UcsiDataBlock->Control),
        reinterpret_cast<PULONG> (&DataBlock->Control),
        sizeof(m_UcsiDataBlock->Control) / sizeof(ULONG));

#endif

    status = EvaluateUcsiDsm(UCSI_DSM_FUNCTION_SEND_DATA_INDEX,
                                  nullptr);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_ACPI);
    return status;
}

PAGED_CODE_SEG
NTSTATUS
Acpi::ReceiveData (
    PUCSI_DATA_BLOCK DataBlockOut
    )
{
    NTSTATUS status;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_ACPI);

    status = EvaluateUcsiDsm(UCSI_DSM_FUNCTION_RECEIVE_DATA_INDEX,
                             nullptr);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    // Right now there is no use case of copying the whole block. Just copying
    // CCI as of now since all the caller would want to know is whether the last
    // command has been completed.
    DataBlockOut->CCI.AsUInt32 = READ_REGISTER_ULONG(reinterpret_cast<volatile ULONG*> (&m_UcsiDataBlock->CCI));

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_ACPI);
    return status;
}

PAGED_CODE_SEG
VOID
Acpi::NotificationCallback(
    PVOID Context,
    ULONG NotifyValue
    )
{
    PAGED_CODE();
    TRACE_FUNC_ENTRY(TRACE_FLAG_ACPI);

    //
    // Get the Acpi object from the Context passed by the notification callback.
    //
    Acpi *acpi = reinterpret_cast<Acpi*>(Context);

    acpi->AcquireCallbackLock();
    if (acpi->m_CallbackContext.m_CallbackFunction)
    {
        acpi->InvokeCallback(NotifyValue);
    }
    acpi->ReleaseCallbackLock();

    TRACE_FUNC_EXIT(TRACE_FLAG_ACPI);
}

PAGED_CODE_SEG
NTSTATUS
Acpi::RegisterNotificationCallbackInternal(
    _In_ PACPI_NOTIFICATION_CALLBACK_CONTEXT CallbackContext
    )
{
    NTSTATUS status;

    PAGED_CODE();

    TRACE_FUNC_ENTRY(TRACE_FLAG_ACPI);

    status = m_AcpiInterface.RegisterForDeviceNotifications(m_AcpiInterface.Context,
                    Acpi::NotificationCallback, this);
    if (!NT_SUCCESS(status))
    {
       TRACE_ERROR(TRACE_FLAG_ACPI, "[Device: 0x%p] Registering for ACPI notifications failed - %!STATUS!",
           m_OwningDevice, status);
        goto Exit;
    }

    AcquireCallbackLock();
    
    m_CallbackContext.m_PpmObject = CallbackContext->m_PpmObject;
    m_CallbackContext.m_CallbackFunction = CallbackContext->m_CallbackFunction;

    ReleaseCallbackLock();

    status = STATUS_SUCCESS;

Exit:

    TRACE_FUNC_EXIT(TRACE_FLAG_ACPI);
    return status;
}

PAGED_CODE_SEG
VOID
Acpi::UnRegisterNotificationCallback()
{
    PAGED_CODE();
    TRACE_FUNC_ENTRY(TRACE_FLAG_ACPI);

    //
    // Acquire the lock to synchronize with outstanding callbacks, and then clear the callback
    // pointer to prevent making additional callbacks. The ACPI callback may still fire, because the
    // ACPI driver does not guarantee that it won't make additional calls after
    // UnregisterForDeviceNotifications has returned, so clearing the callback pointer prevents us
    // from accidentally making more callbacks after this function returns.
    //
    AcquireCallbackLock();

    m_CallbackContext.m_PpmObject = nullptr;
    m_CallbackContext.m_CallbackFunction = nullptr;

    ReleaseCallbackLock();

    m_AcpiInterface.UnregisterForDeviceNotifications(m_AcpiInterface.Context);

    TRACE_INFO(TRACE_FLAG_ACPI, "[Device: 0x%p] Unregistered ACPI notifications", m_OwningDevice);

    TRACE_FUNC_EXIT(TRACE_FLAG_ACPI);
}

}
