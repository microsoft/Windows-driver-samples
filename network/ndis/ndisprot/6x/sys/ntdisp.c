/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ntdisp.c

Abstract:

    NT Entry points and dispatch routines for NDISPROT.

Environment:

    Kernel mode only.

Revision History:

--*/

#include "precomp.h"

#define __FILENUMBER 'PSID'


#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, NdisprotUnload)
#pragma alloc_text(PAGE, NdisprotOpen)
#pragma alloc_text(PAGE, NdisprotClose)
#pragma alloc_text(PAGE, NdisprotIoControl)

#endif // ALLOC_PRAGMA


//
//  Globals:
//
NDISPROT_GLOBALS         Globals = {0};

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   pDriverObject,
    IN PUNICODE_STRING  pRegistryPath
    )
/*++

Routine Description:

    Called on loading. We create a device object to handle user-mode requests
    on, and register ourselves as a protocol with NDIS.

Arguments:

    pDriverObject - Pointer to driver object created by system.

    pRegistryPath - Pointer to the Unicode name of the registry path
        for this driver.

Return Value:

    NT Status code
    
--*/
{
    NDIS_PROTOCOL_DRIVER_CHARACTERISTICS   protocolChar = {0};
    NTSTATUS                        status = STATUS_SUCCESS;
    NDIS_STRING                     protoName = NDIS_STRING_CONST("NDISPROT");     
    UNICODE_STRING                  ntDeviceName;
    UNICODE_STRING                  win32DeviceName;
    BOOLEAN                         fSymbolicLink = FALSE;
    PDEVICE_OBJECT                  deviceObject = NULL;
    NDIS_HANDLE  ProtocolDriverContext={0};

    UNREFERENCED_PARAMETER(pRegistryPath);
	
    DEBUGP(DL_LOUD, ("DriverEntry\n"));

    Globals.pDriverObject = pDriverObject;
    Globals.EthType = NPROT_ETH_TYPE;
    NPROT_INIT_EVENT(&Globals.BindsComplete);

    do
    {
        //
        // Create our device object using which an application can
        // access NDIS devices.
        //
        RtlInitUnicodeString(&ntDeviceName, NT_DEVICE_NAME);

        status = IoCreateDevice (pDriverObject,
                                 0,
                                 &ntDeviceName,
                                 FILE_DEVICE_NETWORK,
                                 FILE_DEVICE_SECURE_OPEN,
                                 FALSE,
                                 &deviceObject);
    
        if (!NT_SUCCESS (status))
        {
            //
            // Either not enough memory to create a deviceobject or another
            // deviceobject with the same name exits. This could happen
            // if you install another instance of this device.
            //
            break;
        }

        RtlInitUnicodeString(&win32DeviceName, DOS_DEVICE_NAME);

        status = IoCreateSymbolicLink(&win32DeviceName, &ntDeviceName);

        if (!NT_SUCCESS(status))
        {
            break;
        }

        fSymbolicLink = TRUE;
    
        deviceObject->Flags |= DO_DIRECT_IO;
        Globals.ControlDeviceObject = deviceObject;

        NPROT_INIT_LIST_HEAD(&Globals.OpenList);
        NPROT_INIT_LOCK(&Globals.GlobalLock);

        //
        // Initialize the protocol characterstic structure
        //     
#if (NDIS_SUPPORT_NDIS630)
        {C_ASSERT(sizeof(protocolChar) >= NDIS_SIZEOF_PROTOCOL_DRIVER_CHARACTERISTICS_REVISION_2);}
        protocolChar.Header.Type        = NDIS_OBJECT_TYPE_PROTOCOL_DRIVER_CHARACTERISTICS,
        protocolChar.Header.Size        = NDIS_SIZEOF_PROTOCOL_DRIVER_CHARACTERISTICS_REVISION_2;
        protocolChar.Header.Revision    = NDIS_PROTOCOL_DRIVER_CHARACTERISTICS_REVISION_2;
#elif (NDIS_SUPPORT_NDIS6)
        {C_ASSERT(sizeof(protocolChar) >= NDIS_SIZEOF_PROTOCOL_DRIVER_CHARACTERISTICS_REVISION_1);}
        protocolChar.Header.Type        = NDIS_OBJECT_TYPE_PROTOCOL_DRIVER_CHARACTERISTICS,
        protocolChar.Header.Size        = NDIS_SIZEOF_PROTOCOL_DRIVER_CHARACTERISTICS_REVISION_1;
        protocolChar.Header.Revision    = NDIS_PROTOCOL_DRIVER_CHARACTERISTICS_REVISION_1;
#endif // NDIS MINIPORT VERSION

        protocolChar.MajorNdisVersion            = NDIS_PROT_MAJOR_VERSION;
        protocolChar.MinorNdisVersion            = NDIS_PROT_MINOR_VERSION;
        protocolChar.MajorDriverVersion          = MAJOR_DRIVER_VERSION;
        protocolChar.MinorDriverVersion          = MINOR_DRIVER_VERISON;
        protocolChar.Name                        = protoName;
        protocolChar.SetOptionsHandler           = NULL;
        protocolChar.OpenAdapterCompleteHandlerEx  = NdisprotOpenAdapterComplete;
        protocolChar.CloseAdapterCompleteHandlerEx = NdisprotCloseAdapterComplete;
        protocolChar.SendNetBufferListsCompleteHandler = NdisprotSendComplete;
        protocolChar.OidRequestCompleteHandler   = NdisprotRequestComplete;
        protocolChar.StatusHandlerEx             = NdisprotStatus;
        protocolChar.UninstallHandler            = NULL;
        protocolChar.ReceiveNetBufferListsHandler = NdisprotReceiveNetBufferLists;
        protocolChar.NetPnPEventHandler          = NdisprotPnPEventHandler;
        protocolChar.BindAdapterHandlerEx        = NdisprotBindAdapter;
        protocolChar.UnbindAdapterHandlerEx      = NdisprotUnbindAdapter;

        //
        // Register as a protocol driver
        //
    
        status = NdisRegisterProtocolDriver(ProtocolDriverContext,           // driver context
                                            &protocolChar,
                                            &Globals.NdisProtocolHandle);

        if (status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_WARN, ("Failed to register protocol with NDIS\n"));
            status = STATUS_UNSUCCESSFUL;
            break;
        }

        Globals.PartialCancelId = NdisGeneratePartialCancelId();
        Globals.PartialCancelId <<= ((sizeof(PVOID) - 1) * 8);
        DEBUGP(DL_LOUD, ("DriverEntry: CancelId %lx\n", Globals.PartialCancelId));

        //
        // Now set only the dispatch points we would like to handle.
        //

        pDriverObject->MajorFunction[IRP_MJ_CREATE] = NdisprotOpen;

        pDriverObject->MajorFunction[IRP_MJ_CLOSE]  = NdisprotClose;

        pDriverObject->MajorFunction[IRP_MJ_READ]   = NdisprotRead;

        pDriverObject->MajorFunction[IRP_MJ_WRITE]  = NdisprotWrite;

        pDriverObject->MajorFunction[IRP_MJ_CLEANUP]  = NdisprotCleanup;

        pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = NdisprotIoControl;
        

        pDriverObject->DriverUnload = NdisprotUnload;

        status = STATUS_SUCCESS;

        
    }
    while (FALSE);
       

    if (!NT_SUCCESS(status))
    {
        if (deviceObject)
        {
            KeEnterCriticalRegion();
            IoDeleteDevice(deviceObject);
            KeLeaveCriticalRegion();
            Globals.ControlDeviceObject = NULL;
        }

        if (fSymbolicLink)
        {
            IoDeleteSymbolicLink(&win32DeviceName);
            fSymbolicLink = FALSE;
        }
        
        if (Globals.NdisProtocolHandle)
        {
            NdisDeregisterProtocolDriver(Globals.NdisProtocolHandle);
            Globals.NdisProtocolHandle = NULL;
        }        
    }
    
    return status;
}


