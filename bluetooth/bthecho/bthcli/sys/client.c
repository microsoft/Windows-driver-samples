/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    client.c

Abstract:

    Contains Bluetooth client functionality.
    
Environment:

    Kernel mode only


--*/

#include "clisrv.h"
#include "device.h"
#include "client.h"

#define INITGUID
#include "public.h"

#if defined(EVENT_TRACING)
#include "client.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, BthEchoCliBthQueryInterfaces)
#pragma alloc_text (PAGE, BthEchoCliRetrieveServerSdpRecord)
#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
BthEchoCliBthQueryInterfaces(
    _In_ PBTHECHOSAMPLE_CLIENT_CONTEXT DevCtx
    )
/*++

Description:

    Query profile driver interface

Arguments:

    DevCtx - Client context where we store the interface

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
BthEchoCliRetrieveServerBthAddress(
    _In_ PBTHECHOSAMPLE_CLIENT_CONTEXT DevCtx
    )
/*++

Description:

    Retrieve server Bth address

Arguments:

    DevCtx - Client context where we store bth address

Return Value:

    NTSTATUS Status code.

--*/
{
    NTSTATUS status, statusReuse;
    WDF_MEMORY_DESCRIPTOR outMemDesc;
    WDF_REQUEST_REUSE_PARAMS ReuseParams;
    BTH_DEVICE_INFO serverDeviceInfo;
    
    WDF_REQUEST_REUSE_PARAMS_INIT(&ReuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    statusReuse = WdfRequestReuse(DevCtx->Header.Request, &ReuseParams);    
    NT_ASSERT(NT_SUCCESS(statusReuse));
    UNREFERENCED_PARAMETER(statusReuse);

    RtlZeroMemory( &serverDeviceInfo, sizeof(serverDeviceInfo) );
    
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
        &outMemDesc,
        &serverDeviceInfo,
        sizeof(serverDeviceInfo)
        );

    status = WdfIoTargetSendInternalIoctlSynchronously(
        DevCtx->Header.IoTarget,
        DevCtx->Header.Request,
        IOCTL_INTERNAL_BTHENUM_GET_DEVINFO,
        NULL,   //inMemDesc
        &outMemDesc,
        NULL,   //sendOptions
        NULL    //bytesReturned
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, 
            "Failed to obtain server device info, Status code %!STATUS!\n", status);

        goto exit;
    }

    DevCtx->ServerBthAddress = serverDeviceInfo.address;

exit:
    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
BthEchoCliRetrieveServerSdpRecord(
    _In_ PBTHECHOSAMPLE_CLIENT_CONTEXT DevCtx,
    _Out_ PBTH_SDP_STREAM_RESPONSE * ServerSdpRecord    
    )
