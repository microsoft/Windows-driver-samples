/*++
Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    PNP.C

Abstract:

    This module contains contains the plugplay calls
    PNP / WDM BUS driver.


Environment:

    kernel mode only

Notes:


--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, Serenum_AddDevice)
#pragma alloc_text (PAGE, Serenum_PnP)
#pragma alloc_text (PAGE, Serenum_FDO_PnP)
#pragma alloc_text (PAGE, Serenum_PDO_PnP)
#pragma alloc_text (PAGE, Serenum_PnPRemove)
#pragma alloc_text (PAGE, SerenumStartDeviceWorker)
//#pragma alloc_text (PAGE, Serenum_Remove)
#endif

// disable warnings

#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning(disable:4127) // conditional expression is constant

NTSTATUS
Serenum_AddDevice(IN PDRIVER_OBJECT DriverObject,
                  IN PDEVICE_OBJECT BusPhysicalDeviceObject)
/*++
Routine Description.
    A bus has been found.  Attach our FDO to it.
    Allocate any required resources.  Set things up.  And be prepared for the
    first ``start device.''

Arguments:
    BusPhysicalDeviceObject - Device object representing the bus.  That to which
        we attach a new FDO.

    DriverObject - This very self referenced driver.

--*/
{
   NTSTATUS status;
   PDEVICE_OBJECT deviceObject;
   PFDO_DEVICE_DATA pDeviceData;
   HANDLE keyHandle;
   ULONG actualLength;
   PCOMMON_DEVICE_DATA commonData;

   PAGED_CODE();

   Serenum_KdPrint_Def(SER_DBG_PNP_TRACE, ("Add Device: 0x%p\n",
                                           BusPhysicalDeviceObject));
   //
   // Create our FDO
   //

   status = IoCreateDevice(DriverObject, sizeof(FDO_DEVICE_DATA), NULL,
                           FILE_DEVICE_BUS_EXTENDER, 0, TRUE, &deviceObject);

   if (NT_SUCCESS(status)) {
      pDeviceData = (PFDO_DEVICE_DATA)deviceObject->DeviceExtension;
      RtlFillMemory (pDeviceData, sizeof (FDO_DEVICE_DATA), 0);

      pDeviceData->IsFDO = TRUE;
      pDeviceData->DebugLevel = SER_DEFAULT_DEBUG_OUTPUT_LEVEL;
      pDeviceData->Self = deviceObject;
      pDeviceData->AttachedPDO = NULL;
      pDeviceData->NumPDOs = 0;
      pDeviceData->DeviceState = PowerDeviceD0;
      pDeviceData->SystemState = PowerSystemWorking;
      pDeviceData->PDOForcedRemove = FALSE;

      pDeviceData->SystemWake=PowerSystemUnspecified;
      pDeviceData->DeviceWake=PowerDeviceUnspecified;

      pDeviceData->Removed = FALSE;
      commonData=(PCOMMON_DEVICE_DATA)deviceObject->DeviceExtension;
      IoInitializeRemoveLock(&commonData->RemoveLock, SERENUM_POOL_TAG, 0, 0);
	  

      //
      // Set the PDO for use with PlugPlay functions
      //

      pDeviceData->UnderlyingPDO = BusPhysicalDeviceObject;


      //
      // Attach our filter driver to the device stack.
      // the return value of IoAttachDeviceToDeviceStack is the top of the
      // attachment chain.  This is where all the IRPs should be routed.
      //
      // Our filter will send IRPs to the top of the stack and use the PDO
      // for all PlugPlay functions.
      //

      pDeviceData->TopOfStack
         = IoAttachDeviceToDeviceStack(deviceObject, BusPhysicalDeviceObject);
      
      if (!pDeviceData->TopOfStack) {
         Serenum_KdPrint(pDeviceData, SER_DBG_PNP_ERROR,
                         ("AddDevice: IoAttach failed (%x)", status));
         IoDeleteDevice(deviceObject);
         return STATUS_UNSUCCESSFUL;
      }

      //
      // Set the type of IO we do
      //

      if (pDeviceData->TopOfStack->Flags & DO_BUFFERED_IO) {
         deviceObject->Flags |= DO_BUFFERED_IO;
      } else if (pDeviceData->TopOfStack->Flags & DO_DIRECT_IO) {
         deviceObject->Flags |= DO_DIRECT_IO;
      }

           
      KeInitializeSemaphore(&pDeviceData->CreateSemaphore, 1, 1);
      KeInitializeSpinLock(&pDeviceData->EnumerationLock);



      //
      // Tell the PlugPlay system that this device will need an interface
      // device class shingle.
      //
      // It may be that the driver cannot hang the shingle until it starts
      // the device itself, so that it can query some of its properties.
      // (Aka the shingles guid (or ref string) is based on the properties
      // of the device.)
      //

      status = IoRegisterDeviceInterface(BusPhysicalDeviceObject,
                                         (LPGUID)&GUID_SERENUM_BUS_ENUMERATOR,
                                         NULL,
#pragma warning(suppress: 6014)
                                         &pDeviceData->DevClassAssocName);
      // pDeviceData->DevClassAssocName is freed when processing IRP_MN_REMOVE_DEVICE or in the error code paths of this routine
      if (!NT_SUCCESS(status)) {
         Serenum_KdPrint(pDeviceData, SER_DBG_PNP_ERROR,
                         ("AddDevice: IoRegisterDCA failed (%x)", status));
		 
         IoDetachDevice(pDeviceData->TopOfStack);
         IoDeleteDevice(deviceObject);
         return status;
      }

      //
      // If for any reason you need to save values in a safe location that
      // clients of this DeviceClassAssociate might be interested in reading
      // here is the time to do so, with the function
      // IoOpenDeviceClassRegistryKey
      // the symbolic link name used is was returned in
      // pDeviceData->DevClassAssocName (the same name which is returned by
      // IoGetDeviceClassAssociations and the SetupAPI equivs.
      //

#if DBG
      {
         PWCHAR deviceName = NULL;
         ULONG nameLength = 0;

         status = IoGetDeviceProperty(BusPhysicalDeviceObject,
                                      DevicePropertyPhysicalDeviceObjectName, 0,
                                      NULL, &nameLength);

         if ((nameLength != 0) && (status == STATUS_BUFFER_TOO_SMALL)) {
            deviceName = ExAllocatePoolZero(NonPagedPoolNx, nameLength,SERENUM_POOL_TAG);

            if (NULL == deviceName) {
               goto someDebugStuffExit;
            }

            IoGetDeviceProperty(BusPhysicalDeviceObject,
                                DevicePropertyPhysicalDeviceObjectName,
                                nameLength, deviceName, &nameLength);

            Serenum_KdPrint(pDeviceData, SER_DBG_PNP_TRACE,
                            ("AddDevice: %p to %p->%p (%ws) \n", deviceObject,
                             pDeviceData->TopOfStack, BusPhysicalDeviceObject,
                             deviceName));
         }

         someDebugStuffExit:;
         if (deviceName != NULL) {
            ExFreePoolWithTag(deviceName,SERENUM_POOL_TAG);
         }
      }
#endif // DBG

      //
      // Turn on the shingle and point it to the given device object.
      //
      status = IoSetDeviceInterfaceState(&pDeviceData->DevClassAssocName,
                                         TRUE);
	  
      if (!NT_SUCCESS(status)) {
         Serenum_KdPrint(pDeviceData, SER_DBG_PNP_ERROR,
                         ("AddDevice: IoSetDeviceClass failed (%x)", status));
		 
         RtlFreeUnicodeString(&pDeviceData->DevClassAssocName);
         return status;
      }

      //
      // Open the regW2stry and read in our settings
      //

      status = IoOpenDeviceRegistryKey(pDeviceData->UnderlyingPDO,
                                       PLUGPLAY_REGKEY_DEVICE,
                                       STANDARD_RIGHTS_READ, &keyHandle);

      if (status == STATUS_SUCCESS) {
         status
            = Serenum_GetRegistryKeyValue(keyHandle, L"SkipEnumerations",
                                          sizeof(L"SkipEnumerations"),
                                          &pDeviceData->SkipEnumerations,
                                          sizeof(pDeviceData->SkipEnumerations),
                                          &actualLength);

         if ((status != STATUS_SUCCESS)
             || (actualLength != sizeof(pDeviceData->SkipEnumerations))) {
            pDeviceData->SkipEnumerations = 0;
            status = STATUS_SUCCESS;

         }

         ZwClose(keyHandle);
      }
   }

   if (NT_SUCCESS(status)) {
      deviceObject->Flags |= DO_POWER_PAGABLE;
      deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

   }
   
   return status;
}