VOID
NdisprotUnload(
    IN PDRIVER_OBJECT DriverObject
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

    UNICODE_STRING     win32DeviceName;
    PAGED_CODE();
    UNREFERENCED_PARAMETER(DriverObject);
	
    DEBUGP(DL_LOUD, ("Unload Enter\n"));

    //
    // First delete the Control deviceobject and the corresponding
    // symbolicLink
    //
    RtlInitUnicodeString(&win32DeviceName, DOS_DEVICE_NAME);

    IoDeleteSymbolicLink(&win32DeviceName);           


    if (Globals.ControlDeviceObject)
    {
        IoDeleteDevice(Globals.ControlDeviceObject);
        Globals.ControlDeviceObject = NULL;
    }


    ndisprotDoProtocolUnload();

#if DBG
    ndisprotAuditShutdown();
#endif

    DEBUGP(DL_LOUD, ("Unload Exit\n"));
}



NTSTATUS
NdisprotOpen(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    )
/*++

Routine Description:

    This is the dispatch routine for handling IRP_MJ_CREATE.
    We simply succeed this.

Arguments:

    pDeviceObject - Pointer to the device object.

    pIrp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PIO_STACK_LOCATION      pIrpSp;
    NTSTATUS                NtStatus = STATUS_SUCCESS;
    PAGED_CODE();
    UNREFERENCED_PARAMETER(pDeviceObject);
	
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pIrpSp->FileObject->FsContext = NULL;

    DEBUGP(DL_INFO, ("Open: FileObject %p\n", pIrpSp->FileObject));

    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = NtStatus;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return NtStatus;
}

NTSTATUS
NdisprotClose(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    )
/*++

Routine Description:

    This is the dispatch routine for handling IRP_MJ_CLOSE.
    We simply succeed this.

Arguments:

    pDeviceObject - Pointer to the device object.

    pIrp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    NTSTATUS                NtStatus;
    PIO_STACK_LOCATION      pIrpSp;
    PNDISPROT_OPEN_CONTEXT   pOpenContext;
    PAGED_CODE();
    UNREFERENCED_PARAMETER(pDeviceObject);
	
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pOpenContext = pIrpSp->FileObject->FsContext;

    DEBUGP(DL_INFO, ("Close: FileObject %p\n",
        IoGetCurrentIrpStackLocation(pIrp)->FileObject));

    if (pOpenContext != NULL)
    {
        NPROT_STRUCT_ASSERT(pOpenContext, oc);

        //
        //  Deref the endpoint
        //
        NPROT_DEREF_OPEN(pOpenContext);  // Close
    }

    pIrpSp->FileObject->FsContext = NULL;
    NtStatus = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = NtStatus;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return NtStatus;
}

    

NTSTATUS
NdisprotCleanup(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    )
/*++

Routine Description:

    This is the dispatch routine for handling IRP_MJ_CLEANUP.

Arguments:

    pDeviceObject - Pointer to the device object.

    pIrp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PIO_STACK_LOCATION      pIrpSp;
    NTSTATUS                NtStatus;
    NDIS_STATUS             NdisStatus;
    PNDISPROT_OPEN_CONTEXT   pOpenContext;
    ULONG                   PacketFilter;
    ULONG                   BytesProcessed;

    
    UNREFERENCED_PARAMETER(pDeviceObject);
	
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pOpenContext = pIrpSp->FileObject->FsContext;

    DEBUGP(DL_VERY_LOUD, ("Cleanup: FileObject %p, Open %p\n",
        pIrpSp->FileObject, pOpenContext));

    if (pOpenContext != NULL)
    {
        NPROT_STRUCT_ASSERT(pOpenContext, oc);

        //
        //  Set the packet filter to 0, telling NDIS that we aren't
        //  interested in any more receives.
        //
        PacketFilter = 0;
        NdisStatus = ndisprotValidateOpenAndDoRequest(
                        pOpenContext,
                        NdisRequestSetInformation,
                        OID_GEN_CURRENT_PACKET_FILTER,
                        &PacketFilter,
                        sizeof(PacketFilter),
                        &BytesProcessed,
                        FALSE   // Don't wait for device to be powered on
                        );
    
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_INFO, ("Cleanup: Open %p, set packet filter (%x) failed: %x\n",
                    pOpenContext, PacketFilter, NdisStatus));
            //
            //  Ignore the result. If this failed, we may continue
            //  to get indicated receives, which will be handled
            //  appropriately.
            //
            NdisStatus = NDIS_STATUS_SUCCESS;
        }
        
        //
        //  Mark this endpoint.
        //
        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

        NPROT_SET_FLAGS(pOpenContext->Flags, NPROTO_OPEN_FLAGS, NPROTO_OPEN_IDLE);
        pOpenContext->pFileObject = NULL;

        NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

        //
        //  Cancel any pending reads.
        //
        ndisprotCancelPendingReads(pOpenContext);
        //
        // Clean up the receive packet queue
        //
        ndisprotFlushReceiveQueue(pOpenContext);
    }

    NtStatus = STATUS_SUCCESS;

    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = NtStatus;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    DEBUGP(DL_INFO, ("Cleanup: OpenContext %p\n", pOpenContext));

    return (NtStatus);
}

NTSTATUS
NdisprotIoControl(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    )
/*++

Routine Description:

    This is the dispatch routine for handling device ioctl requests.

Arguments:

    pDeviceObject - Pointer to the device object.

    pIrp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PIO_STACK_LOCATION      pIrpSp;
    ULONG                   FunctionCode;
    NTSTATUS                NtStatus;
    NDIS_STATUS             Status;
    PNDISPROT_OPEN_CONTEXT  pOpenContext;
    ULONG                   BytesReturned;
#if !DBG
    UNREFERENCED_PARAMETER(pDeviceObject);
#endif
    PAGED_CODE();
    DEBUGP(DL_LOUD, ("IoControl: DevObj %p, Irp %p\n", pDeviceObject, pIrp));

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    FunctionCode = pIrpSp->Parameters.DeviceIoControl.IoControlCode;
    pOpenContext = (PNDISPROT_OPEN_CONTEXT)pIrpSp->FileObject->FsContext;
    BytesReturned = 0;

    switch (FunctionCode)
    {
        case IOCTL_NDISPROT_BIND_WAIT:
            //
            //  Block until we have seen a NetEventBindsComplete event,
            //  meaning that we have finished binding to all running
            //  adapters that we are supposed to bind to.
            //
            //  If we don't get this event in 5 seconds, time out.
            //
            NPROT_ASSERT((FunctionCode & 0x3) == METHOD_BUFFERED);
            
            if (NPROT_WAIT_EVENT(&Globals.BindsComplete, 5000))
            {
                NtStatus = STATUS_SUCCESS;
            }
            else
            {
                NtStatus = STATUS_TIMEOUT;
            }
            DEBUGP(DL_INFO, ("IoControl: BindWait returning %x\n", NtStatus));
            break;

        case IOCTL_NDISPROT_QUERY_BINDING:
            
            NPROT_ASSERT((FunctionCode & 0x3) == METHOD_BUFFERED);
            
            Status = ndisprotQueryBinding(
                            pIrp->AssociatedIrp.SystemBuffer,
                            pIrpSp->Parameters.DeviceIoControl.InputBufferLength,
                            pIrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                            &BytesReturned
                            );

            NDIS_STATUS_TO_NT_STATUS(Status, &NtStatus);

            DEBUGP(DL_LOUD, ("IoControl: QueryBinding returning %x\n", NtStatus));

            break;

        case IOCTL_NDISPROT_OPEN_DEVICE:

            NPROT_ASSERT((FunctionCode & 0x3) == METHOD_BUFFERED);
            if (pOpenContext != NULL)
            {
                NPROT_STRUCT_ASSERT(pOpenContext, oc);
                DEBUGP(DL_WARN, ("IoControl: OPEN_DEVICE: FileObj %p already"
                    " associated with open %p\n", pIrpSp->FileObject, pOpenContext));

                NtStatus = STATUS_DEVICE_BUSY;
                break;
            }

            NtStatus = ndisprotOpenDevice(
                            pIrp->AssociatedIrp.SystemBuffer,
                            pIrpSp->Parameters.DeviceIoControl.InputBufferLength,
                            pIrpSp->FileObject,
                            &pOpenContext
                            );

            if (NT_SUCCESS(NtStatus))
            {

                DEBUGP(DL_VERY_LOUD, ("IoControl OPEN_DEVICE: Open %p <-> FileObject %p\n",
                        pOpenContext, pIrpSp->FileObject));

            }
            else
            {
                DEBUGP(DL_INFO, ("NdisprotIoControl: IOCTL_NDISPROT_OPEN_DEVICE, ndisprotOpenDevice failed %lx\n", NtStatus));
            }

            break;

        case IOCTL_NDISPROT_QUERY_OID_VALUE:

            NPROT_ASSERT((FunctionCode & 0x3) == METHOD_BUFFERED);
            if (pOpenContext != NULL)
            {
                Status = ndisprotQueryOidValue(
                            pOpenContext,
                            pIrp->AssociatedIrp.SystemBuffer,
                            pIrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                            &BytesReturned
                            );

                NDIS_STATUS_TO_NT_STATUS(Status, &NtStatus);
            }
            else
            {
                NtStatus = STATUS_DEVICE_NOT_CONNECTED;
            }
            break;

        case IOCTL_NDISPROT_SET_OID_VALUE:

            NPROT_ASSERT((FunctionCode & 0x3) == METHOD_BUFFERED);
            if (pOpenContext != NULL)
            {
                Status = ndisprotSetOidValue(
                            pOpenContext,
                            pIrp->AssociatedIrp.SystemBuffer,
                            pIrpSp->Parameters.DeviceIoControl.InputBufferLength
                            );

                BytesReturned = 0;

                NDIS_STATUS_TO_NT_STATUS(Status, &NtStatus);
            }
            else
            {
                NtStatus = STATUS_DEVICE_NOT_CONNECTED;
            }
            break;
                        
        default:

            NtStatus = STATUS_NOT_SUPPORTED;
            break;
    }

    if (NtStatus != STATUS_PENDING)
    {
        pIrp->IoStatus.Information = BytesReturned;
        pIrp->IoStatus.Status = NtStatus;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    }

    return NtStatus;
}



NTSTATUS
ndisprotOpenDevice(
    _In_reads_bytes_(DeviceNameLength) IN PUCHAR     pDeviceName,
    IN ULONG                                    DeviceNameLength,
    IN PFILE_OBJECT                             pFileObject,
    OUT PNDISPROT_OPEN_CONTEXT *                ppOpenContext
    )
/*++

Routine Description:

    Helper routine called to process IOCTL_NDISPROT_OPEN_DEVICE. Check if
    there is a binding to the specified device, and is not associated with
    a file object already. If so, make an association between the binding
    and this file object.

Arguments:

    pDeviceName - pointer to device name string
    DeviceNameLength - length of above
    pFileObject - pointer to file object being associated with the device binding

Return Value:

    Status is returned.
--*/
{
    PNDISPROT_OPEN_CONTEXT   pOpenContext;
    NTSTATUS                NtStatus;
    ULONG                   PacketFilter;
    NDIS_STATUS             NdisStatus;
    ULONG                   BytesProcessed;
    PNDISPROT_OPEN_CONTEXT   pCurrentOpenContext = NULL;

    pOpenContext = NULL;

    do
    {
        pOpenContext = ndisprotLookupDevice(
                        pDeviceName,
                        DeviceNameLength
                        );

        if (pOpenContext == NULL)
        {
            DEBUGP(DL_WARN, ("ndisprotOpenDevice: couldn't find device\n"));
            NtStatus = STATUS_OBJECT_NAME_NOT_FOUND;
            break;
        }

        //
        //  else ndisprotLookupDevice would have addref'ed the open.
        //
        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

        if (!NPROT_TEST_FLAGS(pOpenContext->Flags, NPROTO_OPEN_FLAGS, NPROTO_OPEN_IDLE))
        {
            NPROT_ASSERT(pOpenContext->pFileObject != NULL);

            DEBUGP(DL_WARN, ("ndisprotOpenDevice: Open %p/%x already associated"
                " with another FileObject %p\n", 
                pOpenContext, pOpenContext->Flags, pOpenContext->pFileObject));
            
            NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

            NPROT_DEREF_OPEN(pOpenContext); // ndisprotOpenDevice failure
            NtStatus = STATUS_DEVICE_BUSY;
            break;
        }
        //
        // This InterlockedXXX function performs an atomic operation: First it compare
        // pFileObject->FsContext with NULL, if they are equal, the function puts  pOpenContext
        // into FsContext, and return NULL. Otherwise, it return pFileObject->FsContext without
        // changing anything.
        // 
            
        if ((pCurrentOpenContext = InterlockedCompareExchangePointer (& (pFileObject->FsContext), pOpenContext, NULL)) != NULL)
        {
            //
            // pFileObject->FsContext already is used by other open
            //
            DEBUGP(DL_WARN, ("ndisprotOpenDevice: FileObject %p already associated"
                " with another Open %p/%x\n", 
                pFileObject, pCurrentOpenContext, pCurrentOpenContext->Flags));  //BUG
            
            NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

            NPROT_DEREF_OPEN(pOpenContext); // ndisprotOpenDevice failure
            NtStatus = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        pOpenContext->pFileObject = pFileObject;

        NPROT_SET_FLAGS(pOpenContext->Flags, NPROTO_OPEN_FLAGS, NPROTO_OPEN_ACTIVE);

        NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

        //
        //  Set the packet filter now.
        //
        PacketFilter = NPROTO_PACKET_FILTER;
        NdisStatus = ndisprotValidateOpenAndDoRequest(
                        pOpenContext,
                        NdisRequestSetInformation,
                        OID_GEN_CURRENT_PACKET_FILTER,
                        &PacketFilter,
                        sizeof(PacketFilter),
                        &BytesProcessed,
                        TRUE    // Do wait for power on
                        );
    
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_WARN, ("openDevice: Open %p: set packet filter (%x) failed: %x\n",
                    pOpenContext, PacketFilter, NdisStatus));

            //
            //  Undo all that we did above.
            //
            NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);
            //
            // Need to set pFileObject->FsContext to NULL again, so others can open a device
            // for this file object later
            //
            pCurrentOpenContext = InterlockedCompareExchangePointer (& (pFileObject->FsContext), NULL, pOpenContext);
            
     
            NPROT_ASSERT(pCurrentOpenContext == pOpenContext);
            
            NPROT_SET_FLAGS(pOpenContext->Flags, NPROTO_OPEN_FLAGS, NPROTO_OPEN_IDLE);
            pOpenContext->pFileObject = NULL;

            NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

            NPROT_DEREF_OPEN(pOpenContext); // ndisprotOpenDevice failure

            NDIS_STATUS_TO_NT_STATUS(NdisStatus, &NtStatus);
            break;
        }

        *ppOpenContext = pOpenContext;
        
        NtStatus = STATUS_SUCCESS;
    }
    while (FALSE);

    return (NtStatus);
}


