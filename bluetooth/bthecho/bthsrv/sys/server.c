/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Server.c

Abstract:

    Contains Bluetooth server functionality.

Environment:

    Kernel mode only


--*/

#include "clisrv.h"
#include "device.h"
#include "server.h"

#if defined(EVENT_TRACING)
#include "server.tmh"
#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
BthEchoSrvQueryInterfaces(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT DevCtx
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, BthEchoSrvInitialize)
#pragma alloc_text (PAGE, BthEchoSrvUnregisterPSM)
#pragma alloc_text (PAGE, BthEchoSrvUnregisterL2CAPServer)
#pragma alloc_text (PAGE, BthEchoSrvPublishSdpRecord)
#pragma alloc_text (PAGE, BthEchoSrvRemoveSdpRecord)
#pragma alloc_text (PAGE, BthEchoSrvQueryInterfaces)
#endif

void
InsertConnectionEntryLocked(
    PBTHECHOSAMPLE_SERVER_CONTEXT devCtx,
    PLIST_ENTRY ple    
    )
{
    WdfSpinLockAcquire(devCtx->ConnectionListLock);

    InsertTailList(&devCtx->ConnectionList, ple);

    WdfSpinLockRelease(devCtx->ConnectionListLock);    
}

void
RemoveConnectionEntryLocked(
    PBTHECHOSAMPLE_SERVER_CONTEXT devCtx,
    PLIST_ENTRY ple    
    )
{
    WdfSpinLockAcquire(devCtx->ConnectionListLock);

    RemoveEntryList(ple);

    WdfSpinLockRelease(devCtx->ConnectionListLock);    
}
   
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
BthEchoSrvQueryInterfaces(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT DevCtx
    )
/*++

Description:

    Query profile driver interface and store it in server context

Arguments:

    DevCtx - Server device context where to store the queried interface

Return Value:

    NTSTATUS Status code.

--*/
{
    NTSTATUS status;

    PAGED_CODE();

    status = WdfFdoQueryForInterface(
        DevCtx->Header.Device,
        &GUID_BTHDDI_PROFILE_DRIVER_INTERFACE, 
        (PINTERFACE) (&DevCtx->Header.ProfileDrvInterface),
        sizeof(DevCtx->Header.ProfileDrvInterface), 
        BTHDDI_PROFILE_DRIVER_INTERFACE_VERSION_FOR_QI, 
        NULL
        );
                
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, 
            "QueryInterface failed for Interface profile driver interface, version %d, Status code %!STATUS!\n", 
            BTHDDI_PROFILE_DRIVER_INTERFACE_VERSION_FOR_QI,
            status);

        goto exit;
    }    