NTSTATUS
Serenum_PnP(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++
Routine Description:
    Answer the plethora of Irp Major PnP IRPS.
--*/
{
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                status;
    PCOMMON_DEVICE_DATA     commonData;

    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);

    commonData = (PCOMMON_DEVICE_DATA)DeviceObject->DeviceExtension;
    status=IoAcquireRemoveLock(&commonData->RemoveLock,Irp);
    if(!NT_SUCCESS(status)){
        Irp->IoStatus.Information=0;
        Irp->IoStatus.Status=status;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
        Serenum_KdPrint(commonData, SER_DBG_PNP_TRACE,
                       ("PNP: removed DO: %p got IRP: %p\n", DeviceObject,
                         Irp));
        return status;
    }

    

    //
    // Call either the FDO or PDO Pnp code
    //

    if (commonData->IsFDO) {
        Serenum_KdPrint(commonData, SER_DBG_PNP_TRACE,
                        ("PNP: Functional DO: %p IRP: %p MJ: %X MIN: %X\n",
                         DeviceObject, Irp, irpStack->MajorFunction,
                         irpStack->MinorFunction));

        status = Serenum_FDO_PnP(DeviceObject, Irp, irpStack,
                                 (PFDO_DEVICE_DATA)commonData);
        goto PnPDone;

    }

    //
    // PDO
    //

    Serenum_KdPrint(commonData, SER_DBG_PNP_TRACE,
                    ("PNP: Physical DO: %p IRP: %p MJ: %X MIN: %X\n",
                     DeviceObject, Irp, irpStack->MajorFunction,
                         irpStack->MinorFunction));

    status = Serenum_PDO_PnP(DeviceObject, Irp, irpStack,
                             (PPDO_DEVICE_DATA)commonData);

PnPDone:;
    return status;
}


