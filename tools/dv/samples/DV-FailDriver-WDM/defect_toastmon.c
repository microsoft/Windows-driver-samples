/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    defect_toastmon.c

Abstract: This sample is designed to demonstrate the Driver Verifier 
          functionality present in Windows.  A bug violating the 
          IrqlPsPassive rule is injected in the ToasterDrvCancelQueuedReadIrps
          callback.  Running the Device Fundamentals PNP Surprise Remove test
          with Driver Verifier enabled on this Defect_Toastmon driver will create
          a bugcheck (0xC4) describing the error.  

          This driver should not be used as a sample to build a new driver from.

Environment:

    Kernel mode

Revision History:

         Injected defect to demonstrate DV capabilities and cleaned sample 
         (7/14/2015)

         Added module to demonstrate how to register and receive WMI
         notification in Kernel-mode (3/3/2004)

--*/
#include "defect_toastmon.h"
#include <initguid.h>
#include <wdmguid.h>
#include "public.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, Defect_ToastMon_AddDevice)
#pragma alloc_text (PAGE, Defect_ToastMon_DispatchPnp)
#pragma alloc_text (PAGE, Defect_ToastMon_DispatchPower)
#pragma alloc_text (PAGE, Defect_ToastMon_Dispatch)
#pragma alloc_text (PAGE, Defect_ToastMon_DispatchRead)
#pragma alloc_text (PAGE, Defect_ToastMon_DispatchSystemControl)
#pragma alloc_text (PAGE, Defect_ToastMon_Unload)
#pragma alloc_text (PAGE, Defect_ToastMon_PnpNotifyDeviceChange)
#pragma alloc_text (PAGE, Defect_ToastMon_PnpNotifyInterfaceChange)
#pragma alloc_text (PAGE, Defect_ToastMon_GetTargetDevicePdo)
#pragma alloc_text (PAGE, Defect_ToastMon_OpenTargetDevice)
#pragma alloc_text (PAGE, Defect_ToastMon_CloseTargetDevice)
#endif

NTSTATUS
DriverEntry(
    __in PDRIVER_OBJECT  DriverObject,
    __in PUNICODE_STRING RegistryPath
    )
	//++
/*

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS

--*/
{

    UNREFERENCED_PARAMETER (RegistryPath);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Entered Driver Entry\n"));

    //
    // Create dispatch points for the IRPs.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE]         = Defect_ToastMon_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = Defect_ToastMon_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Defect_ToastMon_Dispatch;
	DriverObject->MajorFunction[IRP_MJ_READ]           = Defect_ToastMon_DispatchRead;
    DriverObject->MajorFunction[IRP_MJ_PNP]            = Defect_ToastMon_DispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = Defect_ToastMon_DispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] =
                                                Defect_ToastMon_DispatchSystemControl;
    DriverObject->DriverExtension->AddDevice           = Defect_ToastMon_AddDevice;
    DriverObject->DriverUnload                         = Defect_ToastMon_Unload;

    return STATUS_SUCCESS;
}


NTSTATUS
Defect_ToastMon_AddDevice(
    __in PDRIVER_OBJECT DriverObject,
    __in PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Routine Description:

    The Plug & Play subsystem is handing us a brand new PDO, for which we
    (by means of INF registration) have been asked to provide a driver.

    We need to determine if we need to be in the driver stack for the device.
    Create a functional device object to attach to the stack
    Initialize that device object
    Return status success.

    Remember: we can NOT actually send ANY non pnp IRPS to the given driver
    stack, UNTIL we have received an IRP_MN_START_DEVICE.

Arguments:

    DeviceObject - pointer to a device object.

    PhysicalDeviceObject -  pointer to a device object created by the
                            underlying bus driver.

Return Value:

    NT status code.

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDEVICE_OBJECT          deviceObject = NULL;
    PDEVICE_EXTENSION       deviceExtension;

    PAGED_CODE();

    //
    // Create a device object.
    //

    status = IoCreateDevice (DriverObject,
                             sizeof (DEVICE_EXTENSION),
                             NULL,
                             FILE_DEVICE_UNKNOWN,
                             FILE_DEVICE_SECURE_OPEN,
                             FALSE,
                             &deviceObject);


    if (!NT_SUCCESS (status)) {
        //
        // Either not enough memory to create a deviceobject or another
        // deviceobject with the same name exits. This could happen
        // if you install another instance of this device.
        //
        return status;
    }

    deviceExtension = (PDEVICE_EXTENSION) deviceObject->DeviceExtension;

    // initialize the device wide spinlock
    KeInitializeSpinLock(&deviceExtension->deviceSpinlock);

    deviceExtension->TopOfStack = IoAttachDeviceToDeviceStack (
                                       deviceObject,
                                       PhysicalDeviceObject);
    if (NULL == deviceExtension->TopOfStack) {
        IoDeleteDevice(deviceObject);
        return STATUS_DEVICE_REMOVED;
    }

    IoInitializeRemoveLock (&deviceExtension->RemoveLock ,
                            DRIVER_TAG,
                            1, // MaxLockedMinutes
                            5); // HighWatermark, this parameter is
                                // used only on checked build.
    //
    // Set the flag if the device is not holding a pagefile
    // crashdump file or hibernate file.
    //

    deviceObject->Flags |=  DO_POWER_PAGABLE;
    deviceObject->Flags |= DO_BUFFERED_IO;

    deviceExtension->DeviceObject = deviceObject;
    INITIALIZE_PNP_STATE(deviceExtension);

    //
    // We will keep list of all the toaster devices we
    // interact with and protect the access to the list
    // with a mutex.
    //
    InitializeListHead(&deviceExtension->DeviceListHead);
	InitializeListHead(&deviceExtension->RecvQueueHead);
	ExInitializeFastMutex (&deviceExtension->ListMutex);

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Defect_Toastmon: AddDevice: %p to %p->%p \n", deviceObject,
                       deviceExtension->TopOfStack,
                       PhysicalDeviceObject);

    //
    // Register for TOASTER device interface changes.
    //
    // We will get an ARRIVAL callback for every TOASTER device that is
    // started and a REMOVAL callback for every TOASTER that is removed
    //

    status = IoRegisterPlugPlayNotification (
                EventCategoryDeviceInterfaceChange,
                PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                (PVOID)&GUID_DEVINTERFACE_TOASTER,
                DriverObject,
                (PDRIVER_NOTIFICATION_CALLBACK_ROUTINE)
                                        Defect_ToastMon_PnpNotifyInterfaceChange,
                (PVOID)deviceExtension,
#pragma warning(suppress: 6014) // handle is released with unregister call during remove IRP processing
                &deviceExtension->NotificationHandle);

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Defect_Toastmon: RegisterPnPNotifiction failed: %x\n", status);
    }

    RegisterForWMINotification(deviceExtension);

    return STATUS_SUCCESS;

}

