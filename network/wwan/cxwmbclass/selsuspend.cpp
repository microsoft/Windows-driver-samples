//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include <precomp.h>

_Function_class_(IO_COMPLETION_ROUTINE)
NTSTATUS
MbbUsbBusIdleRequestCompletion(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp, __in PVOID Context)
{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)Context;

    KeSetEvent(&BusObject->UsbSsIrpComplete, IO_NO_INCREMENT, FALSE);

    (*BusObject->IdleNotificationComplete)(BusObject->ProtocolHandle, Irp->IoStatus.Status);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
MbbBusIdleNotification(__in MBB_BUS_HANDLE BusHandle, __in BOOLEAN ForceIdle)
{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IoSp = NULL;

    //
    // This routine is invoked by NDIS when it has detected that the miniport
    // is idle.  The miniport must prepare to issue its selective suspend IRP
    // to the USB stack.  The driver can return NDIS_STATUS_BUSY if it is
    // not ready to go to idle at this moment; NDIS will then retry later.
    // Otherwise, the miniport should return NDIS_STATUS_PENDING.
    //

    KeResetEvent(&BusObject->UsbSsIrpComplete);

    IoReuseIrp(BusObject->UsbSsIrp, STATUS_NOT_SUPPORTED);

    IoSp = IoGetNextIrpStackLocation(BusObject->UsbSsIrp);

    IoSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    IoSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION;
    IoSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(BusObject->UsbSsCallback);
    IoSp->Parameters.DeviceIoControl.Type3InputBuffer = &BusObject->UsbSsCallback;

    IoSetCompletionRoutine(BusObject->UsbSsIrp, MbbUsbBusIdleRequestCompletion, BusObject, TRUE, TRUE, TRUE);

    Status = IoCallDriver(BusObject->Fdo, BusObject->UsbSsIrp);

    if (!NT_SUCCESS(Status))
    {
        KeSetEvent(&BusObject->UsbSsIrpComplete, IO_NO_INCREMENT, FALSE);

        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_PENDING;
}

VOID MbbBusCancelIdleNotification(__in MBB_BUS_HANDLE BusHandle)
{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;

    // This routine is called if NDIS needs to cancel an idle notification.
    // All that is needed is to cancel the selective suspend IRP.
    //

    IoCancelIrp(BusObject->UsbSsIrp);

    return;
}

VOID MbbBusMiniportUsbIdle(PVOID Context)
{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)Context;

    (*BusObject->IdleConfirmCallback)(BusObject->ProtocolHandle, PowerDeviceD2);
    return;
}
