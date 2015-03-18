/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Event.c

Abstract:

    The purpose of this sample is to demonstrate how a kernel-mode driver can notify
    an user-app about a device event.  There are several different techniques. This sample
    will demonstrate two very commonly used techniques.

    1) Using an event:
        The application creates an event object using CreateEvent().
        The app passes the event handle to the driver in a private IOCTL.
        The driver is running in the app's thread context during the IOCTL so
        there is a valid user-mode handle at that time.
        The driver dereferences the user-mode handle into system space & saves
        the event object pointer for later use.
        The driver signals the event via KeSetEvent() at IRQL <= DISPATCH_LEVEL.
        The driver deletes the references to the event object.

    2) Pending Irp: This technique is useful if you want to send a message
        to the app along with the notification. In this, an application sends
        a synchronous or asynchronous (overlapped) ioctl to the driver. The driver
        would then pend the IRP until the device event occurs. When the hardware
        event occurs, the driver will complete the IRP. This will cause the thread that
        sent the request to come out of DeviceIoControl call if it's  synchronous or signal
        the event that the thread is waiting on in the usermode it's has done a
        OVERLAPPED call. Another advantage of this technique over the event model
        is that the driver doesn't have to be in the context of the process that
        sent the IOCTL request. You can't guarantee the process context in multi-level
        drivers.

    3) Using WMI to fire events. Check the wmifilter sample in the DDK.

    4) Using PNP custom notification scheme. Walter Oney's book describes this.
        Can be used only in PNP drivers.

    4) Named events: In that an app creates a named event in the usermode
        and driver opens that in kernel and signal it. This technique is deprecated
        by the kb article (Q228785)

        This sample demonstrates the first two techniques. This sample is an
        improvised version of the event sample available in the KB article
        Q176415


Enviroment:

    Kernel Mode Only

Revision History:

--*/

#include <ntddk.h>
#include "public.h" //common to app and driver
#include "event.h" // private to driver

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, EventCreateClose)
#pragma alloc_text (PAGE, EventUnload)
#endif

_Use_decl_annotations_
NTSTATUS
DriverEntry(
    PDRIVER_OBJECT  DriverObject,
    PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine gets called by the system to initialize the driver.

Arguments:

    DriverObject    - the system supplied driver object.
    RegistryPath    - the system supplied registry path for this driver.

Return Value:

    NTSTATUS

--*/

{

    

    PDEVICE_OBJECT      deviceObject;
    PDEVICE_EXTENSION   deviceExtension;
    UNICODE_STRING      ntDeviceName;
    UNICODE_STRING      symbolicLinkName;
    NTSTATUS            status;

    UNREFERENCED_PARAMETER(RegistryPath);

    DebugPrint(("==>DriverEntry\n"));

    //
    // Create the device object
    //
    RtlInitUnicodeString(&ntDeviceName, NTDEVICE_NAME_STRING);

    status = IoCreateDevice(DriverObject,               // DriverObject
                            sizeof(DEVICE_EXTENSION), // DeviceExtensionSize
                            &ntDeviceName,              // DeviceName
                            FILE_DEVICE_UNKNOWN,        // DeviceType
                            FILE_DEVICE_SECURE_OPEN,    // DeviceCharacteristics
                            FALSE,                      // Not Exclusive
                            &deviceObject               // DeviceObject
                           );

    if (!NT_SUCCESS(status)) {
        DebugPrint(("\tIoCreateDevice returned 0x%x\n", status));
        return(status);
    }

    //
    // Set up dispatch entry points for the driver.
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE]          = EventCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = EventCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]         = EventCleanup;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = EventDispatchIoControl;
    DriverObject->DriverUnload                          = EventUnload;

    //
    // Create a symbolic link for userapp to interact with the driver.
    //
    RtlInitUnicodeString(&symbolicLinkName, SYMBOLIC_NAME_STRING);
    status = IoCreateSymbolicLink(&symbolicLinkName, &ntDeviceName);

    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(deviceObject);
        DebugPrint(("\tIoCreateSymbolicLink returned 0x%x\n", status));
        return(status);
    }

    //
    // Initialize the device extension.
    //
    deviceExtension = deviceObject->DeviceExtension;

    InitializeListHead(&deviceExtension->EventQueueHead);

    KeInitializeSpinLock(&deviceExtension->QueueLock);

    deviceExtension->Self = deviceObject;

    //
    // Establish user-buffer access method.
    //
    deviceObject->Flags |= DO_BUFFERED_IO;

    DebugPrint(("<==DriverEntry\n"));

    ASSERT(NT_SUCCESS(status));

    return status;
}