NTSTATUS
Defect_ToastMon_CompletionRoutine(
    PDEVICE_OBJECT   DeviceObject,
    PIRP             Irp,
    PVOID            Context
    )
/*++

Routine Description:

    The completion routine for plug & play irps that needs to be
    processed first by the lower drivers.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

   Context - pointer to an event object.

Return Value:

      NT status code

--*/
{
    PKEVENT             event;

    event = (PKEVENT) Context;

    UNREFERENCED_PARAMETER(DeviceObject);

    //
    // If the lower driver didn't return STATUS_PENDING, we don't need to
    // set the event because we won't be waiting on it.
    // This optimization avoids grabbing the dispatcher lock and improves perf.
    //
    if (Irp->PendingReturned == TRUE) {
#pragma warning(suppress: 28183)
        KeSetEvent(event, 0, FALSE);
    }
    //
    // Allows the caller to reuse the IRP
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
Defect_ToastMon_DispatchPnp (
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    The plug and play dispatch routines.

    Most of these the driver will completely ignore.
    In all cases it must pass the IRP to the next lower driver.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PIO_STACK_LOCATION          irpStack;
    NTSTATUS                    status = STATUS_SUCCESS;
    KEVENT                      event;
    PDEVICE_EXTENSION           deviceExtension;
    PLIST_ENTRY                 thisEntry;
    PDEVICE_INFO                list;





    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
	


    irpStack = IoGetCurrentIrpStackLocation(Irp);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Defect_Toastmon: %s IRP:0x%p \n",
                PnPMinorFunctionString(irpStack->MinorFunction), Irp);
         

    status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS (status)) 
    {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }
    
	


    
    

    switch (irpStack->MinorFunction) {
    case IRP_MN_START_DEVICE:

        //
        // The device is starting.
        //
        // We cannot touch the device (send it any non pnp irps) until a
        // start device has been passed down to the lower drivers.
        //
        IoCopyCurrentIrpStackLocationToNext(Irp);
        KeInitializeEvent(&event,
                          NotificationEvent,
                          FALSE
                          );

        IoSetCompletionRoutine(Irp,
                           (PIO_COMPLETION_ROUTINE)Defect_ToastMon_CompletionRoutine,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE); // No need for Cancel

        status = IoCallDriver(deviceExtension->TopOfStack, Irp);

        if (STATUS_PENDING == status) {
            KeWaitForSingleObject(
               &event,
               Executive, // Waiting for reason of a driver
               KernelMode, // Waiting in kernel mode
               FALSE, // No alert
               NULL); // No timeout
            status = Irp->IoStatus.Status;
        }

        if (NT_SUCCESS(status)) {

            SET_NEW_PNP_STATE(deviceExtension, Started);
        }

        //
        // We must now complete the IRP, since we stopped it in the
        // completion routine with STATUS_MORE_PROCESSING_REQUIRED.
        //
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        return status;

    case IRP_MN_REMOVE_DEVICE:


        //
        // Wait for all outstanding requests to complete
        //
        IoReleaseRemoveLockAndWait(&deviceExtension->RemoveLock, Irp);
        SET_NEW_PNP_STATE(deviceExtension, Deleted);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(deviceExtension->TopOfStack, Irp);

        //
        // Unregister the interface notification
        //
        IoUnregisterPlugPlayNotification(deviceExtension->NotificationHandle);
        //
        // Close all the handles to the target device
        //
        ExAcquireFastMutex (&deviceExtension->ListMutex);
        while(!IsListEmpty(&deviceExtension->DeviceListHead))
        {
            thisEntry = RemoveHeadList(&deviceExtension->DeviceListHead);
            list = CONTAINING_RECORD(thisEntry, DEVICE_INFO, ListEntry);
            Defect_ToastMon_CloseTargetDevice(list);
        }
        ExReleaseFastMutex (&deviceExtension->ListMutex);

        UnregisterForWMINotification(deviceExtension);

        IoDetachDevice(deviceExtension->TopOfStack);
        IoDeleteDevice(DeviceObject);
		
		return status;


    case IRP_MN_QUERY_STOP_DEVICE:
        SET_NEW_PNP_STATE(deviceExtension, StopPending);
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_STOP_DEVICE:

        //
        // Check to see whether you have received cancel-stop
        // without first receiving a query-stop. This could happen if someone
        // above us fails a query-stop and passes down the subsequent
        // cancel-stop.
        //

        if (StopPending == deviceExtension->DevicePnPState)
        {
            //
            // We did receive a query-stop, so restore.
            //
            RESTORE_PREVIOUS_PNP_STATE(deviceExtension);
        }
        status = STATUS_SUCCESS; // We must not fail this IRP.
        break;

    case IRP_MN_STOP_DEVICE:
        SET_NEW_PNP_STATE(deviceExtension, Stopped);
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:

        SET_NEW_PNP_STATE(deviceExtension, RemovePending);
        status = STATUS_SUCCESS;  
        break;

    case IRP_MN_SURPRISE_REMOVAL:


	    SET_NEW_PNP_STATE(deviceExtension, SurpriseRemovePending);
	    ToasterDrvCancelQueuedReadIrps(deviceExtension);
	status = STATUS_SUCCESS;
	break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:

        //
        // Check to see whether you have received cancel-remove
        // without first receiving a query-remove. This could happen if
        // someone above us fails a query-remove and passes down the
        // subsequent cancel-remove.
        //

        if (RemovePending == deviceExtension->DevicePnPState)
        {
            //
            // We did receive a query-remove, so restore.
            //
            RESTORE_PREVIOUS_PNP_STATE(deviceExtension);
        }
        status = STATUS_SUCCESS; // We must not fail this IRP.
        break;


    default:
        //
        // If you don't handle any IRP you must leave the
        // status as is.
        //
        status = Irp->IoStatus.Status;

        break;
    }

    //
    // Pass the IRP down and forget it.
    //
    Irp->IoStatus.Status = status;
    IoSkipCurrentIrpStackLocation (Irp);
    status = IoCallDriver (deviceExtension->TopOfStack, Irp);
    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
    return status;
}

