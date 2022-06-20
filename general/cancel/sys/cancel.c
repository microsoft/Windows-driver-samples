/*++
Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.


Module Name:

    cancel.c

Abstract:   Demonstrates the use of new Cancel-Safe queue
            APIs to perform queuing of IRPs without worrying about
            any synchronization issues between cancel lock in the I/O
            manager and the driver's queue lock.

            This driver is written for an hypothetical data acquisition
            device that requires polling at a regular interval.
            The device has some settling period between two reads.
            Upon user request the driver reads data and records the time.
            When the next read request comes in, it checks the interval
            to see if it's reading the device too soon. If so, it pends
            the IRP and sleeps for while and tries again.

            Upon arrival, IRPs are queued in a cancel-safe queue and a
            semaphore is signaled. A polling thread indefinitely waits on the
            semaphore to process queued IRPs sequentially.

            This sample is adapted from the original cancel
            sample (KB Q188276) available in MSDN.

Environment:

    Kernel mode

--*/

#include "cancel.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGE, CsampCreateClose)
#pragma alloc_text( PAGE, CsampUnload)
#pragma alloc_text( PAGE, CsampRead)
#endif // ALLOC_PRAGMA

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    registryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    UNICODE_STRING      unicodeDeviceName;
    UNICODE_STRING      unicodeDosDeviceName;
    PDEVICE_OBJECT      deviceObject;
    PDEVICE_EXTENSION   devExtension;
    HANDLE              threadHandle;
    UNICODE_STRING      sddlString;

    UNREFERENCED_PARAMETER (RegistryPath);

    CSAMP_KDPRINT(("DriverEntry Enter \n"));

    //
    // Opt-in to using non-executable pool memory on Windows 8 and later.
    // https://msdn.microsoft.com/en-us/library/windows/hardware/hh920402(v=vs.85).aspx
    //

    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    (void) RtlInitUnicodeString(&unicodeDeviceName, CSAMP_DEVICE_NAME_U);

    (void) RtlInitUnicodeString( &sddlString, L"D:P(A;;GA;;;SY)(A;;GA;;;BA)");

    //
    // We will create a secure deviceobject so that only processes running
    // in admin and local system account can access the device. Refer
    // "Security Descriptor String Format" section in the platform
    // SDK documentation to understand the format of the sddl string.
    // We need to do because this is a legacy driver and there is no INF
    // involved in installing the driver. For PNP drivers, security descriptor
    // is typically specified for the FDO in the INF file.
    //

    status = IoCreateDeviceSecure(
                DriverObject,
                sizeof(DEVICE_EXTENSION),
                &unicodeDeviceName,
                FILE_DEVICE_UNKNOWN,
                FILE_DEVICE_SECURE_OPEN,
                (BOOLEAN) FALSE,
                &sddlString,
                (LPCGUID)&GUID_DEVCLASS_CANCEL_SAMPLE,
                &deviceObject
                );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    DbgPrint("DeviceObject %p\n", deviceObject);

    //
    // Allocate and initialize a Unicode String containing the Win32 name
    // for our device.
    //

    (void)RtlInitUnicodeString( &unicodeDosDeviceName, CSAMP_DOS_DEVICE_NAME_U );


    status = IoCreateSymbolicLink(
                (PUNICODE_STRING) &unicodeDosDeviceName,
                (PUNICODE_STRING) &unicodeDeviceName
                );

    if (!NT_SUCCESS(status))
    {
        IoDeleteDevice(deviceObject);
        return status;
    }

    devExtension = deviceObject->DeviceExtension;

    DriverObject->MajorFunction[IRP_MJ_CREATE]=
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CsampCreateClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = CsampRead;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = CsampCleanup;

    DriverObject->DriverUnload = CsampUnload;

    //
    // Set the flag signifying that we will do buffered I/O. This causes NT
    // to allocate a buffer on a ReadFile operation which will then be copied
    // back to the calling application by the I/O subsystem
    //

    deviceObject->Flags |= DO_BUFFERED_IO;

    //
    // This is used to serailize access to the queue.
    //

    KeInitializeSpinLock(&devExtension->QueueLock);

    KeInitializeSemaphore(&devExtension->IrpQueueSemaphore, 0, MAXLONG );

    //
    // Initialize the pending Irp devicequeue
    //

    InitializeListHead( &devExtension->PendingIrpQueue );

    //
    // Initialize the cancel safe queue
    //
    IoCsqInitialize( &devExtension->CancelSafeQueue,
                     CsampInsertIrp,
                     CsampRemoveIrp,
                     CsampPeekNextIrp,
                     CsampAcquireLock,
                     CsampReleaseLock,
                     CsampCompleteCanceledIrp );
    //
    // 10 is multiplied because system time is specified in 100ns units
    //

    devExtension->PollingInterval.QuadPart = Int32x32To64(
                                CSAMP_RETRY_INTERVAL, -10);
    //
    // Note down system time
    //

    KeQuerySystemTime (&devExtension->LastPollTime);

    //
    // Start the polling thread.
    //

    devExtension->ThreadShouldStop = FALSE;

    status = PsCreateSystemThread(&threadHandle,
                                (ACCESS_MASK)0,
                                NULL,
                                (HANDLE) 0,
                                NULL,
                                CsampPollingThread,
                                deviceObject );

    if ( !NT_SUCCESS( status ))
    {
        IoDeleteSymbolicLink( &unicodeDosDeviceName );
        IoDeleteDevice( deviceObject );
        return status;
    }

    //
    // Convert the Thread object handle into a pointer to the Thread object
    // itself. Then close the handle.
    //

    ObReferenceObjectByHandle(threadHandle,
                            THREAD_ALL_ACCESS,
                            NULL,
                            KernelMode,
                            &devExtension->ThreadObject,
                            NULL );

    ZwClose(threadHandle);

    CSAMP_KDPRINT(("DriverEntry Exit = %x\n", status));

    ASSERT(NT_SUCCESS(status));

    return status;
}