_Use_decl_annotations_
VOID
EventUnload(
    PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine gets called to remove the driver from the system.

Arguments:

    DriverObject    - the system supplied driver object.

Return Value:

    NTSTATUS

--*/

{

    PDEVICE_OBJECT       deviceObject = DriverObject->DeviceObject;
    PDEVICE_EXTENSION    deviceExtension = deviceObject->DeviceExtension;
    UNICODE_STRING      symbolicLinkName;

    DebugPrint(("==>Unload\n"));

    PAGED_CODE();

   if (!IsListEmpty(&deviceExtension->EventQueueHead)) {
        ASSERTMSG("Event Queue is not empty\n", FALSE);
    }

    //
    // Delete the user-mode symbolic link and deviceobjct.
    //
    RtlInitUnicodeString(&symbolicLinkName, SYMBOLIC_NAME_STRING);
    IoDeleteSymbolicLink(&symbolicLinkName);
    IoDeleteDevice(deviceObject);

    return;
}

_Use_decl_annotations_
NTSTATUS
EventCreateClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This device control dispatcher handles create & close IRPs.

Arguments:

    DeviceObject - Context for the activity.
    Irp          - The device control argument block.

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS            status;
    PFILE_CONTEXT       fileContext;

    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(irpStack->FileObject != NULL);    

    switch (irpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
            DebugPrint(("IRP_MJ_CREATE\n"));

            fileContext = ExAllocatePoolWithQuotaTag(NonPagedPool, 
                                              sizeof(FILE_CONTEXT),
                                              TAG);

            if (NULL == fileContext) {
                status =  STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            IoInitializeRemoveLock(&fileContext->FileRundownLock, TAG, 0, 0);

            //
            // Make sure nobody is using the FsContext scratch area.
            //
            ASSERT(irpStack->FileObject->FsContext == NULL);    

            //
            // Store the context in the FileObject's scratch area.
            //
            irpStack->FileObject->FsContext = (PVOID) fileContext;
            
            status = STATUS_SUCCESS;
            break;

        case IRP_MJ_CLOSE:
            DebugPrint(("IRP_MJ_CLOSE\n"));

            fileContext = irpStack->FileObject->FsContext;
            
            ExFreePoolWithTag(fileContext, TAG);
            
            status = STATUS_SUCCESS;
            break;

        default:
            ASSERT(FALSE);  // should never hit this
            status = STATUS_NOT_IMPLEMENTED;
            break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;

}

_Use_decl_annotations_
NTSTATUS
EventCleanup(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This device control dispatcher handles Cleanup IRP.

Arguments:

    DeviceObject - Context for the activity.
    Irp          - The device control argument block.

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION  irpStack;
    NTSTATUS            status ;
    KIRQL               oldIrql;
    PLIST_ENTRY         thisEntry, nextEntry, listHead;
    PNOTIFY_RECORD      notifyRecord;
    PDEVICE_EXTENSION   deviceExtension;
    LIST_ENTRY          cleanupList;
    PFILE_CONTEXT       fileContext;

    DebugPrint(("==>EventCleanup\n"));

    deviceExtension = DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(irpStack->FileObject != NULL);    

    fileContext = irpStack->FileObject->FsContext;    

    //
    // This acquire cannot fail because you cannot get more than one
    // cleanup for the same handle.
    //
    status = IoAcquireRemoveLock(&fileContext->FileRundownLock, Irp);
    ASSERT(NT_SUCCESS(status));

    //
    // Wait for all the threads that are currently dispatching to exit and 
    // prevent any threads dispatching I/O on the same handle beyond this point.
    //
    IoReleaseRemoveLockAndWait(&fileContext->FileRundownLock, Irp);
    
    InitializeListHead(&cleanupList);

    //
    // Walk the list and remove all the pending notification records
    // that belong to this filehandle.
    //

    KeAcquireSpinLock(&deviceExtension->QueueLock, &oldIrql);

    listHead = &deviceExtension->EventQueueHead;

    for (thisEntry = listHead->Flink;
         thisEntry != listHead;
         thisEntry = nextEntry)
    {
        nextEntry = thisEntry->Flink;

        notifyRecord = CONTAINING_RECORD(thisEntry, NOTIFY_RECORD, ListEntry);

        if (irpStack->FileObject == notifyRecord->FileObject)  {

            //
            // KeCancelTimer returns if the timer is successfully cancelled.
            // If it returns FALSE, there are two possibilities. Either the
            // TimerDpc has just run and waiting to acquire the lock or it
            // has run to completion. We wouldn't be here if it had run to
            // completion because we wouldn't found the record in the list.
            // So the only possibility is that it's waiting to acquire the lock.
            // In that case, we will just let the DPC to complete the request
            // and free the record.
            //
            if (KeCancelTimer(&notifyRecord->Timer)) {

                DebugPrint(("\tCanceled timer\n"));
                RemoveEntryList(thisEntry);

                switch (notifyRecord->Type) {
                case IRP_BASED:
                    //
                    // Clear the cancel-routine and check the return value to
                    // see whether it was cleared by us or by the I/O manager.
                    //
                    if (IoSetCancelRoutine (notifyRecord->Message.PendingIrp, NULL) != NULL) {

                        //
                        // We cleared it and as a result we own the IRP and
                        // nobody can cancel it anymore. We will queue the IRP
                        // in the local cleanup list so that we can complete
                        // all the IRPs outside the lock to avoid deadlocks in
                        // the completion routine of the driver above us re-enters
                        // our driver.
                        //
                        InsertTailList(&cleanupList,
                                       &notifyRecord->Message.PendingIrp->Tail.Overlay.ListEntry);
                        ExFreePoolWithTag(notifyRecord, TAG);

                    } else {
                        //
                        // The I/O manager cleared it and called the cancel-routine.
                        // Cancel routine is probably waiting to acquire the lock.
                        // So reinitialze the ListEntry so that it doesn't crash
                        // when it tries to remove the entry from the list and
                        // set the CancelRoutineFreeMemory to indicate that it should
                        // free the notification record.
                        //
                        InitializeListHead(&notifyRecord->ListEntry);
                        notifyRecord->CancelRoutineFreeMemory = TRUE;
                    }
                    break;

                case EVENT_BASED:
                    ObDereferenceObject(notifyRecord->Message.Event);
                    ExFreePoolWithTag(notifyRecord, TAG);
                    break;
                default: break;

                }
             }
        }
    }

    KeReleaseSpinLock(&deviceExtension->QueueLock, oldIrql);

    //
    // Walk through the cleanup list and cancel all
    // the IRPs.
    //
    while (!IsListEmpty(&cleanupList))
    {
        PIRP                pendingIrp;   
        //
        // Complete the IRP
        //
        thisEntry = RemoveHeadList(&cleanupList);
        pendingIrp = CONTAINING_RECORD(thisEntry, IRP, Tail.Overlay.ListEntry);
        
        DebugPrint(("\t canceled IRP %p\n", pendingIrp));
        
        pendingIrp->Tail.Overlay.DriverContext[3] = NULL;
        pendingIrp->IoStatus.Information = 0;
        pendingIrp->IoStatus.Status = STATUS_CANCELLED;
        
        IoCompleteRequest(pendingIrp, IO_NO_INCREMENT);
    }

    //
    // Finally complete the cleanup Irp
    //
    Irp->IoStatus.Status = status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    DebugPrint(("<== EventCleanup\n"));
    return status;

}

_Use_decl_annotations_
NTSTATUS
EventDispatchIoControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This device control dispatcher handles IOCTLs.

Arguments:

    DeviceObject - Context for the activity.
    Irp          - The device control argument block.

Return Value:

    NTSTATUS

--*/

{

    PIO_STACK_LOCATION  irpStack;
    PREGISTER_EVENT registerEvent;
    NTSTATUS    status;
    PFILE_CONTEXT fileContext;

    DebugPrint(("==> EventDispatchIoControl\n"));

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    
    ASSERT(irpStack->FileObject != NULL);

    fileContext = irpStack->FileObject->FsContext;    

    status = IoAcquireRemoveLock(&fileContext->FileRundownLock, Irp);
    if (!NT_SUCCESS(status)) {
        //
        // Lock is in a removed state. That means we have already received 
        // cleaned up request for this handle. 
        //
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }
    
    switch (irpStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_REGISTER_EVENT:

            DebugPrint(("\tIOCTL_REGISTER_EVENT\n"));

            //
            // First validate the parameters.
            //
            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                    SIZEOF_REGISTER_EVENT) {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            registerEvent = (PREGISTER_EVENT)Irp->AssociatedIrp.SystemBuffer;

            switch (registerEvent->Type) {
            case IRP_BASED:
                status = RegisterIrpBasedNotification(DeviceObject, Irp);
                break;
            case EVENT_BASED:
                status = RegisterEventBasedNotification(DeviceObject, Irp);
                break;
            default:
                ASSERTMSG("\tUnknow notification type from user-mode\n", FALSE);
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            break;

        default:
            ASSERT(FALSE);  // should never hit this
            status = STATUS_NOT_IMPLEMENTED;
            break;

    } // switch IoControlCode

    if (status != STATUS_PENDING) {
        //
        // complete the Irp
        //
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    //
    // We don't hold the lock for IRP that's pending in the list because this
    // lock is meant to rundown currently dispatching threads when the cleanup
    // is handled.
    //
    IoReleaseRemoveLock(&fileContext->FileRundownLock, Irp);        

    DebugPrint(("<== EventDispatchIoControl\n"));
    return status;
}

_Use_decl_annotations_
VOID
EventCancelRoutine(
    PDEVICE_OBJECT   DeviceObject,
    PIRP             Irp
    )

/*++

Routine Description:

    The cancel routine. It will remove the IRP from the queue
    and will complete it. The cancel spin lock is already acquired
    when this routine is called. This routine is not required if
    you are just using the event based notification.

Arguments:

    DeviceObject - pointer to the device object.

    Irp - pointer to the IRP to be cancelled.


Return Value:

    VOID.

--*/
{
    PDEVICE_EXTENSION   deviceExtension;
    KIRQL               oldIrql ;
    PNOTIFY_RECORD      notifyRecord;

    DebugPrint (("==>EventCancelRoutine irp %p\n", Irp));

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Release the cancel spinlock
    //
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    //
    // Acquire the queue spinlock
    //
    KeAcquireSpinLock(&deviceExtension->QueueLock, &oldIrql);

    notifyRecord = Irp->Tail.Overlay.DriverContext[3];
    ASSERT(NULL != notifyRecord);
    ASSERT(IRP_BASED == notifyRecord->Type);

    RemoveEntryList(&notifyRecord->ListEntry);

    //
    // Clear the pending Irp field because we complete the IRP no matter whether
    // we succeed or fail to cancel the timer. TimerDpc will check this field
    // before dereferencing the IRP.
    //
    notifyRecord->Message.PendingIrp = NULL;

    if (KeCancelTimer(&notifyRecord->Timer)) {
        DebugPrint(("\t canceled timer\n"));
        ExFreePoolWithTag(notifyRecord, TAG);
        notifyRecord = NULL;
    } else {
        //
        // Here the possibilities are:
        // 1) DPC is fired and waiting to acquire the lock.
        // 2) DPC has run to completion.
        // 3) DPC has been cancelled by the cleanup routine.
        // By checking the CancelRoutineFreeMemory, we can figure out whether
        // dpc is waiting to acquire the lock and access the notifyRecord memory.
        //
        if (notifyRecord->CancelRoutineFreeMemory == FALSE) {
            //
            // This is case 1 where the DPC is waiting to run.
            //
            InitializeListHead(&notifyRecord->ListEntry);
        } else {
            //
            // This is either 2 or 3.
            //
            ExFreePoolWithTag(notifyRecord, TAG);
            notifyRecord = NULL;
        }

    }

    KeReleaseSpinLock(&deviceExtension->QueueLock, oldIrql);

    DebugPrint (("\t canceled IRP %p\n", Irp));
    Irp->Tail.Overlay.DriverContext[3] = NULL;
    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    DebugPrint (("<==EventCancelRoutine irp %p\n", Irp));
    return;

}

_Use_decl_annotations_
VOID
CustomTimerDPC(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )

/*++

Routine Description:

    This is the DPC associated with this drivers Timer object setup in ioctl routine.

Arguments:

    Dpc             -   our DPC object associated with our Timer
    DeferredContext -   Context for the DPC that we setup in DriverEntry
    SystemArgument1 -
    SystemArgument2 -

Return Value:

    Nothing.

--*/
{
    PNOTIFY_RECORD      notifyRecord = DeferredContext;
    PDEVICE_EXTENSION deviceExtension;
    PIRP irp;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    DebugPrint(("==> CustomTimerDPC \n"));

    ASSERT(notifyRecord != NULL); // can't be NULL
    _Analysis_assume_(notifyRecord != NULL);

    deviceExtension = notifyRecord->DeviceExtension;

    KeAcquireSpinLockAtDpcLevel(&deviceExtension->QueueLock);

    RemoveEntryList(&notifyRecord->ListEntry);

    switch (notifyRecord->Type) {
    case IRP_BASED:
        irp = notifyRecord->Message.PendingIrp;
        if (irp != NULL) {
            if (IoSetCancelRoutine(irp, NULL) != NULL) {
                
                irp->Tail.Overlay.DriverContext[3] = NULL;

                //
                // Drop the lock before completing the request.
                //
                KeReleaseSpinLockFromDpcLevel(&deviceExtension->QueueLock);

                irp->IoStatus.Status = STATUS_SUCCESS;
                irp->IoStatus.Information = 0;
                IoCompleteRequest(irp, IO_NO_INCREMENT);

                KeAcquireSpinLockAtDpcLevel(&deviceExtension->QueueLock);

            } else {
                //
                // Cancel routine will run as soon as we release the lock.
                // So let it complete the request and free the record.
                //
                InitializeListHead(&notifyRecord->ListEntry);
                notifyRecord->CancelRoutineFreeMemory = TRUE;
                notifyRecord = NULL;
            }
        } else {
            //
            // Cancel routine has run and completed the IRP. So just free
            // the record.
            //
            ASSERT(notifyRecord->CancelRoutineFreeMemory == FALSE);
        }

        break;

    case EVENT_BASED:
        //
        // Signal the Event created in user-mode.
        //
        KeSetEvent(notifyRecord->Message.Event, 0, FALSE);

        //
        // Dereference the object as we are done with it.
        //
        ObDereferenceObject(notifyRecord->Message.Event);

        break;

    default:
        ASSERT(FALSE);
        break;
    }

    KeReleaseSpinLockFromDpcLevel(&deviceExtension->QueueLock);

    //
    // Free the memory outside the lock for better performance.
    //
    if (notifyRecord != NULL) {
        ExFreePoolWithTag(notifyRecord, TAG);
        notifyRecord = NULL;
    }
    
    DebugPrint(("<== CustomTimerDPC\n"));

    return;
}

_Use_decl_annotations_
NTSTATUS
RegisterIrpBasedNotification(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

        This routine  queues a IRP based notification record to be
        handled by a DPC.


Arguments:

    DeviceObject - Context for the activity.
    Irp          - The device control argument block.

Return Value:

    NTSTATUS - If the status is not STATUS_PENDING, the caller
    will complete the request.


--*/
{
    PDEVICE_EXTENSION   deviceExtension;
    PNOTIFY_RECORD notifyRecord;
    PIO_STACK_LOCATION irpStack;
    KIRQL   oldIrql;
    PREGISTER_EVENT registerEvent;

    DebugPrint(("\tRegisterIrpBasedNotification\n"));

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = DeviceObject->DeviceExtension;
    registerEvent = (PREGISTER_EVENT)Irp->AssociatedIrp.SystemBuffer;

    //
    // Allocate a record and save all the event context.
    //

    notifyRecord = ExAllocatePoolWithQuotaTag(NonPagedPool, 
                                              sizeof(NOTIFY_RECORD),
                                              TAG);

    if (NULL == notifyRecord) {
        return  STATUS_INSUFFICIENT_RESOURCES;
    }

    InitializeListHead(&notifyRecord->ListEntry);

    notifyRecord->FileObject = irpStack->FileObject;
    notifyRecord->DeviceExtension = deviceExtension;
    notifyRecord->Type = IRP_BASED;
    notifyRecord->Message.PendingIrp = Irp;

    //
    // Start the timer to run the CustomTimerDPC in DueTime seconds to
    // simulate an interrupt (which would queue a DPC).
    // The user's event object is signaled or the IRP is completed in the DPC to
    // notify the hardware event.
    //

    // ensure relative time for this sample

    if (registerEvent->DueTime.QuadPart > 0) {
        registerEvent->DueTime.QuadPart = -(registerEvent->DueTime.QuadPart);
    }

    KeInitializeDpc(&notifyRecord->Dpc, // Dpc
                    CustomTimerDPC,     // DeferredRoutine
                    notifyRecord        // DeferredContext
                   );

    KeInitializeTimer(&notifyRecord->Timer);

    //
    // We will set the cancel routine and TimerDpc within the
    // lock so that they don't modify the list before we are
    // completely done.
    //
    KeAcquireSpinLock(&deviceExtension->QueueLock, &oldIrql);

    //
    // Set the cancel routine. This is required if the app decides to
    // exit or cancel the event prematurely.
    //
    IoSetCancelRoutine (Irp, EventCancelRoutine);

    //
    // Before we queue the IRP, we must check to see if it's cancelled.
    //
    if (Irp->Cancel) {

        //
        // Clear the cancel-routine automically and check the return value.
        // We will complete the IRP here if we succeed in clearing it. If
        // we fail then we will let the cancel-routine complete it.
        //
        if (IoSetCancelRoutine (Irp, NULL) != NULL) {

            //
            // We are able to successfully clear the routine. Either the
            // the IRP is cancelled before we set the cancel-routine or
            // we won the race with I/O manager in clearing the routine.
            // Return STATUS_CANCELLED so that the caller can complete
            // the request.

            KeReleaseSpinLock(&deviceExtension->QueueLock, oldIrql);

            ExFreePoolWithTag(notifyRecord, TAG);

            return STATUS_CANCELLED;
        } else {
            //
            // The IRP got cancelled after we set the cancel-routine and the
            // I/O manager won the race in clearing it and called the cancel
            // routine. So queue the request so that cancel-routine can dequeue
            // and complete it. Note the cancel-routine cannot run until we
            // drop the queue lock.
            //
        }
    }

    IoMarkIrpPending(Irp);

    InsertTailList(&deviceExtension->EventQueueHead,
                   &notifyRecord->ListEntry);

    notifyRecord->CancelRoutineFreeMemory = FALSE;

    //
    // We will save the record pointer in the IRP so that we can get to
    // it directly in the CancelRoutine.
    //
    Irp->Tail.Overlay.DriverContext[3] =  notifyRecord;

    KeSetTimer(&notifyRecord->Timer,   // Timer
               registerEvent->DueTime,         // DueTime
               &notifyRecord->Dpc      // Dpc
              );

    KeReleaseSpinLock(&deviceExtension->QueueLock, oldIrql);

    //
    // We will return pending as we have marked the IRP pending.
    //
    return STATUS_PENDING;;

}

_Use_decl_annotations_
NTSTATUS
RegisterEventBasedNotification(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine  queues a event based notification record
    to be handled by a DPC.

Arguments:

    DeviceObject - Context for the activity.
    Irp          - The device control argument block.

Return Value:

    NTSTATUS - If the status is not STATUS_PENDING, the caller
    will complete the request.

--*/
{
    PDEVICE_EXTENSION   deviceExtension;
    PNOTIFY_RECORD notifyRecord;
    NTSTATUS status;
    PIO_STACK_LOCATION irpStack;
    PREGISTER_EVENT registerEvent;
    KIRQL oldIrql;

    DebugPrint(("\tRegisterEventBasedNotification\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    registerEvent = (PREGISTER_EVENT)Irp->AssociatedIrp.SystemBuffer;

    //
    // Allocate a record and save all the event context.
    //
    notifyRecord = ExAllocatePoolWithQuotaTag(NonPagedPool, 
                                              sizeof(NOTIFY_RECORD),
                                              TAG);

    if (NULL == notifyRecord) {
        return  STATUS_INSUFFICIENT_RESOURCES;
    }

    InitializeListHead(&notifyRecord->ListEntry);

    notifyRecord->FileObject = irpStack->FileObject;
    notifyRecord->DeviceExtension = deviceExtension;
    notifyRecord->Type = EVENT_BASED;

    //
    // Get the object pointer from the handle. Note we must be in the context
    // of the process that created the handle.
    //
    status = ObReferenceObjectByHandle(registerEvent->hEvent,
                                       SYNCHRONIZE | EVENT_MODIFY_STATE,
                                       *ExEventObjectType,
                                       Irp->RequestorMode,
                                       &notifyRecord->Message.Event,
                                       NULL
                                      );

    if (!NT_SUCCESS(status)) {

        DebugPrint(("\tUnable to reference User-Mode Event object, Error = 0x%x\n", status));
        ExFreePoolWithTag(notifyRecord, TAG);
        return status;
    }

    //
    // Start the timer to run the CustomTimerDPC in DueTime seconds to
    // simulate an interrupt (which would queue a DPC).
    // The user's event object is signaled or the IRP is completed in the DPC to
    // notify the hardware event.
    //
    if (registerEvent->DueTime.QuadPart > 0) {
        registerEvent->DueTime.QuadPart = -(registerEvent->DueTime.QuadPart);
    }

    KeInitializeDpc(&notifyRecord->Dpc, // Dpc
                    CustomTimerDPC,     // DeferredRoutine
                    notifyRecord        // DeferredContext
                   );

    KeInitializeTimer(&notifyRecord->Timer);

    KeAcquireSpinLock(&deviceExtension->QueueLock, &oldIrql);

    InsertTailList(&deviceExtension->EventQueueHead,
                   &notifyRecord->ListEntry);

    KeReleaseSpinLock(&deviceExtension->QueueLock, oldIrql);

    KeSetTimer(&notifyRecord->Timer,   // Timer
               registerEvent->DueTime, // DueTime
               &notifyRecord->Dpc      // Dpc
              );
    return STATUS_SUCCESS;
}