NTSTATUS
SerenumCheckEnumerations(IN PFDO_DEVICE_DATA PFdoData)
{

   KIRQL oldIrql;
   NTSTATUS status;
   PIRP pIrp;
   BOOLEAN sameDevice = TRUE;

   Serenum_KdPrint(PFdoData, SER_DBG_PNP_TRACE, ("Checking enumerations"));

   //
   // If appropriate, check for new devices or if old devices still there.
   //

   if (PFdoData->SkipEnumerations == 0) {

      KeAcquireSpinLock(&PFdoData->EnumerationLock, &oldIrql);

      if (PFdoData->EnumFlags == SERENUM_ENUMFLAG_CLEAN) {
          Serenum_KdPrint(PFdoData, SER_DBG_PNP_TRACE, ("EnumFlag Clean"));

         //
         // If nothing is going on, kick off an enumeration
         //

         PFdoData->EnumFlags |= SERENUM_ENUMFLAG_PENDING;
         KeReleaseSpinLock(&PFdoData->EnumerationLock, oldIrql);


         status = SerenumStartProtocolThread(PFdoData);
      } else if ((PFdoData->EnumFlags
                  & (SERENUM_ENUMFLAG_REMOVED | SERENUM_ENUMFLAG_PENDING))
                 == SERENUM_ENUMFLAG_REMOVED) {
          Serenum_KdPrint(PFdoData, SER_DBG_PNP_TRACE, ("EnumFlag Removed"));
          //
          // Clear the flag and do it synchronously to make sure we
          // get the exact current state
          //

          PFdoData->EnumFlags &= ~SERENUM_ENUMFLAG_REMOVED;

          KeReleaseSpinLock(&PFdoData->EnumerationLock, oldIrql);

          pIrp = IoAllocateIrp(PFdoData->TopOfStack->StackSize + 1, FALSE);

          if (pIrp == NULL) {
              status = STATUS_INSUFFICIENT_RESOURCES;
              goto SerenumCheckEnumerationsOut;
          }

          IoSetNextIrpStackLocation(pIrp);
          status = Serenum_ReenumerateDevices(pIrp, PFdoData, &sameDevice);

#pragma prefast(suppress:__WARNING_REDUNDANT_POINTER_TEST __WARNING_REDUNDANT_POINTER_TEST_FAR_EVIDENCE, "pIrp cannot be NULL; see IoSetNextIrpStackLocation")
          if (pIrp != NULL) {
              IoFreeIrp(pIrp);
          }

          KeAcquireSpinLock(&PFdoData->EnumerationLock, &oldIrql);

          if (status == STATUS_SUCCESS) {
              PFdoData->AttachedPDO = PFdoData->NewPDO;
              PFdoData->PdoData = PFdoData->NewPdoData;
              PFdoData->NumPDOs = PFdoData->NewNumPDOs;
              PFdoData->PDOForcedRemove = PFdoData->NewPDOForcedRemove;
          }

          KeReleaseSpinLock(&PFdoData->EnumerationLock, oldIrql);

      } else if (PFdoData->EnumFlags & SERENUM_ENUMFLAG_DIRTY) {

          Serenum_KdPrint(PFdoData, SER_DBG_PNP_TRACE, ("EnumFlag Dirty"));

         //
         // If there is a new value, use the new values
         //

         PFdoData->AttachedPDO = PFdoData->NewPDO;
         PFdoData->PdoData = PFdoData->NewPdoData;
         PFdoData->NumPDOs = PFdoData->NewNumPDOs;
         PFdoData->PDOForcedRemove = PFdoData->NewPDOForcedRemove;
         PFdoData->EnumFlags &= ~SERENUM_ENUMFLAG_DIRTY;

         KeReleaseSpinLock(&PFdoData->EnumerationLock, oldIrql);
         status = STATUS_SUCCESS;
      } else {
          Serenum_KdPrint(PFdoData, SER_DBG_PNP_TRACE, ("EnumFlag default"));

         //
         // Use the current values
         //

         KeReleaseSpinLock(&PFdoData->EnumerationLock, oldIrql);
         status = STATUS_SUCCESS;
      }

   } else {
      status = STATUS_SUCCESS;

      if (PFdoData->SkipEnumerations != 0xffffffff) {
         PFdoData->SkipEnumerations--;
      }
   }

SerenumCheckEnumerationsOut:

   return status;
}




NTSTATUS
Serenum_FDO_PnP (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpStack,
    IN PFDO_DEVICE_DATA     DeviceData
    )
