/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ndisbind.c

Abstract:

    NDIS protocol entry points and utility routines to handle binding
    and unbinding from adapters.

Environment:

    Kernel mode only.

--*/


#include "precomp.h"

#define __FILENUMBER 'DNIB'

NDIS_OID    ndisprotSupportedSetOids[] =
{
    OID_802_11_INFRASTRUCTURE_MODE,
    OID_802_11_AUTHENTICATION_MODE,
    OID_802_11_RELOAD_DEFAULTS,
    OID_802_11_REMOVE_WEP,
    OID_802_11_WEP_STATUS,
    OID_802_11_BSSID_LIST_SCAN,
    OID_802_11_ADD_WEP,
    OID_802_11_SSID,
    OID_802_11_BSSID,
    OID_802_11_BSSID_LIST,
    OID_802_11_DISASSOCIATE,
    OID_802_11_STATISTICS,            // Later used by power management
    OID_802_11_POWER_MODE,            // Later  used by power management
    OID_802_11_NETWORK_TYPE_IN_USE,
    OID_802_11_RSSI,
    OID_802_11_SUPPORTED_RATES,
    OID_802_11_CONFIGURATION,
    OID_802_3_MULTICAST_LIST,
};

NDIS_STATUS
NdisprotBindAdapter(
    IN NDIS_HANDLE                  ProtocolDriverContext,
    IN NDIS_HANDLE                  BindContext,
    IN PNDIS_BIND_PARAMETERS        BindParameters
    )
/*++

Routine Description:

    Protocol Bind Handler entry point called when NDIS wants us
    to bind to an adapter. We go ahead and set up a binding.
    An OPEN_CONTEXT structure is allocated to keep state about
    this binding.

Arguments:


Return Value:

    None

--*/
{
    PNDISPROT_OPEN_CONTEXT          pOpenContext;
    NDIS_STATUS                     Status;
    WDF_IO_QUEUE_CONFIG             queueConfig;
    NTSTATUS                        ntStatus;

    UNREFERENCED_PARAMETER(ProtocolDriverContext);

    do
    {
        //
        //  Allocate our context for this open.
        //
        NPROT_ALLOC_MEM(pOpenContext, sizeof(NDISPROT_OPEN_CONTEXT));
        if (pOpenContext == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        //  Initialize it.
        //
        NPROT_ZERO_MEM(pOpenContext, sizeof(NDISPROT_OPEN_CONTEXT));
        NPROT_SET_SIGNATURE(pOpenContext, oc);

        NPROT_INIT_LOCK(&pOpenContext->Lock);
        //
        // Manual queue for pending IRP_MJ_READ requests.  We will
        // manually remove the requests from the queue and service it in our
        // ProtocolRecv indication handler.
        //

        WDF_IO_QUEUE_CONFIG_INIT(
           &queueConfig,
           WdfIoQueueDispatchManual
           );

        ntStatus = WdfIoQueueCreate (
                      Globals.ControlDevice,
                      &queueConfig,
                      WDF_NO_OBJECT_ATTRIBUTES,
                      &pOpenContext->ReadQueue
                      );

        if(!NT_SUCCESS (ntStatus)){
           Status = NDIS_STATUS_FAILURE;
           DEBUGP(DL_ERROR, ("WdfIoQueueCreate for read Queue failed 0x%x\n", ntStatus));
           break;
        }

        //
        // Register a notification so that we get notified whenever a new
        // request shows up when the queue is idle.
        //
        ntStatus = WdfIoQueueReadyNotify(pOpenContext->ReadQueue,
                                                ndisprotEvtNotifyReadQueue,
                                                pOpenContext);
        if(!NT_SUCCESS (ntStatus)){
           Status = NDIS_STATUS_FAILURE;
           DEBUGP(DL_ERROR, ("WdfIoQueueReadyNotify for read queue failed 0x%x\n", ntStatus));
           break;
        }

        //
        // Create another manual queue to hold pending status-indication ioctl requests.
        //
        WDF_IO_QUEUE_CONFIG_INIT(
           &queueConfig,
           WdfIoQueueDispatchManual
           );

        ntStatus = WdfIoQueueCreate (
                      Globals.ControlDevice,
                      &queueConfig,
                      WDF_NO_OBJECT_ATTRIBUTES,
                      &pOpenContext->StatusIndicationQueue
                      );

        if(!NT_SUCCESS (ntStatus)){
           Status = NDIS_STATUS_FAILURE;
           DEBUGP(DL_ERROR, ("WdfIoQueueCreate for ioctl queue failed 0x%x\n", ntStatus));
           break;
        }

        NPROT_INIT_LIST_HEAD(&pOpenContext->RecvNetBufListQueue);
        NPROT_INIT_EVENT(&pOpenContext->PoweredUpEvent);


        //
        //  Start off by assuming that the device below is powered up.
        //
        NPROT_SIGNAL_EVENT(&pOpenContext->PoweredUpEvent);

        NPROT_REF_OPEN(pOpenContext); // Bind

        //
        //  Add it to the global list.
        //
        NPROT_ACQUIRE_LOCK(&Globals.GlobalLock, FALSE);

        NPROT_INSERT_TAIL_LIST(&Globals.OpenList,
                             &pOpenContext->Link);

        NPROT_RELEASE_LOCK(&Globals.GlobalLock, FALSE);

        pOpenContext->State = NdisprotInitializing;

        //
        // Here we reference the open context to make sure that even if
        // ndisprotCreateBinding failed, open context is still valid
        //
        NPROT_REF_OPEN(pOpenContext);
        //
        //  Set up the NDIS binding, ndisprotCreateBinding does the cleanup for the
        //  binding if somehow it fails to create the binding, the
        //
        Status = ndisprotCreateBinding(
                     pOpenContext,
                     BindParameters,
                     BindContext,
                     (PUCHAR)BindParameters->AdapterName->Buffer,
                     BindParameters->AdapterName->Length);


        if (Status != NDIS_STATUS_SUCCESS)
        {
            //
            // Dereference the open context because we referenced it before we call
            // ndisprotCreateBinding
            //
            NPROT_DEREF_OPEN(pOpenContext);
            break;
        }
        //
        // Dereference the open context because we referenced it before we call
        // ndisprotCreateBinding
        //
        NPROT_DEREF_OPEN(pOpenContext);
    }
    while (FALSE);

    return Status;

}

VOID
NdisprotOpenAdapterComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN NDIS_STATUS                  Status
    )
/*++

Routine Description:

    Completion routine called by NDIS if our call to NdisOpenAdapterEx
    pends. Wake up the thread that called NdisOpenAdapterEx.

Arguments:

    ProtocolBindingContext - pointer to open context structure
    Status - status of the open

Return Value:

    None

--*/
{
    PNDISPROT_OPEN_CONTEXT           pOpenContext;

    pOpenContext = (PNDISPROT_OPEN_CONTEXT)ProtocolBindingContext;
    NPROT_STRUCT_ASSERT(pOpenContext, oc);

    pOpenContext->BindStatus = Status;

    NPROT_SIGNAL_EVENT(&pOpenContext->BindEvent);
}


NDIS_STATUS
NdisprotUnbindAdapter(
    IN NDIS_HANDLE                  UnbindContext,
    IN NDIS_HANDLE                  ProtocolBindingContext
    )
