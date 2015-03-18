/*++
Copyright(c) 1992-2000  Microsoft Corporation

Module Name:

    protocol.c

Abstract:

    NDIS Protocol Entry points and utility functions for the NDIS
    MUX Intermediate Miniport sample.

    The protocol edge binds to Ethernet (NdisMedium802_3) adapters,
    and initiates creation of zero or more Virtual Ethernet LAN (VELAN)
    miniport instances by calling NdisIMInitializeDeviceInstanceEx once
    for each VELAN configured over a lower binding.

Environment:

    Kernel mode.

Revision History:


--*/


#include "precomp.h"
#pragma hdrstop


#define MODULE_NUMBER           MODULE_PROT


NDIS_STATUS
PtBindAdapter(
    IN  NDIS_HANDLE             ProtocolDriverContext,
    IN  NDIS_HANDLE             BindContext,
    IN  PNDIS_BIND_PARAMETERS   BindParameters
    )
/*++

Routine Description:

    Called by NDIS to bind to a miniport below. This routine
    creates a binding by calling NdisOpenAdapterEx, and then
    initiates creation of all configured VELANs on this binding.

Arguments:
    ProtocolDriverContext        A pointer to the driver context
    BindContext                  A pointer to the bind context
    BindParameters               Pointing to related information about this new binding.

Return Value:

    Return Status is set to NDIS_STATUS_SUCCESS if no failure occurred
    while handling this call, otherwise an error code.

--*/
{
    PADAPT                            pAdapt = NULL;
    UINT                              MediumIndex = 0;
    PNDIS_STRING                      pConfigString;
    ULONG                             Length;
    NDIS_STATUS                       Status = NDIS_STATUS_SUCCESS;
    NDIS_OPEN_PARAMETERS              OpenParameters;

    UNREFERENCED_PARAMETER(ProtocolDriverContext);
    UNREFERENCED_PARAMETER(BindContext);
    
    pConfigString = (PNDIS_STRING)BindParameters->ProtocolSection;
    
    DBGPRINT(MUX_LOUD, ("==> Protocol BindAdapter: %ws\n", pConfigString->Buffer));
   
    do
    {
        if (BindParameters->Header.Type != NDIS_OBJECT_TYPE_BIND_PARAMETERS ||
                BindParameters->Header.Revision != NDIS_BIND_PARAMETERS_REVISION_1)
        {
            Status = NDIS_STATUS_INVALID_PARAMETER;
            break;
        }
        
        //
        // Allocate memory for Adapter struct plus the config
        // string with two extra WCHARs for NULL termination.
        //
        Length = sizeof(ADAPT) + 
                    pConfigString->MaximumLength + sizeof(WCHAR);
        pAdapt = NdisAllocateMemoryWithTagPriority(ProtHandle, Length , MUX_TAG, LowPoolPriority);

        if (pAdapt == NULL)
        {
             Status = NDIS_STATUS_RESOURCES;
             break;
        }
        
        //
        // Initialize the adapter structure
        //
        NdisZeroMemory(pAdapt, sizeof(ADAPT));        

        (VOID)PtReferenceAdapter(pAdapt, (PUCHAR)"openadapter");        
        

        //
        //  Copy in the Config string - we will use this to open the
        //  registry section for this adapter at a later point.
        //
        pAdapt->ConfigString.MaximumLength = pConfigString->MaximumLength;
        pAdapt->ConfigString.Length = pConfigString->Length;
        pAdapt->ConfigString.Buffer = (PWCHAR)((PUCHAR)pAdapt + 
                            sizeof(ADAPT));

        NdisMoveMemory(pAdapt->ConfigString.Buffer,
                       pConfigString->Buffer,
                       pConfigString->Length);
        pAdapt->ConfigString.Buffer[pConfigString->Length/sizeof(WCHAR)] = 
                                    ((WCHAR)0);

        NdisInitializeEvent(&pAdapt->Event);
        NdisInitializeListHead(&pAdapt->VElanList);

        pAdapt->PtDevicePowerState = NdisDeviceStateD0;

        //
        // Copy the Link state, this could be updated by PtStatus soon after
        // open operation is complete
        //
        pAdapt->LastIndicatedLinkState.Header.Revision = NDIS_LINK_STATE_REVISION_1;
        pAdapt->LastIndicatedLinkState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        pAdapt->LastIndicatedLinkState.Header.Size = sizeof(NDIS_LINK_STATE);
        pAdapt->LastIndicatedLinkState.MediaConnectState = BindParameters->MediaConnectState;
        pAdapt->LastIndicatedLinkState.MediaDuplexState = BindParameters->MediaDuplexState;
        pAdapt->LastIndicatedLinkState.XmitLinkSpeed = BindParameters->XmitLinkSpeed;
        pAdapt->LastIndicatedLinkState.RcvLinkSpeed = BindParameters->RcvLinkSpeed;
        pAdapt->Flags = 0;

        MUX_INIT_ADAPT_RW_LOCK(pAdapt);
        NdisAllocateSpinLock(&pAdapt->Lock);
        //
        // Now open the adapter below and complete the initialization
        //
        NdisZeroMemory(&OpenParameters, sizeof(NDIS_OPEN_PARAMETERS));

        OpenParameters.Header.Type = NDIS_OBJECT_TYPE_OPEN_PARAMETERS;
        OpenParameters.Header.Revision = NDIS_OPEN_PARAMETERS_REVISION_1;
        OpenParameters.Header.Size = sizeof(NDIS_OPEN_PARAMETERS);
        OpenParameters.AdapterName = BindParameters->AdapterName;
        OpenParameters.MediumArray = MediumArray;
        OpenParameters.MediumArraySize = sizeof(MediumArray) / sizeof(NDIS_MEDIUM);
        OpenParameters.SelectedMediumIndex = &MediumIndex;

        OpenParameters.FrameTypeArray = NULL;
        OpenParameters.FrameTypeArraySize = 0;

        NDIS_DECLARE_PROTOCOL_OPEN_CONTEXT(ADAPT);
        Status = NdisOpenAdapterEx(ProtHandle,
                                   pAdapt,
                                   &OpenParameters,
                                   BindContext,
                                   &pAdapt->BindingHandle);

        if (Status == NDIS_STATUS_PENDING)
        {
              NdisWaitEvent(&pAdapt->Event, 0);
              Status = pAdapt->Status;
        }

        if (Status != NDIS_STATUS_SUCCESS)
        {
              pAdapt->BindingHandle = NULL;
              break;
        }
        pAdapt->Flags |= MUX_BINDING_ACTIVE;
        
        pAdapt->BindingState = MuxAdapterBindingPaused;
        
        pAdapt->Medium = MediumArray[MediumIndex];

        //
        // Add this adapter to the global AdapterList
        //
        MUX_ACQUIRE_MUTEX(&GlobalMutex);

        InsertTailList(&AdapterList, &pAdapt->Link);

        MUX_RELEASE_MUTEX(&GlobalMutex);

        //
        // Copy all the relevant information about the Adapter into 
        // the local structure
        //
        pAdapt->BindParameters = *BindParameters;
        
        if (BindParameters->RcvScaleCapabilities)
        {
            pAdapt->RcvScaleCapabilities = (*BindParameters->RcvScaleCapabilities);
            pAdapt->BindParameters.RcvScaleCapabilities = &pAdapt->RcvScaleCapabilities;
        }
        
        pAdapt->PowerManagementCapabilities = (*BindParameters->PowerManagementCapabilities); 

        
        PtPostProcessPnPCapabilities(&pAdapt->PowerManagementCapabilities,
                                     sizeof(pAdapt->PowerManagementCapabilities));

        //
        // Zeroing out fields that are not needed by the MUX driver
        //
        pAdapt->BindParameters.ProtocolSection= NULL;
        pAdapt->BindParameters.AdapterName = NULL;
        pAdapt->BindParameters.PhysicalDeviceObject = NULL;
        
        //
        // Start all VELANS configured on this adapter.
        //
        Status = PtBootStrapVElans(pAdapt, NULL);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        
    } while(FALSE);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        
        if (pAdapt != NULL)
        {
            //
            // For some reason, the driver cannot create velan for the binding
            //
            if (pAdapt->BindingHandle != NULL)
            {
                //
                // Close the binding the driver opened above
                // 
                PtCloseAdapter(pAdapt);

                MUX_ACQUIRE_MUTEX(&GlobalMutex);

                RemoveEntryList(&pAdapt->Link);

                MUX_RELEASE_MUTEX(&GlobalMutex);
            }
            PtDereferenceAdapter(pAdapt, (PUCHAR)"openadapter");
            pAdapt = NULL;
        }
    }
    


    DBGPRINT(MUX_INFO, ("<== PtBindAdapter: pAdapt %p, Status %x\n", pAdapt, Status));

    return Status;
}


VOID
PtOpenAdapterComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_STATUS             Status
    )
/*++

Routine Description:

    Completion routine for NdisOpenAdapter issued from within the 
    PtBindAdapter. Simply unblock the caller.

Arguments:

    ProtocolBindingContext    Pointer to the adapter
    Status                    Status of the NdisOpenAdapter call

Return Value:

    None

--*/
{
    PADAPT      pAdapt =(PADAPT)ProtocolBindingContext;

    DBGPRINT(MUX_LOUD, ("==> PtOpenAdapterComplete: Adapt %p, Status %x\n", pAdapt, Status));

    pAdapt->Status = Status;
    NdisSetEvent(&pAdapt->Event);

    DBGPRINT(MUX_LOUD, ("<== PtOpenAdapterComplete: Adapt %p, Status %x\n", pAdapt, Status));
}


VOID
PtQueryAdapterInfo(
    IN  PADAPT                  pAdapt
    )
/*++

Routine Description:

    Query the adapter we are bound to for some standard OID values
    which we cache.

Arguments:

    pAdapt              Pointer to the adapter


Return Value:

    None
--*/
{
    //
    // Insert code here to query Adapter info if needed
    //
    UNREFERENCED_PARAMETER(pAdapt);
    
}


VOID
PtRequestAdapterSync(
    IN  PADAPT                      pAdapt,
    IN  NDIS_REQUEST_TYPE           RequestType,
    IN  NDIS_OID                    Oid,
    IN  PVOID                       InformationBuffer,
    IN  ULONG                       InformationBufferLength
    )
/*++

Routine Description:

    Utility routine to query the adapter for a single OID value. This
    blocks for the query to complete.

Arguments:

    pAdapt                      Pointer to the adapter
    RequestType                 The type of the NDIS request
    Oid                         OID to query for
    InformationBuffer           Place for the result
    InformationBufferLength     Length of the above

Return Value:

    None.

--*/
{
    PMUX_NDIS_REQUEST       pMuxNdisRequest = NULL;
    NDIS_STATUS             Status = NDIS_STATUS_FAILURE;

    DBGPRINT(MUX_LOUD, ("==> PtRequestAdapterSync: Adapt %p, OID %8x\n", pAdapt, Oid));
    do
    {
        pMuxNdisRequest = NdisAllocateMemoryWithTagPriority(pAdapt->BindingHandle, sizeof(MUX_NDIS_REQUEST), MUX_TAG, LowPoolPriority);
        if (pMuxNdisRequest == NULL)
        {
            break;
        }

        pMuxNdisRequest->pVElan = NULL; // internal request

        //
        // Set up completion routine.
        //
        pMuxNdisRequest->pCallback = PtCompleteBlockingRequest;
        NdisInitializeEvent(&pMuxNdisRequest->Event);
        
        pMuxNdisRequest->Request.Header.Type = NDIS_OBJECT_TYPE_OID_REQUEST;
        pMuxNdisRequest->Request.Header.Revision = NDIS_OID_REQUEST_REVISION_1;
        pMuxNdisRequest->Request.Header.Size = sizeof(NDIS_OID_REQUEST);

        pMuxNdisRequest->Request.RequestType = RequestType;
        pMuxNdisRequest->Request.DATA.QUERY_INFORMATION.Oid = Oid;
        pMuxNdisRequest->Request.DATA.QUERY_INFORMATION.InformationBuffer =
                            InformationBuffer;
        pMuxNdisRequest->Request.DATA.QUERY_INFORMATION.InformationBufferLength =
                                                InformationBufferLength;
        
        NdisAcquireSpinLock(&pAdapt->Lock);

        pAdapt->OutstandingRequests ++;
        
        if ((pAdapt->Flags & MUX_BINDING_CLOSING)== MUX_BINDING_CLOSING)
        {
            Status = NDIS_STATUS_CLOSING;

            NdisReleaseSpinLock(&pAdapt->Lock);            
        }
        else
        {
            NdisReleaseSpinLock(&pAdapt->Lock);
            Status = NdisOidRequest(pAdapt->BindingHandle,
                                &pMuxNdisRequest->Request);            
        }

        if (Status != NDIS_STATUS_PENDING)
        {
            NdisAcquireSpinLock(&pAdapt->Lock);
            pAdapt->OutstandingRequests --;
                    
            if ((pAdapt->OutstandingRequests == 0) && (pAdapt->CloseEvent != NULL))
            {
                NdisSetEvent(pAdapt->CloseEvent);
                pAdapt->CloseEvent = NULL;
            }
            NdisReleaseSpinLock(&pAdapt->Lock);
        }
        else        
        {
            NdisWaitEvent(&pMuxNdisRequest->Event, 0);
            Status = pMuxNdisRequest->Status;
        }        
    }
    while (FALSE);

    if (NULL != pMuxNdisRequest)
    {
        NdisFreeMemory(pMuxNdisRequest, sizeof(MUX_NDIS_REQUEST), 0);
    }

    DBGPRINT(MUX_LOUD, ("<== PtRequestAdapterSync: Adapt %p, OID %8x, Status %8x\n", pAdapt, Oid, Status));
}



