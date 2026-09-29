/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    AcpiReader.h

Abstract:

    Contains ACPI reader module.

Environment:

    Kernel mode

--*/

#pragma once

#ifndef _ACPIREADER_H_
#define _ACPIREADER_H_

/* make prototypes usable from C++ */
#ifdef __cplusplus
extern "C" {
#endif

#include <acpiioct.h>

// Number of seconds for ACPI request timeout.
#define ACPI_REQUEST_TIMEOUT_SEC 5

//
// Device properties UUID in the ACPI methods.
// {DAFFD814-6EBA-4D8C-8A91-BC9BBF4AA301}
//
DEFINE_GUID(DSD_DEVICE_PROPERTIES_GUID,
    0xDAFFD814, 0x6EBA, 0x4D8C, 0x8A, 0x91, 0xBC, 0x9B, 0xBF, 0x4A, 0xA3, 0x01);

//
// Hierarchical data extension UUID in the ACPI methods.
// {DBB8E3E6-5886-4BA6-8795-1319F52A966B}
//

DEFINE_GUID(DSD_HIERARCHICAL_DATA_EXTENSION_GUID,
    0xDBB8E3E6, 0x5886, 0x4BA6, 0x87, 0x95, 0x13, 0x19, 0xF5, 0x2A, 0x96, 0x6B);

//
// Buffer UUID in ACPI methods.
// {EDB12DD0-363D-4085-A3D2-49522CA160C4}
//

DEFINE_GUID(DSD_BUFFER_GUID,
    0xEDB12DD0, 0x363D, 0x4085, 0xA3, 0xD2, 0x49, 0x52, 0x2C, 0xA1, 0x60, 0xC4);

namespace ACPIREADER
{
    typedef enum
    {
        ACPI_METHOD_SECTION_UNKNOWN = 0,
        ACPI_METHOD_SECTION_DEVICE_PROPERTIES = 1,
        ACPI_METHOD_SECTION_HIERARCHICAL_DATA_EXTENSION = 2,
        ACPI_METHOD_SECTION_BUFFER = 3
    } ACPI_METHOD_SECTION;

    class AcpiReader
    {
    private:
        WDFDEVICE m_AcpiDevice{ nullptr };

        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        NTSTATUS
        ParseULongLong(
            _In_  PACPI_METHOD_ARGUMENT     Argument,
            _Out_ PULONGLONG                Value);

        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        NTSTATUS
        ParseULong(
            _In_  PACPI_METHOD_ARGUMENT     Argument,
            _Out_ PULONG                    Value);

        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        NTSTATUS
        ParseString(
            _In_  PACPI_METHOD_ARGUMENT                     Argument,
            _Out_writes_opt_z_(ValueStringSize) char *      ValueString,
            _In_  ULONG                                     ValueStringSize,
            _Out_ PULONG                                    PropertyValueSize);

        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        NTSTATUS
        ParseBuffer(
            _In_  PACPI_METHOD_ARGUMENT                     Argument,
            _Out_writes_bytes_(ValueBufferSize) PVOID       ValueBuffer,
            _In_  ULONG                                     ValueBufferSize,
            _Out_ PULONG                                    PropertyValueSize);

        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        NTSTATUS
        ParseGuid(
            _In_ PACPI_METHOD_ARGUMENT                Argument,
            _Out_writes_bytes_(BufferLength) PVOID    Buffer,
            _In_ ULONG                                BufferLength);

        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        NTSTATUS
        GetProperty(
            _In_  LPCSTR                                        PropertyName,
            _In_  ACPI_METHOD_SECTION                           PropertySection,
            _In_  WDFMEMORY                                     AcpiEvalOutputBuf,
            _Outptr_result_maybenull_ PACPI_METHOD_ARGUMENT *   PropertyValue);

        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        NTSTATUS
        ParsePropertiesPackage(
            _In_  LPCSTR                                        PropertyName,
            _In_  size_t                                        PropertyLength,
            _In_  PACPI_METHOD_ARGUMENT                         Package,
            _Outptr_result_maybenull_ PACPI_METHOD_ARGUMENT *   PropertyValue);

        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        NTSTATUS
        FindProperty(
            _In_  LPCSTR                                        PropertyName,
            _In_  size_t                                        PropertyLength,
            _In_  PACPI_METHOD_ARGUMENT                         Package,
            _Outptr_result_maybenull_ PACPI_METHOD_ARGUMENT *   PropertyValue);

        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        NTSTATUS
        ParseULongArray(
            _In_  PACPI_METHOD_ARGUMENT                         Argument,
            _Out_writes_(ValueArrayCount) ULONG *               ValueArray,
            _In_  ULONG                                         ValueArrayCount,
            _Out_ PULONG                                        PropertyValueArrayCount);

    public:
        static
        _Must_inspect_result_
        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        NTSTATUS
        _CreateAndInitialize(_In_ WDFDEVICE Device, _In_ RECORDER_LOG Log, _In_ ULONG MemoryTag);

        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        NTSTATUS
        EnumChildren(
            _Out_ WDFMEMORY *   EnumChildrenOutput);

        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        NTSTATUS
        EvaluateMethod(
            _In_  LPCSTR        MethodName,
            _Out_ WDFMEMORY *   ReturnMemory);

        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        NTSTATUS
        EvaluateAdr(
            _In_opt_ LPCSTR        ChildDeviceName,
            _Out_ PULONGLONG       Address);

        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        NTSTATUS
        EvaluateAdr(
            _Out_ PULONGLONG    Address);

        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        NTSTATUS
        GetPropertyULongLong(
            _In_  LPCSTR                       PropertyName,
            _In_  ACPI_METHOD_SECTION          PropertySection,
            _In_  WDFMEMORY                    AcpiEvalOutputBuf,
            _Out_ PULONGLONG                   PropertyValue);

        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        NTSTATUS
        GetPropertyULong(
            _In_  LPCSTR                       PropertyName,
            _In_  ACPI_METHOD_SECTION          PropertySection,
            _In_  WDFMEMORY                    AcpiEvalOutputBuf,
            _Out_ PULONG                       PropertyValue);

        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        NTSTATUS
        GetPropertyString(
            _In_  LPCSTR                                    PropertyName,
            _In_  ACPI_METHOD_SECTION                       PropertySection,
            _In_  WDFMEMORY                                 AcpiEvalOutputBuf,
            _Out_writes_opt_z_(ValueStringSize) char *      ValueString,
            _In_  ULONG                                     ValueStringSize,
            _Out_ PULONG                                    PropertyValueSize);

        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        NTSTATUS
        GetPropertyBuffer(
            _In_  LPCSTR                                    PropertyName,
            _In_  WDFMEMORY                                 AcpiEvalOutputBuf,
            _Out_writes_bytes_(ValueBufferSize) PVOID       ValueBuffer,
            _In_  ULONG                                     ValueBufferSize,
            _Out_ PULONG                                    PropertyValueSize);

        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        NTSTATUS
        GetPropertyULongArray(
            _In_  LPCSTR                                    PropertyName,
            _In_  ACPI_METHOD_SECTION                       PropertySection,
            _In_  WDFMEMORY                                 AcpiEvalOutputBuf,
            _Out_writes_(ValueArrayCount) ULONG *           ValueArray,
            _In_  ULONG                                     ValueArrayCount,
            _Out_ PULONG                                    PropertyValueArrayCount);

        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        VOID
        FreeBuffer(
            _Inout_  WDFMEMORY*         AcpiEvalOutputBuf);

    protected:

        static
        RECORDER_LOG s_AcpiReaderLog;

        static
        ULONG s_MemoryTag;

        static
        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        EVT_WDF_OBJECT_CONTEXT_DESTROY
        EvtContextDestroy;

        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        AcpiReader(_In_ WDFDEVICE Device) : m_AcpiDevice(Device) { PAGED_CODE(); }

        // Placement-new to construct the object inside the WDF context space.
        static
        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        void*
        operator new (
            _In_ size_t /* SizeInBytes */,
            _In_ void* ContextMemory
            )
        {
            PAGED_CODE();
            // We already have the memory courtesy of WDF so we don't have to allocate anything.
            return ContextMemory;
        }

        static
        __drv_maxIRQL(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        void
        operator delete (
            _In_ void* /* ContextMemory */
            )
        {
            PAGED_CODE();
            // Since we didn't allocate the memory, don't try to deallocate it.
        }

    };

    WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(AcpiReader, GetAcpiReaderDeviceContext)
}
/* make internal prototypes usable from C++ */
#ifdef __cplusplus
}
#endif

#endif // _ACPIREADER_H_