/*++
Routine Description:
    Handle requests from the PlugPlay system for the BUS itself

    NB: the various Minor functions of the PlugPlay system will not be
    overlapped and do not have to be reentrant

--*/
{
    NTSTATUS    status;
    KEVENT      event;
    ULONG       length;
    ULONG       i;
    PDEVICE_RELATIONS   relations;

    PCOMMON_DEVICE_DATA commonData=(PCOMMON_DEVICE_DATA)DeviceObject->DeviceExtension;
    PIO_WORKITEM          SerenumStartDeviceWorkItem  = NULL;
	

    PAGED_CODE();

    switch (IrpStack->MinorFunction) {
    case IRP_MN_START_DEVICE:
        //
        // BEFORE you are allowed to ``touch'' the device object to which
        // the FDO is attached (that send an irp from the bus to the Device
        // object to which the bus is attached).   You must first pass down
        // the start IRP.  It might not be powered on, or able to access or
        // something.
        //

        Serenum_KdPrint (DeviceData, SER_DBG_PNP_TRACE, ("Start Device\n"));

        if (DeviceData->Started){
            Serenum_KdPrint (DeviceData, SER_DBG_PNP_TRACE,
                ("Device already started\n"));
            status = STATUS_SUCCESS;
            break;
        }

        // if we get here we know the device is stopped
        IoCopyCurrentIrpStackLocationToNext (Irp);
        SerenumStartDeviceWorkItem = IoAllocateWorkItem(DeviceObject);
        if (SerenumStartDeviceWorkItem==NULL){
            status = STATUS_INSUFFICIENT_RESOURCES;
	    break;
        }

        IoMarkIrpPending(Irp);
        IoSetCompletionRoutine (Irp,
                                SerenumStartDeviceCompletion,
                                SerenumStartDeviceWorkItem,
                                TRUE,
                                TRUE,
                                TRUE);

        IoCallDriver (DeviceData->TopOfStack, Irp);
        status = STATUS_PENDING;
        return status;
        break;

    case IRP_MN_QUERY_STOP_DEVICE:
        Serenum_KdPrint (DeviceData, SER_DBG_PNP_TRACE,
            ("Query Stop Device\n"));

        //
        // Test to see if there are any PDO created as children of this FDO
        // If there are then conclude the device is busy and fail the
        // query stop.
        //
        // CIMEXCIMEX
        // We could do better, by seing if the children PDOs are actually
        // currently open.  If they are not then we could stop, get new
        // resouces, fill in the new resouce values, and then when a new client
        // opens the PDO use the new resources.  But this works for now.
        //

        if (DeviceData->AttachedPDO
            || (DeviceData->EnumFlags & SERENUM_ENUMFLAG_PENDING)) {
            status = STATUS_UNSUCCESSFUL;

        } else {
            status = STATUS_SUCCESS;
        }

        Irp->IoStatus.Status = status;

        if (NT_SUCCESS(status)) {
           IoSkipCurrentIrpStackLocation (Irp);
           status = IoCallDriver (DeviceData->TopOfStack, Irp);
        } else {
          IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }

	      IoReleaseRemoveLock(&commonData->RemoveLock,Irp);
        return status;

    case IRP_MN_CANCEL_STOP_DEVICE:
        //
        // We always succeed a cancel stop
        //

        Serenum_KdPrint (DeviceData, SER_DBG_PNP_TRACE,
                         ("Cancel Stop Device\n"));

        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(DeviceData->TopOfStack, Irp);

        IoReleaseRemoveLock(&commonData->RemoveLock,Irp);
        return status;

    case IRP_MN_STOP_DEVICE:
        Serenum_KdPrint (DeviceData, SER_DBG_PNP_TRACE, ("Stop Device\n"));

        //
        // Wait for the enum thread to complete if it's running
        //

        SerenumWaitForEnumThreadTerminate(DeviceData);

        //
        // After the start IRP has been sent to the lower driver object, the
        // bus may NOT send any more IRPS down ``touch'' until another START
        // has occured.
        // What ever access is required must be done before the Irp is passed
        // on.
        //
        // Stop device means that the resources given durring Start device
        // are no revoked.  So we need to stop using them
        //

        DeviceData->Started = FALSE;

        //
        // We don't need a completion routine so fire and forget.
        //
        // Set the current stack location to the next stack location and
        // call the next device object.
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (DeviceData->TopOfStack, Irp);

        IoReleaseRemoveLock(&commonData->RemoveLock,Irp);
        return status;

    case IRP_MN_REMOVE_DEVICE:
        Serenum_KdPrint (DeviceData, SER_DBG_PNP_TRACE, ("Remove Device\n"));

        //
        // The PlugPlay system has detected the removal of this device.  We
        // have no choice but to detach and delete the device object.
        // (If we wanted to express and interest in preventing this removal,
        // we should have filtered the query remove and query stop routines.)
        //
        // Note! we might receive a remove WITHOUT first receiving a stop.
        // ASSERT (!DeviceData->Removed);


        //
        // Synchronize with the enum thread if it is running and wait
        // for it to finish
        //

		
        SerenumWaitForEnumThreadTerminate(DeviceData);

	// block until all outstanding IO has returned
	//
        // Wait for all outstanding requests to complete
        //
        Serenum_KdPrint (DeviceData, SER_DBG_PNP_TRACE,("Waiting for outstanding requests\n"));
        // We will accept no new requests
        //
        DeviceData->Removed = TRUE; 

        //
        // Complete any outstanding IRPs queued by the driver here.
        //

        //
        // Make the DCA go away.  Some drivers may choose to remove the DCA
        // when they receive a stop or even a query stop.  We just don't care.
        //
        (void)IoSetDeviceInterfaceState (&DeviceData->DevClassAssocName, FALSE);

        //
        // Here if we had any outstanding requests in a personal queue we should
        // complete them all now.
        //
        // Note, the device is guarenteed stopped, so we cannot send it any non-
        // PNP IRPS.
        //

        //
        // Fire and forget
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (DeviceData->TopOfStack, Irp);

        IoReleaseRemoveLockAndWait(&commonData->RemoveLock,Irp);
        //
        // Free the associated resources
        //

        //
        // Detach from the underlying devices.
        //
        Serenum_KdPrint(DeviceData, SER_DBG_PNP_INFO,
                        ("IoDetachDevice: 0x%p\n", DeviceData->TopOfStack));
        IoDetachDevice (DeviceData->TopOfStack);

        //
        // Clean up any resources here
        //

        
        ExFreePoolWithTag (DeviceData->DevClassAssocName.Buffer,SERENUM_POOL_TAG);
        // DeviceData->DevClassAssocName was initialized in AddDevice
#pragma warning(suppress: 6001)
        RtlFreeUnicodeString(&DeviceData->DevClassAssocName);
        Serenum_KdPrint(DeviceData, SER_DBG_PNP_INFO,
                        ("IoDeleteDevice: 0x%p\n", DeviceObject));

        //
        // Remove any PDO's we ejected
        //

        if (DeviceData->AttachedPDO != NULL) {
           ASSERT(DeviceData->NumPDOs == 1);

           Serenum_PnPRemove(DeviceData->AttachedPDO, DeviceData->PdoData);
           DeviceData->PdoData = NULL;
           DeviceData->AttachedPDO = NULL;
           DeviceData->NumPDOs = 0;
        }

        IoDeleteDevice(DeviceObject);

        return status;

    case IRP_MN_QUERY_DEVICE_RELATIONS:
        if (BusRelations != IrpStack->Parameters.QueryDeviceRelations.Type) {
            //
            // We don't support this
            //
            Serenum_KdPrint (DeviceData, SER_DBG_PNP_TRACE,
                ("Query Device Relations - Non bus\n"));
            goto SER_FDO_PNP_DEFAULT;
        }

        Serenum_KdPrint (DeviceData, SER_DBG_PNP_TRACE,
            ("Query Bus Relations\n"));

        status = SerenumCheckEnumerations(DeviceData);

	if(!NT_SUCCESS(status))
	{
           Irp->IoStatus.Status = status;
           IoCompleteRequest(Irp, IO_NO_INCREMENT);
           IoReleaseRemoveLock(&commonData->RemoveLock,Irp);
           return status;
	}
        //
        // Tell the plug and play system about all the PDOs.
        //
        // There might also be device relations below and above this FDO,
        // so, be sure to propagate the relations from the upper drivers.
        //
        // No Completion routine is needed so long as the status is preset
        // to success.  (PDOs complete plug and play irps with the current
        // IoStatus.Status and IoStatus.Information as the default.)
        //

        //KeAcquireSpinLock (&DeviceData->Spin, &oldIrq);

        i = (0 == Irp->IoStatus.Information) ? 0 :
            ((PDEVICE_RELATIONS) Irp->IoStatus.Information)->Count;
        // The current number of PDOs in the device relations structure

        length = sizeof(DEVICE_RELATIONS) +
                ((DeviceData->NumPDOs + i) * sizeof (PDEVICE_OBJECT));

        relations = (PDEVICE_RELATIONS) ExAllocatePoolZero(NonPagedPoolNx, length,SERENUM_POOL_TAG);

        if (NULL == relations) {
           Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
           IoCompleteRequest(Irp, IO_NO_INCREMENT);
           IoReleaseRemoveLock(&commonData->RemoveLock,Irp);
           return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Copy in the device objects so far
        //
        if (i) {
            RtlCopyMemory (
                  relations->Objects,
                  ((PDEVICE_RELATIONS) Irp->IoStatus.Information)->Objects,
                  i * sizeof (PDEVICE_OBJECT));
        }
        relations->Count = DeviceData->NumPDOs + i;

        //
        // For each PDO on this bus add a pointer to the device relations
        // buffer, being sure to take out a reference to that object.
        // The PlugPlay system will dereference the object when it is done with
        // it and free the device relations buffer.
        //

        if (DeviceData->NumPDOs) {
            relations->Objects[relations->Count-1] = DeviceData->AttachedPDO;
            ObReferenceObject (DeviceData->AttachedPDO);
        }

        //
        // Set up and pass the IRP further down the stack
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;

        if (0 != Irp->IoStatus.Information) {
            ExFreePoolWithTag ((PVOID) Irp->IoStatus.Information,SERENUM_POOL_TAG);
        }
        Irp->IoStatus.Information = (ULONG_PTR)relations;

        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (DeviceData->TopOfStack, Irp);

        IoReleaseRemoveLock(&commonData->RemoveLock,Irp);

        return status;

    case IRP_MN_QUERY_REMOVE_DEVICE:
        //
        // If we were to fail this call then we would need to complete the
        // IRP here.  Since we are not, set the status to SUCCESS and
        // call the next driver.
        //

        Serenum_KdPrint (DeviceData, SER_DBG_PNP_TRACE,
            ("Query Remove Device\n"));

        //
        // Wait for the enum thread to complete if it's running
        //
        SerenumWaitForEnumThreadTerminate(DeviceData);


        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (DeviceData->TopOfStack, Irp);
        IoReleaseRemoveLock(&commonData->RemoveLock,Irp);
        return status;


    case IRP_MN_QUERY_CAPABILITIES: {

        PIO_STACK_LOCATION  irpSp;

        //
        // Send this down to the PDO first
        //

        KeInitializeEvent (&event, NotificationEvent, FALSE);
        IoCopyCurrentIrpStackLocationToNext (Irp);

        IoSetCompletionRoutine (Irp,
                                SerenumSyncCompletion,
                                &event,
                                TRUE,
                                TRUE,
                                TRUE);

        status = IoCallDriver (DeviceData->TopOfStack, Irp);

        if (STATUS_PENDING == status) {
            // wait for it...

            status = KeWaitForSingleObject (&event,
                                            Executive,
                                            KernelMode,
                                            FALSE, // Not allertable
                                            NULL); // No timeout structure
#if DBG
            ASSERT (STATUS_SUCCESS == status);
#endif
            if(!NT_SUCCESS(status)){
                Irp->IoStatus.Status=status;
		IoReleaseRemoveLock(&commonData->RemoveLock,Irp);
		IoCompleteRequest (Irp, IO_NO_INCREMENT);
		return status;
            }
            status = Irp->IoStatus.Status;
        }

        if (NT_SUCCESS(status)) {

            irpSp = IoGetCurrentIrpStackLocation(Irp);

            DeviceData->SystemWake
                = irpSp->Parameters.DeviceCapabilities.Capabilities->SystemWake;
            DeviceData->DeviceWake
                = irpSp->Parameters.DeviceCapabilities.Capabilities->DeviceWake;
        }

        break;
    }



SER_FDO_PNP_DEFAULT:
    default:
        //
        // In the default case we merely call the next driver since
        // we don't know what to do.
        //
        Serenum_KdPrint (DeviceData, SER_DBG_PNP_TRACE, ("Default Case\n"));

        //
        // Fire and Forget
        //
        IoSkipCurrentIrpStackLocation (Irp);

        //
        // Done, do NOT complete the IRP, it will be processed by the lower
        // device object, which will complete the IRP
        //

        status = IoCallDriver (DeviceData->TopOfStack, Irp);
        IoReleaseRemoveLock(&commonData->RemoveLock,Irp);
        return status;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    IoReleaseRemoveLock(&commonData->RemoveLock,Irp);
    return status;
}

VOID
SerenumMarkPdoRemoved(PFDO_DEVICE_DATA PFdoData)
{
    KIRQL oldIrql;

    KeAcquireSpinLock(&PFdoData->EnumerationLock, &oldIrql);

    PFdoData->EnumFlags |= SERENUM_ENUMFLAG_REMOVED;

    KeReleaseSpinLock(&PFdoData->EnumerationLock, oldIrql);
}


NTSTATUS
Serenum_PDO_PnP (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
                 IN PIO_STACK_LOCATION IrpStack, IN PPDO_DEVICE_DATA DeviceData)
/*++
Routine Description:
    Handle requests from the PlugPlay system for the devices on the BUS

--*/
{
   PDEVICE_CAPABILITIES    deviceCapabilities;
   PWCHAR                  buffer;
   ULONG                   length;
   NTSTATUS                status;
   PWCHAR returnBuffer = NULL;
   PCOMMON_DEVICE_DATA commonData=(PCOMMON_DEVICE_DATA)DeviceObject->DeviceExtension;


   PAGED_CODE();

   status = Irp->IoStatus.Status;

   //
   // NB: since we are a bus enumerator, we have no one to whom we could
   // defer these irps.  Therefore we do not pass them down but merely
   // return them.
   //

   switch (IrpStack->MinorFunction) {
   case IRP_MN_QUERY_CAPABILITIES:

      Serenum_KdPrint (DeviceData, SER_DBG_PNP_TRACE, ("Query Caps \n"));

      //
      // Get the packet.
      //

      deviceCapabilities=IrpStack->Parameters.DeviceCapabilities.Capabilities;

      //
      // Set the capabilities.
      //

      deviceCapabilities->Version = 1;
      deviceCapabilities->Size = sizeof (DEVICE_CAPABILITIES);

      //
      // We cannot wake the system.
      //

      deviceCapabilities->SystemWake
          = ((PFDO_DEVICE_DATA)DeviceData->ParentFdo->DeviceExtension)
            ->SystemWake;
      deviceCapabilities->DeviceWake
          = ((PFDO_DEVICE_DATA)DeviceData->ParentFdo->DeviceExtension)
            ->DeviceWake;

      //
      // We have no latencies
      //

      deviceCapabilities->D1Latency = 0;
      deviceCapabilities->D2Latency = 0;
      deviceCapabilities->D3Latency = 0;

      deviceCapabilities->UniqueID = FALSE;
      status = STATUS_SUCCESS;
      break;

   case IRP_MN_QUERY_DEVICE_TEXT: {
      if ((IrpStack->Parameters.QueryDeviceText.DeviceTextType
          != DeviceTextDescription) || DeviceData->DevDesc.Buffer == NULL) {
         break;
      }

      returnBuffer = ExAllocatePoolZero(PagedPool, DeviceData->DevDesc.Length,SERENUM_POOL_TAG);

      if (returnBuffer == NULL) {
         status = STATUS_INSUFFICIENT_RESOURCES;
         break;
      }

      status = STATUS_SUCCESS;

      RtlCopyMemory(returnBuffer, DeviceData->DevDesc.Buffer,
                    DeviceData->DevDesc.Length);

      Serenum_KdPrint(DeviceData, SER_DBG_PNP_TRACE,
                            ("TextID: buf 0x%p\n", returnBuffer));

      Irp->IoStatus.Information = (ULONG_PTR)returnBuffer;
      break;
   }


   case IRP_MN_QUERY_ID:
      //
      // Query the IDs of the device
      //

      Serenum_KdPrint(DeviceData, SER_DBG_PNP_TRACE,
                      ("QueryID: 0x%x\n", IrpStack->Parameters.QueryId.IdType));

      switch (IrpStack->Parameters.QueryId.IdType) {


      case BusQueryInstanceID:
         //
         // Build an instance ID.  This is what PnP uses to tell if it has
         // seen this thing before or not.  Build it from the first hardware
         // id and the port number.
         //
         // NB since we do not incorperate the port number
         // this method does not produce unique ids;
         //
         // return 0000 for all devices and have the flag set to not unique
         //

         status = STATUS_SUCCESS;

         length = SERENUM_INSTANCE_IDS_LENGTH * sizeof(WCHAR);
         returnBuffer = ExAllocatePoolZero(PagedPool, length,SERENUM_POOL_TAG);

         if (returnBuffer != NULL) {
            RtlCopyMemory(returnBuffer, SERENUM_INSTANCE_IDS, length);
         } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
         }

         Serenum_KdPrint(DeviceData, SER_DBG_PNP_TRACE,
                      ("InstanceID: buf 0x%p\n", returnBuffer));

         Irp->IoStatus.Information = (ULONG_PTR)returnBuffer;
         break;


      //
      // The other ID's we just copy from the buffers and are done.
      //

      case BusQueryDeviceID:
      case BusQueryHardwareIDs:
      case BusQueryCompatibleIDs:
         {
            PUNICODE_STRING pId = NULL;
            status = STATUS_SUCCESS;

            switch (IrpStack->Parameters.QueryId.IdType) {
            case BusQueryDeviceID:
               pId = &DeviceData->DeviceIDs;
               break;

            case BusQueryHardwareIDs:
               pId = &DeviceData->HardwareIDs;
               break;

            case BusQueryCompatibleIDs:
               pId = &DeviceData->CompIDs;
               break;
            }

            buffer = pId ? pId->Buffer : NULL;

            if (buffer != NULL) {
               length = pId->Length;
               returnBuffer = ExAllocatePoolZero(PagedPool, length + sizeof(WCHAR),SERENUM_POOL_TAG);
               if (returnBuffer != NULL) {
                  RtlZeroMemory(returnBuffer, length + sizeof(WCHAR) );
                  RtlCopyMemory(returnBuffer, buffer, length);

               } else {
                  status = STATUS_INSUFFICIENT_RESOURCES;
               }
            }

            Serenum_KdPrint(DeviceData, SER_DBG_PNP_TRACE,
                            ("ID: Unicode 0x%p\n", pId));
            Serenum_KdPrint(DeviceData, SER_DBG_PNP_TRACE,
                            ("ID: buf 0x%p\n", returnBuffer));

            Irp->IoStatus.Information = (ULONG_PTR)returnBuffer;
         }
         break;
      }
      break;

      case IRP_MN_QUERY_BUS_INFORMATION: {
       PPNP_BUS_INFORMATION pBusInfo;

       ASSERTMSG("Serenum appears not to be the sole bus?!?",
                 Irp->IoStatus.Information == (ULONG_PTR)NULL);

       pBusInfo = ExAllocatePoolZero(PagedPool, sizeof(PNP_BUS_INFORMATION),SERENUM_POOL_TAG);

       if (pBusInfo == NULL) {
          status = STATUS_INSUFFICIENT_RESOURCES;
          break;
       }

       pBusInfo->BusTypeGuid = GUID_BUS_TYPE_SERENUM;
       pBusInfo->LegacyBusType = PNPBus;

       //
       // We really can't track our bus number since we can be torn
       // down with our bus
       //

       pBusInfo->BusNumber = 0;

       Irp->IoStatus.Information = (ULONG_PTR)pBusInfo;
       status = STATUS_SUCCESS;
       break;
       }

   case IRP_MN_QUERY_DEVICE_RELATIONS:
      switch (IrpStack->Parameters.QueryDeviceRelations.Type) {
      case TargetDeviceRelation: {
         PDEVICE_RELATIONS pDevRel;

         //
         // No one else should respond to this since we are the PDO
         //

         ASSERT(Irp->IoStatus.Information == 0);

         if (Irp->IoStatus.Information != 0) {
            break;
         }


         pDevRel = ExAllocatePoolZero(PagedPool, sizeof(DEVICE_RELATIONS),SERENUM_POOL_TAG);

         if (pDevRel == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
         }

         pDevRel->Count = 1;
         pDevRel->Objects[0] = DeviceObject;
         ObReferenceObject(DeviceObject);

         status = STATUS_SUCCESS;
         Irp->IoStatus.Information = (ULONG_PTR)pDevRel;
         break;
      }


      default:
         break;
      }

      break;

   case IRP_MN_START_DEVICE:

      //
      // Save serial number and PnPRev
      //

      if(DeviceData->PnPRev.Length || DeviceData->SerialNo.Length) {
         UNICODE_STRING keyname;
         HANDLE pnpKey;

         status = IoOpenDeviceRegistryKey(DeviceObject, PLUGPLAY_REGKEY_DEVICE,
                                          STANDARD_RIGHTS_WRITE, &pnpKey);
         if (!NT_SUCCESS(status)) {
             Serenum_KdPrint(DeviceData, SER_DBG_PNP_TRACE,
                             ("DoOpenDeviceRegistryKey failed (%x)\n", status));
             //
             // We failed on opening a handle to registry key. The failure is
             // not so intense that we should fail the IRP_MN_START_DEVICE, 
             // thus we set the status to STATUS_SUCCESS and continue. 
             //
             status = STATUS_SUCCESS;
             break;
         }

         if(DeviceData->PnPRev.Length) {
            RtlInitUnicodeString(&keyname, NULL);
            keyname.MaximumLength = sizeof(L"PnPRev");
            keyname.Buffer = ExAllocatePoolZero(PagedPool, keyname.MaximumLength,SERENUM_POOL_TAG);

            if (keyname.Buffer != NULL) {

               RtlAppendUnicodeToString(&keyname, L"PnPRev");
               status = ZwSetValueKey(pnpKey, &keyname, 0, REG_SZ, DeviceData->PnPRev.Buffer, DeviceData->PnPRev.Length+sizeof(WCHAR));
	       if(NT_SUCCESS(status)){
                   Serenum_KdPrint(DeviceData, SER_DBG_PNP_TRACE,
                   ("ZwSetValueKey succeeded (%x)\n", status));
               }else{
		   Serenum_KdPrint(DeviceData, SER_DBG_PNP_TRACE,
                   ("ZwSetValueKey failed (%x)\n", status));
	       }
               ExFreePoolWithTag(keyname.Buffer,SERENUM_POOL_TAG);
            }
         }
         if(DeviceData->SerialNo.Length) {
            RtlInitUnicodeString(&keyname, NULL);
            keyname.MaximumLength = sizeof(L"Serial Number");
            keyname.Buffer = ExAllocatePoolZero(PagedPool, keyname.MaximumLength,SERENUM_POOL_TAG);

            if (keyname.Buffer != NULL) {

               RtlAppendUnicodeToString(&keyname, L"Serial Number");
               status = ZwSetValueKey(pnpKey, &keyname, 0, REG_SZ, DeviceData->SerialNo.Buffer, DeviceData->SerialNo.Length+sizeof(WCHAR));
               if(NT_SUCCESS(status)){
                   Serenum_KdPrint(DeviceData, SER_DBG_PNP_TRACE,
                   ("ZwSetValueKey succeeded (%x)\n", status));
	       }else{
		   Serenum_KdPrint(DeviceData, SER_DBG_PNP_TRACE,
                   ("ZwSetValueKey failed (%x)\n", status));
	       }
               ExFreePoolWithTag(keyname.Buffer,SERENUM_POOL_TAG);

            }
         }

         ZwClose(pnpKey);
      }

      DeviceData->Started = TRUE;
      status = STATUS_SUCCESS;
      break;

   case IRP_MN_STOP_DEVICE:
      Serenum_KdPrint(DeviceData, SER_DBG_PNP_TRACE, ("Stop Device\n"));

      //
      // Here we shut down the device.  The opposite of start.
      //

      DeviceData->Started = FALSE;
      status = STATUS_SUCCESS;
      break;

   case IRP_MN_REMOVE_DEVICE:
      Serenum_KdPrint(DeviceData, SER_DBG_PNP_TRACE, ("Remove Device\n"));

      //
      // Mark as removed so we enumerate devices correctly -- this will
      // cause the next enumeration request to occur synchronously
      //

      SerenumMarkPdoRemoved((PFDO_DEVICE_DATA)DeviceData->ParentFdo
                            ->DeviceExtension);

      //
      // Attached is only set to FALSE by the enumeration process.
      //

      if (!DeviceData->Attached) {

          status = Serenum_PnPRemove(DeviceObject, DeviceData);
      }
      else {
          //
          // Succeed the remove
          ///
          status = STATUS_SUCCESS;
      }
      break;

   case IRP_MN_QUERY_STOP_DEVICE:
      Serenum_KdPrint(DeviceData, SER_DBG_PNP_TRACE, ("Query Stop Device\n"));

      //
      // No reason here why we can't stop the device.
      // If there were a reason we should speak now for answering success
      // here may result in a stop device irp.
      //

      status = STATUS_SUCCESS;
      break;

   case IRP_MN_CANCEL_STOP_DEVICE:
      Serenum_KdPrint(DeviceData, SER_DBG_PNP_TRACE, ("Cancel Stop Device\n"));
      //
      // The stop was canceled.  Whatever state we set, or resources we put
      // on hold in anticipation of the forcoming STOP device IRP should be
      // put back to normal.  Someone, in the long list of concerned parties,
      // has failed the stop device query.
      //

      status = STATUS_SUCCESS;
      break;

   case IRP_MN_QUERY_REMOVE_DEVICE:
      Serenum_KdPrint(DeviceData, SER_DBG_PNP_TRACE, ("Query Remove Device\n"));
      //
      // Just like Query Stop only now the impending doom is the remove irp
      //
      status = STATUS_SUCCESS;
      break;

   case IRP_MN_CANCEL_REMOVE_DEVICE:
      Serenum_KdPrint(DeviceData, SER_DBG_PNP_TRACE, ("Cancel Remove Device"
                                                      "\n"));
      //
      // Clean up a remove that did not go through, just like cancel STOP.
      //
      status = STATUS_SUCCESS;
      break;

   case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
   case IRP_MN_READ_CONFIG:
   case IRP_MN_WRITE_CONFIG: // we have no config space
   case IRP_MN_EJECT:
   case IRP_MN_SET_LOCK:
   case IRP_MN_QUERY_INTERFACE: // We do not have any non IRP based interfaces.
   default:
      Serenum_KdPrint(DeviceData, SER_DBG_PNP_TRACE, ("PNP Not handled 0x%x\n",
                                                      IrpStack->MinorFunction));
      // For PnP requests to the PDO that we do not understand we should
      // return the IRP WITHOUT setting the status or information fields.
      // They may have already been set by a filter (eg acpi).
      break;
   }

   Irp->IoStatus.Status = status;
   IoCompleteRequest (Irp, IO_NO_INCREMENT);
   IoReleaseRemoveLock(&commonData->RemoveLock,Irp);
   return status;
}

NTSTATUS
Serenum_PnPRemove (PDEVICE_OBJECT Device, PPDO_DEVICE_DATA PdoData)
/*++
Routine Description:
    The PlugPlay subsystem has instructed that this PDO should be removed.

    We should therefore
    - Complete any requests queued in the driver
    - If the device is still attached to the system,
      then complete the request and return.
    - Otherwise, cleanup device specific allocations, memory, events...
    - Call IoDeleteDevice
    - Return from the dispatch routine.

    Note that if the device is still connected to the bus (IE in this case
    the control panel has not yet told us that the serial device has
    disappeared) then the PDO must remain around, and must be returned during
    any query Device relaions IRPS.

--*/

{
    PAGED_CODE();
    
   Serenum_KdPrint(PdoData, SER_DBG_PNP_TRACE,
                        ("Serenum_PnPRemove: 0x%p\n", Device));

    //
    // Complete any outstanding requests with STATUS_DELETE_PENDING.
    //
    // Serenum does not queue any irps at this time so we have nothing to do.
    //

    if (PdoData->Attached || PdoData->Removed) {
        return STATUS_SUCCESS;
    }

    PdoData->Removed = TRUE;

    //
    // Free any resources.
    //

    RtlFreeUnicodeString(&PdoData->HardwareIDs);
    RtlFreeUnicodeString(&PdoData->CompIDs);
    RtlFreeUnicodeString(&PdoData->DeviceIDs);

    Serenum_KdPrint(PdoData, SER_DBG_PNP_INFO,
                        ("IoDeleteDevice: 0x%p\n", Device));

    IoDeleteDevice(Device);


    return STATUS_SUCCESS;
}

NTSTATUS
SerenumStartDeviceCompletion(
                PDEVICE_OBJECT  DeviceObject,
                PIRP            Irp,
                PVOID           Context
    )
/*++

Routine Description:

    This routine triggers the event associated with the block semantics of the
    IRP_MN_START_DEVICE
    
Arguments:

    DeviceObject    - Targeted device object
    Irp             - IRP_MN_START_DEVICE irp
    Context         - PIO_WORKITEM workItem

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED
            
--*/
{
    PCOMMON_DEVICE_DATA commonData=(PCOMMON_DEVICE_DATA)DeviceObject->DeviceExtension;
    PIO_WORKITEM workItem = (PIO_WORKITEM)Context;
    NTSTATUS status;

    _Analysis_assume_(workItem != NULL); // Not NULL when passed to IoQueueWorkItem()

    status = Irp->IoStatus.Status;

    if (NT_SUCCESS(status)){
        IoQueueWorkItemEx(workItem,
                          SerenumStartDeviceWorker,
                          DelayedWorkQueue,
                          Irp);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        IoFreeWorkItem(workItem);
    }
    IoReleaseRemoveLock( &commonData->RemoveLock,Irp );
    return STATUS_CONTINUE_COMPLETION;
}



VOID SerenumStartDeviceWorker(
    PVOID deviceObject,
    PVOID irp,
    PIO_WORKITEM WorkItem
    )
{
	
    PDEVICE_OBJECT DeviceObject=(PDEVICE_OBJECT)deviceObject;
    PIRP Irp=(PIRP)irp;
    PCOMMON_DEVICE_DATA commonData=(PCOMMON_DEVICE_DATA)DeviceObject->DeviceExtension;
    PFDO_DEVICE_DATA DeviceData=(PFDO_DEVICE_DATA)DeviceObject->DeviceExtension;
    PRTL_QUERY_REGISTRY_TABLE QueryTable = NULL;
    ULONG DebugLevelDefault = SER_DEFAULT_DEBUG_OUTPUT_LEVEL;
    PAGED_CODE();
	
    _Analysis_assume_(Irp != NULL); // Not NULL when passed to IoQueueWorkItem()

    if (NULL == (QueryTable = ExAllocatePoolZero(
                               PagedPool,
                               sizeof(RTL_QUERY_REGISTRY_TABLE)*2,
                               SERENUM_POOL_TAG
                               ))){
        Serenum_KdPrint (DeviceData, SER_DBG_PNP_ERROR,
        ("Failed to allocate memory to query registy\n"));
        DeviceData->DebugLevel = DebugLevelDefault;
        // This Irp is passed as the context parameter 
        // This is the same Irp which is sent by the kernel to the completion routine
	Irp->IoStatus.Status=STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        IoFreeWorkItem(WorkItem);
        IoReleaseRemoveLock( &commonData->RemoveLock,Irp);
        return;
    }else{
        RtlZeroMemory(QueryTable,sizeof(RTL_QUERY_REGISTRY_TABLE)*2);
        QueryTable[0].QueryRoutine = NULL;
        QueryTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;;
        QueryTable[0].EntryContext = &commonData->DebugLevel;
        QueryTable[0].Name      = L"DebugLevel";
        QueryTable[0].DefaultType = (REG_DWORD << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT) | REG_DWORD;
        QueryTable[0].DefaultData   = &DebugLevelDefault;
        QueryTable[0].DefaultLength= sizeof(ULONG);

        // CIMEXCIMEX: The rest of the table isn't filled in!

        if (!NT_SUCCESS(RtlQueryRegistryValues(
                    RTL_REGISTRY_SERVICES,
                    L"Serenum",
                    QueryTable,
                    NULL,
                    NULL))){
                    Serenum_KdPrint (DeviceData,SER_DBG_PNP_ERROR,
                        ("Failed to get debug level from registry.  "
                         "Using default\n"));
                    DeviceData->DebugLevel = DebugLevelDefault;
                }
        ExFreePoolWithTag(QueryTable,SERENUM_POOL_TAG);
                
     }

     Serenum_KdPrint (DeviceData, SER_DBG_PNP_TRACE,
                     ("Start Device: Device started successfully\n"));
     DeviceData->Started = TRUE;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    IoFreeWorkItem(WorkItem);
    IoReleaseRemoveLock( &commonData->RemoveLock,Irp);
    return;
}



#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

#pragma warning(default:4127) // conditional expression is constant