NTSTATUS
Defect_ToastMon_DispatchPower(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for power irps.
    Does nothing except forwarding the IRP to the next device
    in the stack.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    NT Status code
--*/
{
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  stack = IoGetCurrentIrpStackLocation(Irp);
    POWER_STATE_TYPE    powerType = stack->Parameters.Power.Type;
    UCHAR               minorFunction = stack->MinorFunction;
    NTSTATUS status= STATUS_SUCCESS;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

	status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS (status)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    switch( minorFunction ) 
    {
		case IRP_MN_QUERY_POWER:
			PoStartNextPowerIrp(Irp);
			IoSkipCurrentIrpStackLocation(Irp);
			status=PoCallDriver(deviceExtension->TopOfStack, Irp);
			IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
			return status;
			break;
		case IRP_MN_SET_POWER:
			switch( powerType ) 
			{
				case DevicePowerState:
				    PoStartNextPowerIrp(Irp);
				    IoSkipCurrentIrpStackLocation(Irp);
				    status=PoCallDriver(deviceExtension->TopOfStack, Irp);
			            IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
			            return status;
				    break;
				case SystemPowerState:
				    PoStartNextPowerIrp(Irp);
				    IoSkipCurrentIrpStackLocation(Irp);
				    status=PoCallDriver(deviceExtension->TopOfStack, Irp);
			            IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
		 	            return status;
				    break;
				default:
				    status = Irp->IoStatus.Status;
				    PoStartNextPowerIrp(Irp);
				    IoSkipCurrentIrpStackLocation(Irp);
				    status=PoCallDriver(deviceExtension->TopOfStack, Irp);
			             IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
			             return status;
			}

			break;
		default:

			status = Irp->IoStatus.Status;
			PoStartNextPowerIrp(Irp);
			IoSkipCurrentIrpStackLocation(Irp);
			status=PoCallDriver(deviceExtension->TopOfStack, Irp);
			IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
			return status;
		}
}

NTSTATUS
Defect_ToastMon_DispatchSystemControl(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for WMI irps.
    Does nothing except forwarding the IRP to the next device
    in the stack.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    NT Status code
--*/
{
    PDEVICE_EXTENSION   deviceExtension;
	NTSTATUS status= STATUS_SUCCESS;
    
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS (status)) 
    {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }
    IoSkipCurrentIrpStackLocation(Irp);
    status=IoCallDriver(deviceExtension->TopOfStack, Irp);
    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
    return status;
}


VOID
Defect_ToastMon_Unload(
    __in PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object.

Return Value:

    VOID.

--*/
{
    UNREFERENCED_PARAMETER(DriverObject);

    PAGED_CODE ();

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Entered Defect_ToastMon_Unload\n"));

    return;
}

NTSTATUS
Defect_ToastMon_Dispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:
    This routine is the dispatch handler for the driver.  It is responsible
    for processing the IRPs.

Arguments:

    pDO - Pointer to device object.

    Irp - Pointer to the current IRP.

Return Value:

    STATUS_SUCCESS if the IRP was processed successfully, otherwise an error
    indicating the reason for failure.

--*/

{
    PDEVICE_EXTENSION       deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                status;

    Irp->IoStatus.Information = 0;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Entered Defect_ToastMon_Dispatch\n"));

    status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS (status)) {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    if (NotStarted == deviceExtension->DevicePnPState) 
	{
        //
        // We fail all the IRPs that arrive before the device is started.
        //
        Irp->IoStatus.Status = status = STATUS_DEVICE_NOT_READY;
        IoCompleteRequest(Irp, IO_NO_INCREMENT );
        IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        return status;
    }

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    // Dispatch based on major fcn code.

    switch (irpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:
            // We don't need any special processing on open/close so we'll
            // just return success.
            status = STATUS_SUCCESS;
            break;

        case IRP_MJ_DEVICE_CONTROL:
        default:
            status = STATUS_NOT_IMPLEMENTED;
            break;
    }

    //
    // We're done with I/O request.  Record the status of the I/O action.
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT );
    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
    return status;
}


NTSTATUS
#pragma warning(suppress: 28208)
Defect_ToastMon_PnpNotifyInterfaceChange(
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION NotificationStruct,
    PVOID                        Context
    )
/*++

Routine Description:

    This routine is the PnP "interface change notification" callback routine.

    This gets called on a Toaster triggered device interface arrival or
    removal.
      - Interface arrival corresponds to a Toaster device being STARTED
      - Interface removal corresponds to a Toaster device being REMOVED

    On arrival:
      - Get the target deviceobject pointer by using the symboliclink.
      - Get the PDO of the target device, in case you need to set the
        device parameters of the target device.
      - Regiter for EventCategoryTargetDeviceChange notification on the fileobject
        so that you can cleanup whenever associated device is removed.

    On removal:
      - This callback is a NO-OP for interface removal because we
      for PnP EventCategoryTargetDeviceChange callbacks and
      use that callback to clean up when their associated toaster device goes
      away.

Arguments:

    NotificationStruct  - Structure defining the change.

    Context -    pointer to the device extension.
                 (supplied as the "context" when we
                  registered for this callback)
Return Value:

    STATUS_SUCCESS - always, even if something goes wrong

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDEVICE_INFO            list = NULL;
    PDEVICE_EXTENSION       deviceExtension = Context;
    PUNICODE_STRING         symbolicLinkName;

    PAGED_CODE();

    symbolicLinkName = NotificationStruct->SymbolicLinkName;

    //
    // Verify that interface class is a toaster device interface.
    //
    // Any other InterfaceClassGuid is an error, but let it go since
    //   it is not fatal to the machine.
    //

    if ( !IsEqualGUID( (LPGUID)&(NotificationStruct->InterfaceClassGuid),
                      (LPGUID)&GUID_DEVINTERFACE_TOASTER) ) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, ("Defect_Toastmon: Bad interfaceClassGuid in Defect_ToastMon_PnpNotifyInterfaceChange\n"));
        return STATUS_SUCCESS;
    }

    //
    // Check the callback event.
    //
    if (IsEqualGUID( (LPGUID)&(NotificationStruct->Event),
                     (LPGUID)&GUID_DEVICE_INTERFACE_ARRIVAL )) {

		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Arrival Notification in Defect_ToastMon_PnpNotifyInterfaceChange\n"));

        //
        // Allocate memory for the deviceinfo
        //

        list = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(DEVICE_INFO), DRIVER_TAG);
        if (list == NULL)
        {
            goto Error;
        }
        RtlZeroMemory(list, sizeof(DEVICE_INFO));

        //
        // Copy the symbolic link
        //
        list->SymbolicLink.MaximumLength = symbolicLinkName->Length +
                                          sizeof(UNICODE_NULL);
        list->SymbolicLink.Length = symbolicLinkName->Length;
        list->SymbolicLink.Buffer = ExAllocatePool2(
                                        POOL_FLAG_PAGED,
                                        list->SymbolicLink.MaximumLength,
                                        DRIVER_TAG);
        if (list->SymbolicLink.Buffer == NULL)
        {
            goto Error;
        }
        RtlCopyUnicodeString(&list->SymbolicLink,symbolicLinkName);

        list->DeviceExtension = deviceExtension;
        InitializeListHead(&list->ListEntry);

        //
        // Warning: It's not recommended to open the targetdevice
        // from a pnp notification callback routine, because if
        // the target device initiates any kind of PnP action as
        // a result of this open, the PnP manager could deadlock.
        // You should queue a workitem to do that.
        // For example, SWENUM devices in conjunction with KS
        // initiate an enumeration of a device when you do the
        // open on the device interface.
        // For simplicity, I'm opening the device here, because
        // I know the toaster function driver doesn't trigger
        // any pnp action.
        // For an example on how to queue a workitem, take a look
        // at ToasterQueuePassiveLevelCallback in func\featured2\
        // wake.c file.
        //

        status = Defect_ToastMon_OpenTargetDevice(list);
        if (!NT_SUCCESS( status)) {
            goto Error;
        }

        //
        // Finally queue the Deviceinfo.
        //
        ExAcquireFastMutex (&deviceExtension->ListMutex);
        InsertTailList(&deviceExtension->DeviceListHead, &list->ListEntry);
        ExReleaseFastMutex (&deviceExtension->ListMutex);
    }  
    else 
    {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Removal Interface Notification\n"));
    }
    return STATUS_SUCCESS;

 Error:
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Defect_Toastmon: PnPNotifyInterfaceChange failed: %x\n", status);
    Defect_ToastMon_CloseTargetDevice(list);
    return STATUS_SUCCESS;
}

NTSTATUS
#pragma warning(suppress: 28208)
Defect_ToastMon_PnpNotifyDeviceChange(
    PTARGET_DEVICE_REMOVAL_NOTIFICATION    NotificationStruct,
    PVOID                                  Context
    )
/*++

Routine Description:

    This routine is the PnP "Device Change Notification" callback routine.

    This gets called on a when the target is query or surprise removed.

      - Interface arrival corresponds to a toaster device being STARTed
      - Interface removal corresponds to a toaster device being REMOVEd

    On Query_Remove or Remove_Complete:
      - Find the targetdevice from the list by matching the fileobject pointers.
      - Dereference the FileObject (this generates a  close to the target device)
      - Free the resources.

Arguments:

    NotificationStruct  - Structure defining the change.

    Context -    pointer to the device extension.
                 (supplied as the "context" when we
                  registered for this callback)
Return Value:

    STATUS_SUCCESS - always, even if something goes wrong

--*/
{
    NTSTATUS                status;
    PDEVICE_INFO            list = Context;
    PDEVICE_EXTENSION       deviceExtension = list->DeviceExtension;

    PAGED_CODE();

    //
    // if the event is query_remove
    //
    if ( (IsEqualGUID( (LPGUID)&(NotificationStruct->Event),
                      (LPGUID)&GUID_TARGET_DEVICE_QUERY_REMOVE))){
        PTARGET_DEVICE_REMOVAL_NOTIFICATION removalNotification;

#pragma warning(suppress: 28930) // used in assert
        removalNotification =
                    (PTARGET_DEVICE_REMOVAL_NOTIFICATION)NotificationStruct;

		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Device Removal (query remove) Notification\n"));

        ASSERT(list->FileObject == removalNotification->FileObject);
        //
        // Deref the fileobject so that we don't prevent
        // the target device from being removed.
        //
        ObDereferenceObject(list->FileObject);
        list->FileObject = NULL;
        //
        // Deref the PDO to compensate for the reference taken
        // by the bus driver when it returned the PDO in response
        // to the query-device-relations (target-relations).
        //
        ObDereferenceObject(list->Pdo);
        list->Pdo = NULL;
        //
        // We will defer freeing other resources to remove-complete
        // notification because if query-remove is vetoed, we would reopen
        // the device in remove-cancelled notification.
        //

    } else if (IsEqualGUID( (LPGUID)&(NotificationStruct->Event),
                      (LPGUID)&GUID_TARGET_DEVICE_REMOVE_COMPLETE) ) {
        //
        // Device is gone. Let us cleanup our resources.
        //
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Device Removal (remove complete) Notification\n"));

        ExAcquireFastMutex (&deviceExtension->ListMutex);
        RemoveEntryList(&list->ListEntry);
        ExReleaseFastMutex (&deviceExtension->ListMutex);

        Defect_ToastMon_CloseTargetDevice(list);

    } else if ( IsEqualGUID( (LPGUID)&(NotificationStruct->Event),
                      (LPGUID)&GUID_TARGET_DEVICE_REMOVE_CANCELLED) ) {

		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Device Removal (remove cancelled) Notification\n"));

        // Should be null because we cleared it in query-remove

        ASSERT(!list->FileObject);
        ASSERT(!list->Pdo);

        //
        // Unregister the previous notification because when we reopen
        // the device we will register again on the new fileobject.
        //
        IoUnregisterPlugPlayNotification(list->NotificationHandle);

        //
        // Reopen the device
        //
#pragma warning(suppress: 28183)
        status = Defect_ToastMon_OpenTargetDevice(list);
        if (!NT_SUCCESS (status)) {
            //
            // Couldn't reopen the device. Cleanup.
            //
            ExAcquireFastMutex (&deviceExtension->ListMutex);
            RemoveEntryList(&list->ListEntry);
            ExReleaseFastMutex (&deviceExtension->ListMutex);

            Defect_ToastMon_CloseTargetDevice(list);
        }

    } else {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Unknown Device Notification\n"));
    }

    return STATUS_SUCCESS;
}

NTSTATUS
Defect_ToastMon_GetTargetDevicePdo(
    __in PDEVICE_OBJECT DeviceObject,
    __out PDEVICE_OBJECT *PhysicalDeviceObject
    )
/*++

Routine Description:

    This builds and send a pnp irp to get the PDO a device.

Arguments:

    DeviceObject - This is the top of the device in the device stack
                   the irp is to be sent to.

   PhysicalDeviceObject - Address where the PDO pointer is returned

Return Value:

   NT status code
--*/
{

    KEVENT                  event;
    NTSTATUS                status;
    PIRP                    irp;
    IO_STATUS_BLOCK         ioStatusBlock;
    PIO_STACK_LOCATION      irpStack;
    PDEVICE_RELATIONS       deviceRelations;

    PAGED_CODE();

    KeInitializeEvent( &event, NotificationEvent, FALSE );

    irp = IoBuildSynchronousFsdRequest( IRP_MJ_PNP,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        &event,
                                        &ioStatusBlock );

    if (irp == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto End;
    }

    irpStack = IoGetNextIrpStackLocation( irp );
    irpStack->MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
    irpStack->Parameters.QueryDeviceRelations.Type = TargetDeviceRelation;

    //
    // Initialize the status to error in case the bus driver decides not to
    // set it correctly.
    //

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED ;


    status = IoCallDriver( DeviceObject, irp );

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );
        status = ioStatusBlock.Status;
    }

    if (NT_SUCCESS( status)) {
        deviceRelations = (PDEVICE_RELATIONS)ioStatusBlock.Information;
        ASSERT(deviceRelations);
        //
        // You must dereference the PDO when it's no longer
        // required.
        //
        *PhysicalDeviceObject = deviceRelations->Objects[0];
        ExFreePool(deviceRelations);
    }