exit:
    return status;    
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
BthEchoSrvInitialize(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT DevCtx
    )
{
    NTSTATUS status;
    
    PAGED_CODE();

    status = BthEchoSrvQueryInterfaces(DevCtx);
    if (!NT_SUCCESS(status))
    {
        goto exit;
    }
    
exit:
    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
void
BthEchoSrvIndicationCallback(
    _In_ PVOID Context,
    _In_ INDICATION_CODE Indication,
    _In_ PINDICATION_PARAMETERS Parameters
    )

/*++

Description:

    Indication callback passed to bth stack while registering server.
    Bth stack sends notification related to the server.
    We receive connect notifications in this callback.
    
Arguments:

    Context - We receive server device context as the context
    Indication - Type of indication
    Parameters - Parameters of indication

--*/

{
    switch(Indication)
    {
        case IndicationAddReference:
        case IndicationReleaseReference:
            break;
        case IndicationRemoteConnect:
        {
            PBTHECHOSAMPLE_SERVER_CONTEXT devCtx = (PBTHECHOSAMPLE_SERVER_CONTEXT) Context;

            BthEchoSrvSendConnectResponse(devCtx, Parameters);
            break;
        }
        case IndicationRemoteDisconnect:
        {
            //
            // We register BthEchoSrvConnectionIndicationCallback for disconnect
            //
            NT_ASSERT(FALSE);
            break;
        }
        case IndicationRemoteConfigRequest:
        case IndicationRemoteConfigResponse:
        case IndicationFreeExtraOptions:
            break;
    }
}


#if (NTDDI_VERSION >= NTDDI_WIN8)

_IRQL_requires_max_(DISPATCH_LEVEL)
void
BthEchoSrvConnectionIndicationCallback(
    _In_ PVOID Context,
    _In_ INDICATION_CODE Indication,
    _In_ PINDICATION_PARAMETERS_ENHANCED Parameters
    )
/*++

Description:

    Indication callback passed to bth stack while responding to connect.
    Bth stack sends notification related to the connection.
    We receive disconnect notifications in this callback
    
Arguments:

    Context - We receive connection object as the context
    Indication - Type of indication
    Parameters - Parameters of indication

--*/
{
    UNREFERENCED_PARAMETER(Parameters);
   
    switch(Indication)
    {
        case IndicationAddReference:
            break;
        case IndicationReleaseReference:
            break;
        case IndicationRemoteConnect:
        {
            //
            // We don't expect connect on this callback
            //
            NT_ASSERT(FALSE);
            break;
        }
        case IndicationRemoteDisconnect:
        {
            WDFOBJECT connectionObject = (WDFOBJECT) Context;
            PBTHECHO_CONNECTION connection = GetConnectionObjectContext(connectionObject);

            BthEchoSrvDisconnectConnection(connection);
            
            break;
        }
        case IndicationRemoteConfigRequest:
        case IndicationRemoteConfigResponse:
        case IndicationFreeExtraOptions:
            break;
        default:
            //
            // We don't expect any other indications on this callback
            //
            NT_ASSERT(FALSE);
    }
}

#else


_IRQL_requires_max_(DISPATCH_LEVEL)
void
BthEchoSrvConnectionIndicationCallback(
    _In_ PVOID Context,
    _In_ INDICATION_CODE Indication,
    _In_ PINDICATION_PARAMETERS Parameters
    )
/*++

Description:

    Indication callback passed to bth stack while responding to connect.
    Bth stack sends notification related to the connection.
    We receive disconnect notifications in this callback
    
Arguments:

    Context - We receive connection object as the context
    Indication - Type of indication
    Parameters - Parameters of indication

--*/
{
    UNREFERENCED_PARAMETER(Parameters);
   
    switch(Indication)
    {
        case IndicationAddReference:
            break;
        case IndicationReleaseReference:
            break;
        case IndicationRemoteConnect:
        {
            //
            // We don't expect connect on this callback
            //
            NT_ASSERT(FALSE);
            break;
        }
        case IndicationRemoteDisconnect:
        {
            WDFOBJECT connectionObject = (WDFOBJECT) Context;
            PBTHECHO_CONNECTION connection = GetConnectionObjectContext(connectionObject);

            BthEchoSrvDisconnectConnection(connection);
            
            break;
        }
        case IndicationRemoteConfigRequest:
        case IndicationRemoteConfigResponse:
        case IndicationFreeExtraOptions:
            break;
        default:
            //
            // We don't expect any other indications on this callback
            //
            NT_ASSERT(FALSE);
    }
}


#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
BthEchoSrvRegisterPSM(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT DevCtx
    )
/*++

Description:

    Registers server PSM.

Arguments:

    DevCtx - Device context of the server

Return Value:

    NTSTATUS Status code.

--*/
{
    NTSTATUS status;
    struct _BRB_PSM * brb;

    DevCtx->Header.ProfileDrvInterface.BthReuseBrb(
        &(DevCtx->RegisterUnregisterBrb), 
        BRB_REGISTER_PSM
        );

    brb = (struct _BRB_PSM *)
            &(DevCtx->RegisterUnregisterBrb);
    
    //
    // Send in our preferred PSM
    //
    brb->Psm = DevCtx->Psm;

    status = BthEchoSharedSendBrbSynchronously(
        DevCtx->Header.IoTarget,
        DevCtx->Header.Request,
        (PBRB) brb,
        sizeof(*brb)
        );
    
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, 
            "BRB_REGISTER_PSM failed, Status code %!STATUS!\n", status);
        goto exit;        
    }

    //
    // Store PSM obtained
    //
    DevCtx->Psm = brb->Psm;

exit:
    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
BthEchoSrvUnregisterPSM(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT DevCtx
    )
/*++

Description:

    Unregisters server PSM.

Arguments:

    DevCtx - Device context of the server

Return Value:

    NTSTATUS Status code.

--*/
{
    NTSTATUS status;
    struct _BRB_PSM * brb;

    PAGED_CODE();

    if (0 == DevCtx->Psm)
    {
        return;
    }

    DevCtx->Header.ProfileDrvInterface.BthReuseBrb(
        &(DevCtx->RegisterUnregisterBrb), 
        BRB_UNREGISTER_PSM
        );

    brb = (struct _BRB_PSM *)
            &(DevCtx->RegisterUnregisterBrb);

    //
    // Format Brb
    //
    
    brb->Psm = DevCtx->Psm;

    status = BthEchoSharedSendBrbSynchronously(
        DevCtx->Header.IoTarget,
        DevCtx->Header.Request,
        (PBRB) brb,
        sizeof(*(brb))
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, 
            "BRB_UNREGISTER_PSM failed, Status code %!STATUS!\n", status);

        //
        // Send does not fail for resource reasons
        //
        NT_ASSERT(FALSE);
        
        goto exit;        
    }

exit:
    return;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
BthEchoSrvRegisterL2CAPServer(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT DevCtx
    )