/*++

Description:

    Retrive server SDP record.
    We call this function on every file open to get the PSM

Arguments:

    DevCtx - Client context
    ServerSdpRecord - SDP record retrieved

Return Value:

    NTSTATUS Status code.

--*/
{
    NTSTATUS status, statusReuse, disconnectStatus;
    WDF_MEMORY_DESCRIPTOR inMemDesc;
    WDF_MEMORY_DESCRIPTOR outMemDesc;
    WDF_REQUEST_REUSE_PARAMS ReuseParams;    
    BTH_SDP_CONNECT connect = {0};
    BTH_SDP_DISCONNECT disconnect = {0};
    BTH_SDP_SERVICE_ATTRIBUTE_SEARCH_REQUEST requestSdp = {0};
    BTH_SDP_STREAM_RESPONSE responseSdp = {0};
    ULONG requestSize;
    PBTH_SDP_STREAM_RESPONSE serverSdpRecord = NULL;
    WDFREQUEST request;
    WDF_OBJECT_ATTRIBUTES attributes;
    

    PAGED_CODE();

    //
    // Allocate the request we will use for obtaining sdp record
    // NOTE that we do it for every file open, hence we
    //
    // can't use reserve request from the context
    //
    
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    status = WdfRequestCreate(
        &attributes,
        DevCtx->Header.IoTarget,
        &request
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, 
            "Failed to allocate request for retriving server sdp record, Status code %!STATUS!\n", status);

        goto exit;        
    }

    connect.bthAddress = DevCtx->ServerBthAddress;
    connect.requestTimeout = SDP_REQUEST_TO_DEFAULT;
    connect.fSdpConnect = 0;

    //
    // Connect to the SDP service.
    //

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
        &inMemDesc,
        &connect,
        sizeof(connect)
        );
    
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
        &outMemDesc,
        &connect,
        sizeof(connect)
        );

    status = WdfIoTargetSendIoctlSynchronously(
        DevCtx->Header.IoTarget,
        request,
        IOCTL_BTH_SDP_CONNECT,
        &inMemDesc,
        &outMemDesc,
        NULL,   //sendOptions
        NULL    //bytesReturned
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, 
            "IOCTL_BTH_SDP_CONNECT failed, Status code %!STATUS!\n", status);

        goto exit1;
    }

    //
    // Obtain the required size of the SDP record
    //
    requestSdp.hConnection = connect.hConnection;
    requestSdp.uuids[0].u.uuid128 = BTHECHOSAMPLE_SVC_GUID;
    requestSdp.uuids[0].uuidType = SDP_ST_UUID128;
    requestSdp.range[0].minAttribute = 0;
    requestSdp.range[0].maxAttribute = 0xFFFF;

    WDF_REQUEST_REUSE_PARAMS_INIT(&ReuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    statusReuse = WdfRequestReuse(request, &ReuseParams);    
    NT_ASSERT(NT_SUCCESS(statusReuse));
    UNREFERENCED_PARAMETER(statusReuse);

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
        &inMemDesc,
        &requestSdp,
        sizeof(requestSdp)
        );
    
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
        &outMemDesc,
        &responseSdp,
        sizeof(responseSdp)
        );

    status = WdfIoTargetSendIoctlSynchronously(
        DevCtx->Header.IoTarget,
        request,
        IOCTL_BTH_SDP_SERVICE_ATTRIBUTE_SEARCH,
        &inMemDesc,
        &outMemDesc,
        NULL,   //sendOptions
        NULL    //bytesReturned
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, 
            "IOCTL_BTH_SDP_SERVICE_ATTRIBUTE_SEARCH failed while querying response size, "
            "status code %!STATUS!\n", status);

        goto exit2;
    }

    //
    // Allocate the required size for SDP record
    //

    status = RtlULongAdd(
        responseSdp.requiredSize, 
        sizeof(BTH_SDP_STREAM_RESPONSE), 
        &requestSize
        );

    if(!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, 
            "SDP record size too large, status code %!STATUS!\n", status);

        goto exit2;
    }

    serverSdpRecord = ExAllocatePool2(POOL_FLAG_NON_PAGED, requestSize, POOLTAG_BTHECHOSAMPLE);
    if (NULL == serverSdpRecord)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, 
            "Allocating SDP record failed, returning status code %!STATUS!\n", status); 

        goto exit2;
    }

    //
    // Send request with required size
    //
    
    WDF_REQUEST_REUSE_PARAMS_INIT(&ReuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    statusReuse = WdfRequestReuse(request, &ReuseParams);    
    NT_ASSERT(NT_SUCCESS(statusReuse));
    UNREFERENCED_PARAMETER(statusReuse);

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
        &inMemDesc,
        &requestSdp,
        sizeof(requestSdp)
        );
    
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
        &outMemDesc,
        serverSdpRecord,
        requestSize
        );

    status = WdfIoTargetSendIoctlSynchronously(
        DevCtx->Header.IoTarget,
        request,
        IOCTL_BTH_SDP_SERVICE_ATTRIBUTE_SEARCH,
        &inMemDesc,
        &outMemDesc,
        NULL,   //sendOptions
        NULL    //bytesReturned
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, 
            "IOCTL_BTH_SDP_SERVICE_ATTRIBUTE_SEARCH failed, status code %!STATUS!\n", status);

        ExFreePoolWithTag(serverSdpRecord, POOLTAG_BTHECHOSAMPLE);
    }
    else
    {
        *ServerSdpRecord = serverSdpRecord;
    }
    