End:
    return status;

}

NTSTATUS
Defect_ToastMon_OpenTargetDevice(
    __in PDEVICE_INFO        List
    )
/*++

Routine Description:

    Open the target device, get the PDO and register
    for TargetDeviceChange notification on the fileobject.

Arguments:


Return Value:

   NT status code
--*/
{
    NTSTATUS             status;

    PAGED_CODE();

    //
    // Get a pointer to and open a handle to the toaster device
    //

    status = IoGetDeviceObjectPointer(&List->SymbolicLink,
                                      STANDARD_RIGHTS_ALL,
                                      &List->FileObject,
                                      &List->TargetDeviceObject);
    if ( !NT_SUCCESS(status) ) {
        goto Error;
    }

    //
    // Register for TargerDeviceChange notification on the fileobject.
    //
    status = IoRegisterPlugPlayNotification (
            EventCategoryTargetDeviceChange,
            0,
            (PVOID)List->FileObject,
            List->DeviceExtension->DeviceObject->DriverObject,
            (PDRIVER_NOTIFICATION_CALLBACK_ROUTINE)Defect_ToastMon_PnpNotifyDeviceChange,
            (PVOID)List,
            &List->NotificationHandle);
    if (!NT_SUCCESS (status)) {
        goto Error;
    }

    //
    // Get the PDO. This is required in case you need to set
    // the target device's parameters using IoOpenDeviceRegistryKey
    //
    status = Defect_ToastMon_GetTargetDevicePdo(List->TargetDeviceObject,
                                            &List->Pdo);
    if (!NT_SUCCESS (status)) {
        goto Error;
    }

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Defect_Toastmon: Target device Toplevel DO:0x%p and PDO: 0x%p\n",
                    List->TargetDeviceObject, List->Pdo);