/*++

Description:

    Registers L2CAP server.

Arguments:

    DevCtx - Device context of the server

Return Value:

    NTSTATUS Status code.

--*/
{
    NTSTATUS status;
    struct _BRB_L2CA_REGISTER_SERVER *brb;

    DevCtx->Header.ProfileDrvInterface.BthReuseBrb(
        &(DevCtx->RegisterUnregisterBrb), 
        BRB_L2CA_REGISTER_SERVER
        );

    brb = (struct _BRB_L2CA_REGISTER_SERVER *)
            &(DevCtx->RegisterUnregisterBrb);

    //
    // Format brb
    //
    brb->BtAddress = BTH_ADDR_NULL;
    brb->PSM = 0; //we have already registered the PSM
    brb->IndicationCallback = &BthEchoSrvIndicationCallback;
    brb->IndicationCallbackContext = DevCtx;
    brb->IndicationFlags = 0;
    brb->ReferenceObject = WdfDeviceWdmGetDeviceObject(DevCtx->Header.Device);

    status = BthEchoSharedSendBrbSynchronously(
        DevCtx->Header.IoTarget,
        DevCtx->Header.Request,
        (PBRB) brb,
        sizeof(*brb)
        );
    
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, 
            "BRB_REGISTER_PSM failed, Status code %!STATUS!\n", status);
        goto exit;        
    }

    //
    // Store server handle
    //
    DevCtx->L2CAPServerHandle = brb->ServerHandle;

exit:
    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
BthEchoSrvUnregisterL2CAPServer(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT DevCtx
    )
/*++

Description:

    Unregisters L2CAP server.

Arguments:

    DevCtx - Device context of the server

Return Value:

    NTSTATUS Status code.

--*/
{
    NTSTATUS status;
    struct _BRB_L2CA_UNREGISTER_SERVER *brb;

    PAGED_CODE();

    if (NULL == DevCtx->L2CAPServerHandle)
    {
        return;
    }

    DevCtx->Header.ProfileDrvInterface.BthReuseBrb(
        &(DevCtx->RegisterUnregisterBrb), 
        BRB_L2CA_UNREGISTER_SERVER
        );

    brb = (struct _BRB_L2CA_UNREGISTER_SERVER *)
            &(DevCtx->RegisterUnregisterBrb);

    //
    // Format Brb
    //
    brb->BtAddress = BTH_ADDR_NULL ;//DevCtx->LocalAddress;
    brb->Psm = 0; //since we will use unregister PSM to unregister.
    brb->ServerHandle = DevCtx->L2CAPServerHandle;

    status = BthEchoSharedSendBrbSynchronously(
        DevCtx->Header.IoTarget,
        DevCtx->Header.Request,
        (PBRB) brb,
        sizeof(*(brb))
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, 
            "BRB_L2CA_UNREGISTER_SERVER failed, Status code %!STATUS!\n", status);

        //
        // Send does not fail for resource reasons
        //
        NT_ASSERT(FALSE);
        
        goto exit;        
    }

    DevCtx->L2CAPServerHandle = NULL;

exit:
    return;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
BthEchoSrvPublishSdpRecord(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT DevCtx,
    _In_ PUCHAR SdpRecord,
    _In_ ULONG  SdpRecordLength
    )
/*++

Description:

    Publishes server SDP record.
    
    We first build SDP record using CreateSdpRecord in sdp.c
    and call this function to publish the record.

Arguments:

    DevCtx - Device context of the server
    SdpRecord - Sdp record to publish
    SdpRecordLength - Sdp record length

Return Value:

    NTSTATUS Status code.

--*/
{
    NTSTATUS status, statusReuse;
    WDF_MEMORY_DESCRIPTOR inMemDesc;
    WDF_MEMORY_DESCRIPTOR outMemDesc;
    WDF_REQUEST_REUSE_PARAMS ReuseParams;
    HANDLE_SDP sdpRecordHandle;

    PAGED_CODE();
    
    WDF_REQUEST_REUSE_PARAMS_INIT(&ReuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    statusReuse = WdfRequestReuse(DevCtx->Header.Request, &ReuseParams);    
    NT_ASSERT(NT_SUCCESS(statusReuse));
    UNREFERENCED_PARAMETER(statusReuse);

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
        &inMemDesc,
        SdpRecord,
        SdpRecordLength
        );

    RtlZeroMemory( &sdpRecordHandle, sizeof(HANDLE_SDP) );
    
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
        &outMemDesc,
        &sdpRecordHandle,
        sizeof(HANDLE_SDP)
        );

    status = WdfIoTargetSendIoctlSynchronously(
        DevCtx->Header.IoTarget,
        DevCtx->Header.Request,
        IOCTL_BTH_SDP_SUBMIT_RECORD,
        &inMemDesc,
        &outMemDesc,
        NULL,   //sendOptions
        NULL    //bytesReturned
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, 
            "IOCTL_BTH_SDP_SUBMIT_RECORD failed, Status code %!STATUS!\n", status);

        goto exit;
    }

    DevCtx->SdpRecordHandle = sdpRecordHandle;