exit2:
    
    //
    // Disconnect from SDP service.
    //
    
    WDF_REQUEST_REUSE_PARAMS_INIT(&ReuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    statusReuse = WdfRequestReuse(request, &ReuseParams);    
    NT_ASSERT(NT_SUCCESS(statusReuse));
    UNREFERENCED_PARAMETER(statusReuse);

    disconnect.hConnection = connect.hConnection;

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
        &inMemDesc,
        &disconnect,
        sizeof(disconnect)
        );
    
    disconnectStatus = WdfIoTargetSendIoctlSynchronously(
        DevCtx->Header.IoTarget,
        request,
        IOCTL_BTH_SDP_DISCONNECT,
        &inMemDesc,
        NULL,   //outMemDesc
        NULL,   //sendOptions
        NULL    //bytesReturned
        );

    NT_ASSERT(NT_SUCCESS(disconnectStatus)); //Disconnect should not fail

    if (!NT_SUCCESS(disconnectStatus))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, 
            "IOCTL_BTH_SDP_DISCONNECT failed, Status code %!STATUS!\n", status);
    }
    
exit1:    
    WdfObjectDelete(request);
exit:
    return status;
}

NTSTATUS
BthEchoCliRetrievePsmFromSdpRecord(
    _In_ PBTHDDI_SDP_PARSE_INTERFACE SdpParseInterface,
    _In_ PBTH_SDP_STREAM_RESPONSE ServerSdpRecord,
    _Out_ USHORT * Psm
    )
/*++

Description:

    Retrieve PSM from the SDP record

Arguments:

    sdpParseInterface - Parse interface used for sdp parse functions
    ServerSdpRecord - SDP record to obtain Psm from
    Psm - Psm retrieved

Return Value:

    NTSTATUS Status code.

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    PUCHAR      nextElement;
    ULONG       nextElementSize;

    PSDP_TREE_ROOT_NODE sdpTree = NULL;
    PSDP_NODE           nodeProtoDescList = NULL;
    PSDP_NODE           nodeProto0 = NULL;
    PSDP_NODE           nodeProto0UUID = NULL;
    PSDP_NODE           nodeProto0SParam0 = NULL;

    SdpParseInterface->SdpGetNextElement(
         &(ServerSdpRecord->response[0]),
         ServerSdpRecord->responseSize,
         NULL,
         &nextElement,
         &nextElementSize
         );
    
    if(nextElementSize == 0)
    {
        status = STATUS_DEVICE_DATA_ERROR;
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, 
            "Getting first element from SDP record failed, returning status code %!STATUS!\n", status);
        goto exit;
    }

    status = SdpParseInterface->SdpConvertStreamToTree(
        nextElement,
        nextElementSize,
        &sdpTree,
        POOLTAG_BTHECHOSAMPLE
        );
    
    if(!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, 
            "Converting SDP record to tree failed, status code %!STATUS!\n", status);
        goto exit;
    }

    //
    //Find PROTOCOL_DESCRIPTOR_LIST in the tree
    //
    status = SdpParseInterface->SdpFindAttributeInTree(
        sdpTree,
        (USHORT)SDP_ATTRIB_PROTOCOL_DESCRIPTOR_LIST,
        &nodeProtoDescList
        );
    
    if(!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, 
            "FindAttribute failed for SDP_ATTRIB_PROTOCOL_DESCRIPTOR_LIST, status code %!STATUS!\n", status);
        goto exit0;
    }

    if(nodeProtoDescList->hdr.Type != SDP_TYPE_SEQUENCE)
    {
        goto SdpFormatError;
    }

    //
    // Get the next sequence.
    //
    
    if(nodeProtoDescList->u.sequence.Link.Flink == NULL)
    {
        goto SdpFormatError;
    }

    nodeProto0 = CONTAINING_RECORD(nodeProtoDescList->u.sequence.Link.Flink, SDP_NODE, hdr.Link);

    if(nodeProto0->hdr.Type != SDP_TYPE_SEQUENCE)
    {
        goto SdpFormatError;
    }

    if(nodeProto0->u.sequence.Link.Flink == NULL)
    {
        goto SdpFormatError;
    }

    //
    // Get the first GUID, (L2CAP)
    //
    
    nodeProto0UUID = CONTAINING_RECORD(nodeProto0->u.sequence.Link.Flink, SDP_NODE, hdr.Link);

    if(nodeProto0UUID->hdr.Type != SDP_TYPE_UUID)
    {
        goto SdpFormatError;
    }
    
    if(nodeProto0UUID->hdr.Link.Flink == NULL)
    {
        goto SdpFormatError;
    }

    //
    // Get the PSM
    //
    
    nodeProto0SParam0 = CONTAINING_RECORD(nodeProto0UUID->hdr.Link.Flink, SDP_NODE, hdr.Link);
    if(nodeProto0SParam0->hdr.SpecificType != SDP_ST_UINT16)
    {
        goto SdpFormatError;
    }
        
    *Psm = nodeProto0SParam0->u.uint16;

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_SDP, 
        "Psm: %d", *Psm);
    
    goto exit0;

SdpFormatError:

    status = STATUS_DEVICE_DATA_ERROR;

    TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, 
        "Parsing error due to invalid SDP record, returning status code %!STATUS!\n", status);
    
exit0:

    SdpParseInterface->SdpFreeTree(sdpTree);

exit:    
    return status;    
}


EVT_WDF_REQUEST_COMPLETION_ROUTINE
BthEchoCliRemoteConnectCompletion;


#if (NTDDI_VERSION >= NTDDI_WIN8)

_IRQL_requires_max_(DISPATCH_LEVEL)
void
BthEchoCliIndicationCallback(
    _In_ PVOID Context,
    _In_ INDICATION_CODE Indication,
    _In_ PINDICATION_PARAMETERS_ENHANCED Parameters
    )
/*++

Description:

    Indication callback passed to bth stack while sending open channel BRB
    Bth stack sends notification related to the connection.
    
Arguments:

    Context - We receive data connection as the context
    Indication - Type of indication
    Parameters - Parameters of indication

--*/
{
    PBTHECHO_CONNECTION connection = (PBTHECHO_CONNECTION) Context;

    UNREFERENCED_PARAMETER(Parameters);

    //
    // Only supporting connect and disconnect
    //
    
    switch(Indication)
    {
        //
        // We don't add/release reference to anything because our connection
        // is scoped within file object lifetime
        //
        case IndicationAddReference:
        case IndicationReleaseReference:
            break;
        case IndicationRemoteConnect:
        {
            //
            // We don't expect connection
            //
            NT_ASSERT(FALSE);
            break;
        }
        case IndicationRemoteDisconnect:
        {
            //
            // This is an indication that server has disconnected
            // In response we disconnect from our end
            //
            
            BthEchoConnectionObjectRemoteDisconnect(
                connection->DevCtxHdr,
                connection
                );

            break;
        }
        case IndicationRemoteConfigRequest:
        case IndicationRemoteConfigResponse:
        case IndicationFreeExtraOptions:
            break;
    }
}