Error:

    return status;
}


__drv_arg(List->SymbolicLink.Buffer, __drv_freesMem(SymLink))
VOID
#pragma warning(suppress: 6014)
Defect_ToastMon_CloseTargetDevice(
    __in __drv_freesMem(List) PDEVICE_INFO        List
    )
/*++

Routine Description:

    Close all the handles and free the list.

Arguments:


Return Value:
    VOID
--*/
{
    PAGED_CODE();

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Defect_Toastmon: Closing handle to 0x%p\n", List->TargetDeviceObject);
    if (List->FileObject){
        ObDereferenceObject(List->FileObject);
    }
    if (List->NotificationHandle) {
        IoUnregisterPlugPlayNotification(List->NotificationHandle);
    }
    if (List->Pdo) {
        ObDereferenceObject(List->Pdo);
    }
    RtlFreeUnicodeString(&List->SymbolicLink);
    ExFreePool(List);
}

PCHAR
PnPMinorFunctionString (
    __in UCHAR MinorFunction
)
{
    switch (MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            return "IRP_MN_START_DEVICE";
        case IRP_MN_QUERY_REMOVE_DEVICE:
            return "IRP_MN_QUERY_REMOVE_DEVICE";
        case IRP_MN_REMOVE_DEVICE:
            return "IRP_MN_REMOVE_DEVICE";
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            return "IRP_MN_CANCEL_REMOVE_DEVICE";
        case IRP_MN_STOP_DEVICE:
            return "IRP_MN_STOP_DEVICE";
        case IRP_MN_QUERY_STOP_DEVICE:
            return "IRP_MN_QUERY_STOP_DEVICE";
        case IRP_MN_CANCEL_STOP_DEVICE:
            return "IRP_MN_CANCEL_STOP_DEVICE";
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            return "IRP_MN_QUERY_DEVICE_RELATIONS";
        case IRP_MN_QUERY_INTERFACE:
            return "IRP_MN_QUERY_INTERFACE";
        case IRP_MN_QUERY_CAPABILITIES:
            return "IRP_MN_QUERY_CAPABILITIES";
        case IRP_MN_QUERY_RESOURCES:
            return "IRP_MN_QUERY_RESOURCES";
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            return "IRP_MN_QUERY_RESOURCE_REQUIREMENTS";
        case IRP_MN_QUERY_DEVICE_TEXT:
            return "IRP_MN_QUERY_DEVICE_TEXT";
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            return "IRP_MN_FILTER_RESOURCE_REQUIREMENTS";
        case IRP_MN_READ_CONFIG:
            return "IRP_MN_READ_CONFIG";
        case IRP_MN_WRITE_CONFIG:
            return "IRP_MN_WRITE_CONFIG";
        case IRP_MN_EJECT:
            return "IRP_MN_EJECT";
        case IRP_MN_SET_LOCK:
            return "IRP_MN_SET_LOCK";
        case IRP_MN_QUERY_ID:
            return "IRP_MN_QUERY_ID";
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            return "IRP_MN_QUERY_PNP_DEVICE_STATE";
        case IRP_MN_QUERY_BUS_INFORMATION:
            return "IRP_MN_QUERY_BUS_INFORMATION";
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            return "IRP_MN_DEVICE_USAGE_NOTIFICATION";
        case IRP_MN_SURPRISE_REMOVAL:
            return "IRP_MN_SURPRISE_REMOVAL";
        case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
            return "IRP_MN_QUERY_LEGACY_BUS_INFORMATION";
        default:
            return "unknown_pnp_irp";
    }
}


