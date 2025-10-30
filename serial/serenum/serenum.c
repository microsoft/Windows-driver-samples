/*++
Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    SERENUM.C

Abstract:

    This module contains contains the entry points for a standard bus
    PNP / WDM driver.


Environment:

    kernel mode only

Notes:



--*/

#include "pch.h"

//
// Declare some entry functions as pageable, and make DriverEntry
// discardable
//

DRIVER_INITIALIZE DriverEntry;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, Serenum_DriverUnload)
#endif

// disable warnings

#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning(disable:4127) // conditional expression is constant

NTSTATUS
DriverEntry (
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING UniRegistryPath
    )
/*++
Routine Description:

    Initialize the entry points of the driver.

--*/
{
    ULONG i;

    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    UNREFERENCED_PARAMETER (UniRegistryPath);

    Serenum_KdPrint_Def (SER_DBG_SS_TRACE, ("Driver Entry\n"));
    Serenum_KdPrint_Def (SER_DBG_SS_TRACE, ("RegPath: %p\n", UniRegistryPath));

    //
    // Set ever slot to initially pass the request through to the lower
    // device object
    //
    for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
       DriverObject->MajorFunction[i] = Serenum_DispatchPassThrough;
    }

    //
    // Fill in the Dispatch slots intercepted by the filter driver.
    //
    DriverObject->MajorFunction [IRP_MJ_CREATE] =
    DriverObject->MajorFunction [IRP_MJ_CLOSE] = Serenum_CreateClose;
    DriverObject->MajorFunction [IRP_MJ_PNP] = Serenum_PnP;
    DriverObject->MajorFunction [IRP_MJ_POWER] = Serenum_Power;
    DriverObject->MajorFunction [IRP_MJ_DEVICE_CONTROL] = Serenum_IoCtl;
    DriverObject->MajorFunction [IRP_MJ_INTERNAL_DEVICE_CONTROL]
        = Serenum_InternIoCtl;
    DriverObject->DriverUnload = Serenum_DriverUnload;
    DriverObject->DriverExtension->AddDevice = Serenum_AddDevice;


#if DBG
   SerenumLogInit();
#endif

    return STATUS_SUCCESS;
}

IO_COMPLETION_ROUTINE
SerenumSyncCompletion;

