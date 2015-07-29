/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ntdisp.c

Abstract:

    NT Entry points and dispatch routines for NDISPROT.

Environment:

    Kernel mode only.

--*/

#include "precomp.h"

#define __FILENUMBER 'PSID'


#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)

#endif // ALLOC_PRAGMA


//
//  Globals:
//
NDISPROT_GLOBALS         Globals = {0};

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
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
    NDIS_PROTOCOL_DRIVER_CHARACTERISTICS   protocolChar;
    NTSTATUS                        status = STATUS_SUCCESS;
    NDIS_STRING                     protoName = NDIS_STRING_CONST("NDISPROT");
    WDF_DRIVER_CONFIG               config;
    WDFDRIVER                       hDriver;
    PWDFDEVICE_INIT                 pInit = NULL;

    UNREFERENCED_PARAMETER(RegistryPath);

    DEBUGP(DL_LOUD, ("DriverEntry\n"));

    Globals.DriverObject = DriverObject;
    Globals.EthType = NPROT_ETH_TYPE;
    NPROT_INIT_EVENT(&Globals.BindsComplete);

    WDF_DRIVER_CONFIG_INIT(
        &config,
        WDF_NO_EVENT_CALLBACK // This is a non-pnp driver.
    );

    //
    // Tell the framework that this is non-pnp driver so that it doesn't
    // set the default AddDevice routine.
    //
    config.DriverInitFlags |= WdfDriverInitNonPnpDriver;


    //
    // We need an unload routine to free control device created below. For
    // non-pnp drivers, framework doesn't provide Unload routine.
    //
    config.EvtDriverUnload = NdisProtEvtDriverUnload;

    //
    // Create a framework driver object to represent our driver.
    //
    status = WdfDriverCreate(DriverObject,
                            RegistryPath,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &config,
                            &hDriver);
    if (!NT_SUCCESS(status)) {
        DEBUGP(DL_ERROR, ("WdfDriverCreate failed with status 0x%x\n", status));
        return status;
    }

    //
    //
    // In order to create a control device, we first need to allocate a
    // WDFDEVICE_INIT structure and set all properties.
    //
    pInit = WdfControlDeviceInitAllocate(
                            hDriver,
                            &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R
                            );

    if (pInit == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }

    //
    // Call NdisProtDeviceAdd to create WDFDEVICE to represent our
    // software device.
    //
    status = NdisProtCreateControlDevice(hDriver, pInit);
    if (!NT_SUCCESS(status)) {
        DEBUGP (DL_ERROR, ("NdisProtCreateControlDevice failed with status 0x%x\n", status));
        return status;
    }

    //
    // Initialize the protocol characterstic structure
    //

    NdisZeroMemory(&protocolChar,sizeof(NDIS_PROTOCOL_DRIVER_CHARACTERISTICS));


    protocolChar.Header.Type                = NDIS_OBJECT_TYPE_PROTOCOL_DRIVER_CHARACTERISTICS,
    protocolChar.Header.Size                = sizeof(NDIS_PROTOCOL_DRIVER_CHARACTERISTICS);
    protocolChar.Header.Revision            = NDIS_PROTOCOL_DRIVER_CHARACTERISTICS_REVISION_1;

    protocolChar.MajorNdisVersion            = 6;
    protocolChar.MinorNdisVersion            = 0;
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

    status = NdisRegisterProtocolDriver(NULL,           // driver context
                                        &protocolChar,
                                        &Globals.NdisProtocolHandle);

    if (status != NDIS_STATUS_SUCCESS)
    {
        DEBUGP(DL_WARN, ("Failed to register protocol with NDIS\n"));
        return STATUS_UNSUCCESSFUL;
    }

    NPROT_INIT_LIST_HEAD(&Globals.OpenList);
    NPROT_INIT_LOCK(&Globals.GlobalLock);

    Globals.PartialCancelId = NdisGeneratePartialCancelId();
    Globals.PartialCancelId <<= ((sizeof(PVOID) - 1) * 8);
    DEBUGP(DL_LOUD, ("DriverEntry: CancelId %lx\n", Globals.PartialCancelId));

    return status;

}