/*++

Routine Description:

    NDIS calls this when it wants us to close the binding to an adapter.

Arguments:

    ProtocolBindingContext - pointer to open context structure
    UnbindContext - to use in NdisCompleteUnbindAdapter if we return pending

Return Value:

    pending or success

--*/
{
    PNDISPROT_OPEN_CONTEXT           pOpenContext;

    UNREFERENCED_PARAMETER(UnbindContext);

    pOpenContext = (PNDISPROT_OPEN_CONTEXT)ProtocolBindingContext;
    NPROT_STRUCT_ASSERT(pOpenContext, oc);

    //
    //  Mark this open as having seen an Unbind.
    //
    NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

    NPROT_SET_FLAGS(pOpenContext->Flags, NPROTO_UNBIND_FLAGS, NPROTO_UNBIND_RECEIVED);

    //
    //  In case we had threads blocked for the device below to be powered
    //  up, wake them up.
    //
    NPROT_SIGNAL_EVENT(&pOpenContext->PoweredUpEvent);

    NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

    pOpenContext->State = NdisprotClosing;

    ndisprotShutdownBinding(pOpenContext);

    return NDIS_STATUS_SUCCESS;
}



VOID
NdisprotCloseAdapterComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext
    )
/*++

Routine Description:

    Called by NDIS to complete a pended call to NdisCloseAdapter.
    We wake up the thread waiting for this completion.

Arguments:

    ProtocolBindingContext - pointer to open context structure

Return Value:

    None

--*/
{
    PNDISPROT_OPEN_CONTEXT           pOpenContext;

    pOpenContext = (PNDISPROT_OPEN_CONTEXT)ProtocolBindingContext;
    NPROT_STRUCT_ASSERT(pOpenContext, oc);

    NPROT_SIGNAL_EVENT(&pOpenContext->BindEvent);
}

NDIS_STATUS
NdisprotPnPEventHandler(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN PNET_PNP_EVENT_NOTIFICATION  pNetPnPEventNotification
    )
/*++

Routine Description:

    Called by NDIS to notify us of a PNP event. The most significant
    one for us is power state change.

Arguments:

    ProtocolBindingContext - pointer to open context structure
                this is NULL for global reconfig events.

    pNetPnPEventNotification - pointer to the PNP event notification

Return Value:

    Our processing status for the PNP event.

--*/
{
    PNDISPROT_OPEN_CONTEXT            pOpenContext;
    NDIS_STATUS                       Status = NDIS_STATUS_SUCCESS;
    PUCHAR                            Buffer = NULL;
    ULONG                             BufferLength = 0;
    PNDIS_PROTOCOL_RESTART_PARAMETERS RestartParameters = NULL;

    pOpenContext = (PNDISPROT_OPEN_CONTEXT)ProtocolBindingContext;

    switch (pNetPnPEventNotification->NetPnPEvent.NetEvent)
    {
        case NetEventSetPower:
            NPROT_STRUCT_ASSERT(pOpenContext, oc);
            pOpenContext->PowerState = *(PNET_DEVICE_POWER_STATE)pNetPnPEventNotification->NetPnPEvent.Buffer;

            if (pOpenContext->PowerState > NetDeviceStateD0)
            {
                //
                //  The device below is transitioning to a low power state.
                //  Block any threads attempting to query the device while
                //  in this state.
                //
                NPROT_INIT_EVENT(&pOpenContext->PoweredUpEvent);

                //
                //  Wait for any I/O in progress to complete.
                //
                ndisprotWaitForPendingIO(pOpenContext, FALSE);

                //
                //  Return any receives that we had queued up.
                //
                ndisprotFlushReceiveQueue(pOpenContext);
                DEBUGP(DL_INFO, ("PnPEvent: Open %p, SetPower to %d\n",
                    pOpenContext, pOpenContext->PowerState));
            }
            else
            {
                //
                //  The device below is powered up.
                //
                DEBUGP(DL_INFO, ("PnPEvent: Open %p, SetPower ON: %d\n",
                    pOpenContext, pOpenContext->PowerState));
                NPROT_SIGNAL_EVENT(&pOpenContext->PoweredUpEvent);
            }

            Status = NDIS_STATUS_SUCCESS;
            break;

        case NetEventQueryPower:
            Status = NDIS_STATUS_SUCCESS;
            break;

        case NetEventBindsComplete:
            NPROT_SIGNAL_EVENT(&Globals.BindsComplete);
            if(!ndisprotRegisterExCallBack()){
                DEBUGP(DL_ERROR, ("NdisProtPnPEventHandler: ndisprotRegisterExCallBack failed\n"));
            }
            Status = NDIS_STATUS_SUCCESS;
            break;

        case NetEventPause:
            //
            // Wait all sends to be complete.
            //

            NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);
            pOpenContext->State = NdisprotPausing;

            //
            // we could also complete the PnP Event asynchrously.
            //
            while (TRUE)
            {
                if (pOpenContext->PendedSendCount == 0)
                {
                    break;
                }
                NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);
                DEBUGP(DL_INFO, ("PnPEvent: Open %p, outstanding count is %d\n", pOpenContext,
                    pOpenContext->PendedSendCount));

                NPROT_SLEEP(1);
                NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);
            }

            NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);
            //
            // Return all queued receives.
            //
            ndisprotFlushReceiveQueue(pOpenContext);
            pOpenContext->State = NdisprotPaused;

            break;

        case NetEventRestart:


            ASSERT(pOpenContext->State == NdisprotPaused);
            //
            // Get the updated attributes
            //
            Buffer = pNetPnPEventNotification->NetPnPEvent.Buffer;
            if (Buffer == NULL)
            {
                break;
            }
            BufferLength = pNetPnPEventNotification->NetPnPEvent.BufferLength;

            ASSERT(BufferLength  == sizeof(NDIS_PROTOCOL_RESTART_PARAMETERS));

            RestartParameters = (PNDIS_PROTOCOL_RESTART_PARAMETERS)Buffer;
            ndisprotRestart(pOpenContext,RestartParameters);


            pOpenContext->State = NdisprotRunning;
            break;

        case NetEventQueryRemoveDevice:
        case NetEventCancelRemoveDevice:
        case NetEventReconfigure:
        case NetEventBindList:
        case NetEventPnPCapabilities:
            Status = NDIS_STATUS_SUCCESS;
            break;

        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    DEBUGP(DL_INFO, ("PnPEvent: Open %p, Event %d, Status %x\n",
            pOpenContext, pNetPnPEventNotification->NetPnPEvent.NetEvent, Status));

    return (Status);
}

VOID
NdisprotProtocolUnloadHandler(
    VOID
    )
/*++

Routine Description:

    NDIS calls this on a usermode request to uninstall us.

Arguments:

    None

Return Value:

    None

--*/
{
    ndisprotDoProtocolUnload();
}

NDIS_STATUS
ndisprotCreateBinding(
    IN PNDISPROT_OPEN_CONTEXT                   pOpenContext,
    IN PNDIS_BIND_PARAMETERS                    BindParameters,
    IN NDIS_HANDLE                              BindContext,
    _In_reads_bytes_(BindingInfoLength) IN PUCHAR    pBindingInfo,
    IN ULONG                                    BindingInfoLength
    )