VOID
PtRequestAdapterAsync(
    IN  PADAPT                      pAdapt,
    IN  NDIS_REQUEST_TYPE           RequestType,
    IN  NDIS_OID                    Oid,
    IN  PVOID                       InformationBuffer,
    IN  ULONG                       InformationBufferLength,
    IN  PMUX_REQ_COMPLETE_HANDLER   pCallback
    )
/*++

Routine Description:

    Utility routine to query the adapter for a single OID value.
    This completes asynchronously, i.e. the calling thread is
    not blocked until the request completes.

Arguments:

    pAdapt                      Pointer to the adapter
    RequestType                 NDIS request type
    Oid                         OID to set/query
    InformationBuffer           Input/output buffer
    InformationBufferLength     Length of the above
    pCallback                   Function to call on request completion

Return Value:

    None.

--*/
{
    PMUX_NDIS_REQUEST       pMuxNdisRequest = NULL;
    PNDIS_OID_REQUEST       pNdisRequest;
    NDIS_STATUS             Status = NDIS_STATUS_FAILURE;

    DBGPRINT(MUX_LOUD, ("==> PtRequestAdapterAsync: Adapt %p, OID %8x\n", pAdapt, Oid));
    do
    {
        pMuxNdisRequest = NdisAllocateMemoryWithTagPriority(pAdapt->BindingHandle, sizeof(MUX_NDIS_REQUEST), MUX_TAG, LowPoolPriority);
        if (pMuxNdisRequest == NULL)
        {
            break;
        }

        pMuxNdisRequest->pVElan = NULL; // internal request

        //
        // Set up completion routine.
        //
        pMuxNdisRequest->pCallback = pCallback;

        pNdisRequest = &pMuxNdisRequest->Request;

        pNdisRequest->RequestType = RequestType;
        pNdisRequest->Header.Type = NDIS_OBJECT_TYPE_OID_REQUEST;
        pNdisRequest->Header.Revision = NDIS_OID_REQUEST_REVISION_1;
        pNdisRequest->Header.Size = sizeof(NDIS_OID_REQUEST);

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
                ASSERT(FALSE);
                break;
        }
         
        NdisAcquireSpinLock(&pAdapt->Lock);

        pAdapt->OutstandingRequests ++;
        
        if ((pAdapt->Flags & MUX_BINDING_CLOSING)== MUX_BINDING_CLOSING)
        {
            NdisReleaseSpinLock(&pAdapt->Lock);        
            Status = NDIS_STATUS_CLOSING;           
        }
        else
        {
            NdisReleaseSpinLock(&pAdapt->Lock);        
            Status = NdisOidRequest(
                        pAdapt->BindingHandle,
                        pNdisRequest);
        }
        
        if (Status != NDIS_STATUS_PENDING)
        {
            PtRequestComplete(
                (NDIS_HANDLE)pAdapt,
                pNdisRequest,
                Status);
        }
    }
    while (FALSE);

    DBGPRINT(MUX_LOUD, ("<== PtRequestAdapterAsync: Adapt %p, OID %8x, Status %8x\n", pAdapt, Oid, Status));
}



VOID
PtCloseAdapter(
    IN PADAPT pAdapt
    )
/*++

Routine Description:

    Call either when the protocol is unbinding or the miniport is halting to set 
    the packet filters back to zero and multicast filter back to zero

Arguments:

     pAdapter            Pointer to a virtual adapter
     
Return Value:
    None

--*/

{
    ULONG                   PacketFilter = 0;
    PVOID                   MCastBuf = NULL;
    ULONG                   MCastBufSize = 0;
    NDIS_STATUS             Status;
    NDIS_EVENT              CloseEvent;

    DBGPRINT(MUX_LOUD, ("==> PtCloseAdapter: Adapt %p\n", pAdapt));
    
    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // Clear out the packet filter and multicast list before unbinding
    // from the adapter below is required for NDIS 6.0 protocols
    //
    PtRequestAdapterSync(pAdapt,
                            NdisRequestSetInformation, 
                            OID_GEN_CURRENT_PACKET_FILTER,
                            &PacketFilter, 
                            sizeof(PacketFilter));

    PtRequestAdapterSync(pAdapt,
                            NdisRequestSetInformation, 
                            OID_802_3_MULTICAST_LIST,
                            MCastBuf, 
                            MCastBufSize);
    //
    // Stop sending requests and wait for outstanding requests to complete
    //
    NdisAcquireSpinLock(&pAdapt->Lock);
    pAdapt->Flags |= MUX_BINDING_CLOSING;

    ASSERT(pAdapt->CloseEvent == NULL);

    if (pAdapt->OutstandingRequests != 0)
    {
         NdisInitializeEvent(&CloseEvent);
         pAdapt->CloseEvent = &CloseEvent;                
         NdisReleaseSpinLock(&pAdapt->Lock);
         NdisWaitEvent(&CloseEvent, 0);                
         NdisAcquireSpinLock(&pAdapt->Lock);
    }
    NdisReleaseSpinLock(&pAdapt->Lock);
    //
    // Now Close the binding with the adapter below
    //
    
    NdisResetEvent(&pAdapt->Event);

    Status = NdisCloseAdapterEx(pAdapt->BindingHandle);

    if (Status == NDIS_STATUS_PENDING)
    {
        //
        // Wait for it to complete.
        //
        NdisWaitEvent(&pAdapt->Event, 0);
    }

    pAdapt->BindingHandle = NULL;
    
    DBGPRINT(MUX_LOUD, ("<== PtCloseAdapter: Adapt %p\n", pAdapt));
}

NDIS_STATUS
PtUnbindAdapter(
    IN  NDIS_HANDLE             UnbindContext,
    IN  NDIS_HANDLE             ProtocolBindingContext
    )
/*++

Routine Description:

    Called by NDIS when we are required to unbind to the adapter below.
    Go through all VELANs on the adapter and shut them down.

Arguments:

    Status                    Placeholder for return status
    ProtocolBindingContext    Pointer to the adapter structure
    UnbindContext             Context for NdisUnbindComplete() if this pends

Return Value:

    Status from closing the binding.

--*/
{
    PADAPT          pAdapt =(PADAPT)ProtocolBindingContext;
    PLIST_ENTRY     p;
    PVELAN          pVElan = NULL;
    LOCK_STATE      LockState;
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(UnbindContext);
	
    DBGPRINT(MUX_LOUD, ("==> PtUnbindAdapter: Adapt %p\n", pAdapt));

    //
    // Stop all VELANs associated with the adapter.
    // Repeatedly find the first unprocessed VELAN on
    // the adapter, mark it, and stop it.
    //
    MUX_ACQUIRE_ADAPT_READ_LOCK(pAdapt, &LockState);

    do
    {
        for (p = pAdapt->VElanList.Flink;
             p != &pAdapt->VElanList;
             p = p->Flink)
        {
            pVElan = CONTAINING_RECORD(p, VELAN, Link);
            if (!pVElan->DeInitializing)
            {
                pVElan->DeInitializing = TRUE;
                break;
            }
        }

        if (p != &pAdapt->VElanList)
        {
            ASSERT(pVElan == CONTAINING_RECORD(p, VELAN, Link));

            //
            // Got a VELAN to stop. Add a temp ref
            // so that the VELAN won't go away when
            // we release the ADAPT lock below.
            //
            PtReferenceVElan(pVElan, (PUCHAR)"UnbindTemp");

            //
            // Release the read lock because we want to
            // run StopVElan at passive IRQL.
            //
            MUX_RELEASE_ADAPT_READ_LOCK(pAdapt, &LockState);
    
            PtStopVElan(pVElan);
    
            PtDereferenceVElan(pVElan, (PUCHAR)"UnbindTemp");

            MUX_ACQUIRE_ADAPT_READ_LOCK(pAdapt, &LockState);
        }
        else
        {
            //
            // No unmarked VELAN, so exit.
            //
            break;
        }
    }
    while (TRUE);

    //
    // Wait until all VELANs are unlinked from the adapter.
    // This is so that we don't attempt to forward down packets
    // and/or requests from VELANs after calling NdisCloseAdapter.
    //
    while (!IsListEmpty(&pAdapt->VElanList))
    {
        MUX_RELEASE_ADAPT_READ_LOCK(pAdapt, &LockState);

        DBGPRINT(MUX_INFO, ("PtUnbindAdapter: pAdapt %p, VELANlist not yet empty\n",
                    pAdapt));

        NdisMSleep(2000);

        MUX_ACQUIRE_ADAPT_READ_LOCK(pAdapt, &LockState);
    }

    MUX_RELEASE_ADAPT_READ_LOCK(pAdapt, &LockState);

    //
    // Close the binding to the lower adapter.
    //
    if (pAdapt->BindingHandle != NULL)
    {
        PtCloseAdapter(pAdapt);
    }
    else
    {
        //
        // Binding Handle should not be NULL.
        //
        Status = NDIS_STATUS_FAILURE;
        ASSERT(0);
    }

    //
    // Remove the adapter from the global AdapterList
    //
    
    MUX_ACQUIRE_MUTEX(&GlobalMutex);

    RemoveEntryList(&pAdapt->Link);

    MUX_RELEASE_MUTEX(&GlobalMutex);

    NdisFreeSpinLock(&pAdapt->Lock);
    
    //
    // Free all the resources associated with this Adapter except the
    // ADAPT struct itself, because that will be freed by 
    // PtDereferenceAdapter call when the reference drops to zero. 
    // Note: Every VELAN associated with this Adapter takes a ref count
    // on it. So the adapter memory wouldn't be freed until all the VELANs
    // are shutdown. 
    //
    
    PtDereferenceAdapter(pAdapt, (PUCHAR)"Unbind");

    DBGPRINT(MUX_LOUD, ("<== PtUnbindAdapter: Adapt %p, Status=%08lx\n", pAdapt, Status));

    return Status;
}



VOID
PtCloseAdapterComplete(
    IN NDIS_HANDLE            ProtocolBindingContext
    )
/*++

Routine Description:

    Completion for the CloseAdapter call.

Arguments:

    ProtocolBindingContext    Pointer to the adapter structure
   
Return Value:

    None.

--*/
{
    PADAPT      pAdapt =(PADAPT)ProtocolBindingContext;

    DBGPRINT(MUX_LOUD, ("==> PtCloseAdapterComplete: Adapt %p\n", 
                                pAdapt));

    NdisSetEvent(&pAdapt->Event);

    DBGPRINT(MUX_LOUD, ("<== PtCloseAdapterComplete: Adapt %p\n", 
                                pAdapt));
}


VOID
PtRequestComplete(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  PNDIS_OID_REQUEST           NdisRequest,
    IN  NDIS_STATUS                 Status
    )