NTSTATUS
Defect_ToastMon_DispatchRead (
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    )
/*++

Routine Description:

    Performs read from the NIC.

Arguments:

   FdoData - pointer to a FDO_DATA structure

   Irp - pointer to an I/O Request Packet.

Return Value:

    NT status code


--*/

{
    NTSTATUS    status;
    KIRQL oldIrql;
    BOOLEAN CancelIrp;
    PDEVICE_EXTENSION       deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
	
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Entered Defect_ToastMon_DispatchRead \n"));

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Attempting to acquire remove lock\n"));
    status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS (status)) 
    {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, ("Defect_Toastmon: Acquire remove lock failed; the device is already removed\n"));
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }


    KeAcquireSpinLock(&deviceExtension->RecvQueueLock, &oldIrql);
    CancelIrp = FALSE;
    //
    // Since we are queueing the IRP, we should set the cancel routine.
    //
    IoSetCancelRoutine(Irp, Defect_ToastmonCancelRoutineForReadIrp);

    //
    // Let us check to see if the IRP is cancelled at this point.
    //
    if(Irp->Cancel!=FALSE) 
    {
        //
        // This irp has been marked cancelled. This may have occured
        // before or after we set the cancel routine. If it occured
        // before, our cancel routine will not be called and we must
        // cancel the irp here. If it was cancelled after we set the
        // cancel routine, the thread that is cancelling the irp will
        // call our cancel routine. We determine this by setting the
        // cancel routine to NULL and seeing if the original cancel
        // routine is still there.
        //

		
        status = STATUS_CANCELLED;
	if (IoSetCancelRoutine(Irp, NULL) != NULL) 
	{
            CancelIrp = TRUE;
        }
    }
    else 
    {
        //
        // Queue the IRP and return status pending.
        //
        IoMarkIrpPending(Irp);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Defect_ToastMon_DispatchRead - insert the Read Irp into the Recieve Queue and mark IRP as pending \n "));
        InsertTailList(&deviceExtension->RecvQueueHead,
                        &Irp->Tail.Overlay.ListEntry);
        status = STATUS_PENDING;
        //
        // The IRP shouldn't be accessed after the lock is released
        // It could be grabbed by another thread or the cancel routine
        // is running.
        //
    }

    KeReleaseSpinLock(&deviceExtension->RecvQueueLock, oldIrql);

   if (CancelIrp != FALSE) 
   {
        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Release the removelock \n"));
        IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);
    }
	
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Exiting Defect_ToastMon_DispatchRead \n"));
    return status;
}