_Use_decl_annotations_
NTSTATUS
CsampCreateClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

   Process the Create and close IRPs sent to this device.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT Status code

--*/
{
    PIO_STACK_LOCATION  irpStack;
    NTSTATUS            status = STATUS_SUCCESS;
    PFILE_CONTEXT       fileContext;

    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE ();

    CSAMP_KDPRINT(("CsampCreateClose Enter\n"));

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(irpStack->FileObject != NULL);

    switch(irpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:

            //
            // The dispatch routine for IRP_MJ_CREATE is called when a
            // file object associated with the device is created.
            // This is typically because of a call to CreateFile() in
            // a user-mode program or because a another driver is
            // layering itself over a this driver. A driver is
            // required to supply a dispatch routine for IRP_MJ_CREATE.
            //
            fileContext = ExAllocatePoolQuotaZero(NonPagedPoolNx | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,
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

            CSAMP_KDPRINT(("IRP_MJ_CREATE\n"));
            break;

        case IRP_MJ_CLOSE:
            //
            // The IRP_MJ_CLOSE dispatch routine is called when a file object
            // opened on the driver is being removed from the system; that is,
            // all file object handles have been closed and the reference count
            // of the file object is down to 0.
            //
            fileContext = irpStack->FileObject->FsContext;

            ExFreePoolWithTag(fileContext, TAG);

            CSAMP_KDPRINT(("IRP_MJ_CLOSE\n"));
            break;

        default:
            CSAMP_KDPRINT((" Invalid CreateClose Parameter\n"));
            status = STATUS_INVALID_PARAMETER;
            break;
    }

    //
    // Save Status for return and complete Irp
    //
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    CSAMP_KDPRINT((" CsampCreateClose Exit = %x\n", status));

    return status;
}


_Use_decl_annotations_
NTSTATUS
CsampRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
 )
 /*++
     Routine Description:

           Read disptach routine

     Arguments:

         DeviceObject - pointer to a device object.
                 Irp             - pointer to current Irp

     Return Value:

         NT status code.

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   devExtension;
    PIO_STACK_LOCATION  irpStack;
    LARGE_INTEGER       currentTime;
    PFILE_CONTEXT       fileContext;
    PVOID               readBuffer;
    BOOLEAN             inCriticalRegion;

    PAGED_CODE();

    CSAMP_KDPRINT(("CsampRead Enter:0x%p\n", Irp));

    devExtension = DeviceObject->DeviceExtension;
    inCriticalRegion = FALSE;

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

    //
    // First make sure there is enough room.
    //
    if (irpStack->Parameters.Read.Length < sizeof(INPUT_DATA))
    {
        Irp->IoStatus.Status = status = STATUS_BUFFER_TOO_SMALL;
        Irp->IoStatus.Information  = 0;
        IoReleaseRemoveLock(&fileContext->FileRundownLock, Irp);
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    //
    // FOR TESTING:
    // Initialize the data to mod 2 of some random number.
    // With this value you can control the number of times the
    // Irp will be queued before completion. Check
    // CsampPollDevice routine to know how this works.
    //

    KeQuerySystemTime(&currentTime);

    readBuffer = Irp->AssociatedIrp.SystemBuffer;

    *((PULONG)readBuffer) = ((currentTime.LowPart/13)%2);

    //
    // To avoid the thread from being suspended after it has queued the IRP and
    // before it signalled the semaphore, we will enter critical region.
    //
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    KeEnterCriticalRegion();
    inCriticalRegion = TRUE;

    //
    // Queue the IRP and return STATUS_PENDING after signalling the
    // polling thread.
    // Note: IoCsqInsertIrp marks the IRP pending.
    //
    IoCsqInsertIrp(&devExtension->CancelSafeQueue, Irp, NULL);

    //
    // Do not touch the IRP once it has been queued because another thread
    // could remove the IRP and complete it before this one gets to run.
    //

    //
    // A semaphore remains signaled as long as its count is greater than
    // zero, and non-signaled when the count is zero. Following function
    // increments the semaphore count by 1.
    //

    KeReleaseSemaphore(&devExtension->IrpQueueSemaphore,
                        0,// No priority boost
                        1,// Increment semaphore by 1
                        FALSE );// No WaitForXxx after this call
    if (inCriticalRegion == TRUE) {
        KeLeaveCriticalRegion();
    }
    //
    // We don't hold the lock for IRP that's pending in the list because this
    // lock is meant to rundown currently dispatching threads when the cleanup
    // is handled.
    //
    IoReleaseRemoveLock(&fileContext->FileRundownLock, Irp);

    return STATUS_PENDING;
}

VOID
CsampPollingThread(
    _In_ PVOID Context
    )
/*++

Routine Description:

    This is the main thread that removes IRP from the queue
    and peforms I/O on it.

Arguments:

    Context     -- pointer to the device object

--*/
{
    PDEVICE_OBJECT DeviceObject = Context;
    PDEVICE_EXTENSION DevExtension =  DeviceObject->DeviceExtension;
    PIRP Irp;
    NTSTATUS    Status;

    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY );

    //
    // Now enter the main IRP-processing loop
    //
    for(;;)
    {
        //
        // Wait indefinitely for an IRP to appear in the work queue or for
        // the Unload routine to stop the thread. Every successful return
        // from the wait decrements the semaphore count by 1.
        //
        KeWaitForSingleObject(&DevExtension->IrpQueueSemaphore,
                            Executive,
                            KernelMode,
                            FALSE,
                            NULL );

        //
        // See if thread was awakened because driver is unloading itself...
        //

        if ( DevExtension->ThreadShouldStop ) {
            PsTerminateSystemThread( STATUS_SUCCESS );
        }

        //
        // Remove a pending IRP from the queue.
        //
        Irp = IoCsqRemoveNextIrp(&DevExtension->CancelSafeQueue, NULL);

        if (!Irp) {
            CSAMP_KDPRINT(("Oops, a queued irp got cancelled\n"));
            continue; // go back to waiting
        }

        for(;;) {
            //
            // Perform I/O
            //
            Status = CsampPollDevice(DeviceObject, Irp);
            if (Status == STATUS_PENDING) {

                //
                // Device is not ready, so sleep for a while and try again.
                //
                KeDelayExecutionThread(KernelMode, FALSE,
                                        &DevExtension->PollingInterval);

            } else {

                //
                // I/O is successful, so complete the Irp.
                //
                Irp->IoStatus.Status = Status;
                IoCompleteRequest (Irp, IO_NO_INCREMENT);
                break;
            }

        }
        //
        // Go back to the top of the loop to see if there's another request waiting.
        //
    } // end of while-loop
}