NTSTATUS
SerenumSyncCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
                      IN PVOID Context)
{
    PKEVENT SerenumSyncEvent = (PKEVENT) Context;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    _Analysis_assume_(SerenumSyncEvent != NULL);

    KeSetEvent(SerenumSyncEvent, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
Serenum_CreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++
Routine Description:
    Some outside source is trying to create a file against us.

    If this is for the FDO (the bus itself) then the caller is trying to
    open the propriatary conection to tell us which serial port to enumerate.

    If this is for the PDO (an object on the bus) then this is a client that
    wishes to use the serial port.
--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS status = STATUS_SUCCESS;
    KEVENT completionEvent;
    PDEVICE_OBJECT pNextDevice;
    PCOMMON_DEVICE_DATA commonData=(PCOMMON_DEVICE_DATA)DeviceObject->DeviceExtension;


    UNREFERENCED_PARAMETER(DeviceObject);
    status=IoAcquireRemoveLock(&commonData->RemoveLock,Irp);
    if(!NT_SUCCESS(status)){
        Irp->IoStatus.Information=0;
        Irp->IoStatus.Status=status;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
        return status;
    }


   irpStack = IoGetCurrentIrpStackLocation(Irp);

   if (commonData->IsFDO) {
      pNextDevice = ((PFDO_DEVICE_DATA)DeviceObject->DeviceExtension)
                    ->TopOfStack;
   } else {
      pNextDevice = ((PFDO_DEVICE_DATA)((PPDO_DEVICE_DATA)DeviceObject->
                                        DeviceExtension)->ParentFdo->
                     DeviceExtension)->TopOfStack;
   }

   switch (irpStack->MajorFunction) {
   case IRP_MJ_CREATE:
      Serenum_KdPrint_Def(SER_DBG_SS_TRACE, ("Create"));

      //
      // Pass on the create and the close
      //

      status = Serenum_DispatchPassThroughWithoutAcquire(DeviceObject, Irp);
      break;

   case IRP_MJ_CLOSE:
      Serenum_KdPrint_Def (SER_DBG_SS_TRACE, ("Close \n"));

      //
      // Send the close down; after it finishes we can open and take
      // over the port
      //

      IoCopyCurrentIrpStackLocationToNext(Irp);

      KeInitializeEvent(&completionEvent, SynchronizationEvent, FALSE);

      IoSetCompletionRoutine(Irp, SerenumSyncCompletion, &completionEvent,
                             TRUE, TRUE, TRUE);

      status = IoCallDriver(pNextDevice, Irp);


      if (status == STATUS_PENDING) {
         KeWaitForSingleObject(&completionEvent, Executive, KernelMode, FALSE,
                               NULL);
      }

      status = Irp->IoStatus.Status;

      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      IoReleaseRemoveLock(&commonData->RemoveLock, Irp); 

      break;
   }

   return status;
}


NTSTATUS
Serenum_IoCtl (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++
Routine Description:

--*/
{
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                status;
    PCOMMON_DEVICE_DATA     commonData;
    PFDO_DEVICE_DATA        fdoData;
    HANDLE                  keyHandle;
    ULONG                   actualLength;


	
    status = STATUS_SUCCESS;
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT (IRP_MJ_DEVICE_CONTROL == irpStack->MajorFunction);

    commonData = (PCOMMON_DEVICE_DATA) DeviceObject->DeviceExtension;
    fdoData = (PFDO_DEVICE_DATA) DeviceObject->DeviceExtension;

    status = IoAcquireRemoveLock(&commonData->RemoveLock, Irp);
    if (!NT_SUCCESS(status)){
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }


    //buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    // We only take Device Control requests for the FDO.
    // That is the bus itself.
    //
    // The request is one of the propriatary Ioctls for
    //
    // NB We ARE a filter driver, so we DO pass on the irp if we don't handle it
    //

    //inlen = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    //outlen = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (!commonData->IsFDO) {
        //
        // These commands are only allowed to go to the FDO.  Since they came
        // into the PDO, we need to fire them down to the serenum Fdo.
        //
        return Serenum_Generic_FireAndRelease_RemoveLock(((PPDO_DEVICE_DATA) DeviceObject->DeviceExtension)->ParentFdo,Irp);
    }

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_SERENUM_GET_PORT_NAME:
        //
        // Get the port name from the registry.
        // This IOCTL is used by the modem cpl.
        //

        status = IoOpenDeviceRegistryKey(fdoData->UnderlyingPDO,
                                         PLUGPLAY_REGKEY_DEVICE,
                                         STANDARD_RIGHTS_READ,
                                         &keyHandle);

        if (!NT_SUCCESS(status)) {
           //
           // This is a fatal error.  If we can't get to our registry key,
           // we are sunk.
           //
           Serenum_KdPrint_Def (SER_DBG_PNP_ERROR,
                ("IoOpenDeviceRegistryKey failed - %x \n", status));
           Irp->IoStatus.Information = 0;
        } else {
            status = Serenum_GetRegistryKeyValue(
                keyHandle,
                L"PortName",
                sizeof(L"PortName"),
                Irp->AssociatedIrp.SystemBuffer,
                irpStack->Parameters.DeviceIoControl.OutputBufferLength,
                &actualLength);
            if ( STATUS_OBJECT_NAME_NOT_FOUND == status ||
                 STATUS_INVALID_PARAMETER  == status ) {
                status = Serenum_GetRegistryKeyValue(
                    keyHandle,
                    L"Identifier",
                    sizeof(L"Identifier"),
                    Irp->AssociatedIrp.SystemBuffer,
                    irpStack->Parameters.DeviceIoControl.OutputBufferLength,
                    &actualLength);
            }
            Irp->IoStatus.Information = actualLength;
            ZwClose (keyHandle);
        }
        break;
    default:
        //
        // This is not intended for us - fire and Protect!
        //
        return Serenum_DispatchPassThroughWithoutAcquire(
            DeviceObject,
            Irp);
    }

    IoReleaseRemoveLock(&commonData->RemoveLock, Irp);
    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS
Serenum_InternIoCtl (
    PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++
Routine Description:

--*/
{
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                status;
    PCOMMON_DEVICE_DATA     commonData;
    PPDO_DEVICE_DATA        pdoData;

//    PAGED_CODE();


    commonData = (PCOMMON_DEVICE_DATA) DeviceObject->DeviceExtension;
    status=IoAcquireRemoveLock(&commonData->RemoveLock,Irp);
    if(!NT_SUCCESS(status)){
        Irp->IoStatus.Information=0;
        Irp->IoStatus.Status=status;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
        return status;
    }
    status = STATUS_SUCCESS;
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT (IRP_MJ_INTERNAL_DEVICE_CONTROL == irpStack->MajorFunction);

    
    pdoData = (PPDO_DEVICE_DATA) DeviceObject->DeviceExtension;

    //
    // We only take Internal Device Control requests for the PDO.
    // That is the objects on the bus (representing the serial ports)
    //
    // We do pass on the irp if this comes into the fdo, but not if it comes
    // into the pdo.
    //

    if (commonData->IsFDO) {
        return Serenum_DispatchPassThroughWithoutAcquire(
            DeviceObject,
            Irp);
    }else{
        //buffer = Irp->UserBuffer;

        switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_INTERNAL_SERENUM_REMOVE_SELF:
            Serenum_KdPrint(pdoData, SER_DBG_SS_TRACE, ("Remove self\n"));

            ((PFDO_DEVICE_DATA) pdoData->ParentFdo->DeviceExtension)->
                PDOForcedRemove = TRUE;
            Serenum_PDO_EnumMarkMissing(pdoData->ParentFdo->DeviceExtension, DeviceObject->DeviceExtension);

            IoInvalidateDeviceRelations(
                ((PFDO_DEVICE_DATA) pdoData->ParentFdo->DeviceExtension)->
                UnderlyingPDO,
                BusRelations );
            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;
            break;

        default:
            //
            // Pass it through
            //
            return Serenum_DispatchPassThroughWithoutAcquire(DeviceObject, Irp);
        }
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    IoReleaseRemoveLock(&commonData->RemoveLock,Irp);
    return status;
}


VOID
Serenum_DriverUnload (
    IN PDRIVER_OBJECT Driver
    )
/*++
Routine Description:
    Clean up everything we did in driver entry.

--*/
{
    UNREFERENCED_PARAMETER (Driver);
    PAGED_CODE();

    //
    // All the device objects should be gone.
    //

    ASSERT (NULL == Driver->DeviceObject);

    //
    // Here we free any resources allocated in DriverEntry
    //

#if DBG
    SerenumLogFree();
#endif

    return;
}

NTSTATUS
Serenum_DispatchPassThrough(
                           IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp
                           )
/*++
Routine Description:

    Passes a request on to the lower driver.

--*/
{
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    BOOLEAN waitForEnum = FALSE;
    NTSTATUS rval;
    PFDO_DEVICE_DATA pFdoData;
    BOOLEAN isFdo;
   
    PCOMMON_DEVICE_DATA commonData=(PCOMMON_DEVICE_DATA)DeviceObject->DeviceExtension;
    isFdo = commonData->IsFDO;
   
    rval=IoAcquireRemoveLock(&commonData->RemoveLock,Irp);
    if(!NT_SUCCESS(rval)){
        Irp->IoStatus.Information=0;
        Irp->IoStatus.Status=rval;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
        return rval;
    }

    if (isFdo) {
      pFdoData = (PFDO_DEVICE_DATA)DeviceObject->DeviceExtension;

#if DBG
   switch (IrpStack->MajorFunction) {
   case IRP_MJ_READ:
      LOGENTRY(LOG_PASSTHROUGH, 'SFRD', DeviceObject, Irp, 0);
      break;

   case IRP_MJ_WRITE:
      LOGENTRY(LOG_PASSTHROUGH, 'SFWR', DeviceObject, Irp, 0);
      break;

   case IRP_MJ_DEVICE_CONTROL:
      LOGENTRY(LOG_PASSTHROUGH, 'SFDC', DeviceObject, Irp, 0);
      break;

   case IRP_MJ_CREATE:
      LOGENTRY(LOG_PASSTHROUGH, 'SFCR', DeviceObject, Irp, 0);
      break;

   case IRP_MJ_CLOSE:
      LOGENTRY(LOG_PASSTHROUGH, 'SFCL', DeviceObject, Irp, 0);
      break;

   default:
      break;
   }
#endif // DBG

   } else {
      pFdoData = ((PPDO_DEVICE_DATA) DeviceObject->DeviceExtension)->
                 ParentFdo->DeviceExtension;

#if DBG
   switch (IrpStack->MajorFunction) {
   case IRP_MJ_READ:
      LOGENTRY(LOG_PASSTHROUGH, 'SPRD', DeviceObject, Irp, 0);
      break;

   case IRP_MJ_WRITE:
      LOGENTRY(LOG_PASSTHROUGH, 'SPWR', DeviceObject, Irp, 0);
      break;

   case IRP_MJ_DEVICE_CONTROL:
      LOGENTRY(LOG_PASSTHROUGH, 'SPDC', DeviceObject, Irp, 0);
      break;

   case IRP_MJ_CREATE:
      LOGENTRY(LOG_PASSTHROUGH, 'SPCR', DeviceObject, Irp, 0);
      break;

   case IRP_MJ_CLOSE:
      LOGENTRY(LOG_PASSTHROUGH, 'SPCL', DeviceObject, Irp, 0);
      break;

   default:
      break;
   }
#endif // DBG
   }

   if (IrpStack->MajorFunction == IRP_MJ_CREATE) {
      //
      // If we're doing an enumeration, we must wait here
      //

      waitForEnum = TRUE;

      LOGENTRY(LOG_PASSTHROUGH, 'SPCW', DeviceObject, Irp, 0);

      rval = KeWaitForSingleObject(&pFdoData->CreateSemaphore, Executive,
                                   KernelMode, FALSE, NULL);
      if (!NT_SUCCESS(rval)) {
         LOGENTRY(LOG_PASSTHROUGH, 'SPCF', DeviceObject, Irp, rval);
         Irp->IoStatus.Status = rval;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         IoReleaseRemoveLock(&commonData->RemoveLock, Irp);
         return rval;
      }
   }

   //
   // Pass the IRP to the target
   //

   

    if (waitForEnum) {
        IoSkipCurrentIrpStackLocation (Irp);
        rval = IoCallDriver(pFdoData->TopOfStack, Irp);
        KeReleaseSemaphore(&pFdoData->CreateSemaphore, IO_NO_INCREMENT, 1, FALSE);
        IoReleaseRemoveLock(&commonData->RemoveLock, Irp);
    }else{
        rval=Serenum_Generic_FireAndRelease_RemoveLock(pFdoData->TopOfStack,Irp);
    }
    return rval;
}


NTSTATUS Serenum_Generic_FireAndRelease_RemoveLock(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
    NTSTATUS status;
    // we should not block on start irp instead completion routine should do processing
    IoCopyCurrentIrpStackLocationToNext(Irp);
    // use ExIoSetCompletionRoutineEx to make sure non pnp drivers are unloaded while completion routines could still have to run
    IoSetCompletionRoutine(Irp,
                           (PIO_COMPLETION_ROUTINE)Serenum_CompletionRoutine,
                           NULL,
                           TRUE,
                           TRUE,
                           TRUE); 
    status = IoCallDriver(DeviceObject, Irp);
    return status;
}

NTSTATUS
Serenum_DispatchPassThroughWithoutAcquire(
                           IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp
                           )
/*++
Routine Description:

    Passes a request on to the lower driver.

--*/
{
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    BOOLEAN waitForEnum = FALSE;
    NTSTATUS rval;
    PFDO_DEVICE_DATA pFdoData;
    BOOLEAN isFdo;
   
    PCOMMON_DEVICE_DATA commonData=(PCOMMON_DEVICE_DATA)DeviceObject->DeviceExtension;
    isFdo = commonData->IsFDO;

    if (isFdo) {
        pFdoData = (PFDO_DEVICE_DATA)DeviceObject->DeviceExtension;

#if DBG
        switch (IrpStack->MajorFunction) {
        case IRP_MJ_READ:
            LOGENTRY(LOG_PASSTHROUGH, 'SFRD', DeviceObject, Irp, 0);
            break;

        case IRP_MJ_WRITE:
            LOGENTRY(LOG_PASSTHROUGH, 'SFWR', DeviceObject, Irp, 0);
            break;

        case IRP_MJ_DEVICE_CONTROL:
            LOGENTRY(LOG_PASSTHROUGH, 'SFDC', DeviceObject, Irp, 0);
            break;

        case IRP_MJ_CREATE:
            LOGENTRY(LOG_PASSTHROUGH, 'SFCR', DeviceObject, Irp, 0);
            break;

        case IRP_MJ_CLOSE:
            LOGENTRY(LOG_PASSTHROUGH, 'SFCL', DeviceObject, Irp, 0);
            break;

        default:
            break;
    }
#endif // DBG

    } else {
        pFdoData = ((PPDO_DEVICE_DATA) DeviceObject->DeviceExtension)->ParentFdo->DeviceExtension;

#if DBG
        switch (IrpStack->MajorFunction) {
            case IRP_MJ_READ:
                LOGENTRY(LOG_PASSTHROUGH, 'SPRD', DeviceObject, Irp, 0);
                break;

            case IRP_MJ_WRITE:
                LOGENTRY(LOG_PASSTHROUGH, 'SPWR', DeviceObject, Irp, 0);
                break;

            case IRP_MJ_DEVICE_CONTROL:
                LOGENTRY(LOG_PASSTHROUGH, 'SPDC', DeviceObject, Irp, 0);
                break;

            case IRP_MJ_CREATE:
                LOGENTRY(LOG_PASSTHROUGH, 'SPCR', DeviceObject, Irp, 0);
                break;

            case IRP_MJ_CLOSE:
                LOGENTRY(LOG_PASSTHROUGH, 'SPCL', DeviceObject, Irp, 0);
                break;

            default:
                break;
        }
#endif // DBG
    }

    if (IrpStack->MajorFunction == IRP_MJ_CREATE) {
        //
        // If we're doing an enumeration, we must wait here
        //

        waitForEnum = TRUE;

        LOGENTRY(LOG_PASSTHROUGH, 'SPCW', DeviceObject, Irp, 0);

        rval = KeWaitForSingleObject(&pFdoData->CreateSemaphore, Executive,
                                   KernelMode, FALSE, NULL);
        if (!NT_SUCCESS(rval)) {
            LOGENTRY(LOG_PASSTHROUGH, 'SPCF', DeviceObject, Irp, rval);
            Irp->IoStatus.Status = rval;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            IoReleaseRemoveLock(&commonData->RemoveLock, Irp);
            return rval;
        }
    }

    //
    // Pass the IRP to the target
    //

   

    if (waitForEnum){
        IoSkipCurrentIrpStackLocation (Irp);
        rval = IoCallDriver(pFdoData->TopOfStack, Irp);
        KeReleaseSemaphore(&pFdoData->CreateSemaphore, IO_NO_INCREMENT, 1, FALSE);
        IoReleaseRemoveLock(&commonData->RemoveLock, Irp);
    }else{
        rval=Serenum_Generic_FireAndRelease_RemoveLock(pFdoData->TopOfStack,Irp);
    }
    return rval;
}



void
Serenum_InitPDO (
    _In_ __drv_aliasesMem PDEVICE_OBJECT      Pdo,
    PFDO_DEVICE_DATA    FdoData
    )
/*
Description:
    Common code to initialize a newly created serenum pdo.
    Called either when the control panel exposes a device or when Serenum senses
    a new device was attached.

Parameters:
    Pdo - The pdo
    FdoData - The fdo's device extension
*/
{
    ULONG FdoFlags = FdoData->Self->Flags;
    PPDO_DEVICE_DATA pdoData = Pdo->DeviceExtension;
    KIRQL oldIrql;

    //
    // Check the IO style
    //
    if (FdoFlags & DO_BUFFERED_IO) {
        Pdo->Flags |= DO_BUFFERED_IO;
    } else if (FdoFlags & DO_DIRECT_IO) {
        Pdo->Flags |= DO_DIRECT_IO;
    }

    //
    // Increment the pdo's stacksize so that it can pass irps through
    //
    Pdo->StackSize += FdoData->Self->StackSize;

    //
    // Initialize the rest of the device extension
    //
    pdoData->IsFDO = FALSE;
    pdoData->Self = Pdo;

    pdoData->ParentFdo = FdoData->Self;

    pdoData->Started = FALSE; // irp_mn_start has yet to be received
    pdoData->Attached = TRUE; // attached to the bus
    //pdoData->Removed = FALSE; // no irp_mn_remove as of yet
    pdoData->DebugLevel = FdoData->DebugLevel;  // Copy the debug level

    pdoData->DeviceState = PowerDeviceD0;
    pdoData->SystemState = PowerSystemWorking;

    //
    // Add the pdo to serenum's list
    //

    ASSERT(FdoData->NewPDO == NULL);
    ASSERT(FdoData->NewPdoData == NULL);
    ASSERT(FdoData->NewNumPDOs == 0);

    KeAcquireSpinLock(&FdoData->EnumerationLock, &oldIrql);

    FdoData->NewPDO = Pdo;
    FdoData->NewPdoData = pdoData;
    FdoData->NewNumPDOs = 1;

    FdoData->EnumFlags |= SERENUM_ENUMFLAG_DIRTY;

    KeReleaseSpinLock(&FdoData->EnumerationLock, oldIrql);

    Pdo->Flags &= ~DO_DEVICE_INITIALIZING;
    Pdo->Flags |= DO_POWER_PAGABLE;
}

#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

#pragma warning(default:4127) // conditional expression is constant