/*++

Routine Description:

    Completion handler for an NDIS request sent to a lower
    miniport.

Arguments:

    ProtocolBindingContext    Pointer to the adapter structure
    NdisRequest               The completed request, must be an element of an instance of MUX_NDIS_REQUEST
    Status                    Completion status

Return Value:

    None

--*/
{
    PADAPT              pAdapt = (PADAPT)ProtocolBindingContext;
    PMUX_NDIS_REQUEST   pMuxNdisRequest;


    DBGPRINT(MUX_LOUD, ("==> PtRequestComplete: Adapt %p, Request %p, Status %8x\n", 
                                pAdapt, NdisRequest, Status));

    //get the Super-structure for NDIS_REQUEST before getting the callback functions
    //so make sure NdisRequest is a filled into a MUX_NDIS_REQUEST before using this function
    pMuxNdisRequest = CONTAINING_RECORD(NdisRequest, MUX_NDIS_REQUEST, Request);
   
    ASSERT(pMuxNdisRequest->pCallback != NULL);
    

    //
    // Completion is handled by the callback routine:
    //
    (*pMuxNdisRequest->pCallback)(pAdapt, 
                                  pMuxNdisRequest,
                                  Status);

    NdisAcquireSpinLock(&pAdapt->Lock);
    
    pAdapt->OutstandingRequests --;
            
    if ((pAdapt->OutstandingRequests == 0) && (pAdapt->CloseEvent != NULL))
    {
        NdisSetEvent(pAdapt->CloseEvent);
        pAdapt->CloseEvent = NULL;
    }

    NdisReleaseSpinLock(&pAdapt->Lock);

    DBGPRINT(MUX_LOUD, ("<== PtRequestComplete: Adapt %p, Request %p, Status %8x\n", 
                                pAdapt, NdisRequest, Status));

}


VOID
PtCompleteForwardedRequest(
    IN PADAPT                       pAdapt,
    IN PMUX_NDIS_REQUEST            pMuxNdisRequest,
    IN NDIS_STATUS                  Status
    )
/*++

Routine Description:

    Handle completion of an NDIS request that was originally
    submitted to our VELAN miniport and was forwarded down
    to the lower binding.

    We do some postprocessing, to cache the results of
    certain queries.

Arguments:

    pAdapt  - Adapter on which the request was forwarded
    pMuxNdisRequest - super-struct for request
    Status - request completion status

Return Value:

    None

--*/
{
    PVELAN              pVElan = NULL;
    PNDIS_OID_REQUEST   pNdisRequest = &pMuxNdisRequest->Request;
    NDIS_OID            Oid;
    PNDIS_OID_REQUEST   OrigRequest = NULL;
    BOOLEAN             fCompleteRequest = FALSE;

    UNREFERENCED_PARAMETER(pAdapt);
   

    DBGPRINT(MUX_LOUD, ("==> PtCompleteForwardedRequest: Adapt %p, MuxRequest %p, Status %8x\n", 
                                pAdapt, pMuxNdisRequest, Status));
    //
    // Get the originating VELAN. The VELAN will not be dereferenced
    // away until the pended request is completed.
    //
    pVElan = pMuxNdisRequest->pVElan;

    ASSERT(pVElan != NULL);
    ASSERT(pMuxNdisRequest == &pVElan->Request);
    
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DBGPRINT(MUX_WARN, ("PtCompleteForwardedRequest: pVElan %p, OID %x, Status %x\n", 
                    pVElan,
                    pMuxNdisRequest->Request.DATA.QUERY_INFORMATION.Oid,
                    Status));
    }

    NdisAcquireSpinLock(&pVElan->Lock);

    pMuxNdisRequest->Refcount --;
    if (pMuxNdisRequest->Refcount == 0)
    {
        fCompleteRequest = TRUE;
        OrigRequest = pMuxNdisRequest->OrigRequest;
        pMuxNdisRequest->OrigRequest = NULL;
    }
    

    NdisReleaseSpinLock(&pVElan->Lock);    

    if (fCompleteRequest == FALSE)
    {
        return;
    }

    //
    // Complete the original request.
    //
    switch (pNdisRequest->RequestType)
    {
        case NdisRequestQueryInformation:
        case NdisRequestQueryStatistics:

            OrigRequest->DATA.QUERY_INFORMATION.BytesWritten = 
                    pNdisRequest->DATA.QUERY_INFORMATION.BytesWritten;
            OrigRequest->DATA.QUERY_INFORMATION.BytesNeeded = 
                    pNdisRequest->DATA.QUERY_INFORMATION.BytesNeeded;

            //
            // Before completing the request, do any necessary
            // post-processing.
            //
            Oid = pNdisRequest->DATA.QUERY_INFORMATION.Oid;
            if (Status == NDIS_STATUS_SUCCESS)
            {
                if (Oid == OID_GEN_LINK_SPEED)
                {
                    NdisMoveMemory (&pVElan->LinkSpeed,
                                    pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
                                    sizeof(ULONG));
                }
                else if (Oid == OID_PNP_CAPABILITIES)
                {
                    PtPostProcessPnPCapabilities(pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
                                                 pNdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength);
                }
            }

            break;

        case NdisRequestSetInformation:

            OrigRequest->DATA.SET_INFORMATION.BytesRead=
                    pNdisRequest->DATA.SET_INFORMATION.BytesRead;
            OrigRequest->DATA.QUERY_INFORMATION.BytesNeeded=
                    pNdisRequest->DATA.SET_INFORMATION.BytesNeeded;

#if IEEE_VELAN_SUPPORT            
            if ((pNdisRequest->DATA.SET_INFORMATION.Oid == OID_GEN_CURRENT_LOOKAHEAD) 
                    && (pVElan->RestoreLookaheadSize == TRUE))
            {
                pVElan->RestoreLookaheadSize = FALSE;
                *(UNALIGNED PULONG)(Request->DATA.SET_INFORMATION.InformationBuffer) -= VLAN_TAG_HEADER_SIZE;
            }
#endif
            //
            // Before completing the request, cache relevant information
            // in our structure.
            //
            if (Status == NDIS_STATUS_SUCCESS)
            {
                Oid = pNdisRequest->DATA.SET_INFORMATION.Oid;
                switch (Oid)
                {
                    case OID_GEN_CURRENT_LOOKAHEAD:
                        
                        
                        NdisMoveMemory(&pVElan->LookAhead,
                                 pNdisRequest->DATA.SET_INFORMATION.InformationBuffer,
                                 sizeof(ULONG));
                        break;

                    default:
                        break;
                }
            }

            break;

        default:
            ASSERT(FALSE);
            break;
    }

    NdisMOidRequestComplete(pVElan->MiniportAdapterHandle,OrigRequest,Status);

    MUX_DECR_PENDING_SENDS(pVElan);

    DBGPRINT(MUX_LOUD, ("<== PtCompleteForwardedRequest: Adapt %p, MuxRequest %p, Status %8x\n", 
                                pAdapt, pMuxNdisRequest, Status));
}



VOID
PtPostProcessPnPCapabilities(
    IN PVOID                    InformationBuffer,
    IN ULONG                    InformationBufferLength
    )
/*++

Routine Description:

    Postprocess a successfully completed query for OID_PNP_CAPABILITIES.
    We modify the returned information slightly before completing
    it to the VELAN above.

Arguments:

    InformationBuffer - points to buffer for the OID
    InformationBufferLength - byte length of the above.

Return Value:

    None

--*/
{
    PNDIS_PNP_CAPABILITIES          pPNPCapabilities;
    PNDIS_PM_WAKE_UP_CAPABILITIES   pPMstruct;

    DBGPRINT(MUX_LOUD, ("==> PtPostProcessPnPCapabilities\n"));
    
    if (InformationBufferLength >= sizeof(NDIS_PNP_CAPABILITIES))
    {
        pPNPCapabilities = (PNDIS_PNP_CAPABILITIES)InformationBuffer;

        //
        // The following fields must be overwritten by an IM driver.
        //
        pPMstruct= &pPNPCapabilities->WakeUpCapabilities;
        pPMstruct->MinMagicPacketWakeUp = NdisDeviceStateUnspecified;
        pPMstruct->MinPatternWakeUp = NdisDeviceStateUnspecified;
        pPMstruct->MinLinkChangeWakeUp = NdisDeviceStateUnspecified;
    }
    DBGPRINT(MUX_LOUD, ("<== PtPostProcessPnPCapabilities\n"));
}

VOID
PtCompleteBlockingRequest(
    IN PADAPT                   pAdapt,
    IN PMUX_NDIS_REQUEST        pMuxNdisRequest,
    IN NDIS_STATUS              Status
    )
/*++

Routine Description:

    Handle completion of an NDIS request that was originated
    by this driver and the calling thread is blocked waiting
    for completion.

Arguments:

    pAdapt  - Adapter on which the request was forwarded
    pMuxNdisRequest - super-struct for request
    Status - request completion status

Return Value:

    None

--*/
{
	UNREFERENCED_PARAMETER(pAdapt);

    DBGPRINT(MUX_LOUD, ("==> PtCompleteBlockingRequest: Adapt %p, MuxRequest %p, Status %8x\n", 
                                pAdapt, pMuxNdisRequest, Status));
    
    //
    // The request was originated from this driver. Wake up the
    // thread blocked for its completion.
    //
    pMuxNdisRequest->Status = Status;
    NdisSetEvent(&pMuxNdisRequest->Event);

    DBGPRINT(MUX_LOUD, ("<== PtCompleteBlockingRequest: Adapt %p, MuxRequest %p, Status %8x\n", 
                                pAdapt, pMuxNdisRequest, Status));
}


VOID
PtDiscardCompletedRequest(
    IN PADAPT                   pAdapt,
    IN PMUX_NDIS_REQUEST        pMuxNdisRequest,
    IN NDIS_STATUS              Status
    )
/*++

Routine Description:

    Handle completion of an NDIS request that was originated
    by this driver - the request is to be discarded.

Arguments:

    pAdapt  - Adapter on which the request was forwarded
    pMuxNdisRequest - super-struct for request
    Status - request completion status

Return Value:

    None

--*/
{
    UNREFERENCED_PARAMETER(pAdapt);
    UNREFERENCED_PARAMETER(Status);

    NdisFreeMemory(pMuxNdisRequest, sizeof(MUX_NDIS_REQUEST), 0);
}


VOID
PtStatus(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  PNDIS_STATUS_INDICATION     StatusIndication
    )
/*++

Routine Description:

    Handle a status indication on the lower binding (ADAPT).
    If this is a media status indication, we also pass this
    on to all associated VELANs.

Arguments:

    ProtocolBindingContext      Pointer to the adapter structure
    GeneralStatus               Status code
    StatusBuffer                Status buffer
    StatusBufferSize            Size of the status buffer

Return Value:

    None

--*/
{
    PADAPT      pAdapt = (PADAPT)ProtocolBindingContext;
    PLIST_ENTRY p;
    PVELAN      pVElan;
    LOCK_STATE  LockState;
    NDIS_STATUS GeneralStatus = StatusIndication->StatusCode;
    NDIS_STATUS_INDICATION     NewStatusIndication;

    DBGPRINT(MUX_LOUD, ("==> PtStatus: Adapt %p, Status %x\n", pAdapt, GeneralStatus));

    do
    {

        //
        // Ignore status indications that we aren't going
        // to pass up.
        //
        if ((GeneralStatus != NDIS_STATUS_LINK_STATE)
            )
        {
            break;
        }
        
        MUX_ACQUIRE_ADAPT_READ_LOCK(pAdapt, &LockState);

        if (GeneralStatus == NDIS_STATUS_LINK_STATE)
        {
            pAdapt->LastIndicatedLinkState = *((PNDIS_LINK_STATE)(StatusIndication->StatusBuffer));
        }
        for (p = pAdapt->VElanList.Flink;
             p != &pAdapt->VElanList;
             p = p->Flink)
        {
            
            pVElan = CONTAINING_RECORD(p, VELAN, Link);

            MUX_INCR_PENDING_RECEIVES(pVElan);

            //
            // Should the indication be sent on this VELAN?
            //
            if ((pVElan->MiniportInitPending) ||
                (pVElan->MiniportHalting) ||
                (pVElan->MiniportAdapterHandle == NULL) ||   
                MUX_IS_LOW_POWER_STATE(pVElan->MPDevicePowerState))
            {
                MUX_DECR_PENDING_RECEIVES(pVElan);
                if (MUX_IS_LOW_POWER_STATE(pVElan->MPDevicePowerState))
                {
                    //
                    // Keep track of the lastest status to indicated when VELAN power is on
                    // 
                    ASSERT(GeneralStatus == NDIS_STATUS_LINK_STATE);

                    pVElan->LatestUnIndicateStatus = GeneralStatus;

                    if (GeneralStatus == NDIS_STATUS_LINK_STATE)
                    {
                        pVElan->LatestUnIndicateLinkState = *(PNDIS_LINK_STATE)StatusIndication->StatusBuffer;
                    }
                }
                
                continue;
            }

            //
            // Save the last indicated status when 
            pVElan->LastIndicatedStatus = GeneralStatus;
            if (GeneralStatus == NDIS_STATUS_LINK_STATE)
            {
                pVElan->LastIndicatedLinkState = *(PNDIS_LINK_STATE)StatusIndication->StatusBuffer;
            }
            //
            // Allocate a new status indication and set the destination handle to null to ensure that the status
            // is indicated to protocols bound to mux velans. Copy only the fields that makes sense to pass up. For 
            // instance, do not copy PortNumber, RequestId, Flags, Guid, NdisReserved. Port Number is really not   
            // necessary to copy and pass up for the scenario because the port number is an entity that makes sense  
            // between the protocol and the underlying miniport pair.
            //         
        
            NdisZeroMemory(&NewStatusIndication, sizeof(NDIS_STATUS_INDICATION));
            
            NewStatusIndication.Header.Type = NDIS_OBJECT_TYPE_STATUS_INDICATION;
            NewStatusIndication.Header.Revision = NDIS_STATUS_INDICATION_REVISION_1;
            NewStatusIndication.Header.Size = sizeof(NDIS_STATUS_INDICATION);
            
            NewStatusIndication.StatusCode = StatusIndication->StatusCode;
            NewStatusIndication.SourceHandle = pVElan->MiniportAdapterHandle;
            NewStatusIndication.DestinationHandle = NULL;
            
            NewStatusIndication.StatusBuffer = StatusIndication->StatusBuffer;
            NewStatusIndication.StatusBufferSize = StatusIndication->StatusBufferSize;

            NdisMIndicateStatusEx(pVElan->MiniportAdapterHandle, &NewStatusIndication);
            
            //
            // Mark this so that we forward a status complete
            // indication as well.
            //
            pVElan->IndicateStatusComplete = TRUE;

            MUX_DECR_PENDING_RECEIVES(pVElan);
        }

        MUX_RELEASE_ADAPT_READ_LOCK(pAdapt, &LockState);
    }
    while (FALSE);
    
    DBGPRINT(MUX_LOUD, ("<== PtStatus: Adapt %p, Status %x\n", pAdapt, GeneralStatus));
}