_Use_decl_annotations_
NTSTATUS
CsampPollDevice(
    PDEVICE_OBJECT DeviceObject,
    PIRP    Irp
    )

/*++

Routine Description:

   Polls for data

Arguments:

    DeviceObject     -- pointer to the device object
    Irp             -- pointer to the requesing Irp


Return Value:

    STATUS_SUCCESS   -- if the poll succeeded,
    STATUS_TIMEOUT   -- if the poll failed (timeout),
                        or the checksum was incorrect
    STATUS_PENDING   -- if polled too soon

--*/
{
    PINPUT_DATA         pInput;

    UNREFERENCED_PARAMETER( DeviceObject );

    pInput  = (PINPUT_DATA)Irp->AssociatedIrp.SystemBuffer;

#ifdef REAL

    RtlZeroMemory( pInput, sizeof(INPUT_DATA) );

    //
    // If currenttime is less than the lasttime polled plus
    // minimum time required for the device to settle
    // then don't poll  and return STATUS_PENDING
    //

    KeQuerySystemTime(&currentTime);
    if (currentTime->QuadPart < (TimeBetweenPolls +
                devExtension->LastPollTime.QuadPart))
    {
        return  STATUS_PENDING;
    }

    //
    // Read/Write to the port here.
    // Fill the INPUT structure
    //

    //
    // Note down the current time as the last polled time
    //

    KeQuerySystemTime(&devExtension->LastPollTime);


    return STATUS_SUCCESS;
#else

    //
    // With this conditional statement
    // you can control the number of times the
    // i/o should be retried before completing.
    //

    if (pInput->Data-- <= 0)
    {
        Irp->IoStatus.Information = sizeof(INPUT_DATA);
        return STATUS_SUCCESS;
    }
    return STATUS_PENDING;

 #endif

}