/*++

Routine Description:

    Utility function to create an NDIS binding to the indicated device,
    if no such binding exists.

    Here is where we also allocate additional resources (e.g. packet pool)
    for the binding.

    NOTE: this function blocks and finishes synchronously.

Arguments:

    pOpenContext - pointer to open context block
    BindParameters - pointer to NDIS_BIND_PARAMETERS
    BindConext - pointer to NDIS bind context
    pBindingInfo - pointer to unicode device name string
    BindingInfoLength - length in bytes of the above.

Return Value:

    NDIS_STATUS_SUCCESS if a binding was successfully set up.
    NDIS_STATUS_XXX error code on any failure.

--*/
{
    NDIS_STATUS              Status;
    NDIS_MEDIUM              MediumArray[1] = {NdisMedium802_3};
    NDIS_OPEN_PARAMETERS     OpenParameters;
    NET_BUFFER_LIST_POOL_PARAMETERS PoolParameters;
    UINT                     SelectedMediumIndex;
    BOOLEAN                  fOpenComplete = FALSE;
    ULONG                    GenericUlong = 0;
    NET_FRAME_TYPE           FrameTypeArray[2] = {NDIS_ETH_TYPE_802_1X, NDIS_ETH_TYPE_802_1Q};
#if DBG
    PNDISPROT_OPEN_CONTEXT   pTmpOpenContext;
#endif

    DEBUGP(DL_LOUD, ("CreateBinding: open %p/%x, device [%ws]\n",
                pOpenContext, pOpenContext->Flags, (PWSTR)pBindingInfo));

    Status = NDIS_STATUS_SUCCESS;

    do
    {
        //
        //  Check if we already have a binding to this device.
        //
#if DBG
        pTmpOpenContext = ndisprotLookupDevice(pBindingInfo, BindingInfoLength);

        NPROT_ASSERT(pTmpOpenContext == NULL);

        if (pTmpOpenContext != NULL)
        {
            DEBUGP(DL_WARN,
                ("CreateBinding: Binding to device %ws already exists on open %p\n",
                    pTmpOpenContext->DeviceName.Buffer, pTmpOpenContext));

            NPROT_DEREF_OPEN(pTmpOpenContext);  // temp ref added by Lookup
            Status = NDIS_STATUS_FAILURE;
            break;
        }
#endif

        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

        NPROT_SET_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_OPENING);

        NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

        //
        //  Copy in the device name. Add room for a NULL terminator.
        //
        NPROT_ALLOC_MEM(pOpenContext->DeviceName.Buffer, BindingInfoLength + sizeof(WCHAR));
        if (pOpenContext->DeviceName.Buffer == NULL)
        {
            DEBUGP(DL_WARN, ("CreateBinding: failed to alloc device name buf (%d bytes)\n",
                (ULONG)(BindingInfoLength + sizeof(WCHAR))));
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        NPROT_COPY_MEM(pOpenContext->DeviceName.Buffer, pBindingInfo, BindingInfoLength);
#pragma prefast(suppress: 12009, "DeviceName length will not cause overflow")
        *(PWCHAR)((PUCHAR)pOpenContext->DeviceName.Buffer + BindingInfoLength) = L'\0';
        NdisInitUnicodeString(&pOpenContext->DeviceName, pOpenContext->DeviceName.Buffer);

        NdisZeroMemory(&PoolParameters, sizeof(NET_BUFFER_LIST_POOL_PARAMETERS));

        PoolParameters.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        PoolParameters.Header.Revision = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
        PoolParameters.Header.Size = sizeof(PoolParameters);
        PoolParameters.ProtocolId = NDIS_PROTOCOL_ID_IPX ;
        PoolParameters.ContextSize = sizeof(NPROT_SEND_NETBUFLIST_RSVD);
        PoolParameters.fAllocateNetBuffer = TRUE;
        PoolParameters.PoolTag = NPROT_ALLOC_TAG;

        pOpenContext->SendNetBufferListPool = NdisAllocateNetBufferListPool(
                                                    Globals.NdisProtocolHandle,
                                                    &PoolParameters);
        if (pOpenContext->SendNetBufferListPool == NULL)
        {
            DEBUGP(DL_WARN, ("CreateBinding: failed to alloc"
                    " send net buffer list pool\n"));

            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        PoolParameters.ContextSize = 0;

        pOpenContext->RecvNetBufferListPool = NdisAllocateNetBufferListPool(
                                                    Globals.NdisProtocolHandle,
                                                    &PoolParameters);

        if (pOpenContext->RecvNetBufferListPool == NULL)
        {
            DEBUGP(DL_WARN, ("CreateBinding: failed to alloc"
                    " recv net buffer list pool.\n"));

            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        //  Assume that the device is powered up.
        //
        pOpenContext->PowerState = NetDeviceStateD0;

        //
        //  Open the adapter.
        //
        NPROT_INIT_EVENT(&pOpenContext->BindEvent);

        NPROT_ZERO_MEM(&OpenParameters, sizeof(NDIS_OPEN_PARAMETERS));
        OpenParameters.Header.Revision = NDIS_OPEN_PARAMETERS_REVISION_1;
        OpenParameters.Header.Size = sizeof(NDIS_OPEN_PARAMETERS);
        OpenParameters.Header.Type = NDIS_OBJECT_TYPE_OPEN_PARAMETERS;
        OpenParameters.AdapterName = BindParameters->AdapterName;
        OpenParameters.MediumArray = &MediumArray[0];
        OpenParameters.MediumArraySize = sizeof(MediumArray) / sizeof(NDIS_MEDIUM);
        OpenParameters.SelectedMediumIndex = &SelectedMediumIndex;
        OpenParameters.FrameTypeArray = &FrameTypeArray[0];
        OpenParameters.FrameTypeArraySize = sizeof(FrameTypeArray) / sizeof(NET_FRAME_TYPE);


        NDIS_DECLARE_PROTOCOL_OPEN_CONTEXT(NDISPROT_OPEN_CONTEXT);
        Status = NdisOpenAdapterEx(Globals.NdisProtocolHandle,
                          (NDIS_HANDLE)pOpenContext,
                           &OpenParameters,
                           BindContext,
                           &pOpenContext->BindingHandle);

        if (Status == NDIS_STATUS_PENDING)
        {
            NPROT_WAIT_EVENT(&pOpenContext->BindEvent, 0);
            Status = pOpenContext->BindStatus;
        }

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_WARN, ("CreateBinding: NdisOpenAdapter (%ws) failed: %x\n",
                pOpenContext->DeviceName.Buffer, Status));
            break;
        }

        pOpenContext->State = NdisprotPaused;

        fOpenComplete = TRUE;

        //
        //  Get the friendly name for the adapter. It is not fatal for this
        //  to fail.
        //
        (VOID)NdisQueryAdapterInstanceName(
                &pOpenContext->DeviceDescr,
                pOpenContext->BindingHandle
                );

        NdisMoveMemory(&pOpenContext->CurrentAddress[0],
                       BindParameters->CurrentMacAddress,
                       NPROT_MAC_ADDR_LEN);
        //
        //  Get MAC options.
        //
        pOpenContext->MacOptions = BindParameters->MacOptions;


        //
        //  Get the max frame size.
        //
        pOpenContext->MaxFrameSize = BindParameters->MtuSize;

        //
        //  Get the media connect status.
        //
        GenericUlong = BindParameters->MediaConnectState;

        if (GenericUlong == NdisMediaStateConnected)
        {
            NPROT_SET_FLAGS(pOpenContext->Flags, NPROTO_MEDIA_FLAGS, NPROTO_MEDIA_CONNECTED);
        }
        else
        {
            NPROT_SET_FLAGS(pOpenContext->Flags, NPROTO_MEDIA_FLAGS, NPROTO_MEDIA_DISCONNECTED);
        }
        //
        // Get the back fill size
        //
        pOpenContext->DataBackFillSize = BindParameters->DataBackFillSize;
        pOpenContext->ContextBackFillSize = BindParameters->ContextBackFillSize;

        //
        //  Mark this open. Also check if we received an Unbind while
        //  we were setting this up.
        //
        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

        NPROT_SET_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_ACTIVE);

        NPROT_ASSERT(!NPROT_TEST_FLAGS(pOpenContext->Flags, NPROTO_UNBIND_FLAGS, NPROTO_UNBIND_RECEIVED));

        NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);
    }
    while (FALSE);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

        //
        //  Check if we had actually finished opening the adapter.
        //
        if (fOpenComplete)
        {
            NPROT_SET_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_ACTIVE);
        }
        else if (NPROT_TEST_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_OPENING))
        {
            NPROT_SET_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_FAILED);
        }

        NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

        ndisprotShutdownBinding(pOpenContext);
    }

    DEBUGP(DL_INFO, ("CreateBinding: OpenContext %p, Status %x\n",
            pOpenContext, Status));

    return (Status);
}