VOID
ndisprotRefOpen(
    IN PNDISPROT_OPEN_CONTEXT        pOpenContext
    )
/*++

Routine Description:

    Reference the given open context.

    NOTE: Can be called with or without holding the opencontext lock.

Arguments:

    pOpenContext - pointer to open context

Return Value:

    None

--*/
{
    NdisInterlockedIncrement((PLONG)&pOpenContext->RefCount);
}


VOID
ndisprotDerefOpen(
    IN PNDISPROT_OPEN_CONTEXT        pOpenContext
    )
/*++

Routine Description:

    Dereference the given open context. If the ref count goes to zero,
    free it.

    NOTE: called without holding the opencontext lock

Arguments:

    pOpenContext - pointer to open context

Return Value:

    None

--*/
{
    if (NdisInterlockedDecrement((PLONG)&pOpenContext->RefCount) == 0)
    {
        DEBUGP(DL_INFO, ("DerefOpen: Open %p, Flags %x, ref count is zero!\n",
            pOpenContext, pOpenContext->Flags));
        
        NPROT_ASSERT(pOpenContext->BindingHandle == NULL);
        NPROT_ASSERT(pOpenContext->RefCount == 0);
        NPROT_ASSERT(pOpenContext->pFileObject == NULL);

        pOpenContext->oc_sig++;

        //
        //  Free it.
        //
        NPROT_FREE_MEM(pOpenContext);
    }
}