exit:
    return status;    
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
BthEchoSrvRemoveSdpRecord(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT DevCtx
    )
/*++

Description:

    Removes Sdp record

Arguments:

    DevCtx - Device context of the server

Return Value:

    NTSTATUS Status code.

--*/
{
    NTSTATUS status, statusReuse;
    WDF_MEMORY_DESCRIPTOR inMemDesc;
    WDF_REQUEST_REUSE_PARAMS ReuseParams;
    HANDLE_SDP sdpRecordHandle;

    PAGED_CODE();
    
    WDF_REQUEST_REUSE_PARAMS_INIT(&ReuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    statusReuse =  WdfRequestReuse(DevCtx->Header.Request, &ReuseParams);    
    NT_ASSERT(NT_SUCCESS(statusReuse));
    UNREFERENCED_PARAMETER(statusReuse);

    sdpRecordHandle = DevCtx->SdpRecordHandle;

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
        &inMemDesc,
        &sdpRecordHandle,
        sizeof(HANDLE_SDP)
        );
    
    status = WdfIoTargetSendIoctlSynchronously(
        DevCtx->Header.IoTarget,
        DevCtx->Header.Request,
        IOCTL_BTH_SDP_REMOVE_RECORD,
        &inMemDesc,
        NULL,   //outMemDesc
        NULL,   //sendOptions
        NULL    //bytesReturned
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, 
            "IOCTL_BTH_SDP_REMOVE_RECORD failed, Status code %!STATUS!\n", status);

        //
        // Send does not fail for resource reasons
        //
        NT_ASSERT(FALSE);

        goto exit;
    }

    DevCtx->SdpRecordHandle = HANDLE_SDP_NULL;

exit:
    return;    
}

#if (NTDDI_VERSION >= NTDDI_WIN8)

void
BthEchoSrvRemoteConnectResponseCompletion(
    _In_ WDFREQUEST  Request,
    _In_ WDFIOTARGET  Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS  Params,
    _In_ WDFCONTEXT  Context
    )
/*++
Description:

    Completion routine for BRB_L2CA_OPEN_CHANNEL_RESPONSE BRB sent in
    BthEchoSrvSendConnectResponse.

    If the request completed successfully we call BthEchoSrvConnectionStateConnected
    function implemented in device.c. This function initializes and submits
    continuous readers to read the data coming from client which then is used
    for echo.

Arguments:

    Request - Request that completed
    Target - Target to which request was sent
    Params - Request completion parameters
    Context - We receive Brb as the context. This Brb is 
              connection->ConnectDisconnectBrb, and is not allocated separately
              hence it is not freed here.

Return Value:

    NTSTATUS Status code.
--*/
{
    NTSTATUS status;
    struct _BRB_L2CA_OPEN_ENHANCED_CHANNEL *brb;    
    PBTHECHO_CONNECTION connection;
    WDFOBJECT connectionObject;


    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Request); //we reuse the request, hence it is not needed here
    
    status = Params->IoStatus.Status;

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
        "Connection completion, status: %!STATUS!", status);        

    brb = (struct _BRB_L2CA_OPEN_ENHANCED_CHANNEL *) Context;

    //
    // We receice connection object as the context in the BRB
    //
    connectionObject = (WDFOBJECT) brb->Hdr.ClientContext[0];
    connection = GetConnectionObjectContext(connectionObject);
    
    if(NT_SUCCESS(status))
    {
        connection->OutMTU = brb->OutResults.Params.Mtu;
        connection->InMTU = brb->InResults.Params.Mtu;
        connection->ChannelHandle = brb->ChannelHandle;
        connection->RemoteAddress = brb->BtAddress;

        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
            "Connection (0x%x) established with client", brb->OutResults.Params.RetransmissionAndFlow.Mode); 

        //
        // Check if we already received a disconnect request
        // and if so disconnect
        //
        
        WdfSpinLockAcquire(connection->ConnectionLock);

        if (connection->ConnectionState == ConnectionStateDisconnecting) 
        {
            //
            // We allow transition to disconnected state only from
            // connected state
            //
            // If we are in disconnection state this means that
            // we were waiting for connect to complete before we
            // can send disconnect down
            //
            // Set the state to Connected and call BthEchoConnectionObjectRemoteDisconnect
            //
            connection->ConnectionState = ConnectionStateConnected;
            WdfSpinLockRelease(connection->ConnectionLock);

            TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
                "Remote connect response completion: "
                "disconnect has been received "
                "for connection 0x%p", connection);        
    
            BthEchoConnectionObjectRemoteDisconnect(connection->DevCtxHdr, connection);
        }
        else
        {
            connection->ConnectionState = ConnectionStateConnected;
            WdfSpinLockRelease(connection->ConnectionLock);

            TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
                "Connection completed, connection: 0x%p", connection);

            //
            // Call BthEchoSrvConnectionStateConnected to perform post connect processing
            // (namely initializing continuous readers)
            //
            status = BthEchoSrvConnectionStateConnected(connectionObject);
            if (!NT_SUCCESS(status))    
            {
                //
                // If the post connect processing failed, let us disconnect
                //
                BthEchoConnectionObjectRemoteDisconnect(connection->DevCtxHdr, connection);
            }            

        }
    }
    else
    {
        BOOLEAN setDisconnectEvent = FALSE;
        
        WdfSpinLockAcquire(connection->ConnectionLock);
        if (connection->ConnectionState == ConnectionStateDisconnecting) 
        {
            setDisconnectEvent = TRUE;
        }

        connection->ConnectionState = ConnectionStateConnectFailed;
        WdfSpinLockRelease(connection->ConnectionLock);

        if (setDisconnectEvent)
        {
            KeSetEvent(&connection->DisconnectEvent,
                                0,
                                FALSE);
        }
        
    }

    if (!NT_SUCCESS(status))
    {        
        RemoveConnectionEntryLocked(
            (PBTHECHOSAMPLE_SERVER_CONTEXT) connection->DevCtxHdr,
            &connection->ConnectionListEntry
            );
        
        WdfObjectDelete(connectionObject);        
    }
    
    return;    
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
BthEchoSrvSendConnectResponse(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT DevCtx,
    _In_ PINDICATION_PARAMETERS ConnectParams
    )