BOOLEAN
PtMulticastMatch(
    IN PVELAN                       pVElan,
    IN PUCHAR                       pDstMac
    )
/*++

Routine Description:

    Check if the given multicast destination MAC address matches
    any of the multicast address entries set on the VELAN.

    NOTE: the caller is assumed to hold a READ/WRITE lock
    to the parent ADAPT structure. This is so that the multicast
    list on the VELAN is invariant for the duration of this call.

Arguments:

    pVElan  - VELAN to look in
    pDstMac - Destination MAC address to compare

Return Value:

    TRUE iff the address matches an entry in the VELAN

--*/
{
    ULONG           i;
    UINT            AddrCompareResult;

    for (i = 0; i < pVElan->McastAddrCount; i++)
    {
        ETH_COMPARE_NETWORK_ADDRESSES_EQ(pVElan->McastAddrs[i],
                                         pDstMac,
                                         &AddrCompareResult);
        
        if (AddrCompareResult == 0)
        {
            break;
        }
    }

    return (i != pVElan->McastAddrCount);
}


BOOLEAN
PtMatchPacketToVElan(
    IN PVELAN                       pVElan,
    IN PUCHAR                       pDstMac,
    IN BOOLEAN                      bIsMulticast,
    IN BOOLEAN                      bIsBroadcast
    )
/*++

Routine Description:

    Check if the destination address of a received packet
    matches the receive criteria on the specified VELAN.

    NOTE: the caller is assumed to hold a READ/WRITE lock
    to the parent ADAPT structure.

Arguments:

    pVElan  - VELAN to check on
    pDstMac - Destination MAC address in received packet
    bIsMulticast - is this a multicast address
    bIsBroadcast - is this a broadcast address

Return Value:

    TRUE iff this packet should be received on the VELAN

--*/
{
    UINT            AddrCompareResult;
    ULONG           PacketFilter;
    BOOLEAN         bPacketMatch;

    DBGPRINT(MUX_VERY_LOUD, ("==> PtMatchPacketToVElan: VElan %p\n", pVElan));
    //
    PacketFilter = pVElan->PacketFilter;

    //
    // Handle the directed packet case first.
    //
    if (!bIsMulticast)
    {
        //
        // If the VELAN is not in promisc. mode, check if
        // the destination MAC address matches the local
        // address.
        //
        if ((PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS) == 0)
        {
            ETH_COMPARE_NETWORK_ADDRESSES_EQ(pVElan->CurrentAddress,
                                             pDstMac,
                                             &AddrCompareResult);

            bPacketMatch = ((AddrCompareResult == 0) &&
                           ((PacketFilter & NDIS_PACKET_TYPE_DIRECTED) != 0));
        }
        else
        {
            bPacketMatch = TRUE;
        }
     }
     else
     {
        //
        // Multicast or broadcast packet.
        //

        //
        // Indicate if the filter is set to promisc mode ...
        //
        if ((PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS)
                ||

            //
            // or if this is a broadcast packet and the filter
            // is set to receive all broadcast packets...
            //
            (bIsBroadcast &&
             (PacketFilter & NDIS_PACKET_TYPE_BROADCAST))
                ||

            //
            // or if this is a multicast packet, and the filter is
            // either set to receive all multicast packets, or
            // set to receive specific multicast packets. In the
            // latter case, indicate receive only if the destn
            // MAC address is present in the list of multicast
            // addresses set on the VELAN.
            //
            (!bIsBroadcast &&
             ((PacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST) ||
              ((PacketFilter & NDIS_PACKET_TYPE_MULTICAST) &&
               PtMulticastMatch(pVElan, pDstMac))))
           )
        {
            bPacketMatch = TRUE;
        }
        else
        {
            //
            // No protocols above are interested in this
            // multicast/broadcast packet.
            //
            bPacketMatch = FALSE;
        }
    }
     
    DBGPRINT(MUX_VERY_LOUD, ("<== PtMatchPacketToVElan: VElan %p, PacketMatch %x\n", pVElan, bPacketMatch));
    
    return (bPacketMatch);
}


NDIS_STATUS
PtPnPNetEventSetPower(
    IN PADAPT                       pAdapt,
    IN PNET_PNP_EVENT_NOTIFICATION  pNetPnPEventNotification
    )
/*++
Routine Description:

    This is a notification to our protocol edge of the power state
    of the lower miniport. If it is going to a low-power state, we must
    wait here for all outstanding sends and requests to complete.

Arguments:

    pAdapt - Pointer to the adpater structure
    pNetPnPEvent - The Net Pnp Event. this contains the new device state

Return Value:

    NDIS_STATUS_SUCCESS

--*/
{
    PLIST_ENTRY                 p;
    PVELAN                      pVElan;
    LOCK_STATE                  LockState;
    NDIS_STATUS                 Status;

    //
    // Store the new power state.
    //
    
    pAdapt->PtDevicePowerState = *(PNDIS_DEVICE_POWER_STATE)pNetPnPEventNotification->NetPnPEvent.Buffer;

    DBGPRINT(MUX_LOUD, ("==> PnPNetEventSetPower: Adapt %p, SetPower to %d\n",
            pAdapt, pAdapt->PtDevicePowerState));

    //
    // Check if the miniport below is going to a low power state.
    //
    if (MUX_IS_LOW_POWER_STATE(pAdapt->PtDevicePowerState))
    {
        ULONG       i;

        //
        // It is going to a low power state. Wait for outstanding
        // I/O to complete on the adapter.
        //
        for (i = 0; i < 10000; i++)
        {
            MUX_ACQUIRE_ADAPT_READ_LOCK(pAdapt, &LockState);

            for (p = pAdapt->VElanList.Flink;
                 p != &pAdapt->VElanList;
                 p = p->Flink)
            {
                pVElan = CONTAINING_RECORD(p, VELAN, Link);
                if ((pVElan->OutstandingSends != 0) ||
                    (pVElan->OutstandingReceives != 0))
                {
                    break;
                }
            }

            MUX_RELEASE_ADAPT_READ_LOCK(pAdapt, &LockState);

            if (p == &pAdapt->VElanList)
            {
                //
                // There are no VELANs with pending I/O.
                //
                break;
            }
            
            DBGPRINT(MUX_INFO, ("SetPower: Adapt %p, waiting for pending IO to complete\n",
                                pAdapt));

            NdisMSleep(1000);
        }

    }
    else
    {
        //
        // The device below is powered on. If we had requests
        // pending on any VELANs, send them down now.
        //
        MUX_ACQUIRE_ADAPT_READ_LOCK(pAdapt, &LockState);

        for (p = pAdapt->VElanList.Flink;
             p != &pAdapt->VElanList;
             p = p->Flink)
        {
            pVElan = CONTAINING_RECORD(p, VELAN, Link);

            //
            // Need to make sure other threads do not try to acquire the write lock while holding
            // the same spin lock
            //
            NdisAcquireSpinLock(&pVElan->Lock);
            if (pVElan->QueuedRequest)
            {
                pVElan->QueuedRequest = FALSE;
                NdisReleaseSpinLock(&pVElan->Lock);

                
                NdisAcquireSpinLock(&pAdapt->Lock);
                
                pAdapt->OutstandingRequests ++;
                
                if ((pAdapt->Flags & MUX_BINDING_CLOSING)== MUX_BINDING_CLOSING)
                {
                    NdisReleaseSpinLock(&pAdapt->Lock);        
                    Status = NDIS_STATUS_CLOSING;           
                }
                else
                {
                    NdisReleaseSpinLock(&pAdapt->Lock);        
                    Status = NdisOidRequest(
                                pAdapt->BindingHandle,
                                &pVElan->Request.Request);
                }                
                if (Status != NDIS_STATUS_PENDING)
                {
                    PtRequestComplete(pAdapt,
                                      &pVElan->Request.Request,
                                      Status);
                }
            }
            else
            {
                NdisReleaseSpinLock(&pVElan->Lock);
            }
        }

        MUX_RELEASE_ADAPT_READ_LOCK(pAdapt, &LockState);
    }

    DBGPRINT(MUX_LOUD, ("<== PnPNetEventSetPower: Adapt %p, SetPower to %d\n",
            pAdapt, pAdapt->PtDevicePowerState));
    
    return (NDIS_STATUS_SUCCESS);
}


NDIS_STATUS
PtPNPHandler(
    IN NDIS_HANDLE                 ProtocolBindingContext,
    IN PNET_PNP_EVENT_NOTIFICATION pNetPnPEventNotification
    )

/*++
Routine Description:

    This is called by NDIS to notify us of a PNP event related to a lower
    binding. Based on the event, this dispatches to other helper routines.

Arguments:

    ProtocolBindingContext - Pointer to our adapter structure. Can be NULL
                for "global" notifications

    pNetPnPEvent - Pointer to the PNP event to be processed.

Return Value:

    NDIS_STATUS code indicating status of event processing.

--*/
{
    PADAPT              pAdapt  =(PADAPT)ProtocolBindingContext;
    NDIS_STATUS         Status  = NDIS_STATUS_SUCCESS;
    PLIST_ENTRY         p;
    NDIS_EVENT          PauseEvent;

    DBGPRINT(MUX_LOUD, ("==> PtPnPHandler: Adapt %p, NetPnPEvent %d\n", pAdapt, 
                            pNetPnPEventNotification->NetPnPEvent.NetEvent));

    switch (pNetPnPEventNotification->NetPnPEvent.NetEvent)
    {
        case NetEventSetPower:

            Status = PtPnPNetEventSetPower(pAdapt, pNetPnPEventNotification);
            break;

        case NetEventReconfigure:
            //
            // Rescan configuration and bring up any VELANs that
            // have been newly added. Make sure that the global
            // adapter list is undisturbed while we traverse it.
            //
            MUX_ACQUIRE_MUTEX(&GlobalMutex);

            for (p = AdapterList.Flink;
                 p != &AdapterList;
                 p = p->Flink)
            {
                pAdapt = CONTAINING_RECORD(p, ADAPT, Link);

                PtBootStrapVElans(pAdapt, NULL);
            }

            MUX_RELEASE_MUTEX(&GlobalMutex);
                
            Status = NDIS_STATUS_SUCCESS;
            break;
        case NetEventIMReEnableDevice:
            MUX_ACQUIRE_MUTEX(&GlobalMutex);

            for (p = AdapterList.Flink;
                 p != &AdapterList;
                 p = p->Flink)
            {
                pAdapt = CONTAINING_RECORD(p, ADAPT, Link);

                PtBootStrapVElans(pAdapt, pNetPnPEventNotification->NetPnPEvent.Buffer);
            }
            
            
            MUX_RELEASE_MUTEX(&GlobalMutex);

            Status = NDIS_STATUS_SUCCESS;
            break;


        case NetEventPause:
            NdisAcquireSpinLock(&pAdapt->Lock);
            pAdapt->BindingState = MuxAdapterBindingPausing;

            ASSERT(pAdapt->PauseEvent == NULL);
            
            if (pAdapt->OutstandingSends != 0)
            {
                NdisInitializeEvent(&PauseEvent);
                
                pAdapt->PauseEvent = &PauseEvent;
                
                NdisReleaseSpinLock(&pAdapt->Lock);

                NdisWaitEvent(&PauseEvent, 0);
                
                NdisAcquireSpinLock(&pAdapt->Lock);
            }
            
            pAdapt->BindingState = MuxAdapterBindingPaused;
            NdisReleaseSpinLock(&pAdapt->Lock);
            
            Status = NDIS_STATUS_SUCCESS;
            break;

        case NetEventRestart:
            pAdapt->BindingState = MuxAdapterBindingRunning;
            Status = NDIS_STATUS_SUCCESS;
            break;
          
 
        default:
            Status = NDIS_STATUS_SUCCESS;

            break;
    }

    DBGPRINT(MUX_LOUD, ("<== PtPnPHandler: Adapt %p, NetPnPEvent %d, Status %8x\n", pAdapt, 
                            pNetPnPEventNotification->NetPnPEvent.NetEvent, Status));
    return Status;
}