#if DBG
VOID
ndisprotDbgRefOpen(
    IN PNDISPROT_OPEN_CONTEXT        pOpenContext,
    IN ULONG                        FileNumber,
    IN ULONG                        LineNumber
    )
{
    DEBUGP(DL_VERY_LOUD, ("  RefOpen: Open %p, old ref %d, File %c%c%c%c, line %d\n",
            pOpenContext,
            pOpenContext->RefCount,
            (CHAR)(FileNumber),
            (CHAR)(FileNumber >> 8),
            (CHAR)(FileNumber >> 16),
            (CHAR)(FileNumber >> 24),
            LineNumber));

    ndisprotRefOpen(pOpenContext);
}

VOID
ndisprotDbgDerefOpen(
    IN PNDISPROT_OPEN_CONTEXT        pOpenContext,
    IN ULONG                        FileNumber,
    IN ULONG                        LineNumber
    )
{
    DEBUGP(DL_VERY_LOUD, ("DerefOpen: Open %p, old ref %d, File %c%c%c%c, line %d\n",
            pOpenContext,
            pOpenContext->RefCount,
            (CHAR)(FileNumber),
            (CHAR)(FileNumber >> 8),
            (CHAR)(FileNumber >> 16),
            (CHAR)(FileNumber >> 24),
            LineNumber));

    ndisprotDerefOpen(pOpenContext);
}

#endif // DBG