VOID
ndisprotShutdownBinding(
    IN PNDISPROT_OPEN_CONTEXT        pOpenContext
    )
/*++

Routine Description:

    Utility function to shut down the NDIS binding, if one exists, on
    the specified open. This is written to be called from:

        ndisprotCreateBinding - on failure
        NdisprotUnbindAdapter

    We handle the case where a binding is in the process of being set up.
    This precaution is not needed if this routine is only called from
    the context of our UnbindAdapter handler, but they are here in case
    we initiate unbinding from elsewhere (e.g. on processing a user command).

    NOTE: this blocks and finishes synchronously.

Arguments:

    pOpenContext - pointer to open context block

Return Value:

    None

--*/
{
    NDIS_STATUS             Status;
    BOOLEAN                 DoCloseBinding = FALSE;
    NPROT_EVENT             ClosingEvent;

    do
    {
        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

        if (NPROT_TEST_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_OPENING))
        {
            //
            //  We are still in the process of setting up this binding.
            //
            NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);
            break;
        }

        if (NPROT_TEST_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_ACTIVE))
        {
            NPROT_ASSERT(pOpenContext->ClosingEvent == NULL);
            pOpenContext->ClosingEvent = NULL;

            NPROT_SET_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_CLOSING);

            if (pOpenContext->PendedSendCount != 0)
            {
                pOpenContext->ClosingEvent = &ClosingEvent;
                NPROT_INIT_EVENT(&ClosingEvent);
            }

            DoCloseBinding = TRUE;
        }

        NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

        if (DoCloseBinding)
        {
            ULONG    PacketFilter = 0;
            ULONG    BytesRead = 0;

            //
            // Set Packet filter to 0 before closing the binding
            //
            Status = ndisprotDoRequest(
                        pOpenContext,
                        NDIS_DEFAULT_PORT_NUMBER,
                        NdisRequestSetInformation,
                        OID_GEN_CURRENT_PACKET_FILTER,
                        &PacketFilter,
                        sizeof(PacketFilter),
                        &BytesRead);

            if (Status != NDIS_STATUS_SUCCESS)
            {
                DEBUGP(DL_WARN, ("ShutDownBinding: set packet filter failed: %x\n", Status));
            }

            //
            // Set multicast list to null before closing the binding
            //
            Status = ndisprotDoRequest(
                        pOpenContext,
                        NDIS_DEFAULT_PORT_NUMBER,
                        NdisRequestSetInformation,
                        OID_802_3_MULTICAST_LIST,
                        NULL,
                        0,
                        &BytesRead);

            if (Status != NDIS_STATUS_SUCCESS)
            {
                DEBUGP(DL_WARN, ("ShutDownBinding: set multicast list failed: %x\n", Status));
            }

            //
            //  Cancel any pending reads.
            //
            WdfIoQueuePurgeSynchronously(pOpenContext->ReadQueue);
            //
            // Cancel pending control request for status indication.
            //
            WdfIoQueuePurgeSynchronously(pOpenContext->StatusIndicationQueue);

            //
            //  Discard any queued receives.
            //
            ndisprotFlushReceiveQueue(pOpenContext);

            //
            //  Close the binding now.
            //
            NPROT_INIT_EVENT(&pOpenContext->BindEvent);

            DEBUGP(DL_INFO, ("ShutdownBinding: Closing OpenContext %p,"
                    " BindingHandle %p\n",
                    pOpenContext, pOpenContext->BindingHandle));

            Status = NdisCloseAdapterEx(pOpenContext->BindingHandle);

            if (Status == NDIS_STATUS_PENDING)
            {
                NPROT_WAIT_EVENT(&pOpenContext->BindEvent, 0);
                Status = pOpenContext->BindStatus;
            }

            NPROT_ASSERT(Status == NDIS_STATUS_SUCCESS);

            pOpenContext->BindingHandle = NULL;

            NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

            NPROT_SET_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_IDLE);

            NPROT_SET_FLAGS(pOpenContext->Flags, NPROTO_UNBIND_FLAGS, 0);

            NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

        }
    } while (FALSE);


    //
    //  Remove it from the global list.
    //
    NPROT_ACQUIRE_LOCK(&Globals.GlobalLock, FALSE);

    NPROT_REMOVE_ENTRY_LIST(&pOpenContext->Link);

    NPROT_RELEASE_LOCK(&Globals.GlobalLock, FALSE);

    //
    //  Free any other resources allocated for this bind.
    //
    ndisprotFreeBindResources(pOpenContext);

    NPROT_DEREF_OPEN(pOpenContext);  // Shutdown binding

}


VOID
ndisprotFreeBindResources(
    IN PNDISPROT_OPEN_CONTEXT       pOpenContext
    )
/*++

Routine Description:

    Free any resources set up for an NDIS binding.

Arguments:

    pOpenContext - pointer to open context block

Return Value:

    None

--*/
{
    if (pOpenContext->SendNetBufferListPool != NULL)
    {
        NdisFreeNetBufferListPool(pOpenContext->SendNetBufferListPool);
        pOpenContext->SendNetBufferListPool = NULL;
    }

    if (pOpenContext->RecvNetBufferListPool != NULL)
    {
        NdisFreeNetBufferListPool(pOpenContext->RecvNetBufferListPool);
        pOpenContext->RecvNetBufferListPool = NULL;
    }

    if (pOpenContext->DeviceName.Buffer != NULL)
    {
        NPROT_FREE_MEM(pOpenContext->DeviceName.Buffer);
        pOpenContext->DeviceName.Buffer = NULL;
        pOpenContext->DeviceName.Length =
        pOpenContext->DeviceName.MaximumLength = 0;
    }

    if (pOpenContext->DeviceDescr.Buffer != NULL)
    {
        //
        // this would have been allocated by NdisQueryAdpaterInstanceName.
        //
        NdisFreeMemory(pOpenContext->DeviceDescr.Buffer, 0, 0);
        pOpenContext->DeviceDescr.Buffer = NULL;
    }

    if (pOpenContext->ReadQueue) {
        WdfObjectDelete(pOpenContext->ReadQueue);
        pOpenContext->ReadQueue = NULL;
    }

    if (pOpenContext->StatusIndicationQueue) {
        WdfObjectDelete(pOpenContext->StatusIndicationQueue);
        pOpenContext->StatusIndicationQueue = NULL;
    }

}


VOID
ndisprotWaitForPendingIO(
    IN PNDISPROT_OPEN_CONTEXT            pOpenContext,
    IN BOOLEAN                           DoCancelReads
    )
/*++

Routine Description:

    Utility function to wait for all outstanding I/O to complete
    on an open context. It is assumed that the open context
    won't go away while we are in this routine.

Arguments:

    pOpenContext - pointer to open context structure
    DoCancelReads - do we wait for pending reads to go away (and cancel them)?

Return Value:

    None

--*/
{
    //
    //  Wait for any pending sends or requests on the binding to complete.
    //
    if (pOpenContext->PendedSendCount == 0)
    {
        NPROT_ASSERT(pOpenContext->ClosingEvent == NULL);
    }
    else
    {
        NPROT_ASSERT(pOpenContext->ClosingEvent != NULL);
        DEBUGP(DL_WARN, ("WaitForPendingIO: Open %p, %d pended sends\n",
                pOpenContext, pOpenContext->PendedSendCount));

        NPROT_WAIT_EVENT(pOpenContext->ClosingEvent, 0);

    }

    if (DoCancelReads)
    {
        //
        //  Wait for any pended reads to complete/cancel.
        //
        while (pOpenContext->PendedReadCount != 0)
        {
            DEBUGP(DL_INFO, ("WaitForPendingIO: Open %p, %d pended reads\n",
                pOpenContext, pOpenContext->PendedReadCount));

            //
            //  Cancel any pending reads.
            //
            WdfIoQueuePurgeSynchronously(pOpenContext->ReadQueue);

            NPROT_SLEEP(1);
        }
    }

}