NDIS_STATUS
PtCreateAndStartVElan(
    IN  PADAPT                      pAdapt,
    IN  PNDIS_STRING                pVElanKey
)
/*++

Routine Description:

    Create and start a VELAN with the given key name. Check if a VELAN
    with this key name already exists; if so do nothing.

    ASSUMPTION: this is called from either the BindAdapter handler for
    the underlying adapter, or from the PNP reconfig handler. Both these
    routines are protected by NDIS against pre-emption by UnbindAdapter.
    If this routine will be called from any other context, it should
    be protected against a simultaneous call to our UnbindAdapter handler.
    
Arguments:

    pAdapt        - Pointer to Adapter structure
    pVElanKey     - Points to a Unicode string naming the VELAN to create. 
    
Return Value:

    NDIS_STATUS_SUCCESS if we either found a duplicate VELAN or
    successfully initiated a new ELAN with the given key.

    NDIS_STATUS_XXX error code otherwise (failure initiating a new VELAN).

--*/
{
    NDIS_STATUS             Status;
    PVELAN                  pVElan;
    
    Status = NDIS_STATUS_SUCCESS;
    pVElan = NULL;

    DBGPRINT(MUX_LOUD, ("=> PtCreateAndStartVElan: Adapter %p, ElanKey %ws\n", 
                            pAdapt, pVElanKey->Buffer));

    do
    {
        //
        //  Weed out duplicates.
        //
        if (pVElanKey != NULL)
        {

            pVElan = PtFindVElan(pAdapt, pVElanKey);

            if (NULL != pVElan)
            {
                //
                // Duplicate - bail out silently.
                //
                DBGPRINT(MUX_WARN, ("CreateElan: found duplicate pVElan %p\n", pVElan));

                Status = NDIS_STATUS_SUCCESS;
                pVElan = NULL;
                break;
            }
        }
        
        pVElan = NULL;
        
        if (pVElanKey != NULL)
        {
            pVElan = PtAllocateAndInitializeVElan(pAdapt, pVElanKey);
        }

        if (pVElan == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }
        //
        // Request NDIS to initialize the virtual miniport. Set
        // the flag below just in case an unbind occurs before
        // MiniportInitialize is called.
        //
        PtReferenceVElan(pVElan,(UCHAR*) "CreatVelan");
        pVElan->MiniportInitPending = TRUE;
        NdisInitializeEvent(&pVElan->MiniportInitEvent);

        Status = NdisIMInitializeDeviceInstanceEx(DriverHandle,
                                                  &pVElan->CfgDeviceName,
                                                  pVElan);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            if (pVElan->MiniportHalting == FALSE)
            {
                PtUnlinkVElanFromAdapter(pVElan);   // IMInit failed
            }
            PtDereferenceVElan(pVElan, (UCHAR*) "CreatVelan");
            pVElan = NULL;
            break;
        }
        
        PtDereferenceVElan(pVElan,(UCHAR*) "CreatVelan");
    
    }
    while (FALSE);

    DBGPRINT(MUX_LOUD, ("<== PtCreateAndStartVElan: Adapter %p, VELAN %p, Status %8x\n", pAdapt, pVElan, Status));

    return Status;
}


PVELAN
PtAllocateAndInitializeVElan(
    IN PADAPT                       pAdapt,
    IN PNDIS_STRING                 pVElanKey
    )