NTSTATUS
NdisProtCreateControlDevice(
    IN WDFDRIVER Driver,
    IN PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:

    Called by the DriverEntry to create a control-device. This call is
    responsible for freeing the memory for DeviceInit.

Arguments:

    Driver  - a pointer to the framework object that represents this device
    driver.

    DeviceInit - Pointer to a driver-allocated WDFDEVICE_INIT structure.

Return Value:

    STATUS_SUCCESS if initialized; an error otherwise.

--*/
{
    NTSTATUS                       status;
    WDF_OBJECT_ATTRIBUTES           objectAttribs;
    WDF_IO_QUEUE_CONFIG         ioQueueConfig;
    WDF_FILEOBJECT_CONFIG       fileConfig;
    WDFDEVICE                       controlDevice = NULL;
    WDFQUEUE                        queue;
    DECLARE_CONST_UNICODE_STRING(ntDeviceName, NT_DEVICE_NAME) ;
    DECLARE_CONST_UNICODE_STRING(dosDeviceName, DOS_DEVICE_NAME) ;

    UNREFERENCED_PARAMETER( Driver );

    DEBUGP(DL_LOUD, ("NdisProtCreateControlDevice DeviceInit %p\n", DeviceInit));

    //
    // I/O type is Buffered by default. We want to do direct I/O for Reads
    // and Writes so set it explicitly.
    //
    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);

    status = WdfDeviceInitAssignName(DeviceInit, &ntDeviceName);

    if (!NT_SUCCESS(status)) {
        goto Error;
    }

    //
    // Initialize WDF_FILEOBJECT_CONFIG_INIT struct to tell the
    // framework whether you are interested in handle Create, Close and
    // Cleanup requests that gets genereated when an application or another
    // kernel component opens an handle to the device. If you don't register,
    // the framework default behaviour would be complete these requests
    // with STATUS_SUCCESS. A driver might be interested in registering these
    // events if it wants to do security validation and also wants to maintain
    // per handle (fileobject) context.
    //

    WDF_FILEOBJECT_CONFIG_INIT(
        &fileConfig,
        NdisProtEvtDeviceFileCreate,
        NdisProtEvtFileClose,
        NdisProtEvtFileCleanup
        );

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&objectAttribs,
                                    FILE_OBJECT_CONTEXT);

    WdfDeviceInitSetFileObjectConfig(DeviceInit,
                                       &fileConfig,
                                       &objectAttribs);


    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttribs);

    status = WdfDeviceCreate(&DeviceInit,
                             &objectAttribs,
                             &controlDevice);
    if (!NT_SUCCESS(status)) {
        goto Error;
    }

    //
    // DeviceInit is set to NULL if the device is created successfully.
    //

    //
    // Create a symbolic link for the control object so that usermode can open
    // the device.
    //
    status = WdfDeviceCreateSymbolicLink(controlDevice, &dosDeviceName);

    if (!NT_SUCCESS(status)) {
        goto Error;
    }

    //
    // Configure a default queue associated with the control device to
    // to receive read, write, and ioctl requests in parallel.
    // A default queue gets all the requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching.
    //

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig,
                                    WdfIoQueueDispatchParallel);

    ioQueueConfig.EvtIoWrite = NdisProtEvtIoWrite;
    ioQueueConfig.EvtIoRead = NdisProtEvtIoRead;
    ioQueueConfig.EvtIoDeviceControl = NdisProtEvtIoDeviceControl;

    status = WdfIoQueueCreate(controlDevice,
                                        &ioQueueConfig,
                                        WDF_NO_OBJECT_ATTRIBUTES,
                                        &queue // pointer to default queue
                                        );
    if (!NT_SUCCESS(status)) {
        goto Error;
    }

    //
    // Control devices must notify WDF when they are done initializing.   I/O is
    // rejected until this call is made.
    //
    WdfControlFinishInitializing(controlDevice);

    //
    // Create our device object using which an application can
    // access NDIS devices.
    //

    Globals.ControlDevice = controlDevice;

    return status;

Error:

    if(DeviceInit != NULL) {
        //
        // Free the WDFDEVICE_INIT structure only if the device
        // creation fails. Otherwise framework frees the memory
        // itself.
        //
        WdfDeviceInitFree(DeviceInit);
    }

    return status;

}