VOID
ndisprotDoProtocolUnload(
    VOID
    )
/*++

Routine Description:

    Utility routine to handle unload from the NDIS protocol side.

Arguments:

    None

Return Value:

    None

--*/
{
    NDIS_HANDLE     ProtocolHandle;

    DEBUGP(DL_INFO, ("ProtocolUnload: ProtocolHandle %p\n",
        Globals.NdisProtocolHandle));

    if (Globals.NdisProtocolHandle != NULL)
    {
        ProtocolHandle = Globals.NdisProtocolHandle;
        Globals.NdisProtocolHandle = NULL;

        NdisDeregisterProtocolDriver(ProtocolHandle);

    }
}


NDIS_STATUS
ndisprotDoRequest(
    IN PNDISPROT_OPEN_CONTEXT       pOpenContext,
    IN NDIS_PORT_NUMBER             PortNumber,
    IN NDIS_REQUEST_TYPE            RequestType,
    IN NDIS_OID                     Oid,
    IN PVOID                        InformationBuffer,
    IN ULONG                        InformationBufferLength,
    OUT PULONG                      pBytesProcessed
    )
/*++

Routine Description:

    Utility routine that forms and sends an NDIS_REQUEST to the
    miniport, waits for it to complete, and returns status
    to the caller.

    NOTE: this assumes that the calling routine ensures validity
    of the binding handle until this returns.

Arguments:

    pOpenContext - pointer to our open context
    PortNumber - the port to issue the request
    RequestType - NdisRequest[Set|Query|Method]Information
    Oid - the object being set/queried
    InformationBuffer - data for the request
    InformationBufferLength - length of the above
    pBytesProcessed - place to return bytes read/written

Return Value:

    Status of the set/query/method request

--*/
{
    NDISPROT_REQUEST            ReqContext;
    PNDIS_OID_REQUEST           pNdisRequest = &ReqContext.Request;
    NDIS_STATUS                 Status;


    NdisZeroMemory(&ReqContext, sizeof(ReqContext));

    NPROT_INIT_EVENT(&ReqContext.ReqEvent);
    pNdisRequest->Header.Type = NDIS_OBJECT_TYPE_OID_REQUEST;
    pNdisRequest->Header.Revision = NDIS_OID_REQUEST_REVISION_1;
    pNdisRequest->Header.Size = sizeof(NDIS_OID_REQUEST);
    pNdisRequest->RequestType = RequestType;
    pNdisRequest->PortNumber = PortNumber;

    switch (RequestType)
    {
        case NdisRequestQueryInformation:
            pNdisRequest->DATA.QUERY_INFORMATION.Oid = Oid;
            pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer =
                                    InformationBuffer;
            pNdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength =
                                    InformationBufferLength;
            break;

        case NdisRequestSetInformation:
            pNdisRequest->DATA.SET_INFORMATION.Oid = Oid;
            pNdisRequest->DATA.SET_INFORMATION.InformationBuffer =
                                    InformationBuffer;
            pNdisRequest->DATA.SET_INFORMATION.InformationBufferLength =
                                    InformationBufferLength;
            break;

        default:
            NPROT_ASSERT(FALSE);
            break;
    }

    pNdisRequest->RequestId = NPROT_GET_NEXT_CANCEL_ID();
    Status = NdisOidRequest(pOpenContext->BindingHandle,
                            pNdisRequest);


    if (Status == NDIS_STATUS_PENDING)
    {

        NPROT_WAIT_EVENT(&ReqContext.ReqEvent, 0);
        Status = ReqContext.Status;
    }

    if (Status == NDIS_STATUS_SUCCESS)
    {
        *pBytesProcessed = (RequestType == NdisRequestQueryInformation)?
                            pNdisRequest->DATA.QUERY_INFORMATION.BytesWritten:
                            pNdisRequest->DATA.SET_INFORMATION.BytesRead;

        //
        // The driver below should set the correct value to BytesWritten
        // or BytesRead. But now, we just truncate the value to InformationBufferLength
        //
        if (*pBytesProcessed > InformationBufferLength)
        {
            *pBytesProcessed = InformationBufferLength;
        }
    }

    return (Status);
}


NDIS_STATUS
ndisprotValidateOpenAndDoRequest(
    IN PNDISPROT_OPEN_CONTEXT        pOpenContext,
    IN NDIS_REQUEST_TYPE             RequestType,
    IN NDIS_OID                      Oid,
    IN PVOID                         InformationBuffer,
    IN ULONG                         InformationBufferLength,
    OUT PULONG                       pBytesProcessed,
    IN BOOLEAN                       bWaitForPowerOn
    )
/*++

Routine Description:

    Utility routine to prevalidate and reference an open context
    before calling ndisprotDoRequest. This routine makes sure
    we have a valid binding.

Arguments:

    pOpenContext - pointer to our open context
    RequestType - NdisRequest[Set|Query]Information
    Oid - the object being set/queried
    InformationBuffer - data for the request
    InformationBufferLength - length of the above
    pBytesProcessed - place to return bytes read/written
    bWaitForPowerOn - Wait for the device to be powered on if it isn't already.

Return Value:

    Status of the set/query request

--*/
{
    NDIS_STATUS             Status;

    do
    {
        if (pOpenContext == NULL)
        {
            DEBUGP(DL_WARN, ("ValidateOpenAndDoRequest: request on unassociated file object!\n"));
            Status = NDIS_STATUS_INVALID_DATA;
            break;
        }

        NPROT_STRUCT_ASSERT(pOpenContext, oc);

        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

        //
        //  Proceed only if we have a binding.
        //
        if (!NPROT_TEST_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_ACTIVE))
        {
            NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);
            Status = NDIS_STATUS_INVALID_DATA;
            break;
        }

        NPROT_ASSERT(pOpenContext->BindingHandle != NULL);

        //
        //  Make sure that the binding does not go away until we
        //  are finished with the request.
        //
        pOpenContext->PendedSendCount++;

        NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

        if (bWaitForPowerOn)
        {
            //
            //  Wait for the device below to be powered up.
            //  We don't wait indefinitely here - this is to avoid
            //  a PROCESS_HAS_LOCKED_PAGES bugcheck that could happen
            //  if the calling process terminates, and this IRP doesn't
            //  complete within a reasonable time. An alternative would
            //  be to explicitly handle cancellation of this IRP.
            //
            if ( NPROT_WAIT_EVENT(&pOpenContext->PoweredUpEvent, 4500) ){};
        }

        if (pOpenContext->PowerState == NetDeviceStateD0)
        {

            Status = ndisprotDoRequest(
                        pOpenContext,
                        NDIS_DEFAULT_PORT_NUMBER,
                        RequestType,
                        Oid,
                        InformationBuffer,
                        InformationBufferLength,
                        pBytesProcessed);
        }
        else
        {
            Status = NDIS_STATUS_ADAPTER_NOT_READY;
        }

        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);
        //
        //  Let go of the binding.
        //
        pOpenContext->PendedSendCount --;
        if ((NPROT_TEST_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_CLOSING))
                        && (pOpenContext->PendedSendCount == 0))
        {
            NPROT_ASSERT(pOpenContext->ClosingEvent != NULL);
            NPROT_SIGNAL_EVENT(pOpenContext->ClosingEvent);
            pOpenContext->ClosingEvent = NULL;
        }

        NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

    }
    while (FALSE);

    DEBUGP(DL_LOUD, ("ValidateOpenAndDoReq: Open %p/%x, OID %x, Status %x\n",
						pOpenContext, 
						pOpenContext == NULL ? 0 : pOpenContext->Flags, 
						Oid, 
						Status));

    return (Status);
}