VOID
BthEchoCliRemoteConnectCompletion(
    _In_ WDFREQUEST  Request,
    _In_ WDFIOTARGET  Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS  Params,
    _In_ WDFCONTEXT  Context
    )

/*++
Description:

    Completion routine for Create request which we format as open
    channel BRB and send down the stack. We complete the Create request 
    in this routine.

    We receive open channel BRB as the context. This BRB
    is part of the request context and doesn't need to be freed
    explicitly.

    Connection is part of the context in the BRB.

Arguments:

    Request - Create request that we formatted with open channel BRB
    Target - Target to which we sent the request
    Params - Completion params
    Context - We receive BRB as the context          

Return Value:

    NTSTATUS Status code.
--*/

{
    NTSTATUS status;
    struct _BRB_L2CA_OPEN_ENHANCED_CHANNEL *brb;    
    PBTHECHOSAMPLE_CLIENT_CONTEXT DevCtx;
    PBTHECHO_CONNECTION connection;
    
    DevCtx = GetClientDeviceContext(WdfIoTargetGetDevice(Target));

    status = Params->IoStatus.Status;

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
        "Connection completion, status: %!STATUS!", status);        

    brb = (struct _BRB_L2CA_OPEN_ENHANCED_CHANNEL *) Context;

    connection = (PBTHECHO_CONNECTION) brb->Hdr.ClientContext[0];

    //
    // In the client we don't check for ConnectionStateDisconnecting state 
    // because only file close generates disconnect which
    // cannot happen before create completes. And we complete Create
    // only after we process this completion.
    //

    if(NT_SUCCESS(status))
    {
        connection->OutMTU = brb->OutResults.Params.Mtu;
        connection->InMTU = brb->InResults.Params.Mtu;
        connection->ChannelHandle = brb->ChannelHandle;
        connection->RemoteAddress = brb->BtAddress;

        connection->ConnectionState = ConnectionStateConnected;

        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
            "Connection (0x%x) established to server", brb->OutResults.Params.RetransmissionAndFlow.Mode); 

        //
        // Call the function in device.c (BthEchoCliConnectionStateConnected)
        // for any post processing after connection has been established
        //
        status = BthEchoCliConnectionStateConnected(WdfRequestGetFileObject(Request), connection);
        if (!NT_SUCCESS(status))
        {
            //
            // If such post processing fails we disconnect
            //
            BthEchoConnectionObjectRemoteDisconnect(
                &(DevCtx->Header),
                connection
                );
        }            
    }
    else
    {
        connection->ConnectionState = ConnectionStateConnectFailed;
    }

    //
    // Complete the Create request
    //
    WdfRequestComplete(Request, status);

    return;    
}