VOID
Defect_ToastmonCancelRoutineForReadIrp (
    PDEVICE_OBJECT   DeviceObject,
    PIRP             Irp
    )

/*++

Routine Description:

    The cancel routine for IRPs waiting in the RecvQueue.
    The cancel spin lock is already acquired when this routine is called.

Arguments:

    DeviceObject - pointer to the device object.

    Irp - pointer to the IRP to be cancelled.


Return Value:

    VOID.

--*/
{
    PDEVICE_EXTENSION       deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    KIRQL oldIrql;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Canceling Read Request inside Defect_ToastmonCancelRoutineForReadIrp\n"));

    //
    // Release the cancel spin lock and pass Irp->CancelIrql to 
    // restore the original Irql prior to the call to IoAcquireCancelSpinLock 
    //
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Releasing the cancel spinlock\n"));
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    //
    // Acquire the local spinlock and raise the Irql to DISPATCH_LEVEL
    //

    KeAcquireSpinLock(&deviceExtension->RecvQueueLock, &oldIrql);

    //
    // Remove the cancelled IRP from queue and release the queue lock.
    //
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    //
    // Release the spinlock and restore the old Irql
    //
    KeReleaseSpinLock(&deviceExtension->RecvQueueLock, oldIrql);

    //
    // Complete the request with STATUS_CANCELLED.
    //

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Releasing the removelock\n"));
    IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);
    return;

}