VOID
NdisprotRequestComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN PNDIS_OID_REQUEST            pNdisRequest,
    IN NDIS_STATUS                  Status
    )
/*++

Routine Description:

    NDIS entry point indicating completion of a pended NDIS_REQUEST.

Arguments:

    ProtocolBindingContext - pointer to open context
    pNdisRequest - pointer to NDIS request
    Status - status of reset completion

Return Value:

    None

--*/
{
    PNDISPROT_OPEN_CONTEXT       pOpenContext;
    PNDISPROT_REQUEST            pReqContext;

    pOpenContext = (PNDISPROT_OPEN_CONTEXT)ProtocolBindingContext;
    NPROT_STRUCT_ASSERT(pOpenContext, oc);

    //
    //  Get at the request context.
    //
    pReqContext = CONTAINING_RECORD(pNdisRequest, NDISPROT_REQUEST, Request);

    //
    //  Save away the completion status.
    //
    pReqContext->Status = Status;

    //
    //  Wake up the thread blocked for this request to complete.
    //
    NPROT_SIGNAL_EVENT(&pReqContext->ReqEvent);
}

VOID
ndisServiceIndicateStatusIrp(
    IN PNDISPROT_OPEN_CONTEXT   OpenContext,
    IN NDIS_STATUS              GeneralStatus,
    IN PVOID                    StatusBuffer,
    IN UINT                     StatusBufferSize
    )
/*++

Routine Description:

   We process the Request based on the input arguments and complete
   the Request. If the Request was cancelled for some reason we will let
   the cancel routine do the Request completion.

Arguments:

    ProtocolBindingContext - pointer to open context
    GeneralStatus - status code
    StatusBuffer - status-specific additional information
    StatusBufferSize - size of the above
    Cancel - Should the Request be cancelled right away.

Return Value:

    None

--*/
{
    PNDISPROT_INDICATE_STATUS     pIndicateStatus = NULL;
    WDFREQUEST  request = NULL;
    NTSTATUS    ntStatus;
    ULONG       bytes = 0;
    size_t      outBufLength;

    DEBUGP(DL_LOUD, ("-->ndisServiceIndicateStatusIrp\n"));

    NPROT_ACQUIRE_LOCK(&OpenContext->Lock, FALSE);

    do {
        //
        //  Get the first pended Read Request
        //
        ntStatus = WdfIoQueueRetrieveNextRequest(
                         OpenContext->StatusIndicationQueue,
                         &request
                         );
        if(!NT_SUCCESS(ntStatus)){
            ASSERTMSG("WdfIoQueueRetrieveNextRequest failed",  ntStatus == STATUS_NO_MORE_ENTRIES);
            break;
        }

        ntStatus = WdfRequestRetrieveOutputBuffer(request,
                                            sizeof(NDISPROT_INDICATE_STATUS),
                                            &pIndicateStatus,
                                            &outBufLength);
        if( !NT_SUCCESS(ntStatus) ) {
            DEBUGP(DL_ERROR, ("WdfRequestRetrieveOutputBuffer failed 0x%x\n", ntStatus));
            break;
        }

        //
        // Check to see whether the buffer is large enough to accomadate the
        // status buffer data.
        //
        if(outBufLength - sizeof(NDISPROT_INDICATE_STATUS) >= StatusBufferSize){

            pIndicateStatus->IndicatedStatus = GeneralStatus;
            pIndicateStatus->StatusBufferLength = StatusBufferSize;
            pIndicateStatus->StatusBufferOffset = sizeof(NDISPROT_INDICATE_STATUS);

            NPROT_COPY_MEM((PUCHAR)pIndicateStatus +
                            pIndicateStatus->StatusBufferOffset,
                            StatusBuffer,
                            StatusBufferSize);


            ntStatus = STATUS_SUCCESS;

        } else {
            ntStatus = STATUS_BUFFER_OVERFLOW;
        }
        //
        // Number of bytes copied or number of bytes required.
        //
        bytes = sizeof(NDISPROT_INDICATE_STATUS) + StatusBufferSize;
    }while(FALSE);

    NPROT_RELEASE_LOCK(&OpenContext->Lock, FALSE);

    if(request){
        WdfRequestCompleteWithInformation(request, ntStatus, bytes);
    }

    DEBUGP(DL_LOUD, ("<--ndisServiceIndicateStatusIrp\n"));

    return;

}

VOID
NdisprotStatus(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN PNDIS_STATUS_INDICATION      StatusIndication
    )
/*++

Routine Description:

    Protocol entry point called by NDIS to indicate a change
    in status at the miniport.

    We make note of reset and media connect status indications.

Arguments:

    ProtocolBindingContext - pointer to open context
    StatusIndication - pointer to NDIS_STATUS_INDICATION

Return Value:

    None

--*/
{
    PNDISPROT_OPEN_CONTEXT       pOpenContext;
    NDIS_STATUS                  GeneralStatus;
    PNDIS_LINK_STATE             LinkState;


    pOpenContext = (PNDISPROT_OPEN_CONTEXT)ProtocolBindingContext;
    NPROT_STRUCT_ASSERT(pOpenContext, oc);


    if ((StatusIndication->Header.Type != NDIS_OBJECT_TYPE_STATUS_INDICATION)
            || (StatusIndication->Header.Size != sizeof(NDIS_STATUS_INDICATION)))
    {
        DEBUGP(DL_INFO, ("Status: Received an invalid status indication: Open %p, StatusIndication %p\n",
                    pOpenContext, StatusIndication));
        return;
    }


    GeneralStatus = StatusIndication->StatusCode;

    DEBUGP(DL_INFO, ("Status: Open %p, Status %x\n",
            pOpenContext, GeneralStatus));

    ndisServiceIndicateStatusIrp(pOpenContext,
                                GeneralStatus,
                                StatusIndication->StatusBuffer,
                                StatusIndication->StatusBufferSize
                                );

    NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

    do
    {
        if (pOpenContext->PowerState != NetDeviceStateD0)
        {
            //
            //
            //  The device is in a low power state.

            //
            //  We continue and make note of status indications
            //

            //
            //  NOTE that any actions we take based on these
            //  status indications should take into account
            //  the current device power state.
            //
        }

        switch(GeneralStatus)
        {
            case NDIS_STATUS_RESET_START:

                NPROT_ASSERT(!NPROT_TEST_FLAGS(pOpenContext->Flags,
                                             NPROTO_RESET_FLAGS,
                                             NPROTO_RESET_IN_PROGRESS));

                NPROT_SET_FLAGS(pOpenContext->Flags,
                               NPROTO_RESET_FLAGS,
                               NPROTO_RESET_IN_PROGRESS);

                break;

            case NDIS_STATUS_RESET_END:

                NPROT_ASSERT(NPROT_TEST_FLAGS(pOpenContext->Flags,
                                            NPROTO_RESET_FLAGS,
                                            NPROTO_RESET_IN_PROGRESS));

                NPROT_SET_FLAGS(pOpenContext->Flags,
                               NPROTO_RESET_FLAGS,
                               NPROTO_NOT_RESETTING);

                break;

             case NDIS_STATUS_LINK_STATE:

                NPROT_ASSERT(StatusIndication->StatusBufferSize >= sizeof(NDIS_LINK_STATE));


                LinkState = (PNDIS_LINK_STATE)StatusIndication->StatusBuffer;

                if (LinkState->MediaConnectState == MediaConnectStateConnected)
                {
                    NPROT_SET_FLAGS(pOpenContext->Flags,
                                   NPROTO_MEDIA_FLAGS,
                                   NPROTO_MEDIA_CONNECTED);
                }
                else
                {
                    NPROT_SET_FLAGS(pOpenContext->Flags,
                                   NPROTO_MEDIA_FLAGS,
                                   NPROTO_MEDIA_DISCONNECTED);
                }

                break;

            default:
                break;
        }
    }
    while (FALSE);

    NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);
}


