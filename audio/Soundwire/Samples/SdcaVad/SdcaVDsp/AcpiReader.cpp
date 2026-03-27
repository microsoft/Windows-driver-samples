/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    AcpiReader.cpp

Abstract:

    Implements Acpi reader module.

Environment:

    Kernel mode

--*/

#include "private.h"

#include <stdunk.h>

#include "AcpiReader.h"

#ifndef __INTELLISENSE__
#include "AcpiReader.tmh"
#endif

namespace ACPIREADER
{
    RECORDER_LOG AcpiReader::s_AcpiReaderLog { nullptr };
    ULONG AcpiReader::s_MemoryTag { 0 };

    _Use_decl_annotations_
    PAGED_CODE_SEG
    NTSTATUS
    AcpiReader::_CreateAndInitialize(_In_ WDFDEVICE Device, _In_ RECORDER_LOG Log, _In_ ULONG MemoryTag)
    {
        NTSTATUS status = STATUS_SUCCESS;
        WDF_OBJECT_ATTRIBUTES attributes;
        AcpiReader * This{ nullptr };
        VOID * contextAddress;

        PAGED_CODE();

        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, AcpiReader);
        attributes.EvtDestroyCallback = EvtContextDestroy;

        status = WdfObjectAllocateContext(Device, &attributes, &contextAddress);
        if (!NT_SUCCESS(status))
        {
            goto exit;
        }

        s_AcpiReaderLog = Log;
        s_MemoryTag = MemoryTag;

        This = new (contextAddress) AcpiReader(Device);

    exit:
        return status;
    }

    _Use_decl_annotations_
    PAGED_CODE_SEG
    NTSTATUS
    AcpiReader::ParseGuid(
        _In_ PACPI_METHOD_ARGUMENT              Argument,
        _Out_writes_bytes_(BufferLength) PVOID  Buffer,
        _In_ ULONG                              BufferLength
    )
    /*++

    Routine Description:

        This function parses the content of an ACPI method argument into a GUID.

    Arguments:

        Argument - Supplies the ACPI argument to parse.

        Buffer - Supplies a pointer to the buffer to store the GUID.

        BufferLength - Supplies the buffer size in bytes.

    Return Value:

        NTSTATUS

    --*/
    {

        NTSTATUS status = STATUS_SUCCESS;

        PAGED_CODE();

        DrvLogEnter(s_AcpiReaderLog);

        if (Argument->Type != ACPI_METHOD_ARGUMENT_BUFFER)
        {
            status = STATUS_INVALID_PARAMETER;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Unexpected Argument Type. Expected: %lu, Actual: %lu, %!STATUS!", ACPI_METHOD_ARGUMENT_BUFFER, Argument->Type, status);
            ASSERT(FALSE);
            goto exit;
        }

        if (BufferLength < sizeof(GUID))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Buffer too small. Expected: %lu, Actual: %lu, %!STATUS!", sizeof(GUID), BufferLength, status);
            ASSERT(FALSE);
            goto exit;
        }

        if (Argument->DataLength != sizeof(GUID))
        {
            status = STATUS_ACPI_INVALID_ARGTYPE;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Unexpected Argument DataLength. Expected: %lu, Actual: %lu, %!STATUS!", sizeof(GUID), Argument->DataLength, status);
            ASSERT(FALSE);
            goto exit;
        }

        RtlCopyMemory((PUCHAR)Buffer, Argument->Data, Argument->DataLength);

    exit:
        DrvLogExit(s_AcpiReaderLog);
        return status;
    }

    _Use_decl_annotations_
    PAGED_CODE_SEG
    NTSTATUS
    AcpiReader::ParseULongLong(
        _In_  PACPI_METHOD_ARGUMENT     Argument,
        _Out_ PULONGLONG                Value
    )
    /*++
    Routine Description:

        This function parses the content of an ACPI method argument into a ULONGLONG.

    Arguments:

        Argument - Supplies the ACPI argument to parse.

        Value - Supplies a pointer to the buffer to store the ULONGLONG value.

    Return Value:

        NTSTATUS

    --*/
    {
        NTSTATUS status = STATUS_SUCCESS;

        DrvLogEnter(s_AcpiReaderLog);

        PAGED_CODE();

        if (Argument->Type != ACPI_METHOD_ARGUMENT_INTEGER)
        {
            status = STATUS_INVALID_PARAMETER;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Unexpected Argument Type. Expected: %lu, Actual: %lu, %!STATUS!", ACPI_METHOD_ARGUMENT_INTEGER, Argument->Type, status);
            ASSERT(FALSE);
            goto exit;
        }

        if (Argument->DataLength != sizeof(ULONGLONG))
        {
            status = STATUS_ACPI_INVALID_ARGTYPE;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Unexpected Argument DataLength. Expected: %lu, Actual: %lu, %!STATUS!", sizeof(ULONGLONG), Argument->DataLength, status);
            ASSERT(FALSE);
            goto exit;
        }

        RtlCopyMemory(Value, Argument->Data, Argument->DataLength);

    exit:
        DrvLogExit(s_AcpiReaderLog);
        return status;
    }

    _Use_decl_annotations_
    PAGED_CODE_SEG
    NTSTATUS
    AcpiReader::ParseULong(
        _In_  PACPI_METHOD_ARGUMENT     Argument,
        _Out_ PULONG                    Value
    )
    /*++
    Routine Description:

        This function parses the content of an ACPI method argument into a ULONG.

    Arguments:

        Argument - Supplies the ACPI argument to parse.

        Value - Supplies a pointer to the buffer to store the ULONG value.

    Return Value:

        NTSTATUS

    --*/
    {
        NTSTATUS status = STATUS_SUCCESS;

        DrvLogEnter(s_AcpiReaderLog);

        PAGED_CODE();

        if (Argument->Type != ACPI_METHOD_ARGUMENT_INTEGER)
        {
            status = STATUS_INVALID_PARAMETER;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Unexpected Argument Type. Expected: %lu, Actual: %lu, %!STATUS!", ACPI_METHOD_ARGUMENT_INTEGER, Argument->Type, status);
            ASSERT(FALSE);
            goto exit;
        }

        // Even though we are looking for a ULONG value, the DataLength will be set to ULONGLONG since we always use IOCTL_ACPI_EVAL_METHOD_EX
        if (Argument->DataLength != sizeof(ULONGLONG))
        {
            status = STATUS_ACPI_INVALID_ARGTYPE;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Unexpected Argument DataLength. Expected: %lu, Actual: %lu, %!STATUS!", sizeof(ULONGLONG), Argument->DataLength, status);
            ASSERT(FALSE);
            goto exit;
        }

        *Value = (ULONG)Argument->Argument;

    exit:
        DrvLogExit(s_AcpiReaderLog);
        return status;
    }

    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    AcpiReader::ParseString(
        _In_  PACPI_METHOD_ARGUMENT                     Argument,
        _Out_writes_opt_z_(ValueStringSize) char *      ValueString,
        _In_  ULONG                                     ValueStringSize,
        _Out_ PULONG                                    PropertyValueSize
    )
    /*++
    Routine Description:

        This function parses the content of an ACPI method argument into a string.

    Arguments:

        Argument - Supplies the ACPI argument to parse.

        ValueString - Buffer that will hold property value if found.

        ValueStringSize - Size of the output buffer.

        PropertyValueSize - Actual length of the property value.

    Return Value:

        NTSTATUS

    --*/
    {
        NTSTATUS status = STATUS_SUCCESS;

        DrvLogEnter(s_AcpiReaderLog);

        PAGED_CODE();

        if (Argument->Type != ACPI_METHOD_ARGUMENT_STRING)
        {
            status = STATUS_INVALID_PARAMETER;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Unexpected Argument Type. Expected %lu, Actual %lu, %!STATUS!", ACPI_METHOD_ARGUMENT_STRING, Argument->Type, status);
            ASSERT(FALSE);
            goto exit;
        }

        *PropertyValueSize = Argument->DataLength;

        if (ValueStringSize == 0)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            goto exit;
        }
        else if (ValueStringSize < Argument->DataLength)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Output buffer too small. Required %lu, Actual %lu, %!STATUS!", Argument->DataLength, ValueStringSize, status);
            goto exit;
        }