/*++
Description:

    Respond to connect indication received.
    
    This function is invoked by BthEchoSrvIndicationCallback when
    connect indication is received.

Arguments:

    DevCtx - Server device context
    ConnectParams - Connect indication parameters

Return Value:

    NTSTATUS Status code.
--*/
{
    NTSTATUS status, statusReuse;
    WDF_REQUEST_REUSE_PARAMS reuseParams;
    struct _BRB_L2CA_OPEN_ENHANCED_CHANNEL *brb = NULL;
    WDFOBJECT connectionObject = NULL;
    PBTHECHO_CONNECTION connection = NULL;
    ULONG modeConfigFlag = CM_BASIC;
    
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
        "Server receivced connect request");        

    //
    // We create the connection object as the first step so that if we receive 
    // remove before connect response is completed
    // we can wait for connection and disconnect.
    //
    status = BthEchoConnectionObjectCreate(
        &DevCtx->Header,
        DevCtx->Header.Device,
        &connectionObject
        );

    if (!NT_SUCCESS(status))
    {
        goto exit;
    }

    connection = GetConnectionObjectContext(connectionObject);

    connection->ConnectionState = ConnectionStateConnecting;

    //
    // Insert the connection object in the conenction list that we track
    //
    InsertConnectionEntryLocked(DevCtx, &connection->ConnectionListEntry);

    WDF_REQUEST_REUSE_PARAMS_INIT(&reuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    statusReuse = WdfRequestReuse(connection->ConnectDisconnectRequest, &reuseParams);
    NT_ASSERT(NT_SUCCESS(statusReuse));
    UNREFERENCED_PARAMETER(statusReuse);
    
    brb = (struct _BRB_L2CA_OPEN_ENHANCED_CHANNEL*) &(connection->ConnectDisconnectBrb);
    DevCtx->Header.ProfileDrvInterface.BthReuseBrb((PBRB)brb, BRB_L2CA_OPEN_ENHANCED_CHANNEL_RESPONSE);

    brb->Hdr.ClientContext[0] = connectionObject;
    brb->BtAddress = ConnectParams->BtAddress;
    brb->Psm = ConnectParams->Parameters.Connect.Request.PSM;
    brb->ChannelHandle = ConnectParams->ConnectionHandle;
    brb->Response = CONNECT_RSP_RESULT_SUCCESS;

    brb->ChannelFlags = CF_ROLE_EITHER;

    brb->ConfigOut.Flags = CFG_ENHANCED;

    if (DevCtx->Header.LocalFeatures.Mask & BTH_HOST_FEATURE_ENHANCED_RETRANSMISSION_MODE)
    {
        modeConfigFlag |= CM_RETRANSMISSION_AND_FLOW;
        brb->ConfigOut.ModeConfig.Flags = modeConfigFlag;

        //
        // Mode is specified using Flags above and this should be 0.
        //
        brb->ConfigOut.ModeConfig.RetransmissionAndFlow.Mode = 0;
        brb->ConfigOut.ModeConfig.RetransmissionAndFlow.MaxTransmit = L2CAP_RAF_DEFAULT_MAXTRANSMIT;
        brb->ConfigOut.ModeConfig.RetransmissionAndFlow.MaxPDUSize = L2CAP_RAF_DEFAULT_MAX_PDU_SIZE;
        brb->ConfigOut.ModeConfig.RetransmissionAndFlow.TxWindowSize = L2CAP_RAF_DEFAULT_TX_WINDOW_SIZE;
    }
    

    brb->ConfigOut.Flags |= CFG_MTU;
    brb->ConfigOut.Mtu.Max = L2CAP_DEFAULT_MTU;
    brb->ConfigOut.Mtu.Min = L2CAP_MIN_MTU;
    brb->ConfigOut.Mtu.Preferred = L2CAP_DEFAULT_MTU;

    brb->ConfigIn.Flags = CFG_MTU;
    brb->ConfigIn.Mtu.Max = brb->ConfigOut.Mtu.Max;
    brb->ConfigIn.Mtu.Min = brb->ConfigOut.Mtu.Min;
    brb->ConfigIn.Mtu.Preferred = brb->ConfigOut.Mtu.Max;

    //
    // Get notifications about disconnect
    //
    brb->CallbackFlags = CALLBACK_DISCONNECT;
    brb->Callback = &BthEchoSrvConnectionIndicationCallback;
    brb->CallbackContext = connectionObject;
    brb->ReferenceObject = (PVOID) WdfDeviceWdmGetDeviceObject(DevCtx->Header.Device);

    status = BthEchoSharedSendBrbAsync(
        DevCtx->Header.IoTarget,
        connection->ConnectDisconnectRequest,
        (PBRB) brb,
        sizeof(*brb),
        BthEchoSrvRemoteConnectResponseCompletion,
        brb
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONNECT, 
            "BthEchoSharedSendBrbAsync failed, status = %!STATUS!", status);                
    }
    