/*++

Routine Description:

    Allocates and initializes a VELAN structure. Also links it to
    the specified ADAPT.

Arguments:

    pAdapt - Adapter to link VELAN to
    pVElanKey - Key to the VELAN

Return Value:

    Pointer to VELAN structure if successful, NULL otherwise.

--*/
{
    PVELAN          pVElan;
    ULONG           Length;
    NDIS_STATUS     Status;
    LOCK_STATE      LockState;

    DBGPRINT(MUX_LOUD, ("==> PtCreateAndStartVElan: Adapter %p, VELAN Key %ws\n", pAdapt, pVElanKey->Buffer));
    pVElan = NULL;
    Status = NDIS_STATUS_SUCCESS;

    do
    {
        Length = sizeof(VELAN) + pVElanKey->Length + sizeof(WCHAR);
        
        //
        // Allocate a VELAN data structure.
        //
        pVElan = NdisAllocateMemoryWithTagPriority(pAdapt->BindingHandle, Length, MUX_TAG, LowPoolPriority);
        if (pVElan == NULL)
        {
            DBGPRINT(MUX_FATAL, ("AllocateVElan: Failed to allocate %d bytes for VELAN\n",
                                 Length));
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        // Initialize it.
        //
        NdisZeroMemory(pVElan, Length);
        NdisInitializeListHead(&pVElan->Link);
        NdisInitializeListHead(&pVElan->GlobalLink);
        
        //
        // Initialize the built-in request structure to signify
        // that it is used to forward NDIS requests.
        //
        pVElan->Request.pVElan = pVElan;
        NdisInitializeEvent(&pVElan->Request.Event);
       
        //
        // Store in the key name.
        //
        pVElan->CfgDeviceName.Length = 0;
        pVElan->CfgDeviceName.Buffer = (PWCHAR)((PUCHAR)pVElan + 
                    sizeof(VELAN));       
        pVElan->CfgDeviceName.MaximumLength = 
                pVElanKey->Length + sizeof(WCHAR);
        (VOID)NdisUpcaseUnicodeString(&pVElan->CfgDeviceName, pVElanKey);
        pVElan->CfgDeviceName.Buffer[pVElanKey->Length/sizeof(WCHAR)] =
                        ((WCHAR)0);

        // 
        // Initialize LastIndicatedStatus to media connect
        //
        pVElan->LastIndicatedStatus = NDIS_STATUS_LINK_STATE;

        //
        // Set power state of virtual miniport to D0.
        //
        pVElan->MPDevicePowerState = NdisDeviceStateD0;

        //
        // Cache the binding handle for quick reference.
        //
        pVElan->BindingHandle = pAdapt->BindingHandle;
        pVElan->pAdapt = pAdapt;

        //
        // Copy in some adapter parameters.
        //
        pVElan->LookAhead = pAdapt->BindParameters.LookaheadSize;
        pVElan->LinkSpeed = pAdapt->BindParameters.RcvLinkSpeed;
        ASSERT(pAdapt->BindParameters.MacAddressLength == 6);

        if (pAdapt->BindParameters.MacAddressLength == 6)
        {

            NdisMoveMemory(pVElan->PermanentAddress,
                           &pAdapt->BindParameters.CurrentMacAddress,
                           pAdapt->BindParameters.MacAddressLength);

            NdisMoveMemory(pVElan->CurrentAddress,
                           &pAdapt->BindParameters.CurrentMacAddress,
                           pAdapt->BindParameters.MacAddressLength);
        }
        else
        {
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        DBGPRINT(MUX_LOUD, ("Alloced VELAN %p, MAC addr %s\n",
                    pVElan, MacAddrToString(pVElan->CurrentAddress)));

        NdisAllocateSpinLock(&pVElan->Lock);

        NdisAllocateSpinLock(&pVElan->PauseLock);



#ifdef IEEE_VLAN_SUPPORT
        //
        // Allocate lookaside list for tag headers.
        // 
        NdisInitializeNPagedLookasideList (
                &pVElan->TagLookaside,
                NULL,
                NULL,
                0,
                ETH_HEADER_SIZE + VLAN_TAG_HEADER_SIZE + sizeof(IM_SEND_NB_ENTRY),
                MUX_TAG,
                0);

#endif
        //
        // Finally link this VELAN to the Adapter's VELAN list. 
        //
        PtReferenceVElan(pVElan, (PUCHAR)"adapter");        

        MUX_ACQUIRE_ADAPT_WRITE_LOCK(pAdapt, &LockState);

        PtReferenceAdapter(pAdapt, (PUCHAR)"VElan");
        InsertTailList(&pAdapt->VElanList, &pVElan->Link);

        pAdapt->VElanCount++;
        pVElan->VElanNumber = NdisInterlockedIncrement((PLONG)&NextVElanNumber);

        MUX_RELEASE_ADAPT_WRITE_LOCK(pAdapt, &LockState);

        NdisAcquireSpinLock(&GlobalLock);
        InsertTailList(&VElanList, &pVElan->GlobalLink);   
        NdisReleaseSpinLock(&GlobalLock);;
    }
    while (FALSE);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        if (pVElan)
        {
            PtDeallocateVElan(pVElan);
            pVElan = NULL;
        }
    }
    DBGPRINT(MUX_LOUD, ("<== PtCreateAndStartVElan: Adapter %p, VELAN Key %ws, VElan %p\n", pAdapt, pVElanKey->Buffer, pVElan));
    return (pVElan);
}


VOID
PtDeallocateVElan(
    IN PVELAN                   pVElan
    )
/*++

Routine Description:

    Free up all resources allocated to a VELAN, and then the VELAN
    structure itself.

Arguments:

    pVElan - Pointer to VELAN to be deallocated.

Return Value:

    None

--*/
{


    NdisFreeSpinLock(&pVElan->Lock);
    NdisFreeSpinLock(&pVElan->PauseLock);

#ifdef IEEE_VLAN_SUPPORT
    NdisDeleteNPagedLookasideList(&pVElan->TagLookaside);    
#endif

    NdisFreeMemory(pVElan, 0, 0);
}


VOID
PtStopVElan(
    IN  PVELAN            pVElan
)
/*++

Routine Description:

    Stop a VELAN by requesting NDIS to halt the virtual miniport.
    The caller has a reference on the VELAN, so it won't go away
    while we are executing in this routine.

    ASSUMPTION: this is only called in the context of unbinding
    from the underlying miniport. If it may be called from elsewhere,
    this should protect itself from re-entrancy.
    
Arguments:

    pVElan      - Pointer to VELAN to be stopped.
    
Return Value:

    None

--*/
{
    NDIS_STATUS             Status;
    NDIS_HANDLE             MiniportAdapterHandle;
    BOOLEAN                 bMiniportInitCancelled = FALSE;

    DBGPRINT(MUX_LOUD, ("==> PtStopVElan: VELAN %p, Adapt %p\n", pVElan, pVElan->pAdapt));

    //
    // We make blocking calls below.
    //
    ASSERT_AT_PASSIVE();

    //
    // If there was a queued request on this VELAN, fail it now.
    //
    NdisAcquireSpinLock(&pVElan->Lock);
    ASSERT(pVElan->DeInitializing == TRUE);
    if (pVElan->QueuedRequest)
    {
        pVElan->QueuedRequest = FALSE;
        NdisReleaseSpinLock(&pVElan->Lock);

        PtRequestComplete(pVElan->pAdapt,
                          &pVElan->Request.Request,
                          NDIS_STATUS_FAILURE);
    }

    else
    {
        NdisReleaseSpinLock(&pVElan->Lock);
    }
    //
    // Check if we had called NdisIMInitializeDeviceInstanceEx and
    // we are awaiting a call to MiniportInitialize.
    //
    if (pVElan->MiniportInitPending)
    {
        //
        // Attempt to cancel miniport init.
        //
        Status = NdisIMCancelInitializeDeviceInstance(
                    DriverHandle,
                    &pVElan->CfgDeviceName);

        if (Status == NDIS_STATUS_SUCCESS)
        {
            //
            // Successfully cancelled IM initialization; our
            // Miniport Init routine will not be called for this
            // VELAN miniport.
            //
            pVElan->MiniportInitPending = FALSE;
            ASSERT(pVElan->MiniportAdapterHandle == NULL);
            bMiniportInitCancelled = TRUE;
        }
        else
        {
            //
            // Our Miniport Initialize routine will be called
            // (may be running on another thread at this time).
            // Wait for it to finish.
            //
            NdisWaitEvent(&pVElan->MiniportInitEvent, 0);
            ASSERT(pVElan->MiniportInitPending == FALSE);
        }
    }

    //
    // Check if Miniport Init has run. If so, deinitialize the virtual
    // miniport. This will result in a call to our Miniport Halt routine,
    // where the VELAN will be cleaned up.
    //
    MiniportAdapterHandle = pVElan->MiniportAdapterHandle;

    if ((NULL != MiniportAdapterHandle) &&
        (!pVElan->MiniportHalting))
    {
        //
        // The miniport was initialized, and has not yet halted.
        //
        ASSERT(bMiniportInitCancelled == FALSE);
        (VOID)NdisIMDeInitializeDeviceInstance(MiniportAdapterHandle);
    }
    else
    {
        if (bMiniportInitCancelled || 
            ((MiniportAdapterHandle == NULL) && !pVElan->MiniportHalting))
        {
            
            //
            // No NDIS events can come to this VELAN since it
            // was never initialized as a miniport. We need to unlink
            // it explicitly here.
            //
            PtUnlinkVElanFromAdapter(pVElan);            
        }
    }

    DBGPRINT(MUX_LOUD, ("<== PtStopVElan: VELAN %p, Adapt %p\n", pVElan, pVElan->pAdapt));
}


VOID
PtUnlinkVElanFromAdapter(
    IN PVELAN               pVElan
)
/*++

Routine Description:

    Utility routine to unlink a VELAN from its parent ADAPT structure.
    
Arguments:

    pVElan      - Pointer to VELAN to be unlinked.
    
Return Value:

    None

--*/
{
    PADAPT          pAdapt = pVElan->pAdapt;    
    LOCK_STATE      LockState;

    DBGPRINT(MUX_LOUD, ("==> PtUnlinkVElanFromAdapter: VELAN %p, Adapt %p\n", pVElan, pAdapt));
    ASSERT(pAdapt != NULL);


    //
    // Remove this VELAN from the global list
    //
    
    NdisAcquireSpinLock(&GlobalLock);
    RemoveEntryList(&pVElan->GlobalLink);
    NdisReleaseSpinLock(&GlobalLock);

    //
    // Remove this VELAN from the Adapter list
    //
    MUX_ACQUIRE_ADAPT_WRITE_LOCK(pAdapt, &LockState);

    RemoveEntryList(&pVElan->Link);
    pAdapt->VElanCount--;
        
    MUX_RELEASE_ADAPT_WRITE_LOCK(pAdapt, &LockState);
    pVElan->pAdapt = NULL;
    PtDereferenceVElan(pVElan, (PUCHAR)"adapter");

    PtDereferenceAdapter(pAdapt, (PUCHAR)"VElan");
    DBGPRINT(MUX_LOUD, ("<== PtUnlinkVElanFromAdapter: VELAN %p, Adapt %p\n", pVElan, pAdapt));
}


PVELAN
PtFindVElan(
    IN    PADAPT                pAdapt,
    IN    PNDIS_STRING          pVElanKey
)
/*++

Routine Description:

    Find an ELAN by bind name/key

Arguments:

    pAdapt     -    Pointer to an adapter struct.
    pVElanKey  -    The VELAN's device name

Return Value:

    Pointer to matching VELAN or NULL if not found.
    
--*/
{
    PLIST_ENTRY         p;
    PVELAN              pVElan;
    BOOLEAN             Found;
    NDIS_STRING         VElanKeyName = {0, 0, NULL};
    LOCK_STATE          LockState;

    ASSERT_AT_PASSIVE();

    DBGPRINT(MUX_LOUD, ("==> PtFindElan: Adapter %p, ElanKey %ws\n", pAdapt, 
                                        pVElanKey->Buffer));

    pVElan = NULL;
    Found = FALSE;
    VElanKeyName.Buffer = NULL;

    do
    {
        //
        // Make an up-cased copy of the given string.
        //
        VElanKeyName.Buffer = NdisAllocateMemoryWithTagPriority(pAdapt->BindingHandle, 
                                                        pVElanKey->MaximumLength, 
                                                        MUX_TAG,
                                                        LowPoolPriority);
        if (VElanKeyName.Buffer == NULL)
        {
            break;
        }

        VElanKeyName.Length = pVElanKey->Length;
        VElanKeyName.MaximumLength = pVElanKey->MaximumLength;

        (VOID)NdisUpcaseUnicodeString(&VElanKeyName, pVElanKey);

        //
        // Go through all VELANs on the ADAPT structure, looking
        // for a VELAN that has a matching device name.
        //
        MUX_ACQUIRE_ADAPT_READ_LOCK(pAdapt, &LockState);

        p = pAdapt->VElanList.Flink;
        while (p != &pAdapt->VElanList)
        {
            pVElan = CONTAINING_RECORD(p, VELAN, Link);

            if ((VElanKeyName.Length == pVElan->CfgDeviceName.Length) &&
                (memcmp(VElanKeyName.Buffer, pVElan->CfgDeviceName.Buffer, 
                VElanKeyName.Length) == 0))
            {
                Found = TRUE;
                break;
            }
        
            p = p->Flink;
        }

        MUX_RELEASE_ADAPT_READ_LOCK(pAdapt, &LockState);

    }
    while (FALSE);

    if (!Found)
    {
        DBGPRINT(MUX_INFO, ( "FindElan: No match found!\n"));
        pVElan = NULL;
    }

    if (VElanKeyName.Buffer)
    {
        NdisFreeMemory(VElanKeyName.Buffer, VElanKeyName.Length, 0);
    }

    DBGPRINT(MUX_LOUD, ("<== PtFindElan: Adapter %p, ElanKey %ws, VElan %p\n", pAdapt, 
                                        pVElanKey->Buffer, pVElan));
    return pVElan;
}


NDIS_STATUS
PtBootStrapVElans(
    IN  PADAPT            pAdapt,
    IN  PNDIS_STRING      InstanceName OPTIONAL
    
)
/*++

Routine Description:

    Start up the VELANs configured for an adapter.

Arguments:

    pAdapt    - Pointer to ATMLANE Adapter structure

Return Value:

    None

--*/
{
    NDIS_STATUS                     Status;
    NDIS_HANDLE                     AdapterConfigHandle;
    PNDIS_CONFIGURATION_PARAMETER   Param;
    NDIS_STRING                     DeviceStr = NDIS_STRING_CONST("UpperBindings");
    PWSTR                           buffer;
    LOCK_STATE                      LockState;
    NDIS_CONFIGURATION_OBJECT       ConfigObject;    

    DBGPRINT(MUX_LOUD, ("==> PtBootStrapElans:  adapter %p\n", pAdapt));
    //
    //  Initialize.
    //
    Status = NDIS_STATUS_SUCCESS;
    AdapterConfigHandle = NULL;
    
    do
    {
        DBGPRINT(MUX_LOUD, ("PtBootStrapElans: Starting ELANs on adapter %p\n", pAdapt));

        //
        //  Open the protocol configuration section for this adapter.
        //
        ConfigObject.Header.Type = NDIS_OBJECT_TYPE_CONFIGURATION_OBJECT;
        ConfigObject.Header.Revision = NDIS_CONFIGURATION_OBJECT_REVISION_1;
        ConfigObject.Header.Size = sizeof(NDIS_CONFIGURATION_OBJECT);
        ConfigObject.NdisHandle = pAdapt->BindingHandle;
        ConfigObject.Flags = 0;

        Status = NdisOpenConfigurationEx(
                    &ConfigObject,
                    &AdapterConfigHandle);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            AdapterConfigHandle = NULL;
            DBGPRINT(MUX_ERROR, ("PtBootStrapElans: OpenProtocolConfiguration failed\n"));
            Status = NDIS_STATUS_OPEN_FAILED;
            break;
        }        

        //
        // Read the "UpperBindings" reserved key that contains a list
        // of device names representing our miniport instances corresponding
        // to this lower binding. The UpperBindings is a 
        // MULTI_SZ containing a list of device names. We will loop through
        // this list and initialize the virtual miniports.
        //
        NdisReadConfiguration(&Status,
                              &Param,
                              AdapterConfigHandle,
                              &DeviceStr,
                              NdisParameterMultiString);
        if (NDIS_STATUS_SUCCESS != Status)
        {
            DBGPRINT(MUX_ERROR, ("PtBootStrapElans: NdisReadConfiguration failed\n"));
            break;
        }

        //
        // Parse the Multi_sz string to extract the device name of each VELAN.
        // This is used as the key name for the VELAN.
        //
        buffer = (PWSTR)Param->ParameterData.StringData.Buffer;
        while(*buffer != L'\0')
        {
            NDIS_STRING     DeviceName;
            
            NdisInitUnicodeString(&DeviceName, buffer);

            if (InstanceName != NULL)
            {
                if (NdisEqualString(&DeviceName, InstanceName, TRUE))
                {
                    Status = PtCreateAndStartVElan(pAdapt, &DeviceName);
                    break;
                }
            }
            else
            {

            Status = PtCreateAndStartVElan(pAdapt, &DeviceName); 
            }
            if (NDIS_STATUS_SUCCESS != Status)
            {
                DBGPRINT(MUX_ERROR, ("PtBootStrapElans: CreateVElan failed\n"));
                break;
            }
            buffer = (PWSTR)((PUCHAR)buffer + DeviceName.Length + sizeof(WCHAR));
        };
          
    } while (FALSE);

    //
    //    Close config handles
    //        
    if (NULL != AdapterConfigHandle)
    {
        NdisCloseConfiguration(AdapterConfigHandle);
    }
    //
    // If the driver cannot create any velan for the adapter
    // 
    if (Status != NDIS_STATUS_SUCCESS)
    {
        MUX_ACQUIRE_ADAPT_WRITE_LOCK(pAdapt, &LockState);
        //
        // No VElan is created for this adapter
        //
        if (pAdapt->VElanCount != 0)
        {
            Status = NDIS_STATUS_SUCCESS;
        }
        MUX_RELEASE_ADAPT_WRITE_LOCK(pAdapt, &LockState);
    }  
    
    DBGPRINT(MUX_LOUD, ("<== PtBootStrapElans:  adapter %p, Status %8x\n", pAdapt, Status));
    
    return Status;
}

VOID
PtReferenceVElan(
    IN    PVELAN            pVElan,
    IN    PUCHAR            String
    )
/*++

Routine Description:

    Add a references to an Elan structure.

Arguments:

    pElan    -    Pointer to the Elan structure.


Return Value:

    None.

--*/
{
    
    NdisInterlockedIncrement((PLONG)&pVElan->RefCount);

#if !DBG
    UNREFERENCED_PARAMETER(String);
#endif

    DBGPRINT(MUX_LOUD, ("ReferenceElan: Elan %p (%s) new count %d\n",
             pVElan, String, pVElan->RefCount));

    return;
}

ULONG
PtDereferenceVElan(
    IN    PVELAN            pVElan,
    IN    PUCHAR            String
    )
/*++

Routine Description:

    Subtract a reference from an VElan structure. 
    If the reference count becomes zero, deallocate it.

Arguments:

    pElan    -    Pointer to an VElan structure.


Return Value:

    None.

--*/
{
    ULONG        rc;

#if !DBG
    UNREFERENCED_PARAMETER(String);
#endif

    ASSERT(pVElan->RefCount > 0);

    rc = NdisInterlockedDecrement((PLONG)&pVElan->RefCount);

    if (rc == 0)
    {
        //
        // Free memory if there is no outstanding reference.
        // Note: Length field is not required if the memory 
        // is allocated with NdisAllocateMemoryWithTagPriority.
        //
        PtDeallocateVElan(pVElan);
    }
    
    DBGPRINT(MUX_LOUD, ("DereferenceElan: VElan %p (%s) new count %d\n", 
                                    pVElan, String, rc));
    return (rc);
}


BOOLEAN
PtReferenceAdapter(
    IN    PADAPT            pAdapt,
    IN    PUCHAR            String
    )
/*++

Routine Description:

    Add a references to an Adapter structure.

Arguments:

    pAdapt    -    Pointer to the Adapter structure.

Return Value:

    None.

--*/
{
    
#if !DBG
    UNREFERENCED_PARAMETER(String);
#endif

    NdisInterlockedIncrement((PLONG)&pAdapt->RefCount);
    
    DBGPRINT(MUX_LOUD, ("ReferenceAdapter: Adapter %p (%s) new count %d\n",
                    pAdapt, String, pAdapt->RefCount));

    return TRUE;
}

ULONG
PtDereferenceAdapter(
    IN    PADAPT    pAdapt,
    IN    PUCHAR    String
    )