#pragma prefast(suppress:__WARNING_PRECONDITION_NULLTERMINATION_VIOLATION, "ACPI driver returns a NULL-terminated string.")
        status = RtlStringCbCopyA(ValueString, ValueStringSize, (char *)Argument->Data);

    exit:
        DrvLogExit(s_AcpiReaderLog);
        return status;
    }

    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    AcpiReader::ParseBuffer(
        _In_  PACPI_METHOD_ARGUMENT                     Argument,
        _Out_writes_bytes_(ValueBufferSize) PVOID       ValueBuffer,
        _In_  ULONG                                     ValueBufferSize,
        _Out_ PULONG                                    PropertyValueSize
    )
    /*++
    Routine Description:

        This function parses the content of an ACPI method argument into a buffer.

    Arguments:

        Argument - Supplies the ACPI argument to parse.

        ValueBuffer - Buffer that will hold property value if found.

        ValueBufferSize - Size of the output buffer.

        PropertyValueSize - Actual length of the property value.

    Return Value:

        NTSTATUS

    --*/
    {
        NTSTATUS status = STATUS_SUCCESS;

        DrvLogEnter(s_AcpiReaderLog);

        PAGED_CODE();

        if (Argument->Type != ACPI_METHOD_ARGUMENT_BUFFER)
        {
            status = STATUS_INVALID_PARAMETER;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Unexpected Argument Type. Expected %lu, Actual %lu, %!STATUS!", ACPI_METHOD_ARGUMENT_BUFFER, Argument->Type, status);
            ASSERT(FALSE);
            goto exit;
        }

        *PropertyValueSize = Argument->DataLength;

        if (ValueBufferSize < Argument->DataLength)
        {
            status = STATUS_BUFFER_TOO_SMALL;

            // DrvLogVerbose if ValueBufferSize is 0, which means it's being called to determine size. Otherwise, DrvLogError.
            if (ValueBufferSize == 0)
            {
                DrvLogVerbose(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Output buffer too small. Required %lu, Actual %lu, %!STATUS!", Argument->DataLength, ValueBufferSize, status);
            }
            else
            {
                DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Output buffer too small. Required %lu, Actual %lu, %!STATUS!", Argument->DataLength, ValueBufferSize, status);
            }
            
            goto exit;
        }

        RtlCopyMemory(ValueBuffer, Argument->Data, ValueBufferSize);

    exit:
        DrvLogExit(s_AcpiReaderLog);
        return status;
    }

    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    AcpiReader::ParseULongArray(
        _In_  PACPI_METHOD_ARGUMENT                     Argument,
        _Out_writes_(ValueArrayCount) ULONG *           ValueArray,
        _In_  ULONG                                     ValueArrayCount,
        _Out_ PULONG                                    PropertyValueArrayCount
    )
    {
        NTSTATUS                            status = STATUS_SUCCESS;
        PACPI_METHOD_ARGUMENT               currentArgument;
        ULONG                               argumentIndex;

        DrvLogEnter(s_AcpiReaderLog);

        PAGED_CODE();

        if (Argument->Type != ACPI_METHOD_ARGUMENT_PACKAGE)
        {
            status = STATUS_INVALID_PARAMETER;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Unexpected Argument Type. Expected %lu, Actual %lu, %!STATUS!", ACPI_METHOD_ARGUMENT_PACKAGE, Argument->Type, status);
            ASSERT(FALSE);
            goto exit;
        }

        // Initialize everything to 0
        for (ULONG i = 0; i < ValueArrayCount; ++i)
        {
            ValueArray[i] = 0;
        }

        *PropertyValueArrayCount = 0;
        currentArgument = (PACPI_METHOD_ARGUMENT)Argument->Data;

        for (argumentIndex = 0; (PUCHAR)currentArgument < (PUCHAR)Argument->Data + Argument->DataLength; argumentIndex++)
        {
#pragma prefast(suppress:26014, "Incorrect Validation: ACPI driver returns well-formed data that doesn't extend past known length.")
            if (currentArgument->Type != ACPI_METHOD_ARGUMENT_INTEGER)
            {
                status = STATUS_INVALID_PARAMETER;
                DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Unexpected argument in an array, %!STATUS!", status);
                ASSERT(FALSE);
                goto exit;
            }

            (*PropertyValueArrayCount)++;
            currentArgument = ACPI_METHOD_NEXT_ARGUMENT(currentArgument);
        }

        if (ValueArrayCount == 0)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            goto exit;
        }
        else if (ValueArrayCount < *PropertyValueArrayCount)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Output array too small. Required elements %lu, Actual %lu, %!STATUS!", *PropertyValueArrayCount, ValueArrayCount, status);
            goto exit;
        }

        currentArgument = (PACPI_METHOD_ARGUMENT)Argument->Data;
        for (argumentIndex = 0; (PUCHAR)currentArgument < (PUCHAR)Argument->Data + Argument->DataLength && argumentIndex < ValueArrayCount; argumentIndex++)
        {
            status = ParseULong(currentArgument, &ValueArray[argumentIndex]);
            if (!NT_SUCCESS(status))
            {
                DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Unexpected argument in an array, %!STATUS!", status);
                ASSERT(FALSE);
                goto exit;
            }

            currentArgument = ACPI_METHOD_NEXT_ARGUMENT(currentArgument);
        }

    exit:
        DrvLogExit(s_AcpiReaderLog);
        return status;
    }

    _Use_decl_annotations_
    PAGED_CODE_SEG
    NTSTATUS
    AcpiReader::EnumChildren(
        _Out_ WDFMEMORY *   EnumChildrenOutput
    )
    /*++
    Routine Description:

        This function sends IOCTL_ACPI_ENUM_CHILDREN to ACPI to enumerate child devices.

    Arguments:

        EnumChildrenOutput - Supplies a resulting memory object.

    Return Value:

        NTSTATUS code.

    --*/
    {
        NTSTATUS                                status;
        WDFMEMORY                               inputMem{ WDF_NO_HANDLE };
        PACPI_ENUM_CHILDREN_INPUT_BUFFER        inputBuf;
        size_t                                  inputBufSize;
        WDF_MEMORY_DESCRIPTOR                   inputMemDesc;
        WDFMEMORY                               outputMem{ WDF_NO_HANDLE };
        PACPI_ENUM_CHILDREN_OUTPUT_BUFFER       outputBuf;
        size_t                                  outputBufSize;
        WDF_MEMORY_DESCRIPTOR                   outputMemDesc;
        WDF_OBJECT_ATTRIBUTES                   attr;
        ULONG                                   attempts;
        WDFIOTARGET                             acpiIoTarget;
        WDF_REQUEST_SEND_OPTIONS                sendOptions;

        PAGED_CODE();

        DrvLogEnter(s_AcpiReaderLog);

        ASSERT(m_AcpiDevice);

        acpiIoTarget = WdfDeviceGetIoTarget(m_AcpiDevice);

        WDF_OBJECT_ATTRIBUTES_INIT(&attr);
        attr.ParentObject = m_AcpiDevice;

        inputBufSize = sizeof(*inputBuf);
        status = WdfMemoryCreate(
            &attr,
            NonPagedPoolNx,
            s_MemoryTag,
            inputBufSize,
            &inputMem,
            (PVOID*)&inputBuf);

        if (!NT_SUCCESS(status))
        {
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! WdfMemoryCreate failed for inputBuf, %!STATUS!", status);
            ASSERT(FALSE);
            goto exit;
        }

        RtlZeroMemory(inputBuf, inputBufSize);
        inputBuf->Signature = ACPI_ENUM_CHILDREN_INPUT_BUFFER_SIGNATURE;
        inputBuf->Flags = ENUM_CHILDREN_IMMEDIATE_ONLY;

        WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&inputMemDesc, inputMem, nullptr);

        //
        // The initial output buffer allows one child only. It will be re-allocated
        // with the returning "NumberOfChildren" bytes when IOCTL_ACPI_ENUM_CHILDREN 
        // fails with STATUS_BUFFER_OVERFLOW. The returning "NumberOfChildren" is 
        // not the number of children, but the required size in bytes.
        //
        outputBufSize = sizeof(*outputBuf);
        attempts = 0;

        do
        {
            WDF_OBJECT_ATTRIBUTES_INIT(&attr);
            attr.ParentObject = m_AcpiDevice;

            status = WdfMemoryCreate(
                &attr,
                NonPagedPoolNx,
                s_MemoryTag,
                outputBufSize,
                &outputMem,
                (PVOID*)&outputBuf);

            if (!NT_SUCCESS(status))
            {
                DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! WdfMemoryCreate failed for outputBuf, %!STATUS!", status);
                ASSERT(FALSE);
                goto exit;
            }

            WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&outputMemDesc, outputMem, nullptr);

            WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, 0);
            WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions, WDF_REL_TIMEOUT_IN_SEC(ACPI_REQUEST_TIMEOUT_SEC));

            status = WdfIoTargetSendIoctlSynchronously(
                acpiIoTarget,
                NULL,
                IOCTL_ACPI_ENUM_CHILDREN,
                &inputMemDesc,
                &outputMemDesc,
                &sendOptions,
                nullptr);

            if (NT_SUCCESS(status))
            {
                if (outputBuf->Signature != ACPI_ENUM_CHILDREN_OUTPUT_BUFFER_SIGNATURE)
                {
                    status = STATUS_ACPI_INVALID_DATA;
                    DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Invalid data in ACPI_ENUM_CHILDREN_OUTPUT_BUFFER, %!STATUS!", status);
                    ASSERT(FALSE);
                    goto exit;
                }

                //
                // There must be at least one, because this device is included in the list.
                // When IOCTL_ACPI_ENUM_CHILDREN succeeds, "NumberOfChildren" does have 
                // the number of children. (When the IOCTL fails, it's the required size
                // in bytes.)
                //
                if (outputBuf->NumberOfChildren < 1)
                {
                    status = STATUS_ACPI_INVALID_DATA;
                    DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! No child devices in ACPI_ENUM_CHILDREN_OUTPUT_BUFFER, %!STATUS!", status);
                    ASSERT(FALSE);
                    goto exit;
                }

                //
                // Return the output memory object.
                //
                *EnumChildrenOutput = outputMem;
                outputMem = WDF_NO_HANDLE;

                break;
            }

            if (status != STATUS_BUFFER_OVERFLOW)
            {
                DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! IOCTL_ACPI_ENUM_CHILDREN _BUFFER, %!STATUS!", status);
                // No assert since this is common in sdca bringup
                goto exit;
            }

            if (outputBuf->Signature != ACPI_ENUM_CHILDREN_OUTPUT_BUFFER_SIGNATURE)
            {
                status = STATUS_ACPI_INVALID_DATA;
                DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Invalid data in ACPI_ENUM_CHILDREN_OUTPUT_BUFFER, %!STATUS!", status);
                ASSERT(FALSE);
                goto exit;
            }

            //
            // When IOCTL_ACPI_ENUM_CHILDREN fails with STATUS_BUFFER_OVERFLOW,
            // "NumberOfChildren" is not the number of children, but the required 
            // size in bytes.
            //
            outputBufSize = outputBuf->NumberOfChildren;
            WdfObjectDelete(outputMem);
            outputMem = WDF_NO_HANDLE;
            attempts++;
        } while (attempts < 2);

    exit:

        if (inputMem != WDF_NO_HANDLE)
        {
            WdfObjectDelete(inputMem);
            inputMem = WDF_NO_HANDLE;
        }

        if (outputMem != WDF_NO_HANDLE)
        {
            WdfObjectDelete(outputMem);
            outputMem = WDF_NO_HANDLE;
        }

        DrvLogExit(s_AcpiReaderLog);
        return status;
    }

    _Use_decl_annotations_
    PAGED_CODE_SEG
    NTSTATUS
    AcpiReader::EvaluateMethod(
        _In_  LPCSTR        MethodName,
        _Out_ WDFMEMORY *   ReturnMemory
    )
    /*++
    Routine Description:

        This function sends IOCTL_ACPI_EVAL_METHOD_EX to ACPI to evaluate a method.

    Arguments:

        MethodName - Supplies a packed string identifying the method.

        ReturnMemory - Supplies the resulting memory object.

    Return Value:

        NTSTATUS code.

    --*/
    {
        const ULONG                     InitialControlMethodOutputSize = 0x200; // 512 bytes
        UCHAR                           attempts;
        WDF_MEMORY_DESCRIPTOR           inputDesc;
        WDFMEMORY                       outputMem{ WDF_NO_HANDLE };
        PACPI_EVAL_OUTPUT_BUFFER        outputBuf;
        ULONG                           outputBufLength;
        WDF_MEMORY_DESCRIPTOR           outputDesc;
        ULONG_PTR                       sizeReturned;
        ACPI_EVAL_INPUT_BUFFER_EX       inputBuf;
        WDF_OBJECT_ATTRIBUTES           attr;
        WDFIOTARGET                     acpiIoTarget;
        NTSTATUS                        status;
        WDF_REQUEST_SEND_OPTIONS        sendOptions;

        DrvLogEnter(s_AcpiReaderLog);

        PAGED_CODE();

        ASSERT(m_AcpiDevice);

        acpiIoTarget = WdfDeviceGetIoTarget(m_AcpiDevice);

        //
        // Prepare an input buffer.
        //
        inputBuf.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE_EX;

        status = RtlStringCchCopyA(
            inputBuf.MethodName,
            sizeof(inputBuf.MethodName),
            MethodName);

        if (!NT_SUCCESS(status))
        {
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! RtlStringCchCopyA failed to copy ACPI method name, %!STATUS!", status);
            ASSERT(FALSE);
            goto exit;
        }

        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
            &inputDesc,
            (PVOID)&inputBuf,
            sizeof(ACPI_EVAL_INPUT_BUFFER_EX));

        //
        // Set the initial size for the output buffer to be allocated.
        //
        outputBuf = NULL;
        outputBufLength = InitialControlMethodOutputSize;
        attempts = 0;

        do
        {
            WDF_OBJECT_ATTRIBUTES_INIT(&attr);
            attr.ParentObject = m_AcpiDevice;

            status = WdfMemoryCreate(
                &attr,
                NonPagedPoolNx,
                s_MemoryTag,
                outputBufLength,
                &outputMem,
                (PVOID*)&outputBuf);

            if (!NT_SUCCESS(status))
            {
                DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! WdfMemoryCreate failed for %Iu bytes, %!STATUS!", outputBufLength, status);
                ASSERT(FALSE);
                goto exit;
            }

            RtlZeroMemory(outputBuf, outputBufLength);

            WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
                &outputDesc,
                (PVOID)outputBuf,
                outputBufLength);

            WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, 0);
            WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions, WDF_REL_TIMEOUT_IN_SEC(ACPI_REQUEST_TIMEOUT_SEC));

            status = WdfIoTargetSendIoctlSynchronously(
                acpiIoTarget,
                NULL,
                IOCTL_ACPI_EVAL_METHOD_EX,
                &inputDesc,
                &outputDesc,
                &sendOptions,
                &sizeReturned);

            if (NT_SUCCESS(status))
            {
                //
                // IOCTL_ACPI_EVAL_METHOD_EX succeeded.
                //
                if (sizeReturned == 0)
                {
                    status = STATUS_UNSUCCESSFUL;
                    DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! IOCTL_ACPI_EVAL_METHOD_EX returned 0 byte, %!STATUS!", status);
                    ASSERT(FALSE);
                    goto exit;
                }

                if (outputBuf->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE)
                {
                    status = STATUS_ACPI_INVALID_DATA;
                    DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! ACPI_EVAL_OUTPUT_BUFFER signature (0x%x) is incorrect, %!STATUS!", outputBuf->Signature, status);
                    ASSERT(FALSE);
                    goto exit;
                }

                //
                // Return the output memory object.
                //
                *ReturnMemory = outputMem;
                outputMem = WDF_NO_HANDLE;

                break;
            }

            if (status != STATUS_BUFFER_OVERFLOW)
            {
                DrvLogError(s_AcpiReaderLog, FLAG_INIT, "!FUNC! IOCTL_ACPI_EVAL_METHOD_EX failed , %!STATUS!", status);
                // Failure is common when used alongside virtual stack
                goto exit;
            }

            //
            // If the output buffer was insufficient, then re-allocate one with
            // appropriate size and retry.
            //
            outputBufLength = outputBuf->Length;
            WdfObjectDelete(outputMem);
            outputMem = WDF_NO_HANDLE;
            attempts++;

            if (attempts == 2)
            {
                DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! IOCTL_ACPI_EVAL_METHOD_EX has failed for %u times. Stopped retrying., %!STATUS!", attempts, status);
                ASSERT(FALSE);
            }
        } while (attempts < 2);

    exit:

        if (outputMem != WDF_NO_HANDLE)
        {
            WdfObjectDelete(outputMem);
            outputMem = WDF_NO_HANDLE;
        }

        DrvLogExit(s_AcpiReaderLog);

        return status;
    }

    _Use_decl_annotations_
    PAGED_CODE_SEG
    NTSTATUS
    AcpiReader::EvaluateAdr(
        _In_opt_    LPCSTR        ChildDeviceName,
        _Out_       PULONGLONG    Address
    )
    /*++
    Routine Description:

        This function evaluates a _ADR method.

    Arguments:

        ChildDeviceName - Supplies a child device name. If Null, evaluate
                          the _ADR for the current device instead.

        Address - Returning the device address from _ADR.

    Return Value:

        NTSTATUS code.

    --*/
    {
        NTSTATUS                        status;
        CHAR                            fullMethodName[MAX_PATH];
        WDFMEMORY                       outputMem{ WDF_NO_HANDLE };
        PACPI_EVAL_OUTPUT_BUFFER        outputBuf;

        DrvLogEnter(s_AcpiReaderLog);

        PAGED_CODE();

        if (!Address)
        {
            status = STATUS_INVALID_PARAMETER;
            goto exit;
        }

        if (ChildDeviceName != nullptr)
        {
            status = RtlStringCchPrintfA(
                fullMethodName,
                sizeof(fullMethodName),
                "%s._ADR",
                ChildDeviceName);
        }
        else
        {
            status = RtlStringCchCopyA(
                fullMethodName,
                sizeof(fullMethodName),
                "_ADR");
        }

        if (!NT_SUCCESS(status))
        {
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! RtlStringCchPrintfA for creating method name _BUFFER, %!STATUS!", status);
            ASSERT(FALSE);
            goto exit;
        }

        status = EvaluateMethod(
            fullMethodName,
            &outputMem);

        if (!NT_SUCCESS(status))
        {
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! EvaluateMethod failed on [%s], %!STATUS!", fullMethodName, status);
            ASSERT(FALSE);
            goto exit;
        }

        outputBuf = (PACPI_EVAL_OUTPUT_BUFFER)WdfMemoryGetBuffer(outputMem, NULL);

        if (outputBuf->Count < 1)
        {
            status = STATUS_ACPI_INVALID_DATA;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! _ADR of [%s] didn't return anything, %!STATUS!", fullMethodName, status);
            ASSERT(FALSE);
            goto exit;
        }

        if (outputBuf->Argument[0].Type != ACPI_METHOD_ARGUMENT_INTEGER)
        {
            status = STATUS_ACPI_INVALID_DATA;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! _ADR of [%s] returned an unexpected argument of type %hu, %!STATUS!", fullMethodName, outputBuf->Argument[0].Type, status);
            ASSERT(FALSE);
            goto exit;
        }

        status = ParseULongLong(outputBuf->Argument, Address);

        if (!NT_SUCCESS(status))
        {
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Unexpected argument from _ADR of [%s], %!STATUS!", fullMethodName, status);
            ASSERT(FALSE);
            goto exit;
        }

    exit:
        if (outputMem != WDF_NO_HANDLE)
        {
            WdfObjectDelete(outputMem);
            outputMem = WDF_NO_HANDLE;
        }

        DrvLogExit(s_AcpiReaderLog);
        return status;
    }

    _Use_decl_annotations_
    PAGED_CODE_SEG
    NTSTATUS
    AcpiReader::EvaluateAdr(
        _Out_ PULONGLONG    Address
        )
    /*++
    Routine Description:

        This function evaluates the _ADR method for the current device.

    Arguments:

        Address - Returning the device address from _ADR.

    Return Value:

        NTSTATUS code.

    --*/

    {
        NTSTATUS                                  status;

        DrvLogEnter(s_AcpiReaderLog);

        PAGED_CODE();

        status = EvaluateAdr(nullptr, Address);

        DrvLogExit(s_AcpiReaderLog);
        return status;
    }


    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    AcpiReader::GetPropertyString(
        _In_  LPCSTR                                    PropertyName,
        _In_  ACPI_METHOD_SECTION                       PropertySection,
        _In_  WDFMEMORY                                 AcpiEvalOutputBuf,
        _Out_writes_opt_z_(ValueStringSize) char *      ValueString,
        _In_  ULONG                                     ValueStringSize,
        _Out_ PULONG                                    PropertyValueSize
     )
    /*++
    Routine Description:

        This function searches for PropertyName in ACPI_EVAL_OUTPUT_BUFFER and returns
        string value for the property if found.

    Arguments:

        PropertyName - Property name to search for.

        PropertySection - Specifies if property is under device property or hierarchical
                          data extension section.

        AcpiEvalOutputBuf - WDFMEMORY containing ACPI_EVAL_OUTPUT_BUFFER in
                            which property needs to be searched.

        ValueString - Buffer that will hold property value if found.

        ValueStringSize - Size of the output buffer.

        PropertyValueSize - Actual length of the property value.

    Return Value:

        NTSTATUS code.

    --*/
    {
        NTSTATUS                            status = STATUS_NOT_FOUND;
        PACPI_METHOD_ARGUMENT               propValueArg = NULL;

        DrvLogEnter(s_AcpiReaderLog);

        PAGED_CODE();

        if (!PropertyName || !PropertyValueSize || (AcpiEvalOutputBuf == WDF_NO_HANDLE))
        {
            status = STATUS_INVALID_PARAMETER;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Invalid PropertyName, PropertyValueSize or AcpiEvalOutputBuf, %!STATUS!", status);
            goto exit;
        }
        if (ValueStringSize > 0 && !ValueString)
        {
            status = STATUS_INVALID_PARAMETER;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Null ValueString with non-zero ValueStringSize, %!STATUS!", status);
            goto exit;
        }

        status = GetProperty(PropertyName, PropertySection, AcpiEvalOutputBuf, &propValueArg);

        if (NT_SUCCESS(status))
        {
            status = ParseString(propValueArg, ValueString, ValueStringSize, PropertyValueSize);
        }

    exit:
        return status;
    }

    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    AcpiReader::GetPropertyULongLong(
        _In_  LPCSTR                       PropertyName,
        _In_  ACPI_METHOD_SECTION          PropertySection,
        _In_  WDFMEMORY                    AcpiEvalOutputBuf,
        _Out_ PULONGLONG                   PropertyValue
    )
    /*++
    Routine Description:

        This function searches for PropertyName in ACPI_EVAL_OUTPUT_BUFFER and returns
        ULONGLONG value for the property if found.

    Arguments:

        PropertyName - Property name to search for.

        PropertySection - Specifies if property is under device property or hierarchical
                          data extension section.

        AcpiEvalOutputBuf - WDFMEMORY containing ACPI_EVAL_OUTPUT_BUFFER in
                            which property needs to be searched.

        PropertyValue - Value of the property.

    Return Value:

        NTSTATUS code.

    --*/
    {
        NTSTATUS                            status = STATUS_NOT_FOUND;
        PACPI_METHOD_ARGUMENT               propValueArg = NULL;

        DrvLogEnter(s_AcpiReaderLog);

        PAGED_CODE();

        if (!PropertyName || !PropertyValue || (AcpiEvalOutputBuf == WDF_NO_HANDLE))
        {
            status = STATUS_INVALID_PARAMETER;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Invalid PropertyName, PropertyValue or AcpiEvalOutputBuf, %!STATUS!", status);
            goto exit;
        }

        status = GetProperty(PropertyName, PropertySection, AcpiEvalOutputBuf, &propValueArg);

        if (NT_SUCCESS(status))
        {
            status = ParseULongLong(propValueArg, PropertyValue);
        }

    exit:
        return status;
    }

    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    AcpiReader::GetPropertyULong(
        _In_  LPCSTR                       PropertyName,
        _In_  ACPI_METHOD_SECTION          PropertySection,
        _In_  WDFMEMORY                    AcpiEvalOutputBuf,
        _Out_ PULONG                       PropertyValue
    )
    /*++
    Routine Description:

        This function searches for PropertyName in ACPI_EVAL_OUTPUT_BUFFER and returns
        ULONG value for the property if found.

    Arguments:

        PropertyName - Property name to search for.

        PropertySection - Specifies if property is under device property or hierarchical
                          data extension section.

        AcpiEvalOutputBuf - WDFMEMORY containing ACPI_EVAL_OUTPUT_BUFFER in
                            which property needs to be searched.

        PropertyValue - Value of the property.

    Return Value:

        NTSTATUS code.

    --*/
    {
        NTSTATUS                            status = STATUS_NOT_FOUND;
        PACPI_METHOD_ARGUMENT               propValueArg = NULL;

        DrvLogEnter(s_AcpiReaderLog);

        PAGED_CODE();

        if (!PropertyName || !PropertyValue || (AcpiEvalOutputBuf == WDF_NO_HANDLE))
        {
            status = STATUS_INVALID_PARAMETER;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Invalid PropertyName, PropertyValue or AcpiEvalOutputBuf, %!STATUS!", status);
            goto exit;
        }

        status = GetProperty(PropertyName, PropertySection, AcpiEvalOutputBuf, &propValueArg);

        if (NT_SUCCESS(status))
        {
            status = ParseULong(propValueArg, PropertyValue);
        }

    exit:
        DrvLogExit(s_AcpiReaderLog);

        return status;
    }

    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    AcpiReader::GetPropertyBuffer(
        _In_  LPCSTR                                    PropertyName,
        _In_  WDFMEMORY                                 AcpiEvalOutputBuf,
        _Out_writes_bytes_(ValueBufferSize) PVOID       ValueBuffer,
        _In_  ULONG                                     ValueBufferSize,
        _Out_ PULONG                                    PropertyValueSize
    )
    /*++
    Routine Description:

        This function searches for PropertyName in ACPI_EVAL_OUTPUT_BUFFER and returns
        buffer value for the property if found.

    Arguments:

        PropertyName - Property name to search for.

        AcpiEvalOutputBuf - WDFMEMORY containing ACPI_EVAL_OUTPUT_BUFFER in
                            which property needs to be searched.

        ValueBuffer - Buffer that will hold property value if found.

        ValueBufferSize - Size of the output buffer.

        PropertyValueSize - Actual length of the property value.

    Return Value:

        NTSTATUS code.

    --*/
    {
        NTSTATUS                            status = STATUS_NOT_FOUND;
        PACPI_METHOD_ARGUMENT               propValueArg = NULL;
        char                                methodName[MAX_PATH];
        ULONG                               valueSize = 0;
        WDFMEMORY                           bufferBlock = nullptr;
        PACPI_EVAL_OUTPUT_BUFFER            buffer;

        DrvLogEnter(s_AcpiReaderLog);

        PAGED_CODE();

        if (!PropertyName || !PropertyValueSize || (AcpiEvalOutputBuf == WDF_NO_HANDLE))
        {
            status = STATUS_INVALID_PARAMETER;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Invalid PropertyName, PropertyValueSize or AcpiEvalOutputBuf, %!STATUS!", status);
            goto exit;
        }
        //  Property we are searching is expected to return a buffer,
        //  so this property will be under Buffer UUID section
        //   ToUUID("EDB12DD0-363D-4085-A3D2-49522CA160C4"),
        //   Package() {
        //     Package { Property, "BUF0"}
        //   }
        status = GetProperty(PropertyName, ACPI_METHOD_SECTION_BUFFER, AcpiEvalOutputBuf, &propValueArg);

        if (!NT_SUCCESS(status))
        {
            // No need to log an error as it may be an optional property and not expected to be present all the time.
            goto exit;
        }

        // Value of the property will be method name
        status = ParseString(propValueArg, methodName, sizeof(methodName), &valueSize);
        if (!NT_SUCCESS(status))
        {
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Failed to retrieve method name for %hs, %!STATUS!", PropertyName, status);
            goto exit;
        }

        // Evaluate method which will return contents of the buffer
        // e.g. Evaluate method "BUF0"
        status = EvaluateMethod(methodName, &bufferBlock);
        if (!NT_SUCCESS(status))
        {
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Failed to evaluate method %hs, %!STATUS!",
                methodName,
                status);
            goto exit;
        }

        buffer = (PACPI_EVAL_OUTPUT_BUFFER)WdfMemoryGetBuffer(bufferBlock, NULL);
        // This method must contain only one ACPI argument
        if (buffer->Count != 1)
        {
            status = STATUS_INVALID_PARAMETER;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Method %hs has argument count %d, expected 1, %!STATUS!",
                methodName,
                buffer->Count,
                status);
            goto exit;
        }

        status = ParseBuffer(buffer->Argument, ValueBuffer, ValueBufferSize, PropertyValueSize);
 
    exit:
        if (bufferBlock != nullptr)
        {
            WdfObjectDelete(bufferBlock);
            bufferBlock = nullptr;
        }

        DrvLogExit(s_AcpiReaderLog);

        return status;
    }

    __drv_maxIRQL(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    AcpiReader::GetPropertyULongArray(
        _In_  LPCSTR                                    PropertyName,
        _In_  ACPI_METHOD_SECTION                       PropertySection,
        _In_  WDFMEMORY                                 AcpiEvalOutputBuf,
        _Out_writes_(ValueArrayCount) ULONG *           ValueArray,
        _In_  ULONG                                     ValueArrayCount,
        _Out_ PULONG                                    PropertyValueArrayCount
    )
    /*++
    Routine Description:

        This function searches for PropertyName in ACPI_EVAL_OUTPUT_BUFFER and returns
        an array of ULONGs for the property if found.

    Arguments:

        PropertyName - Property name to search for.

        PropertySection - Specifies if property is under device property or hierarchical
                          data extension section.

        AcpiEvalOutputBuf - WDFMEMORY containing ACPI_EVAL_OUTPUT_BUFFER in
                            which property needs to be searched.

        ValueArray - ULONG array that will hold property values if found.

        ValueArrayCount - Total count of elements in ValueArray.

        PropertyValueArrayCount - Valid count of elements in ValueArray.

    Return Value:

        NTSTATUS code.

    --*/
    {
        NTSTATUS                            status = STATUS_NOT_FOUND;
        PACPI_METHOD_ARGUMENT               PropValueArg = NULL;

        DrvLogEnter(s_AcpiReaderLog);

        PAGED_CODE();

        if (!PropertyName || !PropertyValueArrayCount || (AcpiEvalOutputBuf == WDF_NO_HANDLE))
        {
            status = STATUS_INVALID_PARAMETER;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Invalid PropertyName, PropertyValueArrayCount or AcpiEvalOutputBuf, %!STATUS!", status);
            goto exit;
        }

        status = GetProperty(PropertyName, PropertySection, AcpiEvalOutputBuf, &PropValueArg);

        if (NT_SUCCESS(status))
        {
            status = ParseULongArray(PropValueArg, ValueArray, ValueArrayCount, PropertyValueArrayCount);
        }

    exit:
        DrvLogExit(s_AcpiReaderLog);

        return status;
    }

    _Use_decl_annotations_
    PAGED_CODE_SEG
    NTSTATUS
    AcpiReader::GetProperty(
        _In_  LPCSTR                                        PropertyName,
        _In_  ACPI_METHOD_SECTION                           PropertySection,
        _In_  WDFMEMORY                                     AcpiEvalOutputBuf,
        _Outptr_result_maybenull_ PACPI_METHOD_ARGUMENT *   PropertyValue
    )
    /*++
    Routine Description:

        This function searches for PropertyName in ACPI_EVAL_OUTPUT_BUFFER and returns
        ACPI_METHOD_ARGUMENT for the property if found.

    Arguments:

        PropertyName - Property name to search for.

        PropertySection - Specifies if property is under device property, hierarchical
                          data extension or buffer section.

        AcpiEvalOutputBuf - WDFMEMORY containing ACPI_EVAL_OUTPUT_BUFFER in
                            which property needs to be searched.

        PropertyValue - ACPI_MEDHOD_ARGUMENT pointer to property value if the property was found.

    Return Value:

        NTSTATUS code.

    --*/
    {
        NTSTATUS                            status = STATUS_NOT_FOUND;
        PACPI_EVAL_OUTPUT_BUFFER            Buffer;
        PACPI_METHOD_ARGUMENT               currentArgument;
        ULONG                               argumentIndex;
        GUID                                guid;
        ACPI_METHOD_SECTION                 section = ACPI_METHOD_SECTION_UNKNOWN;
        BOOL                                found = FALSE;
        size_t                              PropertyLength;
        size_t                              BufferLength;

        DrvLogEnter(s_AcpiReaderLog);

        PAGED_CODE();

        Buffer = (PACPI_EVAL_OUTPUT_BUFFER)WdfMemoryGetBuffer(AcpiEvalOutputBuf, &BufferLength);

        if (BufferLength < FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument))
        {
            status = STATUS_INVALID_PARAMETER;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! WdfMemoryBuffer length %llu too short, %!STATUS!", BufferLength, status);
            ASSERT(FALSE);
            return status;
        }

        if (Buffer->Length > BufferLength)
        {
            status = STATUS_INVALID_PARAMETER;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! ACPI_EVAL_OUTPUT_BUFFER length %lu exceeds WdfMemoryBuffer length %llu, %!STATUS!", Buffer->Length, BufferLength, status);
            ASSERT(FALSE);
            return status;
        }

        // Method structure
        // Name (Method, Package() {
        //   ToUUID("DAFFD814-6EBA-4D8C-8A91-BC9BBF4AA301"),
        //   Package () {
        //     Package (2) { Property, Value }
        //     :
        //     Package (2) { Property, Value }
        //   },
        //   ToUUID("DBB8E3E6-5886-4BA6-8795-1319F52A966B"),
        //   Package() {
        //     Package { Property, Sub-Package}
        //     :
        //     Package { Property, Sub-Package}
        //   }
        //   ToUUID("EDB12DD0-363D-4085-A3D2-49522CA160C4"),
        //   Package() {
        //     Package { Property, Sub-Package}
        //     :
        //     Package { Property, Sub-Package}
        //   }
        // }

        PropertyLength = strlen(PropertyName) + 1; // Add one for NULL terminator as ACPI_METHOD_ARGUMENT Datalength includes it. 

        currentArgument = ACPI_EVAL_OUTPUT_BUFFER_ARGUMENTS_BEGIN(Buffer);
        for (argumentIndex = 0; argumentIndex < Buffer->Count && !found; argumentIndex++)
        {
            if (((PUCHAR)currentArgument + ACPI_METHOD_ARGUMENT_LENGTH(0) > (PUCHAR)ACPI_EVAL_OUTPUT_BUFFER_ARGUMENTS_END(Buffer)) ||
                ((PUCHAR)currentArgument + ACPI_METHOD_ARGUMENT_LENGTH_FROM_ARGUMENT(currentArgument) > (PUCHAR)ACPI_EVAL_OUTPUT_BUFFER_ARGUMENTS_END(Buffer)))
            {
                status = STATUS_INVALID_PARAMETER;
                DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! ACPI_METHOD_ARGUMENT outside of ACPI_EVAL_OUTPUT_BUFFER length, %!STATUS!", status);
                ASSERT(FALSE);
                return status;
            }

            switch (currentArgument->Type)
            {
                case ACPI_METHOD_ARGUMENT_BUFFER:

                    status = ParseGuid(currentArgument, &guid, sizeof(GUID));
                    if (NT_SUCCESS(status) && guid == DSD_DEVICE_PROPERTIES_GUID)
                    {
                        section = ACPI_METHOD_SECTION_DEVICE_PROPERTIES;
                    }
                    else if (NT_SUCCESS(status) && guid == DSD_HIERARCHICAL_DATA_EXTENSION_GUID)
                    {
                        section = ACPI_METHOD_SECTION_HIERARCHICAL_DATA_EXTENSION;
                    }
                    else if (NT_SUCCESS(status) && guid == DSD_BUFFER_GUID)
                    {
                        section = ACPI_METHOD_SECTION_BUFFER;
                    }
                    else
                    {
                        DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Skipping unexpected ACPI_METHOD_ARGUMENT_BUFFER argument, %!STATUS!", status);
                        ASSERT(FALSE);
                        section = ACPI_METHOD_SECTION_UNKNOWN;
                    }
                    break;

                case ACPI_METHOD_ARGUMENT_PACKAGE:

                    // Caller specified the section in which to search property
                    // so further search only if this package is under that section
                    if (section == PropertySection)
                    {
                        // Parse sub-packages
                        status = ParsePropertiesPackage(PropertyName, PropertyLength, currentArgument, PropertyValue);
                        if (NT_SUCCESS(status))
                        {
                            found = TRUE;
                        }
                    }
                    break;

                default:
                    DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Skipping unexpected argument[%d] of type[%d]", argumentIndex, currentArgument->Type);
                    break;
            }
            currentArgument = ACPI_METHOD_NEXT_ARGUMENT(currentArgument);
        }

        if (!found)
        {
            status = STATUS_NOT_FOUND;
        }

        DrvLogExit(s_AcpiReaderLog);
        return status;
    }

    _Use_decl_annotations_
    PAGED_CODE_SEG
    NTSTATUS
    AcpiReader::ParsePropertiesPackage(
        _In_  LPCSTR                                        PropertyName,
        _In_  size_t                                        PropertyLength,
        _In_  PACPI_METHOD_ARGUMENT                         Package,
        _Outptr_result_maybenull_ PACPI_METHOD_ARGUMENT *   PropertyValue
    )
    /*++
    Routine Description:

        This function searches for PropertyName in a package that contains
        property packages. 

     Arguments:

        PropertyName - Property name to search for.

        Package - Pointer to ACPI_METHOD_ARGUMENT containing package under
                  device property or hierarchical data extension section.

        PropertyValue - ACPI_MEDHOD_ARGUMENT pointer to property value if the property was found.

    Return Value:

        NTSTATUS code.
    --*/
    {
        NTSTATUS                            status = STATUS_NOT_FOUND;
        PACPI_METHOD_ARGUMENT               currentArgument;
        ULONG                               argumentIndex;
        BOOL                                found = FALSE;

        DrvLogEnter(s_AcpiReaderLog);

        PAGED_CODE();

        // Package structure
        // Package () {
        //   Package (2) { Property, Value }
        //   :
        //   Package (2) { Property, Value }
        // }

        if (Package->DataLength < ACPI_METHOD_ARGUMENT_LENGTH(0))
        {
            status = STATUS_INVALID_PARAMETER;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Package ACPI_METHOD_ARGUMENT too small, %!STATUS!", status);
            ASSERT(FALSE);
            return status;
        }

        currentArgument = (PACPI_METHOD_ARGUMENT)Package->Data;
        for (argumentIndex = 0 ; ((PUCHAR)currentArgument < (PUCHAR)Package->Data + Package->DataLength) && !found; argumentIndex++)
        {
            if (((PUCHAR)currentArgument + ACPI_METHOD_ARGUMENT_LENGTH(0) > (PUCHAR)Package->Data + Package->DataLength) ||
                ((PUCHAR)currentArgument + ACPI_METHOD_ARGUMENT_LENGTH_FROM_ARGUMENT(currentArgument) > (PUCHAR)Package->Data + Package->DataLength))
            {
                status = STATUS_INVALID_PARAMETER;
                DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! ACPI_METHOD_ARGUMENT outside of package length, %!STATUS!", status);
                ASSERT(FALSE);
                return status;
            }

            switch (currentArgument->Type)
            {
                case ACPI_METHOD_ARGUMENT_PACKAGE:
                    status = FindProperty(PropertyName, PropertyLength, currentArgument, PropertyValue);
                    if (NT_SUCCESS(status))
                    {
                        found = TRUE;
                    }
                    break;

                default:
                    DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Skipping unexpected argument[%d] of type[%d]", argumentIndex, currentArgument->Type);
                    break;
            }
            currentArgument = ACPI_METHOD_NEXT_ARGUMENT(currentArgument);
        }

        if (!found)
        {
            status = STATUS_NOT_FOUND;
        }

        DrvLogExit(s_AcpiReaderLog);
        return status;
    }

    _Use_decl_annotations_
    PAGED_CODE_SEG
    NTSTATUS
    AcpiReader::FindProperty(
        _In_  LPCSTR                                        PropertyName,
        _In_  size_t                                        PropertyLength,
        _In_  PACPI_METHOD_ARGUMENT                         Package,
        _Outptr_result_maybenull_ PACPI_METHOD_ARGUMENT *   PropertyValue
    )
    /*++
    Routine Description:

        This function searches for PropertyName in a package that contains
        a property name and value.

     Arguments:

        PropertyName - Property name to search for.

        Package - Pointer to ACPI_METHOD_ARGUMENT containing package that has
                  property name and value.

        PropertyValue - ACPI_MEDHOD_ARGUMENT pointer to property value if the property was found.

    Return Value:

        NTSTATUS code.
    --*/
    {
        NTSTATUS                            status = STATUS_NOT_FOUND;
        PACPI_METHOD_ARGUMENT               propNameArgument;
        PACPI_METHOD_ARGUMENT               propValArgument;

        DrvLogEnter(s_AcpiReaderLog);

        PAGED_CODE();

        // Property package structure
        // Package (2) { Property, Value }

        propNameArgument = (PACPI_METHOD_ARGUMENT)Package->Data;
        propValArgument = ACPI_METHOD_NEXT_ARGUMENT(propNameArgument);

        if (Package->DataLength < ACPI_METHOD_ARGUMENT_LENGTH(0))
        {
            status = STATUS_INVALID_PARAMETER;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! Package ACPI_METHOD_ARGUMENT too small, %!STATUS!", status);
            ASSERT(FALSE);
            return status;
        }

        if ((PUCHAR)propNameArgument + ACPI_METHOD_ARGUMENT_LENGTH_FROM_ARGUMENT(propNameArgument) >
            (PUCHAR)Package + ACPI_METHOD_ARGUMENT_LENGTH_FROM_ARGUMENT(Package))
        {
            status = STATUS_INVALID_PARAMETER;
            DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! prop name ACPI_METHOD_ARGUMENT outside of package length, %!STATUS!", status);
            ASSERT(FALSE);
            return status;
        }

        if (propNameArgument->Type == ACPI_METHOD_ARGUMENT_STRING)
        {
            if ((PropertyLength == propNameArgument->DataLength) &&
                 !_strnicmp(PropertyName, (char*)propNameArgument->Data, propNameArgument->DataLength))
            {
                if ((PUCHAR)propValArgument + ACPI_METHOD_ARGUMENT_LENGTH_FROM_ARGUMENT(propValArgument) >
                    (PUCHAR)Package + ACPI_METHOD_ARGUMENT_LENGTH_FROM_ARGUMENT(Package))
                {
                    status = STATUS_INVALID_PARAMETER;
                    DrvLogError(s_AcpiReaderLog, FLAG_INIT, "%!FUNC! prop val ACPI_METHOD_ARGUMENT outside of package length, %!STATUS!", status);
                    ASSERT(FALSE);
                    return status;
                }

                *PropertyValue = propValArgument;
                status = STATUS_SUCCESS;
            }
        }

        DrvLogExit(s_AcpiReaderLog);
        return status;
    }

    _Use_decl_annotations_
    PAGED_CODE_SEG
    VOID
    AcpiReader::FreeBuffer(
        _Inout_  WDFMEMORY *        AcpiEvalOutputBuf
    )
    /*++
    Routine Description:

        This function frees memory object.

    Arguments:

        AcpiEvalOutputBuf - Memory object to be freed.

    Return Value:

        VOID

    --*/
    {
        DrvLogEnter(s_AcpiReaderLog);

        PAGED_CODE();

        ASSERT(*AcpiEvalOutputBuf);
        if ((*AcpiEvalOutputBuf) != WDF_NO_HANDLE)
        {
            WdfObjectDelete(*AcpiEvalOutputBuf);
            *AcpiEvalOutputBuf = WDF_NO_HANDLE;
        }

        DrvLogExit(s_AcpiReaderLog);
        return;
    }

    _Use_decl_annotations_
    PAGED_CODE_SEG
    VOID
    AcpiReader::EvtContextDestroy(WDFOBJECT Object)
    {
        PAGED_CODE();

        AcpiReader * context = GetAcpiReaderDeviceContext(Object);
        context->~AcpiReader();
    }
} // namespace ACPIREADER