_IRQL_requires_same_
NTSTATUS
BthEchoCliOpenRemoteConnection(
    _In_ PBTHECHOSAMPLE_CLIENT_CONTEXT DevCtx,
    _In_ WDFFILEOBJECT FileObject,
    _In_ WDFREQUEST Request
    )
/*++

Description:

    This routine is invoked by BthEchoCliEvtDeviceFileCreate.
    In this routine we send down open channel BRB.

    This routine allocates open channel BRB. If the request
    is sent down successfully completion routine needs to free
    this BRB.
    
Arguments:

    _In_ PBTHECHOSAMPLE_CLIENT_CONTEXT DevCtx - 
    _In_ WDFFILEOBJECT FileObject - 
    _In_ WDFREQUEST Request - 

Return Value:

    NTSTATUS Status code.

--*/
{
    NTSTATUS status;
    WDFOBJECT connectionObject;    
    struct _BRB_L2CA_OPEN_ENHANCED_CHANNEL *brb = NULL;
    PBTHECHO_CONNECTION connection = NULL;
    PBTHECHOSAMPLE_CLIENT_FILE_CONTEXT fileCtx = GetFileContext(FileObject);
    ULONG modeConfigFlag = CM_BASIC;

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
        "Connect request");        

    //
    // Create the connection object that would store information
    // about the open channel
    //
    // Set file object as the parent for this connection object
    //
    status = BthEchoConnectionObjectCreate(
        &DevCtx->Header,
        FileObject, //parent
        &connectionObject
        );

    if (!NT_SUCCESS(status))
    {
        goto exit;
    }

    connection = GetConnectionObjectContext(connectionObject);

    connection->ConnectionState = ConnectionStateConnecting;

    //
    // Get the BRB from request context and initialize it as
    // BRB_L2CA_OPEN_CHANNEL BRB
    //
    brb = (struct _BRB_L2CA_OPEN_ENHANCED_CHANNEL *)GetRequestContext(Request);

    DevCtx->Header.ProfileDrvInterface.BthReuseBrb(
        (PBRB)brb, 
        BRB_L2CA_OPEN_ENHANCED_CHANNEL
        );
    
    brb->Hdr.ClientContext[0] = connection;
    brb->BtAddress = DevCtx->ServerBthAddress;
    brb->Psm = fileCtx->ServerPsm;

    brb->ChannelFlags = CF_ROLE_EITHER;

    brb->ConfigOut.Flags = CFG_ENHANCED;
    //
    // Open an ERTM channel if the local host supports it
    //
    if (DevCtx->Header.LocalFeatures.Mask & BTH_HOST_FEATURE_ENHANCED_RETRANSMISSION_MODE) {
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
    // Get notificaiton about remote disconnect 
    //
    brb->CallbackFlags = CALLBACK_DISCONNECT;                                                   

    brb->Callback = &BthEchoCliIndicationCallback;
    brb->CallbackContext = connection;
    brb->ReferenceObject = (PVOID) WdfDeviceWdmGetDeviceObject(DevCtx->Header.Device);
    brb->IncomingQueueDepth = 50;

    status = BthEchoSharedSendBrbAsync(
        DevCtx->Header.IoTarget,
        Request,
        (PBRB) brb,
        sizeof(*brb),
        BthEchoCliRemoteConnectCompletion,
        brb    //Context
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONNECT, 
            "Sending brb for opening connection failed, returning status code %!STATUS!\n", status);

        goto exit;
    }            