exit:    
    
    if(!NT_SUCCESS(status) && connectionObject)
    {
        //
        // If we failed to connect remove connectionfrom list and
        // delete the object
        //

        //
        // connection should not be NULL if connectionObject is not NULL
        // since first thing we do after creating connectionObject is to
        // get context which gives us connection
        //
        NT_ASSERT(connection != NULL);

        if (connection != NULL)
        {
            connection->ConnectionState = ConnectionStateConnectFailed;

            RemoveConnectionEntryLocked(
                (PBTHECHOSAMPLE_SERVER_CONTEXT) connection->DevCtxHdr,
                &connection->ConnectionListEntry
                );
        }

        WdfObjectDelete(connectionObject);
    }
    
    return status;
}

#else

void
BthEchoSrvRemoteConnectResponseCompletion(
    _In_ WDFREQUEST  Request,
    _In_ WDFIOTARGET  Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS  Params,
    _In_ WDFCONTEXT  Context
    )
/*++
Description:

    Completion routine for BRB_L2CA_OPEN_CHANNEL_RESPONSE BRB sent in
    BthEchoSrvSendConnectResponse.

    If the request completed successfully we call BthEchoSrvConnectionStateConnected
    function implemented in device.c. This function initializes and submits
    continuous readers to read the data coming from client which then is used
    for echo.

Arguments:

    Request - Request that completed
    Target - Target to which request was sent
    Params - Request completion parameters
    Context - We receive Brb as the context. This Brb is 
              connection->ConnectDisconnectBrb, and is not allocated separately
              hence it is not freed here.

Return Value:

    NTSTATUS Status code.
--*/
{
    NTSTATUS status;
    struct _BRB_L2CA_OPEN_CHANNEL *brb;    
    PBTHECHO_CONNECTION connection;
    WDFOBJECT connectionObject;


    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Request); //we reuse the request, hence it is not needed here
    
    status = Params->IoStatus.Status;

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
        "Connection completion, status: %!STATUS!", status);        

    brb = (struct _BRB_L2CA_OPEN_CHANNEL *) Context;

    //
    // We receice connection object as the context in the BRB
    //
    connectionObject = (WDFOBJECT) brb->Hdr.ClientContext[0];
    connection = GetConnectionObjectContext(connectionObject);
    
    if(NT_SUCCESS(status))
    {
        connection->OutMTU = brb->OutResults.Params.Mtu;
        connection->InMTU = brb->InResults.Params.Mtu;
        connection->ChannelHandle = brb->ChannelHandle;
        connection->RemoteAddress = brb->BtAddress;

        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
            "Connection established with client"); 

        //
        // Check if we already received a disconnect request
        // and if so disconnect
        //
        
        WdfSpinLockAcquire(connection->ConnectionLock);

        if (connection->ConnectionState == ConnectionStateDisconnecting) 
        {
            //
            // We allow transition to disconnected state only from
            // connected state
            //
            // If we are in disconnection state this means that
            // we were waiting for connect to complete before we
            // can send disconnect down
            //
            // Set the state to Connected and call BthEchoConnectionObjectRemoteDisconnect
            //
            connection->ConnectionState = ConnectionStateConnected;
            WdfSpinLockRelease(connection->ConnectionLock);

            TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
                "Remote connect response completion: "
                "disconnect has been received "
                "for connection 0x%p", connection);        
    
            BthEchoConnectionObjectRemoteDisconnect(connection->DevCtxHdr, connection);
        }
        else
        {
            connection->ConnectionState = ConnectionStateConnected;
            WdfSpinLockRelease(connection->ConnectionLock);

            TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
                "Connection completed, connection: 0x%p", connection);

            //
            // Call BthEchoSrvConnectionStateConnected to perform post connect processing
            // (namely initializing continuous readers)
            //
            status = BthEchoSrvConnectionStateConnected(connectionObject);
            if (!NT_SUCCESS(status))    
            {
                //
                // If the post connect processing failed, let us disconnect
                //
                BthEchoConnectionObjectRemoteDisconnect(connection->DevCtxHdr, connection);
            }            

        }
    }
    else
    {
        BOOLEAN setDisconnectEvent = FALSE;
        
        WdfSpinLockAcquire(connection->ConnectionLock);
        if (connection->ConnectionState == ConnectionStateDisconnecting) 
        {
            setDisconnectEvent = TRUE;
        }

        connection->ConnectionState = ConnectionStateConnectFailed;
        WdfSpinLockRelease(connection->ConnectionLock);

        if (setDisconnectEvent)
        {
            KeSetEvent(&connection->DisconnectEvent,
                                0,
                                FALSE);
        }
        
    }

    if (!NT_SUCCESS(status))
    {        
        RemoveConnectionEntryLocked(
            (PBTHECHOSAMPLE_SERVER_CONTEXT) connection->DevCtxHdr,
            &connection->ConnectionListEntry
            );
        
        WdfObjectDelete(connectionObject);        
    }
    
    return;    
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
BthEchoSrvSendConnectResponse(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT DevCtx,
    _In_ PINDICATION_PARAMETERS ConnectParams
    )