_Use_decl_annotations_
NTSTATUS
CsampCleanup(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
/*++

Routine Description:
    This dispatch routine is called when the last handle (in
    the whole system) to a file object is closed. In other words, the open
    handle count for the file object goes to 0. A driver that holds pending
    IRPs internally must implement a routine for IRP_MJ_CLEANUP. When the
    routine is called, the driver should cancel all the pending IRPs that
    belong to the file object identified by the IRP_MJ_CLEANUP call. In other
    words, it should cancel all the IRPs that have the same file-object pointer
    as the one supplied in the current I/O stack location of the IRP for the
    IRP_MJ_CLEANUP call. Of course, IRPs belonging to other file objects should
    not be canceled. Also, if an outstanding IRP is completed immediately, the
    driver does not have to cancel it.

Arguments:

    DeviceObject     -- pointer to the device object
    Irp             -- pointer to the requesing Irp

Return Value:

    STATUS_SUCCESS   -- if the poll succeeded,
--*/
{

    PDEVICE_EXTENSION   devExtension;
    PIRP                pendingIrp;
    PIO_STACK_LOCATION  irpStack;
    PFILE_CONTEXT       fileContext;
    NTSTATUS            status;

    CSAMP_KDPRINT(("CsampCleanupIrp enter\n"));

    devExtension = DeviceObject->DeviceExtension;

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

    pendingIrp = IoCsqRemoveNextIrp(&devExtension->CancelSafeQueue,
                                    irpStack->FileObject);

    while(pendingIrp)
    {
        //
        // Cancel the IRP
        //
        pendingIrp->IoStatus.Information = 0;
        pendingIrp->IoStatus.Status = STATUS_CANCELLED;
        CSAMP_KDPRINT(("Cleanup cancelled irp\n"));
        IoCompleteRequest(pendingIrp, IO_NO_INCREMENT);

        pendingIrp = IoCsqRemoveNextIrp(&devExtension->CancelSafeQueue,
                                        irpStack->FileObject);
    }

    //
    // Finally complete the cleanup IRP
    //
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    CSAMP_KDPRINT(("CsampCleanupIrp exit\n"));

    return STATUS_SUCCESS;

}

VOID
CsampUnload(
    _In_ PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object.

Return Value:

    VOID
--*/
{
    PDEVICE_OBJECT      deviceObject = DriverObject->DeviceObject;
    UNICODE_STRING      uniWin32NameString;
    PDEVICE_EXTENSION   devExtension = deviceObject->DeviceExtension;

    PAGED_CODE();

    CSAMP_KDPRINT(("CsampUnload Enter\n"));

    //
    // Set the Stop flag
    //
    devExtension->ThreadShouldStop = TRUE;

    //
    // Make sure the thread wakes up
    //
#pragma prefast(suppress: __WARNING_ERROR, "Passing TRUE as last parameter of KeReleaseSemaphore is just a hint that a wait is next.")
    KeReleaseSemaphore(&devExtension->IrpQueueSemaphore,
                        0,  // No priority boost
                        1,  // Increment semaphore by 1
                        TRUE );// WaitForXxx after this call

    //
    // Wait for the thread to terminate
    //
    KeWaitForSingleObject(devExtension->ThreadObject,
                        Executive,
                        KernelMode,
                        FALSE,
                        NULL );

    ObDereferenceObject(devExtension->ThreadObject);

    //
    // Create counted string version of our Win32 device name.
    //

    RtlInitUnicodeString( &uniWin32NameString, CSAMP_DOS_DEVICE_NAME_U );

    IoDeleteSymbolicLink( &uniWin32NameString );

    IoDeleteDevice( deviceObject );

    CSAMP_KDPRINT(("CsampUnload Exit\n"));
    return;
}

VOID CsampInsertIrp (
    _In_ PIO_CSQ   Csq,
    _In_ PIRP      Irp
    )
{
    PDEVICE_EXTENSION   devExtension;

    devExtension = CONTAINING_RECORD(Csq,
                                 DEVICE_EXTENSION, CancelSafeQueue);

    InsertTailList(&devExtension->PendingIrpQueue,
                         &Irp->Tail.Overlay.ListEntry);
}

VOID CsampRemoveIrp(
    _In_  PIO_CSQ Csq,
    _In_  PIRP    Irp
    )
{
    UNREFERENCED_PARAMETER(Csq);

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
}


PIRP CsampPeekNextIrp(
    _In_  PIO_CSQ Csq,
    _In_  PIRP    Irp,
    _In_  PVOID   PeekContext
    )
{
    PDEVICE_EXTENSION      devExtension;
    PIRP                    nextIrp = NULL;
    PLIST_ENTRY             nextEntry;
    PLIST_ENTRY             listHead;
    PIO_STACK_LOCATION     irpStack;

    devExtension = CONTAINING_RECORD(Csq,
                             DEVICE_EXTENSION, CancelSafeQueue);

    listHead = &devExtension->PendingIrpQueue;

    //
    // If the IRP is NULL, we will start peeking from the listhead, else
    // we will start from that IRP onwards. This is done under the
    // assumption that new IRPs are always inserted at the tail.
    //

    if (Irp == NULL) {
        nextEntry = listHead->Flink;
    } else {
        nextEntry = Irp->Tail.Overlay.ListEntry.Flink;
    }

    while(nextEntry != listHead) {

        nextIrp = CONTAINING_RECORD(nextEntry, IRP, Tail.Overlay.ListEntry);

        irpStack = IoGetCurrentIrpStackLocation(nextIrp);

        //
        // If context is present, continue until you find a matching one.
        // Else you break out as you got next one.
        //

        if (PeekContext) {
            if (irpStack->FileObject == (PFILE_OBJECT) PeekContext) {
                break;
            }
        } else {
            break;
        }
        nextIrp = NULL;
        nextEntry = nextEntry->Flink;
    }

    return nextIrp;

}

//
// CsampAcquireLock modifies the execution level of the current processor.
//
// KeAcquireSpinLock raises the execution level to Dispatch Level and stores
// the current execution level in the Irql parameter to be restored at a later
// time.  KeAcqurieSpinLock also requires us to be running at no higher than
// Dispatch level when it is called.
//
// The annotations reflect these changes and requirments.
//

_IRQL_raises_(DISPATCH_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_Acquires_lock_(CONTAINING_RECORD(Csq,DEVICE_EXTENSION, CancelSafeQueue)->QueueLock)
VOID CsampAcquireLock(
    _In_                                PIO_CSQ Csq,
    _Out_ _At_(*Irql, _Post_ _IRQL_saves_)     PKIRQL  Irql
    )
{
    PDEVICE_EXTENSION   devExtension;

    devExtension = CONTAINING_RECORD(Csq,
                                 DEVICE_EXTENSION, CancelSafeQueue);
    //
    // Suppressing because the address below csq is valid since it's
    // part of DEVICE_EXTENSION structure.
    //
#pragma prefast(suppress: __WARNING_BUFFER_UNDERFLOW, "Underflow using expression 'devExtension->QueueLock'")
    KeAcquireSpinLock(&devExtension->QueueLock, Irql);
}

//
// CsampReleaseLock modifies the execution level of the current processor.
//
// KeReleaseSpinLock assumes we already hold the spin lock and are therefore
// running at Dispatch level.  It will use the Irql parameter saved in a
// previous call to KeAcquireSpinLock to return the thread back to it's original
// execution level.
//
// The annotations reflect these changes and requirments.
//

_IRQL_requires_(DISPATCH_LEVEL)
_Releases_lock_(CONTAINING_RECORD(Csq,DEVICE_EXTENSION, CancelSafeQueue)->QueueLock)
VOID CsampReleaseLock(
    _In_                    PIO_CSQ Csq,
    _In_ _IRQL_restores_    KIRQL   Irql
    )
{
    PDEVICE_EXTENSION   devExtension;

    devExtension = CONTAINING_RECORD(Csq,
                                 DEVICE_EXTENSION, CancelSafeQueue);
    //
    // Suppressing because the address below csq is valid since it's
    // part of DEVICE_EXTENSION structure.
    //
#pragma prefast(suppress: __WARNING_BUFFER_UNDERFLOW, "Underflow using expression 'devExtension->QueueLock'")
    KeReleaseSpinLock(&devExtension->QueueLock, Irql);
}

VOID CsampCompleteCanceledIrp(
    _In_  PIO_CSQ             pCsq,
    _In_  PIRP                Irp
    )
{

    UNREFERENCED_PARAMETER(pCsq);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    CSAMP_KDPRINT(("cancelled irp\n"));
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}