exit:

    if(!NT_SUCCESS(status))
    {
        if (connection)
        {
            //
            // Set the right state to facilitate debugging
            //
            connection->ConnectionState = ConnectionStateConnectFailed;            
        }

        //
        // In case of failure of this routine we will fail
        // Create which will delete file object and since connection object
        // is child of the file object, it will be deleted too
        //
    }
    
    return status;
}


#else

_IRQL_requires_max_(DISPATCH_LEVEL)
void
BthEchoCliIndicationCallback(
    _In_ PVOID Context,
    _In_ INDICATION_CODE Indication,
    _In_ PINDICATION_PARAMETERS Parameters
    )
/*++

Description:

    Indication callback passed to bth stack while sending open channel BRB
    Bth stack sends notification related to the connection.
    
Arguments:

    Context - We receive data connection as the context
    Indication - Type of indication
    Parameters - Parameters of indication

--*/
{
    PBTHECHO_CONNECTION connection = (PBTHECHO_CONNECTION) Context;

    UNREFERENCED_PARAMETER(Parameters);

    //
    // Only supporting connect and disconnect
    //
    
    switch(Indication)
    {
        //
        // We don't add/release reference to anything because our connection
        // is scoped within file object lifetime
        //
        case IndicationAddReference:
        case IndicationReleaseReference:
            break;
        case IndicationRemoteConnect:
        {
            //
            // We don't expect connection
            //
            NT_ASSERT(FALSE);
            break;
        }
        case IndicationRemoteDisconnect:
        {
            //
            // This is an indication that server has disconnected
            // In response we disconnect from our end
            //
            
            BthEchoConnectionObjectRemoteDisconnect(
                connection->DevCtxHdr,
                connection
                );

            break;
        }
        case IndicationRemoteConfigRequest:
        case IndicationRemoteConfigResponse:
        case IndicationFreeExtraOptions:
            break;
    }
}

VOID
BthEchoCliRemoteConnectCompletion(
    _In_ WDFREQUEST  Request,
    _In_ WDFIOTARGET  Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS  Params,
    _In_ WDFCONTEXT  Context
    )

/*++
Description:

    Completion routine for Create request which we format as open
    channel BRB and send down the stack. We complete the Create request 
    in this routine.

    We receive open channel BRB as the context. This BRB
    is part of the request context and doesn't need to be freed
    explicitly.

    Connection is part of the context in the BRB.

Arguments:

    Request - Create request that we formatted with open channel BRB
    Target - Target to which we sent the request
    Params - Completion params
    Context - We receive BRB as the context          

Return Value:

    NTSTATUS Status code.
--*/

{
    NTSTATUS status;
    struct _BRB_L2CA_OPEN_CHANNEL *brb;    
    PBTHECHOSAMPLE_CLIENT_CONTEXT DevCtx;
    PBTHECHO_CONNECTION connection;
    
    DevCtx = GetClientDeviceContext(WdfIoTargetGetDevice(Target));

    status = Params->IoStatus.Status;

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
        "Connection completion, status: %!STATUS!", status);        

    brb = (struct _BRB_L2CA_OPEN_CHANNEL *) Context;

    connection = (PBTHECHO_CONNECTION) brb->Hdr.ClientContext[0];

    //
    // In the client we don't check for ConnectionStateDisconnecting state 
    // because only file close generates disconnect which
    // cannot happen before create completes. And we complete Create
    // only after we process this completion.
    //

    if(NT_SUCCESS(status))
    {
        connection->OutMTU = brb->OutResults.Params.Mtu;
        connection->InMTU = brb->InResults.Params.Mtu;
        connection->ChannelHandle = brb->ChannelHandle;
        connection->RemoteAddress = brb->BtAddress;

        connection->ConnectionState = ConnectionStateConnected;

        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
            "Connection established to server"); 

        //
        // Call the function in device.c (BthEchoCliConnectionStateConnected)
        // for any post processing after connection has been established
        //
        status = BthEchoCliConnectionStateConnected(WdfRequestGetFileObject(Request), connection);
        if (!NT_SUCCESS(status))
        {
            //
            // If such post processing fails we disconnect
            //
            BthEchoConnectionObjectRemoteDisconnect(
                &(DevCtx->Header),
                connection
                );
        }            
    }
    else
    {
        connection->ConnectionState = ConnectionStateConnectFailed;
    }

    //
    // Complete the Create request
    //
    WdfRequestComplete(Request, status);

    return;    
}