VOID
NdisProtEvtDriverUnload(
    IN WDFDRIVER Driver
    )
/*++

Routine Description:

    Free all the allocated resources, etc.

Arguments:

    Driver - pointer to a framework driver object.

Return Value:

    VOID.

--*/
{
    UNREFERENCED_PARAMETER(Driver);

    DEBUGP(DL_LOUD, ("Unload Enter\n"));

    ndisprotUnregisterExCallBack();

    ndisprotDoProtocolUnload();

#if DBG
    ndisprotAuditShutdown();
#endif

    //
    // All the other resources such as control-device, symbolic link,
    // queue associated with the device will be automatically
    // deleted by the framework.
    //
    Globals.ControlDevice = NULL;

    DEBUGP(DL_LOUD, ("Unload Exit\n"));
}


VOID
NdisProtEvtDeviceFileCreate(
    IN WDFDEVICE            Device,
    IN WDFREQUEST           Request,
    IN WDFFILEOBJECT        FileObject
    )
/*++

Routine Description:

    The framework calls a driver's EvtDeviceFileCreate callback
    when the framework receives an IRP_MJ_CREATE request.
    The system sends this request when a user application opens the
    device to perform an I/O operation, such as reading or writing a file.
    This callback is called synchronously, in the context of the thread
    that created the IRP_MJ_CREATE request.

Arguments:

    Device - Handle to a framework device object.
    FileObject - Pointer to fileobject that represents the open handle.
    CreateParams - Parameters of IO_STACK_LOCATION for create

Return Value:

   NT status code

--*/
{
    NTSTATUS             NtStatus = STATUS_SUCCESS;
    PFILE_OBJECT_CONTEXT fileContext;

    UNREFERENCED_PARAMETER(Device);

    DEBUGP(DL_INFO, ("Open: FileObject %p\n", FileObject));

    fileContext = GetFileObjectContext(FileObject);

    fileContext->OpenContext = NULL;

    WdfRequestComplete(Request, NtStatus);

    return;
}

VOID
NdisProtEvtFileClose(
    IN WDFFILEOBJECT    FileObject
    )
/*++

Routine Description:

   EvtFileClose is called when all the handles represented by the FileObject
   is closed and all the references to FileObject is removed. This callback
   may get called in an arbitrary thread context instead of the thread that
   called CloseHandle. If you want to delete any per FileObject context that
   must be done in the context of the user thread that made the Create call,
   you should do that in the EvtDeviceCleanp callback.

Arguments:

    FileObject - Pointer to fileobject that represents the open handle.

Return Value:

   VOID

--*/
{
    PNDISPROT_OPEN_CONTEXT   pOpenContext;
    PFILE_OBJECT_CONTEXT fileContext;

    fileContext = GetFileObjectContext(FileObject);

    pOpenContext = fileContext->OpenContext;

    DEBUGP(DL_INFO, ("Close: FileObject %p\n", FileObject));

    if (pOpenContext != NULL)
    {
        NPROT_STRUCT_ASSERT(pOpenContext, oc);

        //
        //  Deref the endpoint
        //
        NPROT_DEREF_OPEN(pOpenContext);  // Close
    }

    fileContext->OpenContext = NULL;

    return;
}

VOID
NdisProtEvtFileCleanup(
    IN WDFFILEOBJECT    FileObject
    )
/*++

Routine Description:

   EvtFileCleanup is called when the handle represented by the FileObject
   is closed. This callback is invoked in the context of the thread that
   closed the handle.

Arguments:

    FileObject - Pointer to fileobject that represents the open handle.

Return Value:

   VOID

--*/
{
    NTSTATUS                NtStatus;
    NDIS_STATUS             NdisStatus;
    PNDISPROT_OPEN_CONTEXT  pOpenContext;
    ULONG                   PacketFilter;
    ULONG                   BytesProcessed;
    PFILE_OBJECT_CONTEXT    fileContext;

    fileContext = GetFileObjectContext(FileObject);

    pOpenContext = fileContext->OpenContext;

    DEBUGP(DL_VERY_LOUD, ("Cleanup: FileObject %p, Open %p\n",
                                FileObject, pOpenContext));

    if (pOpenContext != NULL)
    {
        NPROT_STRUCT_ASSERT(pOpenContext, oc);

        //
        //  Mark this endpoint.
        //
        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

        NPROT_SET_FLAGS(pOpenContext->Flags, NPROTO_OPEN_FLAGS, NPROTO_OPEN_IDLE);
        pOpenContext->pFileObject = NULL;

        NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

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


        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

        if (NPROT_TEST_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_ACTIVE)){

            NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

            //
            //  Cancel any pending reads.
            //
            WdfIoQueuePurgeSynchronously(pOpenContext->ReadQueue);
            //
            // Cancel pending control request for status indication.
            //
            WdfIoQueuePurgeSynchronously(pOpenContext->StatusIndicationQueue);
        } else {

            NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);
        }

        //
        // Clean up the receive packet queue
        //
        ndisprotFlushReceiveQueue(pOpenContext);
    }

    NtStatus = STATUS_SUCCESS;

    DEBUGP(DL_INFO, ("Cleanup: OpenContext %p\n", pOpenContext));

    return;
}