/*++
Description:

    Respond to connect indication received.
    
    This function is invoked by BthEchoSrvIndicationCallback when
    connect indication is received.

Arguments:

    DevCtx - Server device context
    ConnectParams - Connect indication parameters

Return Value:

    NTSTATUS Status code.
--*/
{
    NTSTATUS status, statusReuse;
    WDF_REQUEST_REUSE_PARAMS reuseParams;
    struct _BRB_L2CA_OPEN_CHANNEL *brb = NULL;
    WDFOBJECT connectionObject = NULL;
    PBTHECHO_CONNECTION connection = NULL;
    
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
        "Server receivced connect request");        

    //
    // We create the connection object as the first step so that if we receive 
    // remove before connect response is completed
    // we can wait for connection and disconnect.
    //
    status = BthEchoConnectionObjectCreate(
        &DevCtx->Header,
        DevCtx->Header.Device,
        &connectionObject
        );

    if (!NT_SUCCESS(status))
    {
        goto exit;
    }

    connection = GetConnectionObjectContext(connectionObject);

    connection->ConnectionState = ConnectionStateConnecting;

    //
    // Insert the connection object in the conenction list that we track
    //
    InsertConnectionEntryLocked(DevCtx, &connection->ConnectionListEntry);

    WDF_REQUEST_REUSE_PARAMS_INIT(&reuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    statusReuse = WdfRequestReuse(connection->ConnectDisconnectRequest, &reuseParams);
    NT_ASSERT(NT_SUCCESS(statusReuse));
    UNREFERENCED_PARAMETER(statusReuse);
    
    brb = (struct _BRB_L2CA_OPEN_CHANNEL*) &(connection->ConnectDisconnectBrb);
    DevCtx->Header.ProfileDrvInterface.BthReuseBrb((PBRB)brb, BRB_L2CA_OPEN_CHANNEL_RESPONSE);

    brb->Hdr.ClientContext[0] = connectionObject;
    brb->BtAddress = ConnectParams->BtAddress;
    brb->Psm = ConnectParams->Parameters.Connect.Request.PSM;
    brb->ChannelHandle = ConnectParams->ConnectionHandle;
    brb->Response = CONNECT_RSP_RESULT_SUCCESS;

    brb->ChannelFlags = CF_ROLE_EITHER;

    brb->ConfigOut.Flags = 0;
    brb->ConfigIn.Flags = 0;

    brb->ConfigOut.Flags |= CFG_MTU;
    brb->ConfigOut.Mtu.Max = L2CAP_DEFAULT_MTU;
    brb->ConfigOut.Mtu.Min = L2CAP_MIN_MTU;
    brb->ConfigOut.Mtu.Preferred = L2CAP_DEFAULT_MTU;

    brb->ConfigIn.Flags = CFG_MTU;
    brb->ConfigIn.Mtu.Max = brb->ConfigOut.Mtu.Max;
    brb->ConfigIn.Mtu.Min = brb->ConfigOut.Mtu.Min;
    brb->ConfigIn.Mtu.Preferred = brb->ConfigOut.Mtu.Max;

    //
    // Get notifications about disconnect
    //
    brb->CallbackFlags = CALLBACK_DISCONNECT;
    brb->Callback = &BthEchoSrvConnectionIndicationCallback;
    brb->CallbackContext = connectionObject;
    brb->ReferenceObject = (PVOID) WdfDeviceWdmGetDeviceObject(DevCtx->Header.Device);

    status = BthEchoSharedSendBrbAsync(
        DevCtx->Header.IoTarget,
        connection->ConnectDisconnectRequest,
        (PBRB) brb,
        sizeof(*brb),
        BthEchoSrvRemoteConnectResponseCompletion,
        brb
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONNECT, 
            "BthEchoSharedSendBrbAsync failed, status = %!STATUS!", status);                
    }
    