/*++

Routine Description:

    Subtract a reference from an Adapter structure. 
    If the reference count becomes zero, deallocate it.

Arguments:

    pAdapt    -    Pointer to an adapter structure.


Return Value:

    None.

--*/
{
    ULONG        rc;

#if !DBG
    UNREFERENCED_PARAMETER(String);
#endif

    ASSERT(pAdapt->RefCount > 0);


    rc = NdisInterlockedDecrement ((PLONG)&pAdapt->RefCount);

    if (rc == 0)
    {
        //
        // Free memory if there is no outstanding reference.
        // Note: Length field is not required if the memory 
        // is allocated with NdisAllocateMemoryWithTagPriority.
        //
        NdisFreeMemory(pAdapt, 0, 0);
    }

    DBGPRINT(MUX_LOUD, ("DereferenceAdapter: Adapter %p (%s) new count %d\n", 
                        pAdapt, String, rc));

    return (rc);
}


VOID 
PtReceiveNBL(
    IN NDIS_HANDLE       ProtocolBindingContext,
    IN PNET_BUFFER_LIST  NetBufferLists,
    IN NDIS_PORT_NUMBER  PortNumber,
    IN ULONG             NumberOfNetBufferLists,
    IN ULONG             ReceiveFlags
    )
/*++

Routine Description:
    ReceiveNetBufferList handler. 

Arguments:
    ProtocolBindingContext                  Pointer to our PADAPT structure
    NetBufferLists                          Net Buffer Lists received
    PortNumber                              Port on which NBLS were received
    NumberOfNetBufferLists                  Number of Net Buffer Lists
    ReceiveFlags                            Flags associated with the receive

Return Value:
    None

NOTE: This receive code path is not efficient, we will optimize it later.

--*/
{
    PADAPT                  pAdapt = (PADAPT)ProtocolBindingContext;
    PVELAN                  pVElan = NULL;
    PUCHAR                  pDstMac;
    BOOLEAN                 bIsMulticast;
    BOOLEAN                 bIsBroadcast;
    PNET_BUFFER_LIST        CurrentNetBufferList = NULL;
    PNET_BUFFER_LIST        ReturnNetBufferList = NULL;
    PNET_BUFFER_LIST        LastReturnNetBufferList = NULL;
    LOCK_STATE              LockState;
    PLIST_ENTRY             p;
    ULONG                   ReturnFlags;
    ULONG                   NewReceiveFlags;
    UCHAR                   Data[6]={0,0,0,0,0,0};
    //BOOLEAN                 DispatchLevel;
    BOOLEAN                 bReturnNbl;
    
#ifdef IEEE_VLAN_SUPPORT
    NDIS_STATUS             NdisStatus;
    NDIS_NET_BUFFER_LIST_8021Q_INFO  NdisPacket8021qInfo;
    BOOLEAN                 bAllocatedContext;
    PRECV_NBL_ENTRY         RecvContext;    
#endif

    UNREFERENCED_PARAMETER(NumberOfNetBufferLists);

    DBGPRINT(MUX_VERY_LOUD,("==>PtReceiveNBL: ProtocolBindingContext %p, NetBufferLists %p\n",ProtocolBindingContext,NetBufferLists));

    //DispatchLevel = NDIS_TEST_RECEIVE_AT_DISPATCH_LEVEL(ReceiveFlags);

    ReturnFlags = 0;

    if (NDIS_TEST_RECEIVE_AT_DISPATCH_LEVEL(ReceiveFlags))
    {
        NDIS_SET_RETURN_FLAG(ReturnFlags, NDIS_RETURN_FLAGS_DISPATCH_LEVEL);
    }

    ASSERT(NetBufferLists != NULL);

    // We could get receives in the interval between
    // initiating a request to set the packet filter on
    // the binding to 0 and completion of that request.
    // Return immediately

    if (pAdapt->PacketFilter == 0)
    {  
        if (NDIS_TEST_RECEIVE_CAN_PEND(ReceiveFlags) == TRUE)
        {
            NdisReturnNetBufferLists(pAdapt->BindingHandle,
                                     NetBufferLists,
                                     ReturnFlags);
        }        
        return;
    }

    while (NetBufferLists != NULL)
    {
        CurrentNetBufferList = NetBufferLists;
        
        NetBufferLists = NET_BUFFER_LIST_NEXT_NBL(NetBufferLists);

        NET_BUFFER_LIST_NEXT_NBL(CurrentNetBufferList) = NULL;

        bReturnNbl = TRUE;

#ifdef IEEE_VLAN_SUPPORT
        bAllocatedContext = FALSE;
#endif

        do
        {
            // Collect some information about the packet
            pDstMac = NdisGetDataBuffer(NET_BUFFER_LIST_FIRST_NB(CurrentNetBufferList), 
                                        6,
                                        (PVOID)Data,
                                        1,
                                        0);

            if (pDstMac == NULL)
            {
                ASSERT(0);
                break;
            }

            // Determine if the packet is broadcast or multicast

            bIsMulticast = ETH_IS_MULTICAST(pDstMac);
            bIsBroadcast = ETH_IS_BROADCAST(pDstMac);

#ifdef IEEE_VLAN_SUPPORT
            // 
            // Create Receive context to save information about tag
            //
            NdisStatus = NdisAllocateNetBufferListContext(CurrentNetBufferList,
                                                          sizeof(RECV_NBL_ENTRY),
                                                          0,
                                                          MUX_TAG);
            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                break;
            }

            bAllocatedContext = TRUE;

            RecvContext = (PRECV_NBL_ENTRY) NET_BUFFER_LIST_CONTEXT_DATA_START(CurrentNetBufferList);        
            NdisZeroMemory(RecvContext, sizeof(RECV_NBL_ENTRY));                   

            //
            // Strip off the VLAN Tag if present
            //
            if ((NET_BUFFER_DATA_LENGTH(NET_BUFFER_LIST_FIRST_NB(CurrentNetBufferList))) < (ETH_HEADER_SIZE + VLAN_TAG_HEADER_SIZE))
            {
                //
                // If the VLAN tag is not present in the buffer, ignore this NBL
                //
                DBGPRINT(MUX_LOUD,("PtReceiveNBL: NBL %p size < VLAN_TAG_HEADER_SIZE\n",CurrentNetBufferList));            
                break;
            }

            if (NET_BUFFER_LIST_INFO(CurrentNetBufferList, Ieee8021QNetBufferListInfo) != 0)
            {
                //
                // If the VLAN info is already in the NBL, take this information it
                //            
                DBGPRINT(MUX_LOUD,("PtReceiveNBL: NBL %p already has Ieee8021QNetBufferListInfo\n",CurrentNetBufferList));    
                
                RtlCopyMemory((PVOID UNALIGNED) &NdisPacket8021qInfo, &NET_BUFFER_LIST_INFO(CurrentNetBufferList, Ieee8021QNetBufferListInfo),sizeof(NdisPacket8021qInfo));				
            }
            else
            {
                NdisStatus = PtStripVlanTagNB(CurrentNetBufferList, &NdisPacket8021qInfo, RecvContext);        

                if (NdisStatus != NDIS_STATUS_SUCCESS)
                {
                     break;
                }
            }
#endif        

            // Lock down the VLAN list on the adapter so that no insertions
            // deletions to this list happen while we loop through it. The packet
            // filter will also not change during this time we hold the read lock.

            MUX_ACQUIRE_ADAPT_READ_LOCK(pAdapt, &LockState);

            // Set up the ref count before we start indicating the packet

            for (p = pAdapt->VElanList.Flink;
                 p != &pAdapt->VElanList;
                 p = p->Flink)
            {
                BOOLEAN  bIndicateReceive;

                pVElan = CONTAINING_RECORD(p, VELAN, Link);

                // Should the packet be indicated up on this VELAN ?

                bIndicateReceive = PtMatchPacketToVElan(pVElan,
                                                        pDstMac,
                                                        bIsMulticast,
                                                        bIsBroadcast);

                if (!bIndicateReceive)
                {
                    continue;
                }

                MUX_INCR_PENDING_RECEIVES(pVElan);

                if ((pVElan->MiniportInitPending)
                     || (pVElan->MiniportHalting)
                     || (MUX_IS_LOW_POWER_STATE(pVElan->MPDevicePowerState)))
                {
                    MUX_DECR_PENDING_RECEIVES(pVElan);
                    continue;
                }
                
                NdisAcquireSpinLock(&pVElan->PauseLock);

                if (!pVElan->Paused)
                {
                    NdisReleaseSpinLock(&pVElan->PauseLock);

#ifdef IEEE_VLAN_SUPPORT
                    NdisStatus = PtHandleReceiveTaggingNB(pVElan, CurrentNetBufferList, &NdisPacket8021qInfo);

                    if (NdisStatus != STATUS_SUCCESS)
                    {
                        MUX_DECR_PENDING_RECEIVES(pVElan);
                        continue;
                    }
#endif

                    MUX_INCR_STATISTICS64(&pVElan->GoodReceives);

                    // Indicate to the protocol(s) bound to the Miniport
                    NewReceiveFlags = ReceiveFlags;

                    //
                    // Indicate with RESOURCES flag if it is not the last VELAN
                    // and NBLs were not indicated with RESOURCES flag to us.
                    //
                    if ((p->Flink != &pAdapt->VElanList) ||
                        (NDIS_TEST_RECEIVE_CANNOT_PEND(ReceiveFlags) == TRUE))
                    {
                        NDIS_SET_RECEIVE_FLAG(NewReceiveFlags, NDIS_RECEIVE_FLAGS_RESOURCES);
                    }
                    else
                    {
                        bReturnNbl = FALSE;
                    }

                    NdisMIndicateReceiveNetBufferLists(pVElan->MiniportAdapterHandle,
                                                       CurrentNetBufferList,
                                                       PortNumber,
                                                       1,
                                                       NewReceiveFlags);
                    //
                    // Decrement OutstandingReceives for the case where the NBL
                    // was indicated with RESOURCES flag. Otherwise it will be
                    // decremented in MPReturnNetBufferLists
                    //
                    if (bReturnNbl)
                    {
                        MUX_DECR_PENDING_RECEIVES(pVElan);

#ifdef IEEE_VLAN_SUPPORT
                        //
                        // If the buffer was advanced, retreat it
                        //
                        NdisStatus = PtRestoreReceiveNBL(CurrentNetBufferList);
                        ASSERT(NdisStatus == NDIS_STATUS_SUCCESS);
#endif
                    }
                }
                else
                {
                    NdisReleaseSpinLock(&pVElan->PauseLock);            
                    MUX_DECR_PENDING_RECEIVES(pVElan);
                }

            }

            MUX_RELEASE_ADAPT_READ_LOCK(pAdapt, &LockState);
        }
        while (FALSE);

        if (bReturnNbl == TRUE)
        {
#ifdef IEEE_VLAN_SUPPORT       
            //
            // Free the context only if we are returning the NBL here.
            // Otherwise MPReturnNetBufferLists will free it.
            //
            if (bAllocatedContext)
            {
                NdisFreeNetBufferListContext(CurrentNetBufferList,
                                             sizeof(RECV_NBL_ENTRY));            
            }
#endif           

            //
            // The NetBufferList is not pending with any upper protocol.
            // Return the NetBufferList back to the miniport if the miniport
            // gave us ownership of it
            //
            if (NDIS_TEST_RECEIVE_CAN_PEND(ReceiveFlags) == TRUE)
            {
                if (ReturnNetBufferList == NULL)
                {
                    ReturnNetBufferList = CurrentNetBufferList;
                }
                else
                {
                    NET_BUFFER_LIST_NEXT_NBL(LastReturnNetBufferList) = CurrentNetBufferList;
                }
                
                LastReturnNetBufferList = CurrentNetBufferList;
                NET_BUFFER_LIST_NEXT_NBL(LastReturnNetBufferList) = NULL;
            }
            else
            {
                //
                // Restore the NetBufferList chain
                //
                NET_BUFFER_LIST_NEXT_NBL(CurrentNetBufferList) = NetBufferLists;
            }
        }
    }

    if (ReturnNetBufferList != NULL)
    {
        NdisReturnNetBufferLists(pAdapt->BindingHandle,
                                 ReturnNetBufferList,
                                 ReturnFlags);
    }

    DBGPRINT(MUX_VERY_LOUD,("<==PtReceiveNBL: ProtocolBindingContext %p, NetBufferLists %p\n",ProtocolBindingContext,NetBufferLists));
}


VOID 
PtSendNBLComplete(
    IN NDIS_HANDLE      ProtocolBindingContext,
    IN PNET_BUFFER_LIST NetBufferLists,
    IN ULONG            SendCompleteFlags
   )
