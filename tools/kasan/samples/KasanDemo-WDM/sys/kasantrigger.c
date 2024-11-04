/*++

Copyright (c) 2024  Microsoft Corporation All Rights Reserved

Module Name:

    kasantrigger.c

Abstract:

    The purpose of this driver is to demonstrate how KASAN detects illegal
    memory accesses.

Environment:

    Kernel mode only.

--*/

#include <ntddk.h>
#include <string.h>
#include "kasantrigger.h"

#define NT_DEVICE_NAME      L"\\Device\\KASANTRIGGER"
#define DOS_DEVICE_NAME     L"\\DosDevices\\KasanTrigger"

DRIVER_INITIALIZE DriverEntry;

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
DRIVER_DISPATCH KasanTriggerCreateClose;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH KasanTriggerDeviceControl;

DRIVER_UNLOAD KasanTriggerUnloadDriver;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, KasanTriggerCreateClose)
#pragma alloc_text(PAGE, KasanTriggerDeviceControl)
#pragma alloc_text(PAGE, KasanTriggerUnloadDriver)
#endif

// -----------------------------------------------------------------------------

UCHAR GlobalVariable[KASANTRIGGER_ARRAY_SIZE];

static
UCHAR
TriggerOobrStack(
    _In_ ULONG Index
    )
{
    UCHAR StackVariable[KASANTRIGGER_ARRAY_SIZE];

    //
    // This copy is only here to force the compiler not to optimize out the
    // stack variable.
    //

    RtlCopyMemory(StackVariable, GlobalVariable, KASANTRIGGER_ARRAY_SIZE);

    //
    // WARNING: this is an intentionally INCORRECT code!
    //

    return StackVariable[Index];
}

static
UCHAR
TriggerOobrGlobal(
    _In_ ULONG Index
    )
{
    //
    // WARNING: this is an intentionally INCORRECT code!
    //

    return GlobalVariable[Index];
}

static
UCHAR
TriggerOobrHeap(
    _In_ ULONG Index
    )
{
    PCHAR buffer;
    UCHAR retVal;

    buffer = ExAllocatePool2(POOL_FLAG_PAGED, KASANTRIGGER_ARRAY_SIZE, 'mDaK');
    if (buffer == NULL) {
        return 0;
    }

    //
    // WARNING: this is an intentionally INCORRECT code!
    //

    retVal = buffer[Index];

    ExFreePool(buffer);

    return retVal;
}

// -----------------------------------------------------------------------------

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS ntStatus;
    UNICODE_STRING ntUnicodeString;
    UNICODE_STRING ntWin32NameString;
    PDEVICE_OBJECT deviceObject;

    UNREFERENCED_PARAMETER(RegistryPath);

    RtlInitUnicodeString(&ntUnicodeString, NT_DEVICE_NAME);

    ntStatus = IoCreateDevice(DriverObject,
                              0,
                              &ntUnicodeString,
                              FILE_DEVICE_UNKNOWN,
                              FILE_DEVICE_SECURE_OPEN,
                              FALSE,
                              &deviceObject);
    if (!NT_SUCCESS(ntStatus)) {
        return ntStatus;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = KasanTriggerCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = KasanTriggerCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = KasanTriggerDeviceControl;
    DriverObject->DriverUnload = KasanTriggerUnloadDriver;

    RtlInitUnicodeString(&ntWin32NameString, DOS_DEVICE_NAME);

    ntStatus = IoCreateSymbolicLink(&ntWin32NameString, &ntUnicodeString);

    if (!NT_SUCCESS(ntStatus)) {
        IoDeleteDevice(deviceObject);
    }

    return ntStatus;
}

NTSTATUS
KasanTriggerCreateClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

VOID
KasanTriggerUnloadDriver(
    _In_ PDRIVER_OBJECT DriverObject
    )
{
    PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
    UNICODE_STRING uniWin32NameString;

    PAGED_CODE();

    RtlInitUnicodeString(&uniWin32NameString, DOS_DEVICE_NAME);
    IoDeleteSymbolicLink(&uniWin32NameString);

    if (deviceObject != NULL) {
        IoDeleteDevice(deviceObject);
    }

    return;
}

NTSTATUS
KasanTriggerDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS ntStatus;
    ULONG inBufLength;
    ULONG inIndex;
    ULONG outBufLength;
    PUCHAR outValue;
    PBOOLEAN isKasanEnabledOnDriver;

    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    inBufLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    outBufLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

    ntStatus = STATUS_SUCCESS;

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_KASANTRIGGER_INFO:

        //
        // Return information. Just a BOOLEAN that indicates whether the
        // driver was compiled with KASAN or not.
        //

        if (inBufLength != 0 || outBufLength != sizeof(BOOLEAN)) {
            ntStatus = STATUS_INVALID_PARAMETER;
            goto End;
        }

        isKasanEnabledOnDriver = (PBOOLEAN)Irp->AssociatedIrp.SystemBuffer;
#if defined(__SANITIZE_ADDRESS__)
        *isKasanEnabledOnDriver = TRUE;
#else
        *isKasanEnabledOnDriver = FALSE;
#endif

        Irp->IoStatus.Information = sizeof(BOOLEAN);
        break;

    case IOCTL_KASANTRIGGER_OOBR_STACK:

        //
        // Trigger an out-of-bounds read on the stack.
        //

        if (inBufLength != sizeof(ULONG) || outBufLength != sizeof(UCHAR)) {
            ntStatus = STATUS_INVALID_PARAMETER;
            goto End;
        }

        inIndex = *((PULONG)Irp->AssociatedIrp.SystemBuffer);
        outValue = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
        *outValue = TriggerOobrStack(inIndex);

        Irp->IoStatus.Information = sizeof(UCHAR);
        break;

    case IOCTL_KASANTRIGGER_OOBR_GLOBAL:

        //
        // Trigger an out-of-bounds read on a global variable.
        //

        if (inBufLength != sizeof(ULONG) || outBufLength != sizeof(UCHAR)) {
            ntStatus = STATUS_INVALID_PARAMETER;
            goto End;
        }

        inIndex = *((PULONG)Irp->AssociatedIrp.SystemBuffer);
        outValue = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
        *outValue = TriggerOobrGlobal(inIndex);

        Irp->IoStatus.Information = sizeof(UCHAR);
        break;

    case IOCTL_KASANTRIGGER_OOBR_HEAP:

        //
        // Trigger an out-of-bounds read on a heap buffer.
        //

        if (inBufLength != sizeof(ULONG) || outBufLength != sizeof(UCHAR)) {
            ntStatus = STATUS_INVALID_PARAMETER;
            goto End;
        }

        inIndex = *((PULONG)Irp->AssociatedIrp.SystemBuffer);
        outValue = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
        *outValue = TriggerOobrHeap(inIndex);

        Irp->IoStatus.Information = sizeof(UCHAR);
        break;

    default:
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

End:

    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return ntStatus;
}