exit:    
    
    if(!NT_SUCCESS(status) && connectionObject)
    {
        //
        // If we failed to connect remove connectionfrom list and
        // delete the object
        //

        //
        // connection should not be NULL if connectionObject is not NULL
        // since first thing we do after creating connectionObject is to
        // get context which gives us connection
        //
        NT_ASSERT(connection != NULL);

        if (connection != NULL)
        {
            connection->ConnectionState = ConnectionStateConnectFailed;

            RemoveConnectionEntryLocked(
                (PBTHECHOSAMPLE_SERVER_CONTEXT) connection->DevCtxHdr,
                &connection->ConnectionListEntry
                );
        }

        WdfObjectDelete(connectionObject);
    }
    
    return status;
}


#endif

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
BthEchoSrvDisconnectConnection(
    _In_ PBTHECHO_CONNECTION Connection
    )    
/*++
Description:

    Disconnects connection, removes it from the list and deletes the connection
    object.
    
    This helper function can be called for disconnect only after connect is
    completed.

Arguments:

    Connection - Connection to disconnect

--*/
{
    WDFOBJECT connectionObject;

    //
    // If BthEchoConnectionObjectRemoteDisconnect returns FALSE
    // connection is already disconnected
    // 
    if (BthEchoConnectionObjectRemoteDisconnect(Connection->DevCtxHdr, Connection))
    {
        RemoveConnectionEntryLocked(
            (PBTHECHOSAMPLE_SERVER_CONTEXT) Connection->DevCtxHdr,
            &Connection->ConnectionListEntry
            );

        connectionObject = WdfObjectContextGetObject(Connection);

        WdfObjectDelete(connectionObject);    
    }
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
BthEchoSrvDisconnectConnectionsOnRemove(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT DevCtx
    )
/*++
Description:

    This routine is invoked by BthEchoSrvEvtDeviceSelfManagedIoCleanup
    during remove. We disconnect all the outstanding connections
    in this routine.

    This routine does not wait for completion of the disconnect. connection object's
    cleanup callback waits for that.

Arguments:

    DevCtx - Server device context

--*/
{
    BOOLEAN True = TRUE;

    //
    // We will not get any new connections beacuse we have unregistered server
    // at this point.
    //
    // Although it is possible that connections from the current list 
    // get disconnected. In such case the following things protect us:
    //     1. Our reference on connection object keeps connection object alive
    //     2. Connection state tracking ensures that disconnect is sent
    //        only once
    //

    while (True)
    {
        PLIST_ENTRY ple;
        PBTHECHO_CONNECTION connection;
        
        WdfSpinLockAcquire(DevCtx->ConnectionListLock);       

        if (IsListEmpty(&DevCtx->ConnectionList))
        {
            WdfSpinLockRelease(DevCtx->ConnectionListLock);       
            break;            
        }

        ple = DevCtx->ConnectionList.Flink;

        RemoveEntryList(ple);

        connection = CONTAINING_RECORD(ple, BTHECHO_CONNECTION, ConnectionListEntry);

        //
        // Add a reference since we will be touching connection object
        // outside of the connection list lock
        //
        WdfObjectReference(WdfObjectContextGetObject(connection));        
        
        WdfSpinLockRelease(DevCtx->ConnectionListLock);       

        //
        // Invoke disconnect outside the lock
        // If client disconnects in the meantime connection state tracking ensures
        // that we send disconnect only once
        //
        BthEchoConnectionObjectRemoteDisconnect(&DevCtx->Header, connection);

        //
        // Release the reference added above
        //
        WdfObjectDereference(WdfObjectContextGetObject(connection));        

        //
        // We don't explicitly delete connection object as it is a child of device object
        // which will shortly be deleted.
        //
    }
}    