NDIS_STATUS
ndisprotQueryBinding(
    IN PUCHAR                       pBuffer,
    IN ULONG                        InputLength,
    IN ULONG                        OutputLength,
    OUT PULONG                      pBytesReturned
    )
/*++

Routine Description:

    Return information about the specified binding.

Arguments:

    pBuffer - pointer to NDISPROT_QUERY_BINDING
    InputLength - input buffer size
    OutputLength - output buffer size
    pBytesReturned - place to return copied byte count.

Return Value:

    NDIS_STATUS_SUCCESS if successful, failure code otherwise.

--*/
{
    PNDISPROT_QUERY_BINDING      pQueryBinding;
    PNDISPROT_OPEN_CONTEXT       pOpenContext;
    PLIST_ENTRY                  pEnt;
    ULONG                        Remaining;
    ULONG                        BindingIndex;
    NDIS_STATUS                  Status;

    do
    {
        if (InputLength < sizeof(NDISPROT_QUERY_BINDING))
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        if (OutputLength < sizeof(NDISPROT_QUERY_BINDING))
        {
            Status = NDIS_STATUS_BUFFER_OVERFLOW;
            break;
        }

        Remaining = OutputLength - sizeof(NDISPROT_QUERY_BINDING);

        pQueryBinding = (PNDISPROT_QUERY_BINDING)pBuffer;
        BindingIndex = pQueryBinding->BindingIndex;

        Status = NDIS_STATUS_ADAPTER_NOT_FOUND;

        pOpenContext = NULL;

        NPROT_ACQUIRE_LOCK(&Globals.GlobalLock, FALSE);

        for (pEnt = Globals.OpenList.Flink;
             pEnt != &Globals.OpenList;
             pEnt = pEnt->Flink)
        {
            pOpenContext = CONTAINING_RECORD(pEnt, NDISPROT_OPEN_CONTEXT, Link);
            NPROT_STRUCT_ASSERT(pOpenContext, oc);

            NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

            //
            //  Skip if not bound.
            //
            if (!NPROT_TEST_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_ACTIVE))
            {
                NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);
                continue;
            }

            if (BindingIndex == 0)
            {
                //
                //  Got the binding we are looking for. Copy the device
                //  name and description strings to the output buffer.
                //
                DEBUGP(DL_INFO,
                    ("QueryBinding: found open %p\n", pOpenContext));

                pQueryBinding->DeviceNameLength = pOpenContext->DeviceName.Length;
                pQueryBinding->DeviceDescrLength = pOpenContext->DeviceDescr.Length;
                if (Remaining < (pQueryBinding->DeviceNameLength + sizeof(WCHAR)) +
                                (pQueryBinding->DeviceDescrLength + sizeof(WCHAR)))
                {
                    NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);
                    Status = NDIS_STATUS_BUFFER_OVERFLOW;
                    break;
                }

                NPROT_ZERO_MEM((PUCHAR)pBuffer + sizeof(NDISPROT_QUERY_BINDING),
                                (pQueryBinding->DeviceNameLength + sizeof (WCHAR)) +
                                (pQueryBinding->DeviceDescrLength + sizeof(WCHAR)));

                pQueryBinding->DeviceNameOffset = sizeof(NDISPROT_QUERY_BINDING);
                NPROT_COPY_MEM((PUCHAR)pBuffer + pQueryBinding->DeviceNameOffset,
                                pOpenContext->DeviceName.Buffer,
                                pOpenContext->DeviceName.Length);

                pQueryBinding->DeviceDescrOffset = pQueryBinding->DeviceNameOffset +
                                                    pQueryBinding->DeviceNameLength + sizeof(WCHAR);
                NPROT_COPY_MEM((PUCHAR)pBuffer + pQueryBinding->DeviceDescrOffset,
                                pOpenContext->DeviceDescr.Buffer,
                                pOpenContext->DeviceDescr.Length);

                NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

                *pBytesReturned = pQueryBinding->DeviceDescrOffset + pQueryBinding->DeviceDescrLength + sizeof(WCHAR);
                Status = NDIS_STATUS_SUCCESS;
                break;
            }

            NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

            BindingIndex--;
        }

        NPROT_RELEASE_LOCK(&Globals.GlobalLock, FALSE);

    }
    while (FALSE);

    return (Status);
}

PNDISPROT_OPEN_CONTEXT
ndisprotLookupDevice(
    _In_reads_bytes_(BindingInfoLength) IN PUCHAR    pBindingInfo,
    IN ULONG                                    BindingInfoLength
    )
/*++

Routine Description:

    Search our global list for an open context structure that
    has a binding to the specified device, and return a pointer
    to it.

    NOTE: we reference the open that we return.

Arguments:

    pBindingInfo - pointer to unicode device name string
    BindingInfoLength - length in bytes of the above.

Return Value:

    Pointer to the matching open context if found, else NULL

--*/
{
    PNDISPROT_OPEN_CONTEXT      pOpenContext;
    PLIST_ENTRY                 pEnt;

    pOpenContext = NULL;

    NPROT_ACQUIRE_LOCK(&Globals.GlobalLock, FALSE);

    for (pEnt = Globals.OpenList.Flink;
         pEnt != &Globals.OpenList;
         pEnt = pEnt->Flink)
    {
        pOpenContext = CONTAINING_RECORD(pEnt, NDISPROT_OPEN_CONTEXT, Link);
        NPROT_STRUCT_ASSERT(pOpenContext, oc);

        //
        //  Check if this has the name we are looking for.
        //
        if ((pOpenContext->DeviceName.Length == BindingInfoLength) &&
            NPROT_MEM_CMP(pOpenContext->DeviceName.Buffer, pBindingInfo, BindingInfoLength))
        {
            NPROT_REF_OPEN(pOpenContext);   // ref added by LookupDevice
            break;
        }

        pOpenContext = NULL;
    }

    NPROT_RELEASE_LOCK(&Globals.GlobalLock, FALSE);

    return (pOpenContext);
}


NDIS_STATUS
ndisprotQueryOidValue(
    IN  PNDISPROT_OPEN_CONTEXT       pOpenContext,
    OUT PVOID                        pDataBuffer,
    IN  ULONG                        BufferLength,
    OUT PULONG                       pBytesWritten
    )
/*++

Routine Description:

    Query an arbitrary OID value from the miniport.

Arguments:

    pOpenContext - pointer to open context representing our binding to the miniport
    pDataBuffer - place to store the returned value
    BufferLength - length of the above
    pBytesWritten - place to return length returned

Return Value:

    NDIS_STATUS_SUCCESS if we successfully queried the OID.
    NDIS_STATUS_XXX error code otherwise.

--*/
{
    NDIS_STATUS             Status;
    PNDISPROT_QUERY_OID      pQuery;
    NDIS_OID                Oid;

    Oid = 0;

    do
    {
        if (BufferLength < sizeof(NDISPROT_QUERY_OID))
        {
            Status = NDIS_STATUS_BUFFER_TOO_SHORT;
            break;
        }

        pQuery = (PNDISPROT_QUERY_OID)pDataBuffer;
        Oid = pQuery->Oid;

        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

        if (!NPROT_TEST_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_ACTIVE))
        {
            DEBUGP(DL_WARN,
                ("QueryOid: Open %p/%x is in invalid state\n",
                    pOpenContext, pOpenContext->Flags));

            NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        //  Make sure the binding doesn't go away.
        //
        pOpenContext->PendedSendCount++;

        NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

        Status = ndisprotDoRequest(
                    pOpenContext,
                    pQuery->PortNumber,
                    NdisRequestQueryInformation,
                    Oid,
                    &pQuery->Data[0],
                    BufferLength - FIELD_OFFSET(NDISPROT_QUERY_OID, Data),
                    pBytesWritten);

        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

        //
        //  Let go of the binding.
        //
        pOpenContext->PendedSendCount --;
        if ((NPROT_TEST_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_CLOSING))
                        && (pOpenContext->PendedSendCount == 0))
        {
            NPROT_ASSERT(pOpenContext->ClosingEvent != NULL);
            NPROT_SIGNAL_EVENT(pOpenContext->ClosingEvent);
            pOpenContext->ClosingEvent = NULL;
        }

        NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

        if (Status == NDIS_STATUS_SUCCESS)
        {
            *pBytesWritten += FIELD_OFFSET(NDISPROT_QUERY_OID, Data);
        }

    }
    while (FALSE);

    DEBUGP(DL_LOUD, ("QueryOid: Open %p/%x, OID %x, Status %x\n",
                pOpenContext, pOpenContext->Flags, Oid, Status));

    return (Status);

}