_IRQL_requires_same_
NTSTATUS
BthEchoCliOpenRemoteConnection(
    _In_ PBTHECHOSAMPLE_CLIENT_CONTEXT DevCtx,
    _In_ WDFFILEOBJECT FileObject,
    _In_ WDFREQUEST Request
    )
/*++

Description:

    This routine is invoked by BthEchoCliEvtDeviceFileCreate.
    In this routine we send down open channel BRB.

    This routine allocates open channel BRB. If the request
    is sent down successfully completion routine needs to free
    this BRB.
    
Arguments:

    _In_ PBTHECHOSAMPLE_CLIENT_CONTEXT DevCtx - 
    _In_ WDFFILEOBJECT FileObject - 
    _In_ WDFREQUEST Request - 

Return Value:

    NTSTATUS Status code.

--*/
{
    NTSTATUS status;
    WDFOBJECT connectionObject;    
    struct _BRB_L2CA_OPEN_CHANNEL *brb = NULL;
    PBTHECHO_CONNECTION connection = NULL;
    PBTHECHOSAMPLE_CLIENT_FILE_CONTEXT fileCtx = GetFileContext(FileObject);

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
        "Connect request");        

    //
    // Create the connection object that would store information
    // about the open channel
    //
    // Set file object as the parent for this connection object
    //
    status = BthEchoConnectionObjectCreate(
        &DevCtx->Header,
        FileObject, //parent
        &connectionObject
        );

    if (!NT_SUCCESS(status))
    {
        goto exit;
    }

    connection = GetConnectionObjectContext(connectionObject);

    connection->ConnectionState = ConnectionStateConnecting;

    //
    // Get the BRB from request context and initialize it as
    // BRB_L2CA_OPEN_CHANNEL BRB
    //
    brb = (struct _BRB_L2CA_OPEN_CHANNEL *)GetRequestContext(Request);

    DevCtx->Header.ProfileDrvInterface.BthReuseBrb(
        (PBRB)brb, 
        BRB_L2CA_OPEN_CHANNEL
        );
    
    brb->Hdr.ClientContext[0] = connection;
    brb->BtAddress = DevCtx->ServerBthAddress;
    brb->Psm = fileCtx->ServerPsm;

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
    // Get notificaiton about remote disconnect 
    //
    brb->CallbackFlags = CALLBACK_DISCONNECT;                                                   

    brb->Callback = &BthEchoCliIndicationCallback;
    brb->CallbackContext = connection;
    brb->ReferenceObject = (PVOID) WdfDeviceWdmGetDeviceObject(DevCtx->Header.Device);
    brb->IncomingQueueDepth = 50;

    status = BthEchoSharedSendBrbAsync(
        DevCtx->Header.IoTarget,
        Request,
        (PBRB) brb,
        sizeof(*brb),
        BthEchoCliRemoteConnectCompletion,
        brb    //Context
        );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONNECT, 
            "Sending brb for opening connection failed, returning status code %!STATUS!\n", status);

        goto exit;
    }            

exit:

    if(!NT_SUCCESS(status))
    {
        if (connection)
        {
            //
            // Set the right state to facilitate debugging
            //
            connection->ConnectionState = ConnectionStateConnectFailed;            
        }

        //
        // In case of failure of this routine we will fail
        // Create which will delete file object and since connection object
        // is child of the file object, it will be deleted too
        //
    }
    
    return status;
}


#endif