VOID
NdisProtEvtIoDeviceControl(
    IN WDFQUEUE     Queue,
    IN WDFREQUEST   Request,
    IN size_t       OutputBufferLength,
    IN size_t       InputBufferLength,
    IN ULONG        IoControlCode
)
/*++

Routine Description:

    This event is called when the framework receives IRP_MJ_DEVICE_CONTROL
    requests from the system.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.
    Request - Handle to a framework request object.

    OutputBufferLength - length of the request's output buffer,
                        if an output buffer is available.
    InputBufferLength - length of the request's input buffer,
                        if an input buffer is available.

    IoControlCode - the driver-defined or system-defined I/O control code
                    (IOCTL) that is associated with the request.
Return Value:

    VOID

--*/
{
    NTSTATUS                NtStatus;
    NDIS_STATUS             Status;
    PNDISPROT_OPEN_CONTEXT  pOpenContext;
    ULONG                   BytesReturned;
    PVOID                   sysBuffer;
    WDFFILEOBJECT           fileObject = WdfRequestGetFileObject(Request);
    size_t                  bufSize;

    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    DEBUGP(DL_LOUD, ("IoControl: Irp %p\n", Request));

    pOpenContext = GetFileObjectContext(fileObject)->OpenContext;

    BytesReturned = 0;


    switch (IoControlCode)
    {
        case IOCTL_NDISPROT_BIND_WAIT:
            //
            //  Block until we have seen a NetEventBindsComplete event,
            //  meaning that we have finished binding to all running
            //  adapters that we are supposed to bind to.
            //
            //  If we don't get this event in 5 seconds, time out.
            //
            NPROT_ASSERT((IoControlCode & 0x3) == METHOD_BUFFERED);

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

            NPROT_ASSERT((IoControlCode & 0x3) == METHOD_BUFFERED);

            NtStatus = WdfRequestRetrieveOutputBuffer(Request,
                                        sizeof(NDISPROT_QUERY_BINDING),
                                        &sysBuffer,
                                        &bufSize);
            if( !NT_SUCCESS(NtStatus) ) {
                DEBUGP(DL_FATAL, ("WdfRequestRetrieveOutputBuffer failed %x\n", NtStatus));
                break;
            }

            Status = ndisprotQueryBinding(
                            sysBuffer,
                            (ULONG) bufSize,
                            (ULONG) bufSize,
                            &BytesReturned
                            );

            NDIS_STATUS_TO_NT_STATUS(Status, &NtStatus);

            DEBUGP(DL_LOUD, ("IoControl: QueryBinding returning %x\n", NtStatus));

            break;

        case IOCTL_NDISPROT_OPEN_DEVICE:

            NPROT_ASSERT((IoControlCode & 0x3) == METHOD_BUFFERED);
            if (pOpenContext != NULL)
            {
                NPROT_STRUCT_ASSERT(pOpenContext, oc);
                DEBUGP(DL_WARN, ("IoControl: OPEN_DEVICE: FileObj %p already"
                    " associated with open %p\n", fileObject, pOpenContext));

                NtStatus = STATUS_DEVICE_BUSY;
                break;
            }

            NtStatus = WdfRequestRetrieveInputBuffer(Request,
                                        0,
                                        &sysBuffer,
                                        &bufSize);
            if( !NT_SUCCESS(NtStatus) ) {
                DEBUGP(DL_FATAL, ("WdfRequestRetrieveInputBuffer failed %x\n", NtStatus));
                break;
            }

            NtStatus = ndisprotOpenDevice(
                            sysBuffer,
                            (ULONG)bufSize,
                            fileObject,
                            &pOpenContext
                            );

            if (NT_SUCCESS(NtStatus))
            {

                DEBUGP(DL_VERY_LOUD, ("IoControl OPEN_DEVICE: Open %p <-> FileObject %p\n",
                        pOpenContext,  fileObject));
            }

            break;

        case IOCTL_NDISPROT_QUERY_OID_VALUE:

            NPROT_ASSERT((IoControlCode & 0x3) == METHOD_BUFFERED);

            NtStatus = WdfRequestRetrieveOutputBuffer(Request,
                                        sizeof(NDISPROT_QUERY_OID),
                                        &sysBuffer,
                                        &bufSize);
            if( !NT_SUCCESS(NtStatus) ) {
                DEBUGP(DL_FATAL, ("WdfRequestRetrieveOutputBuffer failed %x\n", NtStatus));
                break;
            }

            if (pOpenContext != NULL)
            {
                Status = ndisprotQueryOidValue(
                            pOpenContext,
                            sysBuffer,
                            (ULONG)bufSize,
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

            NPROT_ASSERT((IoControlCode & 0x3) == METHOD_BUFFERED);

            NtStatus = WdfRequestRetrieveInputBuffer(Request,
                                        sizeof(NDISPROT_SET_OID),
                                        &sysBuffer,
                                        &bufSize);
            if( !NT_SUCCESS(NtStatus) ) {
                DEBUGP(DL_FATAL, ("WdfRequestRetrieveInputBuffer failed %x\n", NtStatus));
                break;
            }


            if (pOpenContext != NULL)
            {
                Status = ndisprotSetOidValue(
                            pOpenContext,
                            sysBuffer,
                            (ULONG)bufSize
                            );

                BytesReturned = 0;

                NDIS_STATUS_TO_NT_STATUS(Status, &NtStatus);
            }
            else
            {
                NtStatus = STATUS_DEVICE_NOT_CONNECTED;
            }
            break;

        case IOCTL_NDISPROT_INDICATE_STATUS:

            NPROT_ASSERT((IoControlCode & 0x3) == METHOD_BUFFERED);

            if (pOpenContext != NULL)
            {
                NtStatus = WdfRequestForwardToIoQueue(Request,
                                            pOpenContext->StatusIndicationQueue
                                            );
                if(NT_SUCCESS(NtStatus)) {
                    NtStatus = STATUS_PENDING;
                }
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
        WdfRequestCompleteWithInformation(Request, NtStatus, BytesReturned);
    }

    return;
}


NTSTATUS
ndisprotOpenDevice(
    _In_reads_bytes_(DeviceNameLength) IN PUCHAR     pDeviceName,
    IN ULONG                                    DeviceNameLength,
    IN WDFFILEOBJECT                            FileObject,
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
    PFILE_OBJECT_CONTEXT fileContext;

    pOpenContext = NULL;
    fileContext = GetFileObjectContext(FileObject);

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

        if ((pCurrentOpenContext = InterlockedCompareExchangePointer (& (fileContext->OpenContext), pOpenContext, NULL)) != NULL)
        {
            //
            // pFileObject->FsContext already is used by other open
            //
            DEBUGP(DL_WARN, ("ndisprotOpenDevice: FileObject %p already associated"
                " with another Open %p/%x\n",
                FileObject, pCurrentOpenContext, pCurrentOpenContext->Flags));  //BUG

            NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

            NPROT_DEREF_OPEN(pOpenContext); // ndisprotOpenDevice failure
            NtStatus = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        pOpenContext->pFileObject = FileObject;

        //
        // Start the queue because it may be in a purged state if some body
        // has already opened the device earlier.
        //
        WdfIoQueueStart(pOpenContext->ReadQueue);
        WdfIoQueueStart(pOpenContext->StatusIndicationQueue);

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
            pCurrentOpenContext = InterlockedCompareExchangePointer (& (fileContext->OpenContext), NULL, pOpenContext);


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