VOID
#pragma warning(suppress: 28167)
ToasterDrvCancelQueuedReadIrps(
    PDEVICE_EXTENSION       deviceExtension
    )
/*++

Routine Description:

    Cancel all the read IRPs waiting in the RecvQueue.

Arguments:

    FdoData - Pointer to the device extension.

Return Value:

    None

--*/
{
    KIRQL               oldIrql;
    PIRP                irp;
    PLIST_ENTRY         listEntry;
    ULONG               MajorVersion;
    ULONG               MinorVersion;
    ULONG               BuildNumber;
    BOOLEAN             ListEmpty = FALSE;


	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Canceling queued IRPs inside ToasterDrvCancelQueuedReadIrps"));
	
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Acquire the RecvQueueLock before checking the RecvQueue as cancellation may also run at the same time"));
    KeAcquireSpinLock(&deviceExtension->RecvQueueLock, &oldIrql);
    ListEmpty=IsListEmpty(&deviceExtension->RecvQueueHead);
    if(ListEmpty)
    {
		// injected defect for IrqlPsPassive SDV and DV rule
#pragma warning(suppress: 28159) // this is the injected defect 
#pragma warning(suppress: 28121)
		PsGetVersion(&MajorVersion, &MinorVersion, &BuildNumber, NULL);
        KeReleaseSpinLock(&deviceExtension->RecvQueueLock, oldIrql);
    }
    else
    {
        while(!ListEmpty)
        {
            //
            // Remove a request from the queue.
            //
            listEntry = RemoveHeadList(&deviceExtension->RecvQueueHead);
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Extracting IRPs from the list."));
            irp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);
            //
            // Set the cancel routine to NULL. This is an atomic operation.
            //
            if (IoSetCancelRoutine(irp, NULL))
            {
                // injected defect for IrqlPsPassive SDV and DV rule
#pragma warning(suppress: 28159) // this is the injected defect
#pragma warning(suppress: 28121)
                PsGetVersion(&MajorVersion, &MinorVersion, &BuildNumber, NULL);
                irp->IoStatus.Status = STATUS_CANCELLED;
                irp->IoStatus.Information = 0;
                DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Cancelling the queued IRPs from the list."));
                KeReleaseSpinLock(&deviceExtension->RecvQueueLock, oldIrql);
                // do not complete the IRP while holding the lock
                IoCompleteRequest(irp, IO_NO_INCREMENT);
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, ("Defect_Toastmon: Releasing the removelock."));
                IoReleaseRemoveLock (&deviceExtension->RemoveLock, irp);
                KeAcquireSpinLock(&deviceExtension->RecvQueueLock, &oldIrql);
            }
            else
            {
                //
                // Cancel rotuine is running. Leave the irp alone.
                //
                irp = NULL;
            }
            ListEmpty=IsListEmpty(&deviceExtension->RecvQueueHead);
            if(ListEmpty)
            {
                KeReleaseSpinLock(&deviceExtension->RecvQueueLock, oldIrql);
            }
       }
    }
}