NDIS_STATUS
ndisprotSetOidValue(
    IN  PNDISPROT_OPEN_CONTEXT       pOpenContext,
    OUT PVOID                        pDataBuffer,
    IN  ULONG                        BufferLength
    )
/*++

Routine Description:

    Set an arbitrary OID value to the miniport.

Arguments:

    pOpenContext - pointer to open context representing our binding to the miniport
    pDataBuffer - buffer that contains the value to be set
    BufferLength - length of the above

Return Value:

    NDIS_STATUS_SUCCESS if we successfully set the OID
    NDIS_STATUS_XXX error code otherwise.

--*/
{
    NDIS_STATUS             Status;
    PNDISPROT_SET_OID       pSet;
    NDIS_OID                Oid;
    ULONG                   BytesWritten;

    Oid = 0;

    do
    {
        if (BufferLength < sizeof(NDISPROT_SET_OID))
        {
            Status = NDIS_STATUS_BUFFER_TOO_SHORT;
            break;
        }

        pSet = (PNDISPROT_SET_OID)pDataBuffer;
        Oid = pSet->Oid;

        //
        // We should check the OID is settable by the user mode apps
        //
        if (!ndisprotValidOid(Oid))
        {
            DEBUGP(DL_WARN, ("SetOid: Oid %x cannot be set\n", Oid));

            Status = NDIS_STATUS_INVALID_DATA;
            break;
        }

        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

        if (!NPROT_TEST_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_ACTIVE))
        {
            DEBUGP(DL_WARN,
                ("SetOid: Open %p/%x is in invalid state\n",
                    pOpenContext, pOpenContext->Flags));

            NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        //  Make sure the binding doesn't go away.
        //
        pOpenContext->PendedSendCount++;

        NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

        Status = ndisprotDoRequest(
                    pOpenContext,
                    pSet->PortNumber,
                    NdisRequestSetInformation,
                    Oid,
                    &pSet->Data[0],
                    BufferLength - FIELD_OFFSET(NDISPROT_SET_OID, Data),
                    &BytesWritten);

        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock, FALSE);

        //
        //  Let go of the binding.
        //
        pOpenContext->PendedSendCount --;
        if ((NPROT_TEST_FLAGS(pOpenContext->Flags, NPROTO_BIND_FLAGS, NPROTO_BIND_CLOSING))
                        && (pOpenContext->PendedSendCount == 0))
        {
            NPROT_ASSERT(pOpenContext->ClosingEvent != NULL);
            NPROT_SIGNAL_EVENT(pOpenContext->ClosingEvent);
            pOpenContext->ClosingEvent = NULL;
        }


        NPROT_RELEASE_LOCK(&pOpenContext->Lock, FALSE);

    }
    while (FALSE);

    DEBUGP(DL_LOUD, ("SetOid: Open %p/%x, OID %x, Status %x\n",
                pOpenContext, pOpenContext->Flags, Oid, Status));

    return (Status);
}

BOOLEAN
ndisprotValidOid(
    IN  NDIS_OID            Oid
    )
/*++

Routine Description:

    Validate whether the given set OID is settable or not.

Arguments:

    Oid - The OID which the user tries to set.

Return Value:

    TRUE if the OID is allowed to set
    FALSE otherwise.

--*/
{
    UINT    i;
    UINT    NumOids;

    NumOids = sizeof(ndisprotSupportedSetOids) / sizeof(NDIS_OID);

    for (i = 0; i < NumOids; i++)
    {
        if (ndisprotSupportedSetOids[i] == Oid)
        {
            break;
        }
    }

    return (i < NumOids);
}



VOID
ndisprotRestart(
    IN PNDISPROT_OPEN_CONTEXT             pOpenContext,
    IN PNDIS_PROTOCOL_RESTART_PARAMETERS  RestartParameters
    )
/*++

Routine Description:

    Handle restart attributes changes.

Arguments:

    pOpenContext - pointer to open context
    RestartParameters - pointer to ndis restart parameters

Return Value:

    None

NOTE: Protocols should query any attribute:
      1. the attribute is not included in the RestartAttributes
  and 2. The protocol cares about whether the attributes is changed by underlying driver.

--*/

{
    ULONG           Length;
    ULONG           TotalLength = 0;
    PUCHAR          Buffer;
    ULONG           BufferLength;
#define NPROT_MAX_FILTER_NAME_LENGTH          128

    WCHAR           FilterNameBuffer[NPROT_MAX_FILTER_NAME_LENGTH];
    PNDIS_RESTART_ATTRIBUTES          NdisRestartAttributes;
    PNDIS_RESTART_GENERAL_ATTRIBUTES  NdisGeneralAttributes;

    DEBUGP(DL_LOUD, ("ndisprotRestart: Open %p", pOpenContext));
    //
    // Check the filter stack changes
    //
    if (RestartParameters->FilterModuleNameBuffer != NULL)
    {

     Buffer = RestartParameters->FilterModuleNameBuffer;

       while (RestartParameters->FilterModuleNameBufferLength > TotalLength)
       {

           BufferLength = *(PUSHORT)Buffer;

           Length = BufferLength + sizeof(USHORT);
           TotalLength += Length;

           if (BufferLength >= (NPROT_MAX_FILTER_NAME_LENGTH * sizeof(WCHAR)))
           {
               BufferLength = (NPROT_MAX_FILTER_NAME_LENGTH - 1) * sizeof(WCHAR);
           }
           NdisMoveMemory(FilterNameBuffer, Buffer + sizeof(USHORT), BufferLength);

           BufferLength /= sizeof(WCHAR);

           //
           // BufferLength is bounded by the check above. Check again to suppress
           // prefast warning
           //
           if (BufferLength < NPROT_MAX_FILTER_NAME_LENGTH)
           {
               FilterNameBuffer[BufferLength] = 0;
           }

           DEBUGP(DL_INFO, ("Filter: %ws\n", FilterNameBuffer));

           Buffer += Length;
       }
    }
    //
    // Checked for updated attributes
    //
    NdisRestartAttributes = RestartParameters->RestartAttributes;

    //
    // NdisProt is only interested in the generic attributes.
    //
    while (NdisRestartAttributes != NULL)
    {
       if (NdisRestartAttributes->Oid == OID_GEN_MINIPORT_RESTART_ATTRIBUTES)
       {
           break;
       }
       NdisRestartAttributes = NdisRestartAttributes->Next;
    }

    //
    // Pick up the new attributes of interest
    //
    if (NdisRestartAttributes != NULL)
    {
        NdisGeneralAttributes = (PNDIS_RESTART_GENERAL_ATTRIBUTES)NdisRestartAttributes->Data;

        pOpenContext->MacOptions = NdisGeneralAttributes->MacOptions;
        pOpenContext->MaxFrameSize = NdisGeneralAttributes->MtuSize;
    }

    DEBUGP(DL_LOUD, ("ndisprotRestart: Open %p", pOpenContext));

}