/*++

Routine Description:
    Called by NDIS when the miniport below has completed a send.
    We complete the corresponding upper-edge send this represents.

Arguments:
    ProtocolBindingHandle           Points to our PADAPT structure
    NetBufferLists                  Packet being completed by the lower miniport
    SendCompleteFlags               Is the call at DispatchLevel

Return Value:
    None

--*/
{

    PVELAN                  pVElan;
    NDIS_STATUS             Status;
    PIM_NBL_ENTRY           SendContext;
    PNET_BUFFER_LIST        CurrentNetBufferList;
    PADAPT                  pAdapt = (PADAPT)ProtocolBindingContext;
    BOOLEAN                 DispatchLevel = FALSE;
#ifdef IEEE_VLAN_SUPPORT
    PNET_BUFFER             MdlAllocatedNetBuffers;
    ULONG                   Flags = 0;
#endif

    DBGPRINT(MUX_VERY_LOUD,("==> PtSendNBLComplete: ProtocolBindingContext %p, NetBufferLists %p\n",
                ProtocolBindingContext,NetBufferLists));

    DispatchLevel = NDIS_TEST_SEND_COMPLETE_AT_DISPATCH_LEVEL(SendCompleteFlags);
   
    while(NetBufferLists)
    {
        CurrentNetBufferList = NetBufferLists;
        NetBufferLists = NET_BUFFER_LIST_NEXT_NBL(NetBufferLists);
        NET_BUFFER_LIST_NEXT_NBL(CurrentNetBufferList) = NULL;

        SendContext = (PIM_NBL_ENTRY)NET_BUFFER_LIST_CONTEXT_DATA_START(CurrentNetBufferList);

        pVElan = SendContext->pVElan;

        CurrentNetBufferList->SourceHandle = SendContext->PreviousSourceHandle;
#ifdef IEEE_VLAN_SUPPORT
        Flags = SendContext->Flags;
        MdlAllocatedNetBuffers = SendContext->MdlAllocatedNetBuffers;
#endif

        Status = NET_BUFFER_LIST_STATUS(CurrentNetBufferList);

        NdisFreeNetBufferListContext(CurrentNetBufferList,
                                      sizeof(IM_NBL_ENTRY));

#ifdef IEEE_VLAN_SUPPORT
    
        if ((Flags & MUX_RETREAT_DATA)  != 0)
        {
            MPRestoreSendNBL(pVElan, 
                             CurrentNetBufferList, 
                             NULL, 
                             MdlAllocatedNetBuffers);
        }

#endif

        if (Status == NDIS_STATUS_SUCCESS)
        {
            MUX_INCR_STATISTICS64(&pVElan->GoodTransmits);
        }
        else
        {
            MUX_INCR_STATISTICS(&pVElan->TransmitFailuresOther);
        }

        NdisMSendNetBufferListsComplete(pVElan->MiniportAdapterHandle,
                                        CurrentNetBufferList,
                                        SendCompleteFlags);

        
        MUX_DECR_PENDING_SENDS(pVElan);

        MUX_ACQUIRE_SPIN_LOCK(&pAdapt->Lock, DispatchLevel);
        pAdapt->OutstandingSends --;

        if ((pAdapt->OutstandingSends == 0) && (pAdapt->PauseEvent != NULL))
        {
            NdisSetEvent(pAdapt->PauseEvent);
            pAdapt->PauseEvent = NULL;
        }
        MUX_RELEASE_SPIN_LOCK(&pAdapt->Lock, DispatchLevel);

        
    }

    DBGPRINT(MUX_VERY_LOUD,("<== PtSendNBLComplete: ProtocolBindingContext %p, NetBufferLists %p\n",ProtocolBindingContext,NetBufferLists));
}

#ifdef IEEE_VLAN_SUPPORT
NDIS_STATUS 
PtHandleReceiveTaggingNB(
    IN PVELAN                   pVElan,
    IN PNET_BUFFER_LIST         NetBufferList,
    IN PNDIS_NET_BUFFER_LIST_8021Q_INFO  NdisPacket8021qInfo
    )
/*++

Routine Description:

    Parse a received Ethernet frame for 802.1Q tag information.
    If a tag header is present, copy in relevant field values to
    per-packet inforation to the new NET_BUFFER_LIST used
    to indicate up this frame.

Arguments:
    pVElan              Pointer to the VELAN structure
    NetBufferList       Pointer to the indicated packet from the lower miniport
    NdisPacket8021qInfo 802.1Q tag information 

Return Value:
    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_ACCEPTED


--*/
{
    PVOID                           pFrame = NULL;
    PVOID                           pDst;
    NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;
    PRECV_NBL_ENTRY                 RecvContext;
    PVOID                           Storage;
    USHORT UNALIGNED *              pTpid;


    DBGPRINT(MUX_VERY_LOUD,("==> PtHandleReceiveTaggingNB: VElan %p, NetBufferList %p, NdisPacket8021qInfo %p\n",pVElan,NetBufferList, NdisPacket8021qInfo));
    do
    {
        RecvContext = (PRECV_NBL_ENTRY) NET_BUFFER_LIST_CONTEXT_DATA_START(NetBufferList);  
        RecvContext->Flags = 0;        
    
        //
        // If the vlan ID of the virtual miniport is 0, the miniport should
        // act like it doesn't support VELAN tag processing
        //
        if (MuxRecognizedVlanId(pVElan,0))
            break;

        // Check if Tag header is present

        if (NdisPacket8021qInfo->Value == 0)
        {
            break;
        }

        //
        // If E-RIF present then discard header as we do not support
        // this variation
        //

        if (NdisPacket8021qInfo->TagHeader.CanonicalFormatId != 0)
        {
            //
            // Drop packet
            //

            Status = NDIS_STATUS_NOT_ACCEPTED;
            MUX_INCR_STATISTICS(&pVElan->RcvFormatErrors);
            break;
        }

        if ((NdisPacket8021qInfo->TagHeader.VlanId != 0)
            && (!MuxRecognizedVlanId(pVElan, NdisPacket8021qInfo->TagHeader.VlanId)))
        {
            //
            // Drop the packet
            //
            Status = NDIS_STATUS_NOT_ACCEPTED;

            MUX_INCR_STATISTICS(&pVElan->RcvVlanIdErrors);
            break;
        }      
        Storage=NULL;
        pFrame = NdisGetDataBuffer(NET_BUFFER_LIST_FIRST_NB(NetBufferList),
                                   2 * ETH_LENGTH_OF_ADDRESS + VLAN_TAG_HEADER_SIZE,
                                   Storage,
                                   1,
                                   0);

        if (pFrame == NULL)
        {
            ASSERT(0);
            Status = NDIS_STATUS_INVALID_PACKET;
            break;
        }
        
        pTpid = (USHORT UNALIGNED *)((PUCHAR)pFrame + 2 * ETH_LENGTH_OF_ADDRESS);
        
        //
        //Strip header only if it's present in the packet
        //
        if (*pTpid == TPID)
        {
             RecvContext->Flags |= MUX_ADVANCE_DATA;

             //
             // Strip off header
             //
             pDst = (PVOID)((PUCHAR)pFrame + VLAN_TAG_HEADER_SIZE);

             RtlMoveMemory(pDst, pFrame, 2 * ETH_LENGTH_OF_ADDRESS);

             NET_BUFFER_LIST_INFO(NetBufferList, Ieee8021QNetBufferListInfo) = NdisPacket8021qInfo->Value; 
        
             NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(NetBufferList),
                                           VLAN_TAG_HEADER_SIZE,
                                           FALSE,
                                           NULL);
        }

    } while(FALSE);

    DBGPRINT(MUX_VERY_LOUD,("<== PtHandleReceiveTaggingNB: VElan %p, NetBufferList %p, NdisPacket8021qInfo %p, Status %8x\n",pVElan,NetBufferList, NdisPacket8021qInfo, Status));
    return Status;
}

NDIS_STATUS 
PtStripVlanTagNB(
    IN PNET_BUFFER_LIST         NetBufferList,
    OUT PNDIS_NET_BUFFER_LIST_8021Q_INFO NdisPacket8021qInfo,
    OUT PRECV_NBL_ENTRY         RecvContext
    )
/*++

Routine Description:

    Parse a received Ethernet frame for 802.1Q tag information.
    If a tag header is present, copy in the output parameter

Arguments:
    NetBufferList       Pointer to the indicated packet from the lower miniport
    NdisPacket8021qInfo Pointer to the VLAN tag information
    RecvContext         Context for the received NBL

Return Value:
    NDIS_STATUS_SUCCESS


--*/
{
    VLAN_TAG_HEADER UNALIGNED *     pTagHeader;
    USHORT UNALIGNED *              pTpid;
    PVOID                           pFrame = NULL;
    NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;
    PVOID                           Storage;
    

    DBGPRINT(MUX_VERY_LOUD,("==> PtStripVlanTagNB: NetBufferList %p, NdisPacket8021qInfo %p\n",NetBufferList, NdisPacket8021qInfo));
    do
    {
        NdisPacket8021qInfo->Value = NULL;
        Storage=NULL;
        pFrame = NdisGetDataBuffer(NET_BUFFER_LIST_FIRST_NB(NetBufferList),
                                   2 * ETH_LENGTH_OF_ADDRESS + VLAN_TAG_HEADER_SIZE,
                                   Storage,
                                   1,
                                   0);

        if (pFrame == NULL)
        {
            ASSERT(0);
            Status = NDIS_STATUS_INVALID_PACKET;
            break;
        }

        // Get at the Ethertype field

        pTpid = (USHORT UNALIGNED *)((PUCHAR)pFrame + 2 * ETH_LENGTH_OF_ADDRESS);

        // Check if Tag header is present

        if (*pTpid != TPID)
        {
            break;
        }

        pTagHeader = (VLAN_TAG_HEADER UNALIGNED *)(pTpid + 1);

        COPY_TAG_INFO_FROM_HEADER_TO_PACKET_INFO(*NdisPacket8021qInfo, pTagHeader);

        RtlCopyMemory((PVOID UNALIGNED) &RecvContext->TagHeader, pTagHeader, 2);
        
    } while(FALSE);

    DBGPRINT(MUX_VERY_LOUD,("<== PtStripVlanTagNB: NetBufferList %p, NdisPacket8021qInfo %p, Status %8x\n",NetBufferList, NdisPacket8021qInfo, Status));
    return Status;
}

NDIS_STATUS 
PtRestoreReceiveNBL(
    IN PNET_BUFFER_LIST         NetBufferList
    )
/*++

Routine Description:

    Restore the received NBL if the tag header was parsed

Arguments:
    NetBufferList       Pointer to the indicated packet from the lower miniport

Return Value:
    NDIS_STATUS_SUCCESS


--*/
{
    PUCHAR                  pFrame = NULL;
    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;
    PRECV_NBL_ENTRY         ReceiveNblEntry;
    USHORT                  Tpid;
    PVOID                   Storage;

    do
    {
        ReceiveNblEntry = (PRECV_NBL_ENTRY) NET_BUFFER_LIST_CONTEXT_DATA_START(NetBufferList);        

        //
        // Check ifthe NBL was modified
        //
        if ((ReceiveNblEntry->Flags & MUX_ADVANCE_DATA) != MUX_ADVANCE_DATA)
        {
            break;
        }
    
        //
        // Retreat the net buffer list
        //
        Status = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(NetBufferList),
                                               VLAN_TAG_HEADER_SIZE,
                                               0,
                                               NULL);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }
        
        //
        // Find the start address of the frame
        // 
        Storage=NULL;
        pFrame = NdisGetDataBuffer(NET_BUFFER_LIST_FIRST_NB(NetBufferList),
                                   2 * ETH_LENGTH_OF_ADDRESS + VLAN_TAG_HEADER_SIZE,
                                   Storage,
                                   1,
                                   0);

        if (pFrame == NULL)
        {
            ASSERT(0);
            Status = NDIS_STATUS_INVALID_PACKET;
            break;            
        }

        //
        // Insert the VLAN tag to restore the received frame to the
        // original state
        //
        Tpid = TPID;
        
        NdisMoveMemory(pFrame, pFrame + VLAN_TAG_HEADER_SIZE, (2 * ETH_LENGTH_OF_ADDRESS));
        
        NdisMoveMemory(pFrame + (2 * ETH_LENGTH_OF_ADDRESS), &Tpid, 2);
        
        NdisMoveMemory(pFrame + (2 * ETH_LENGTH_OF_ADDRESS) + sizeof(Tpid), &ReceiveNblEntry->TagHeader, 2);
        
        NET_BUFFER_LIST_INFO(NetBufferList, Ieee8021QNetBufferListInfo) = 0;
    }
    while (FALSE);

    return Status;
}

#endif



