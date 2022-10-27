//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include "precomp.h"
#include "adapter.tmh"
#include "WMBClassTelemetry.h"

#define MBB_ADAPTER_INITIAL_REQUEST_TIMEOUT_MS   (10 * 1000 * 1000) // 10 seconds
#define MBB_ADAPTER_PRESHUTDOWN_REQUEST_TIMEOUT_MS   (6 * 1000 * 1000) // 6 seconds

//
// Adapter routines
//
_Acquires_lock_( Adapter->Lock )
__drv_raisesIRQL(DISPATCH_LEVEL)
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_savesIRQLGlobal( NdisSpinLock, Adapter )
VOID
MbbAdapterLock(
    __in PMINIPORT_ADAPTER_CONTEXT Adapter
    )
{
    NdisAcquireSpinLock( &Adapter->Lock );
}

_Releases_lock_( Adapter->Lock )
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_minIRQL(DISPATCH_LEVEL)
__drv_restoresIRQLGlobal( NdisSpinLock, Adapter )
VOID
MbbAdapterUnlock(
    __in PMINIPORT_ADAPTER_CONTEXT Adapter
    )
{
    NdisReleaseSpinLock( &Adapter->Lock );
}

_Acquires_lock_( Adapter->PortsLock )
__drv_raisesIRQL(DISPATCH_LEVEL)
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_savesIRQLGlobal( NdisSpinLock, Adapter )
VOID
MbbAdapterPortsLock(
    __in PMINIPORT_ADAPTER_CONTEXT Adapter
    )
{
    NdisAcquireSpinLock( &Adapter->PortsLock );
}

_Releases_lock_( Adapter->PortsLock )
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_minIRQL(DISPATCH_LEVEL)
__drv_restoresIRQLGlobal( NdisSpinLock, Adapter )
VOID
MbbAdapterPortsUnlock(
    __in PMINIPORT_ADAPTER_CONTEXT Adapter
    )
{
    NdisReleaseSpinLock( &Adapter->PortsLock );
}

_Acquires_lock_( Adapter->SessionIdPortTableLock )
__drv_raisesIRQL(DISPATCH_LEVEL)
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_savesIRQLGlobal( NdisSpinLock, Adapter )
VOID
MbbAdapterSessionIdPortTableLock(
    __in PMINIPORT_ADAPTER_CONTEXT Adapter
    )
{
    NdisAcquireSpinLock( &Adapter->SessionIdPortTableLock );
}

_Releases_lock_( Adapter->SessionIdPortTableLock )
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_minIRQL(DISPATCH_LEVEL)
__drv_restoresIRQLGlobal( NdisSpinLock, Adapter )
VOID
MbbAdapterSessionIdPortTableUnlock(
    __in PMINIPORT_ADAPTER_CONTEXT Adapter
    )
{
     NdisReleaseSpinLock( &Adapter->SessionIdPortTableLock );      
}


VOID
MbbAdapterRef(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    )
{
    InterlockedIncrement( &Adapter->Reference );
}

VOID
MbbAdapterDeref(
    __in PMINIPORT_ADAPTER_CONTEXT Adapter
    )
{
    if( InterlockedDecrement( &Adapter->Reference ) == 0 )
    {
        FreeAdapterBlock( Adapter );
    }
}

VOID
MbbAdapterConnectionChange(
    __in    MBB_PROTOCOL_HANDLE     ProtocolHandle,
    __in    PMBB_CONNECTION_STATE   ConnectionState,
    __in    NDIS_PORT_NUMBER        PortNumber
    )
{
    NDIS_STATUS                 NdisStatus;
    MBB_COMMAND                 IpCommand;
    PMBB_REQUEST_CONTEXT        IpRequest;
    PMBB_REQUEST_MANAGER        RequestManager = NULL;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = (PMINIPORT_ADAPTER_CONTEXT)ProtocolHandle;
    PMBB_PORT                   Port = NULL;

    do
    {
        // Get the port
        Port = MbbWwanTranslatePortNumberToPort(Adapter,PortNumber);

        if(!Port)
        {
            break;            
        }
        
        if( ConnectionState->ConnectionUp == TRUE )
        {
            //
            // Get the IP Address if connection was successful.
            // When IP is retrieved link state would be indicated to NDIS.
            //
            if( (RequestManager = MbbAdapterGetRequestManager( Adapter )) == NULL )
            {
                TraceError( WMBCLASS_INIT, "[MbbAdapter][Luid=0x%I64x] FAILED to reference RequestManager to get IP Address", Adapter->NetLuid.Value );
                break;
            }
            if( (IpRequest = MbbReqMgrCreateRequest(
                                RequestManager,
                                NULL,
                                0,
                                &NdisStatus)) == NULL )
            {
                TraceError( WMBCLASS_INIT, "[MbbAdapter][Luid=0x%I64x] FAILED to allocate RequestContext to get IP Address", Adapter->NetLuid.Value );
                break;
            }
            IpCommand.ServiceId   = MBB_UUID_BASIC_CONNECT;
            IpCommand.CommandId   = MBB_BASIC_CID_IP_ADDRESS_INFO;
            IpRequest->OidHandler = MbbNdisGetOidHandlerByCommand( &IpCommand );
            //
            // Save the connection state for indication to NDIS when IP is retrieved.
            //
            IpRequest->HandlerContext.Parameters.IpAddress.ConnectionState = *ConnectionState;
            IpRequest->HandlerContext.Parameters.IpAddress.SessionId = MbbWwanGetPortSessionId(Port);

            NdisStatus = MbbReqMgrDispatchRequest(
                            IpRequest,
                            TRUE,
                            MbbUtilInternalCIDQuery,
                            MbbUtilInternalCIDCompletion,
                            MbbUtilInternalCIDResponse
                            );
            if( NdisStatus != NDIS_STATUS_PENDING )
            {
                MbbReqMgrDestroyRequest(
                    IpRequest->RequestManager,
                    IpRequest
                    );
            }
        }
        else
        {
            MbbAdapterSetConnectionState(
                Adapter,
                ConnectionState,
                PortNumber 
                );
        }
    }
    while( FALSE );

    if(Port!=NULL)
        Dereference(Port);
    
    if( RequestManager != NULL )
        MbbReqMgrDeref( RequestManager );
}

VOID
MbbAdapterSetConnectionState(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in PMBB_CONNECTION_STATE      ConnectionState,
    __in NDIS_PORT_NUMBER           PortNumber
    )
{ 

    PMBB_PORT Port = NULL;
    BOOLEAN fIndicateLinkState = FALSE;

    Port = MbbWwanTranslatePortNumberToPort(Adapter, PortNumber);

    if(Port != NULL)
    {
        MBB_ACQUIRE_PORT_LOCK(Port);
            
        Port->ConnectionState = *ConnectionState;        
        
        MBB_RELEASE_PORT_LOCK(Port);
    }

    if(ConnectionState->ConnectionUp)
    {
        // Always indicate Link state when on a successful connection
        fIndicateLinkState = TRUE;
    }
    else
    {
        // Connection is disconnected.
        // Look at all the ports in the adapter and  indicate the link state 
        // down only if all the ports are disconnected

        ULONG NumPortsConnected = 0;
        fIndicateLinkState = ((NumPortsConnected = MbbWwanGetNumPortsConnected(Adapter))== 0);

        TraceInfo(  WMBCLASS_REQUEST_MANAGER, "[MbbAdapter] %S status LINK_STATE disconnected for Port =%lu since %d ports are connected", 
                    fIndicateLinkState ? L"Indicating" : L"Not indicating",
                    PortNumber,
                    NumPortsConnected);
    }

    if(fIndicateLinkState)
    {
       // Indicate link down            
        MbbAdapterSetLinkState(
            Adapter,
            ConnectionState,
            PortNumber);    
    }

    // Always indicate the port state
    MbbAdapterSetPortState(
        Adapter,
        ConnectionState,
        PortNumber);

    if(Port != NULL)
    {
        Dereference(Port);     
    }
}

VOID
MbbAdapterSetLinkState(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in PMBB_CONNECTION_STATE      ConnectionState,
    __in NDIS_PORT_NUMBER           PortNumber
    )
{
    NDIS_LINK_STATE LinkState;
    NDIS_STATUS_INDICATION  StatusIndication;

    LinkState.Header.Type           = NDIS_OBJECT_TYPE_DEFAULT;
    LinkState.Header.Revision       = NDIS_LINK_STATE_REVISION_1;
    LinkState.Header.Size           = NDIS_SIZEOF_LINK_STATE_REVISION_1;
    LinkState.MediaConnectState     = ConnectionState->ConnectionUp ? MediaConnectStateConnected : MediaConnectStateDisconnected;
    LinkState.MediaDuplexState      = MediaDuplexStateUnknown;
    LinkState.XmitLinkSpeed         = ConnectionState->UpStreamBitRate;
    LinkState.RcvLinkSpeed          = ConnectionState->DownStreamBitRate;
    LinkState.PauseFunctions        = NdisPauseFunctionsUnsupported;
    LinkState.AutoNegotiationFlags  = 0;

    StatusIndication.Header.Type        = NDIS_OBJECT_TYPE_STATUS_INDICATION;
    StatusIndication.Header.Revision    = NDIS_STATUS_INDICATION_REVISION_1;
    StatusIndication.Header.Size        = NDIS_SIZEOF_STATUS_INDICATION_REVISION_1;
    StatusIndication.SourceHandle       = Adapter->MiniportAdapterHandle;
    StatusIndication.PortNumber         = PortNumber;    
    StatusIndication.StatusCode         = NDIS_STATUS_LINK_STATE;
    StatusIndication.Flags              = 0;
    StatusIndication.DestinationHandle  = NULL;
    StatusIndication.RequestId          = 0;
    StatusIndication.StatusBuffer       = &LinkState; 
    StatusIndication.StatusBufferSize   = sizeof(NDIS_LINK_STATE);

    RtlZeroMemory( &StatusIndication.Guid, sizeof(StatusIndication.Guid) );    

    TraceInfo(  WMBCLASS_REQUEST_MANAGER, "[MbbAdapter] Indicating status LINK_STATE Connected=%!BOOLEAN! to NDIS, Uplink=%I64d, downlink=%I64d for Port =%lu",
                ConnectionState->ConnectionUp,
                ConnectionState->DownStreamBitRate,
                ConnectionState->UpStreamBitRate,
                PortNumber                
                );
    
    MbbUtilNdisMiniportIndicateStatusEx(
        Adapter,
        &StatusIndication
        );
}


VOID
MbbAdapterSetPortState(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in PMBB_CONNECTION_STATE      ConnectionState,
    __in NDIS_PORT_NUMBER           PortNumber
    )
{
    NDIS_PORT_STATE         PortState;
    NDIS_STATUS_INDICATION  StatusIndication;
    PMBB_PORT Port = NULL;
    
    Port = MbbWwanTranslatePortNumberToPort(Adapter, PortNumber);

    if(Port != NULL)
    {
        MBB_ACQUIRE_PORT_LOCK(Port);
            
        Port->ConnectionState = *ConnectionState;        
        
        MBB_RELEASE_PORT_LOCK(Port);
        
        Dereference(Port);        
    }   
    
    PortState.Header.Type            = NDIS_OBJECT_TYPE_DEFAULT;
    PortState.Header.Revision        = NDIS_PORT_STATE_REVISION_1;
    PortState.Header.Size            = NDIS_SIZEOF_PORT_STATE_REVISION_1;
    PortState.MediaConnectState      = ConnectionState->ConnectionUp ? MediaConnectStateConnected : MediaConnectStateDisconnected;
    PortState.XmitLinkSpeed          = ConnectionState->UpStreamBitRate;
    PortState.RcvLinkSpeed           = ConnectionState->DownStreamBitRate;
    PortState.Direction              = NET_IF_DIRECTION_SENDRECEIVE;
    PortState.SendControlState       = NdisPortControlStateUncontrolled;
    PortState.RcvControlState        = NdisPortControlStateUncontrolled;
    PortState.SendAuthorizationState = NdisPortAuthorizationUnknown;
    PortState.RcvAuthorizationState  = NdisPortAuthorizationUnknown;
    PortState.Flags                  = 0;

    StatusIndication.Header.Type        = NDIS_OBJECT_TYPE_STATUS_INDICATION;
    StatusIndication.Header.Revision    = NDIS_STATUS_INDICATION_REVISION_1;
    StatusIndication.Header.Size        = NDIS_SIZEOF_STATUS_INDICATION_REVISION_1;
    StatusIndication.SourceHandle       = Adapter->MiniportAdapterHandle;
    StatusIndication.PortNumber         = PortNumber;    
    StatusIndication.StatusCode         = NDIS_STATUS_PORT_STATE;
    StatusIndication.Flags              = 0;
    StatusIndication.DestinationHandle  = NULL;
    StatusIndication.RequestId          = 0;
    StatusIndication.StatusBuffer       = &PortState; 
    StatusIndication.StatusBufferSize   = sizeof(NDIS_PORT_STATE);

    RtlZeroMemory( &StatusIndication.Guid, sizeof(StatusIndication.Guid) );    

    TraceInfo(  WMBCLASS_REQUEST_MANAGER, "[MbbAdapter] Indicating status PORT_STATE Connected=%!BOOLEAN! to NDIS, Uplink=%I64d, downlink=%I64d for Port =%lu",
            ConnectionState->ConnectionUp,
            ConnectionState->DownStreamBitRate,
            ConnectionState->UpStreamBitRate,
            PortNumber                
            );

    MbbUtilNdisMiniportIndicateStatusEx(
        Adapter,
        &StatusIndication
        );
}

VOID
MbbPortSetIpAddressState(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in PWWAN_IP_ADDRESS_STATE     WwanIpAddressState,
    __in NDIS_PORT_NUMBER           PortNumber
    )
{
    NDIS_STATUS_INDICATION      StatusIndication;
    NDIS_WWAN_IP_ADDRESS_STATE  IpAddressState = {0};
    
    if(!WwanIpAddressState)
    {
        return;
    }
        
    IpAddressState.Header.Type          = NDIS_OBJECT_TYPE_DEFAULT;
    IpAddressState.Header.Size          = SIZEOF_NDIS_WWAN_IP_ADDRESS_STATE_1;
    IpAddressState.Header.Revision      = NDIS_WWAN_IP_ADDRESS_STATE_REVISION_1;      
    IpAddressState.WwanIpAddressState   = *WwanIpAddressState;

    StatusIndication.Header.Type        = NDIS_OBJECT_TYPE_STATUS_INDICATION;
    StatusIndication.Header.Revision    = NDIS_STATUS_INDICATION_REVISION_1;
    StatusIndication.Header.Size        = NDIS_SIZEOF_STATUS_INDICATION_REVISION_1;
    StatusIndication.SourceHandle       = Adapter->MiniportAdapterHandle;
    StatusIndication.PortNumber         = PortNumber;    
    StatusIndication.Flags              = 0;
    StatusIndication.DestinationHandle  = NULL;
    StatusIndication.RequestId          = 0;
    StatusIndication.StatusCode         = NDIS_STATUS_WWAN_IP_ADDRESS_STATE;
    StatusIndication.StatusBuffer       = &IpAddressState;
    StatusIndication.StatusBufferSize   = SIZEOF_NDIS_WWAN_IP_ADDRESS_STATE_1;
    
    RtlZeroMemory( &StatusIndication.Guid, sizeof(StatusIndication.Guid) );

    TraceInfo(  WMBCLASS_REQUEST_MANAGER, "[MbbAdapter] Indicating status NDIS_STATUS_WWAN_IPADDRESS_STATE on NDIS port number %lu",PortNumber);
    
    NdisMIndicateStatusEx(
        Adapter->MiniportAdapterHandle,
        &StatusIndication
        );    
}

BOOLEAN
MbbPortIsConnected(
    __in PMBB_PORT Port
    )
{
    BOOLEAN IsConnected;

    MBB_ACQUIRE_PORT_LOCK(Port);
    IsConnected = Port->ConnectionState.ConnectionUp;
    MBB_RELEASE_PORT_LOCK(Port);

    return IsConnected;
}

VOID
MbbAdapterIndicateD3Exit(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in NDIS_PORT_NUMBER           PortNumber
    )
{
    NDIS_STATUS_INDICATION  StatusIndication; 
    
    StatusIndication.Header.Type        = NDIS_OBJECT_TYPE_STATUS_INDICATION;
    StatusIndication.Header.Revision    = NDIS_STATUS_INDICATION_REVISION_1;
    StatusIndication.Header.Size        = NDIS_SIZEOF_STATUS_INDICATION_REVISION_1;
    StatusIndication.SourceHandle       = Adapter->MiniportAdapterHandle;
    StatusIndication.PortNumber         = PortNumber;
    StatusIndication.StatusCode         = NDIS_STATUS_WWAN_RESERVED_3; //NDIS_STATUS_WWAN_DEVICE_POWER_STATE_D3_EXIT
    StatusIndication.Flags              = 0;
    StatusIndication.DestinationHandle  = NULL;
    StatusIndication.RequestId          = 0;
    StatusIndication.StatusBuffer       = NULL; 
    StatusIndication.StatusBufferSize   = 0;

    RtlZeroMemory( &StatusIndication.Guid, sizeof(StatusIndication.Guid) );    

    TraceInfo(  WMBCLASS_REQUEST_MANAGER, "[MbbAdapter] Indicating status NDIS_STATUS_WWAN_DEVICE_POWER_STATE_D3_EXIT to NDIS for adapter with guid %!GUID! on Port number %lu",
                &(Adapter->NetCfgId),
                PortNumber                
                );

    MbbUtilNdisMiniportIndicateStatusEx(
        Adapter,
        &StatusIndication
        );   
}


NDIS_STATUS
MbbAdapterDispatchQueryDeviceServicesRequest(
    __in MBB_PROTOCOL_HANDLE    AdapterHandle,
    __in PMBB_REQUEST_CONTEXT   Request
    )
{
    NDIS_STATUS     NdisStatus = NDIS_STATUS_SUCCESS;
    MBB_COMMAND     Command;

    do
    {
        RtlCopyMemory(
            &(Command.ServiceId),
            &MBB_UUID_BASIC_CONNECT,
            sizeof(GUID)
            );
        Command.CommandId = MBB_BASIC_CID_DEVICE_SERVICES;

        if( (NdisStatus = MbbUtilSetupCommandMessage(
                                Request,
                                &Command,
                                MBB_COMMAND_TYPE_QUERY,
                                NULL,
                                0
                                )) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter][ReqId=0x%04x] FAILED to setup command message for QueryDeviceServices with status=%!status!",
                        Request->RequestId, NdisStatus );
            break;
        }
        //
        // Call the wrapper routine to send each fragment.
        // The wrapper will cleanup fragments in case of
        // success or failure.
        //
        NdisStatus = MbbUtilSendMessageFragmentsAndLog( Request );
        if( NdisStatus != NDIS_STATUS_SUCCESS &&
            NdisStatus != NDIS_STATUS_PENDING )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter][ReqId=0x%04x] FAILED to send message fragments for QueryDeviceServices with status=%!status!",
                        Request->RequestId, NdisStatus );
        }
    }
    while( FALSE );

    return NdisStatus;
}

NDIS_STATUS
MbbAdapterQueryDeviceServices(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    )
{
    NDIS_STATUS                 NdisStatus;
    LARGE_INTEGER               Timeout;
    MBB_COMMAND                 Command = { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_DEVICE_SERVICES };
    PMBB_REQUEST_CONTEXT        Request = NULL;
    PMBB_REQUEST_MANAGER        RequestManager = NULL;

    do
    {
        if( (RequestManager = MbbAdapterGetRequestManager( Adapter )) == NULL )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter] FAILED to reference RequestManager for QueryDeviceService" );
            NdisStatus = NDIS_STATUS_ADAPTER_NOT_READY;
            break;
        }
        if( (Request = MbbReqMgrCreateRequest(
                            RequestManager,
                            NULL,
                            0,
                            &NdisStatus)) == NULL )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter] FAILED to allocate RequestContext for QueryDeviceService" );
            break;
        }
        Request->OidHandler = MbbNdisGetOidHandlerByCommand( &Command );
        //
        // Ref the request so that the response handler does not free it
        //
        MbbReqMgrRefRequest( Request );

        TraceInfo(  WMBCLASS_INIT, "[MbbAdapter][ReqId=0x%04x] Internal query dispatch for BASIC_CID_DEVICE_SERVICES",
                    Request->RequestId
                    );

        NdisStatus = MbbReqMgrDispatchRequest(
                        Request,
                        TRUE, // Serialized
                        MbbAdapterDispatchQueryDeviceServicesRequest,
                        MbbUtilInternalCIDCompletion,
                        MbbUtilInternalCIDResponse
                        );
        //
        // If the request is pending wait for it to complete.
        // If the request didnt complete within the timeout period drop our ref count and fail to the caller.
        // Something is wrong with a device that does not complete a non-network request within the timeout
        // If dispatching the request failed then our completion callback will not be invoked,
        // in this case destroy the request.
        //
        if( NdisStatus == NDIS_STATUS_PENDING )
        {
            Timeout.QuadPart = -1 * 10 * MBB_ADAPTER_INITIAL_REQUEST_TIMEOUT_MS;
            NdisStatus = KeWaitForSingleObject(
                           &Request->WaitEvent,
                            Executive,
                            KernelMode,
                            TRUE, // Alertable
                           &Timeout // Timeout
                            );
            if( NdisStatus != STATUS_WAIT_0 )
            {
                NdisStatus = NDIS_STATUS_FAILURE;
            }
            else
            {
                //
                // Only if Send was successful then get the response status.
                //
                if( (NdisStatus = Request->HandlerContext.Command.SendStatus) == NDIS_STATUS_SUCCESS )
                    NdisStatus = Request->HandlerContext.Response.NdisStatus;
            }
        }
        else
        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            //
            // It is ok to destroy the request here and deref it later.
            // Destroy will not free the request while there is a pending ref.
            //
            MbbReqMgrDestroyRequest(
                Request->RequestManager,
                Request
                );
        }
    }
    while( FALSE );

    if( RequestManager != NULL )
    {
        MbbReqMgrDeref( RequestManager );
    }
    if( Request != NULL )
    {
        MbbReqMgrDerefRequest( Request );
    }
    return NdisStatus;
}


NDIS_STATUS
MbbAdapterDispatchConfigureRadioStateRequest(
    __in MBB_PROTOCOL_HANDLE    AdapterHandle,
    __in PMBB_REQUEST_CONTEXT   Request
    )
{
    NDIS_STATUS     NdisStatus = NDIS_STATUS_SUCCESS;
    MBB_COMMAND     Command;
    MBB_RADIO_STATE* MbbRadioState;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = NULL;

    do
    {
        RtlCopyMemory(
            &(Command.ServiceId),
            &MBB_UUID_BASIC_CONNECT,
            sizeof(GUID)
            );
        Command.CommandId = MBB_BASIC_CID_RADIO_STATE;

        if( (MbbRadioState = (MBB_RADIO_STATE*) ALLOCATE_NONPAGED_POOL( sizeof(MBB_RADIO_STATE) )) == NULL )
        {
            return NDIS_STATUS_RESOURCES;
        }
        Request->HandlerContext.DataToFreeOnCompletion = MbbRadioState;

        Adapter = (PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle(Request);
        *MbbRadioState = Adapter->RadioOff ? MbbRadioStateOff : MbbRadioStateOn;

        if( (NdisStatus = MbbUtilSetupCommandMessage(
                                Request,
                                &Command,
                                MBB_COMMAND_TYPE_SET,
                                (PUCHAR) MbbRadioState,
                                sizeof(MBB_RADIO_STATE)
                                )) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter][ReqId=0x%04x] FAILED to setup command message for ConfigureRadioState with status=%!status!",
                        Request->RequestId, NdisStatus );
            break;
        }
        //
        // Call the wrapper routine to send each fragment.
        // The wrapper will cleanup fragments in case of
        // success or failure.
        //
        NdisStatus = MbbUtilSendMessageFragmentsAndLog( Request );
        if( NdisStatus != NDIS_STATUS_SUCCESS &&
            NdisStatus != NDIS_STATUS_PENDING )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter][ReqId=0x%04x] FAILED to send message fragments for ConfigureRadioState with status=%!status!",
                        Request->RequestId, NdisStatus );
        }
    }
    while( FALSE );

    return NdisStatus;
}

NDIS_STATUS
MbbAdapterConfigureRadioState(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    )
{
    NDIS_STATUS                 NdisStatus;
    LARGE_INTEGER               Timeout;
    MBB_COMMAND                 Command = { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_RADIO_STATE };
    PMBB_REQUEST_CONTEXT        Request = NULL;
    PMBB_REQUEST_MANAGER        RequestManager = NULL;

    do
    {
        if( (RequestManager = MbbAdapterGetRequestManager( Adapter )) == NULL )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter] FAILED to reference RequestManager for ConfigureRadioState" );
            NdisStatus = NDIS_STATUS_ADAPTER_NOT_READY;
            break;
        }
        if( (Request = MbbReqMgrCreateRequest(
                            RequestManager,
                            NULL,
                            0,
                            &NdisStatus)) == NULL )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter] FAILED to allocate RequestContext for ConfigureRadioState" );
            break;
        }
        Request->OidHandler = MbbNdisGetOidHandlerByCommand( &Command );
        //
        // Ref the request so that the response handler does not free it
        //
        MbbReqMgrRefRequest( Request );

        NdisStatus = MbbReqMgrDispatchRequest(
                        Request,
                        TRUE, // Serialized
                        MbbAdapterDispatchConfigureRadioStateRequest,
                        MbbUtilInternalCIDCompletion,
                        MbbUtilInternalCIDResponse
                        );
        //
        // If the request is pending wait for it to complete.
        // If the request didnt complete within the timeout period drop our ref count and fail to the caller.
        // Something is wrong with a device that does not send response for a non-network request
        // If dispatching the request failed then our completion callback will not be invoked,
        // in this case destroy the request.
        //
        if( NdisStatus == NDIS_STATUS_PENDING )
        {
            Timeout.QuadPart = -1 * 10 * MBB_ADAPTER_INITIAL_REQUEST_TIMEOUT_MS;
            NdisStatus = KeWaitForSingleObject(
                            &Request->WaitEvent,
                            Executive,
                            KernelMode,
                            TRUE, // Alertable
                            &Timeout // Timeout
                            );
            if( NdisStatus != STATUS_WAIT_0 )
            {
                NdisStatus = NDIS_STATUS_FAILURE;
            }
            else
            {
                //
                // Only if Send was successful then get the response status.
                //
                if( (NdisStatus = Request->HandlerContext.Command.SendStatus) == NDIS_STATUS_SUCCESS )
                    NdisStatus = Request->HandlerContext.Response.NdisStatus;
            }
        }
        else if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            //
            // It is ok to destroy the request here and deref it later.
            // Destroy will not free the request while there is a pending ref.
            //
            MbbReqMgrDestroyRequest(
                Request->RequestManager,
                Request
                );
        }
    }
    while( FALSE );

    if( RequestManager != NULL )
    {
        MbbReqMgrDeref( RequestManager );
    }
    if( Request != NULL )
    {
        MbbReqMgrDerefRequest( Request );
    }
    return NdisStatus;
}

NDIS_STATUS
MbbAdapterPerformInitialRequests(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    //
    // We are going to wait for these requests to finish. This will block the 
    // MiniportInitialize call. For NDIS 6.30 drivers, NDIS would not be blocking
    // the PnP IRP until MiniportInitialize finishes. So we are OK with the taking
    // a while for MiniportInitialize
    //

    // Query device services
    if( (Status = MbbAdapterQueryDeviceServices( Adapter )) != NDIS_STATUS_SUCCESS )
    {
        TraceError( WMBCLASS_INIT, "[Init] FAILED to QueryDeviceService, status=%!STATUS!", Status );
        goto Cleanup;
    }

    // Set default subscribe list
    if( (Status = MbbAdapterConfigureDeviceServiceSubscription( Adapter, TRUE, 0, 0 )) != NDIS_STATUS_SUCCESS )
    {
        TraceError( WMBCLASS_INIT, "[Init] FAILED to Set initial subscribe events list, status=%!STATUS!", Status );
        goto Cleanup;
    }
    
    // Set the radio state that has been persisted in registry
    if( (Status = MbbAdapterConfigureRadioState( Adapter )) != NDIS_STATUS_SUCCESS )
    {
        TraceError( WMBCLASS_INIT, "[Init] FAILED to ConfigureRadioState, status=%!STATUS!", Status );
        goto Cleanup;
    }

Cleanup:

    return Status;
}




NDIS_STATUS
MbbAdapterDispatchConfigurePacketFiltersRequest(
    __in MBB_PROTOCOL_HANDLE    AdapterHandle,
    __in PMBB_REQUEST_CONTEXT   Request
    )
{
    NDIS_STATUS     NdisStatus = NDIS_STATUS_SUCCESS;
    MBB_COMMAND     Command;
    MBB_PACKET_FILTERS* MbbPacketFilters=NULL;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = NULL;
    ULONG                       BufferSize=0;

    do
    {
        RtlCopyMemory(
            &(Command.ServiceId),
            &MBB_UUID_BASIC_CONNECT,
            sizeof(GUID)
            );
        Command.CommandId = MBB_BASIC_CID_PACKET_FILTERS;

        Adapter = (PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle(Request);
       
        NdisStatus=MbbUtilWwanToMbbSetPacketFilter(
            Adapter,
            Request->HandlerContext.Parameters.SetPacketFilter.Set,
            Request->HandlerContext.Parameters.SetPacketFilter.PortNumber,
            &MbbPacketFilters,
            &BufferSize
            );

        if (!NT_SUCCESS(NdisStatus))
        {
            break;
        }

        Request->HandlerContext.DataToFreeOnCompletion = MbbPacketFilters;


        if( (NdisStatus = MbbUtilSetupCommandMessage(
                                Request,
                                &Command,
                                MBB_COMMAND_TYPE_SET,
                                (PUCHAR) MbbPacketFilters,
                                BufferSize
                                )) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter][ReqId=0x%04x] FAILED to setup command message for ConfigurePacketFilters with status=%!status!",
                        Request->RequestId, NdisStatus );
            break;
        }
        //
        // Call the wrapper routine to send each fragment.
        // The wrapper will cleanup fragments in case of
        // success or failure.
        //
        NdisStatus = MbbUtilSendMessageFragmentsAndLog( Request );
        if( NdisStatus != NDIS_STATUS_SUCCESS &&
            NdisStatus != NDIS_STATUS_PENDING )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter][ReqId=0x%04x] FAILED to send message fragments for ConfigurePacketFilters with status=%!status!",
                        Request->RequestId, NdisStatus );
        }
    }
    while( FALSE );

    return NdisStatus;
}

NDIS_STATUS
MbbAdapterConfigurePacketFilters(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in BOOLEAN                    Set
    )
    // Algorithm:
    // 1) Iterate through the adapter powerfilter table and check if 
    //      there are any empty slots.
    //
    // 2) Iterate over all the connected sessions and check if they are having 
    //     any WOL patterns associated with them.
    // 
    // 3) If no WOL patterns are found 
    //     {
    //          create a no-match WOL pattern and  add it to the power filter table for that session ID
    //          goto step4
    //     }
    //
    //4) Plumb the WOL patterns for connected sessions
    //
    
    // Assumptions:
    // Since this routine  is called under the context of an OID, there will not be
    // any other routines trying to alter the Adapter->PowerFilterTable
    // 
    // In case there are no WOL patterns corresponding to the session, the code will 
    // try to set no-match pattern for them, if any wake slots are available according to
    // Adapter->PowerFilterTable. We will return on first failure.
{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    ULONG           NumEmptySlots = 0; 
    ULONG           ConnectedPorts[MBB_MAX_NUMBER_OF_PORTS] = {0};
    ULONG           NumConnectedPorts = 0;
    ULONG           i = 0, j = 0;


    // Only if this is a set request for wake filters, get the number of
    // empty slots
   
    if(Set)
    {
        for (i = 0; i < Adapter->BusParams.PowerFiltersSupported; i++)
        {
            if(!Adapter->PowerFilterTable[i].InUse)
            {
                NumEmptySlots++;
            }           
        }
    }
  

    // We will create a transient array of connected ports and send the request to 
    // configure packet filters for them iteratively. No new ports are expected to be added 
    // since this routine is called in context of an OID request and until this OID request completes
    // OID to create a new context can not come. There can however be unsolicited context 
    // deactivations. In that case we will still send the request to the device which should fail it.

    MbbAdapterPortsLock(Adapter);

    for(i = 0; i < Adapter->NumberOfPorts && NumConnectedPorts < MBB_MAX_NUMBER_OF_PORTS; i++)
    {
        NDIS_PORT_NUMBER PortNumber = MBB_INVALID_PORT_NUMBER;
        PMBB_PORT Port = Adapter->PortList[i];

        MBB_ACQUIRE_PORT_LOCK(Port);

        if(Port->ConnectionState.ConnectionUp)
        {
            ASSERT(Port->SessionId != MBB_INVALID_SESSION_ID && (ULONG)Port->SessionId < Adapter->MaxActivatedContexts);
            
            PortNumber = Port->PortNumber;

            MBB_RELEASE_PORT_LOCK(Port);
            
            ConnectedPorts[NumConnectedPorts++] = PortNumber;
                       
            if(Set)
            {          
                // add a no-match WOL pattern if there are empty slots
                if(NumEmptySlots > 0)
                { 
                    // Evaluate and add no-match patterns for this
                    // port if it has no wake filter configured.

                    BOOL fHasWOLPattern = FALSE;

                    for(j = 0; j < Adapter->BusParams.PowerFiltersSupported; j++)
                    {
                        if(Adapter->PowerFilterTable[j].InUse 
                            && Adapter->PowerFilterTable[j].PortNumber == PortNumber)
                        {
                            fHasWOLPattern = TRUE;
                            break;
                        }
                    }

                    if(!fHasWOLPattern)
                    {
                        if((Status = MbbUtilAddNoMatchFilter(Adapter, Port)) == NDIS_STATUS_SUCCESS)
                        {
                            NumEmptySlots--;
                            TraceInfo( WMBCLASS_INIT, "[MbbAdapter] Remaining slots in adapter power filter table after addition of no-match filter = %d", NumEmptySlots);
                        }
                        else
                        {
                            // absorb the error
                            Status = NDIS_STATUS_SUCCESS;
                        }                   
                    }
                }               
                else
                {
                    TraceInfo( WMBCLASS_INIT, "[MbbAdapter] Not evaluating no-match filters for port number %lu since there are no empty slots", PortNumber);
                }                        
            }     
            else
            {
                // Remove the no-match filter for this port, if any              
                MbbUtilRemoveNoMatchFilter(Adapter, Port);
            }
        }
        else
        {
            MBB_RELEASE_PORT_LOCK(Port);
        }
    }

    MbbAdapterPortsUnlock(Adapter);

    if(NumConnectedPorts > 0)
    {
        for(i = 0; i < NumConnectedPorts; i++)
        {
            TraceInfo( WMBCLASS_INIT, "[MbbAdapter] Attempting to %s packet filters or no-match filters for connected port %lu", Set ? "set":"reset", ConnectedPorts[i]);
            
            if( (Status = MbbAdapterConfigurePacketFiltersOnSession(Adapter, Set, ConnectedPorts[i])) != NDIS_STATUS_SUCCESS)
            {
                TraceError( WMBCLASS_INIT, "[MbbAdapter] Failed to %s packet filters or no-match filters for port %lu. Status=%!status!", Set ? "set":"reset", ConnectedPorts[i], Status);
                // continue plumbing as many packet filters as possible
            }
            else
            {
                TraceInfo( WMBCLASS_INIT, "[MbbAdapter] Successfully %s packet filters or no-match filters for connected port %lu", Set ? "set":"reset", ConnectedPorts[i]);
            }        
        }
    }
    else
    {
        TraceWarn( WMBCLASS_INIT, "[MbbAdapter] Not attempting to %s packet filters or no-match filters as there are no connected ports", Set ? "set":"reset");
    }       
    
    return Status ;    
}


NDIS_STATUS
MbbAdapterConfigurePacketFiltersOnSession(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in BOOLEAN                    Set,
    __in NDIS_PORT_NUMBER           PortNumber
    )
{
    NDIS_STATUS                 NdisStatus;
    LARGE_INTEGER               Timeout;
    MBB_COMMAND                 Command = { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_PACKET_FILTERS };
    PMBB_REQUEST_CONTEXT        Request = NULL;
    PMBB_REQUEST_MANAGER        RequestManager = NULL;

    do
    {
        if( (RequestManager = MbbAdapterGetRequestManager( Adapter )) == NULL )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter] FAILED to reference RequestManager for ConfigurePacketFilters" );
            NdisStatus = NDIS_STATUS_ADAPTER_NOT_READY;
            break;
        }
        if( (Request = MbbReqMgrCreateRequest(
                            RequestManager,
                            NULL,
                            0,
                            &NdisStatus)) == NULL )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter] FAILED to allocate RequestContext for ConfigurePacketFilters" );
            break;
        }
        Request->OidHandler = MbbNdisGetOidHandlerByCommand( &Command );

        Request->HandlerContext.Parameters.SetPacketFilter.Set=Set;
        Request->HandlerContext.Parameters.SetPacketFilter.PortNumber = PortNumber;        

        //
        // Ref the request so that the response handler does not free it
        //
        MbbReqMgrRefRequest( Request );

        NdisStatus = MbbReqMgrDispatchRequest(
                        Request,
                        TRUE, // Serialized
                        MbbAdapterDispatchConfigurePacketFiltersRequest,
                        MbbUtilInternalCIDCompletion,
                        MbbUtilInternalCIDResponse
                        );
        //
        // If the request is pending wait for it to complete.
        // If the request didnt complete within the timeout period drop our ref count and fail to the caller.
        // Something is wrong with a device that does not send response for a non-network request
        // If dispatching the request failed then our completion callback will not be invoked,
        // in this case destroy the request.
        //
        if( NdisStatus == NDIS_STATUS_PENDING )
        {   
            Timeout.QuadPart = -1 * 10 * MBB_ADAPTER_INITIAL_REQUEST_TIMEOUT_MS;
            NdisStatus = KeWaitForSingleObject(
                            &Request->WaitEvent,
                            Executive,
                            KernelMode,
                            TRUE, // Alertable
                            &Timeout // Timeout
                            );
            if( NdisStatus != STATUS_WAIT_0 )
            {
                NdisStatus = NDIS_STATUS_FAILURE;
            }
            else
            {
                //
                // Only if Send was successful then get the response status.
                //
                if( (NdisStatus = Request->HandlerContext.Command.SendStatus) == NDIS_STATUS_SUCCESS )
                    NdisStatus = Request->HandlerContext.Response.NdisStatus;
            }
        }
        else if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            //
            // It is ok to destroy the request here and deref it later.
            // Destroy will not free the request while there is a pending ref.
            //
            MbbReqMgrDestroyRequest(
                Request->RequestManager,
                Request
                );
        }
    }
    while( FALSE );

    if( RequestManager != NULL )
    {
        MbbReqMgrDeref( RequestManager );
    }
    if( Request != NULL )
    {
        MbbReqMgrDerefRequest( Request );
    }
    return NdisStatus;
}


NDIS_STATUS
MbbAdapterDispatchConfigureDeviceServiceSubscriptionRequest(
    __in MBB_PROTOCOL_HANDLE    AdapterHandle,
    __in PMBB_REQUEST_CONTEXT   Request
    )
{
    NDIS_STATUS     NdisStatus = NDIS_STATUS_SUCCESS;
    MBB_COMMAND     Command;
    MBB_SUBSCRIBE_EVENT_LIST* MbbDeviceServiceSubscription=NULL;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = NULL;
    ULONG                       BufferSize=0;

    do
    {
        RtlCopyMemory(
            &(Command.ServiceId),
            &MBB_UUID_BASIC_CONNECT,
            sizeof(GUID)
            );
        Command.CommandId = MBB_BASIC_CID_NOTIFY_DEVICE_SERVICE_UPDATES;



        Adapter = (PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle(Request);

        NdisStatus=MbbUtilGenerateSubscribeEventList(
            Adapter,
            Request->HandlerContext.Parameters.SyncDeviceServiceSubription.FullPower,
            Request->HandlerContext.Parameters.SyncDeviceServiceSubription.MediaSpecificWakeUpEvents,
            Request->HandlerContext.Parameters.SyncDeviceServiceSubription.WakeUpFlags,
            Adapter->DeviceServiceState.ExtSubscribeList,
            Adapter->DeviceServiceState.ExtSubscribeListBufferSize,
            &MbbDeviceServiceSubscription,
            &BufferSize
            );



        if (!NT_SUCCESS(NdisStatus))
        {
            break;
        }

        Request->HandlerContext.DataToFreeOnCompletion = MbbDeviceServiceSubscription;


        if( (NdisStatus = MbbUtilSetupCommandMessage(
                                Request,
                                &Command,
                                MBB_COMMAND_TYPE_SET,
                                (PUCHAR) MbbDeviceServiceSubscription,
                                BufferSize
                                )) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter][ReqId=0x%04x] FAILED to setup command message for ConfigureDeviceServiceSubscription with status=%!status!",
                        Request->RequestId, NdisStatus );
            break;
        }
        //
        // Call the wrapper routine to send each fragment.
        // The wrapper will cleanup fragments in case of
        // success or failure.
        //
        NdisStatus = MbbUtilSendMessageFragmentsAndLog( Request );
        if( NdisStatus != NDIS_STATUS_SUCCESS &&
            NdisStatus != NDIS_STATUS_PENDING )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter][ReqId=0x%04x] FAILED to send message fragments for ConfigureDeviceServiceSubscription with status=%!status!",
                        Request->RequestId, NdisStatus );
        }
    }
    while( FALSE );

    return NdisStatus;
}

NDIS_STATUS
MbbAdapterConfigureDeviceServiceSubscription(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in BOOLEAN                    FullPower,
    __in ULONG                      MediaSpecificWakeUpEvents,
    __in ULONG                      WakeUpFlags
    )
{
    NDIS_STATUS                 NdisStatus;
    LARGE_INTEGER               Timeout;
    MBB_COMMAND                 Command = { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_NOTIFY_DEVICE_SERVICE_UPDATES };
    PMBB_REQUEST_CONTEXT        Request = NULL;
    PMBB_REQUEST_MANAGER        RequestManager = NULL;

    do
    {
        if( (RequestManager = MbbAdapterGetRequestManager( Adapter )) == NULL )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter] FAILED to reference RequestManager for ConfigureDeviceServiceSubscription" );
            NdisStatus = NDIS_STATUS_ADAPTER_NOT_READY;
            break;
        }
        if( (Request = MbbReqMgrCreateRequest(
                            RequestManager,
                            NULL,
                            0,
                            &NdisStatus)) == NULL )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter] FAILED to allocate RequestContext for ConfigureDeviceServiceSubscription" );
            break;
        }
        Request->OidHandler = MbbNdisGetOidHandlerByCommand( &Command );

        Request->HandlerContext.Parameters.SyncDeviceServiceSubription.FullPower=FullPower;
        Request->HandlerContext.Parameters.SyncDeviceServiceSubription.MediaSpecificWakeUpEvents=MediaSpecificWakeUpEvents;
        Request->HandlerContext.Parameters.SyncDeviceServiceSubription.WakeUpFlags=WakeUpFlags;
        //
        // Ref the request so that the response handler does not free it
        //
        MbbReqMgrRefRequest( Request );

        NdisStatus = MbbReqMgrDispatchRequest(
                        Request,
                        TRUE, // Serialized
                        MbbAdapterDispatchConfigureDeviceServiceSubscriptionRequest,
                        MbbUtilInternalCIDCompletion,
                        MbbUtilInternalCIDResponse
                        );
        //
        // If the request is pending wait for it to complete.
        // If the request didnt complete within the timeout period drop our ref count and fail to the caller.
        // Something is wrong with a device that does not send response for a non-network request
        // If dispatching the request failed then our completion callback will not be invoked,
        // in this case destroy the request.
        //
        if( NdisStatus == NDIS_STATUS_PENDING )
        {
            Timeout.QuadPart = -1 * 10 * MBB_ADAPTER_INITIAL_REQUEST_TIMEOUT_MS;
            NdisStatus = KeWaitForSingleObject(
                            &Request->WaitEvent,
                            Executive,
                            KernelMode,
                            TRUE, // Alertable
                            &Timeout // Timeout
                            );
            if( NdisStatus != STATUS_WAIT_0 )
            {
                NdisStatus = NDIS_STATUS_FAILURE;
            }
            else
            {
                //
                // Only if Send was successful then get the response status.
                //
                if( (NdisStatus = Request->HandlerContext.Command.SendStatus) == NDIS_STATUS_SUCCESS )
                    NdisStatus = Request->HandlerContext.Response.NdisStatus;
            }
        }
        else if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            //
            // It is ok to destroy the request here and deref it later.
            // Destroy will not free the request while there is a pending ref.
            //
            MbbReqMgrDestroyRequest(
                Request->RequestManager,
                Request
                );
        }
    }
    while( FALSE );

    if( RequestManager != NULL )
    {
        MbbReqMgrDeref( RequestManager );
    }
    if( Request != NULL )
    {
        MbbReqMgrDerefRequest( Request );
    }
    return NdisStatus;
}

NDIS_STATUS
MbbAdapterDispatchSendNetworkIdleHintRequest(
    __in MBB_PROTOCOL_HANDLE    AdapterHandle,
    __in PMBB_REQUEST_CONTEXT   Request
    )
{
    NDIS_STATUS     NdisStatus = NDIS_STATUS_SUCCESS;
    MBB_COMMAND     Command;
    MBB_SUBSCRIBE_EVENT_LIST* MbbDeviceServiceSubscription=NULL;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = NULL;
    ULONG                       BufferSize=0;
    PMBB_NETWORK_IDLE_HINT      IdleHint=NULL;

    do
    {
        RtlCopyMemory(
            &(Command.ServiceId),
            &MBB_UUID_BASIC_CONNECT,
            sizeof(GUID)
            );
        Command.CommandId = MBB_BASIC_CID_NETWORK_IDLE_HINT;

        IdleHint=ALLOCATE_NONPAGED_POOL(sizeof(*IdleHint));

        if (IdleHint == NULL)
        {
            NdisStatus=NDIS_STATUS_RESOURCES;
            break;
        }

        IdleHint->NetworkIdleHintState=MbbNetworkIdleHintEnabled;

        Request->HandlerContext.DataToFreeOnCompletion = IdleHint;


        if( (NdisStatus = MbbUtilSetupCommandMessage(
                                Request,
                                &Command,
                                MBB_COMMAND_TYPE_SET,
                                (PUCHAR) IdleHint,
                                sizeof(*IdleHint)
                                )) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter][ReqId=0x%04x] FAILED to setup command message for SendNetworkIdleHint with status=%!status!",
                        Request->RequestId, NdisStatus );
            break;
        }
        //
        // Call the wrapper routine to send each fragment.
        // The wrapper will cleanup fragments in case of
        // success or failure.
        //
        NdisStatus = MbbUtilSendMessageFragmentsAndLog( Request );
        if( NdisStatus != NDIS_STATUS_SUCCESS &&
            NdisStatus != NDIS_STATUS_PENDING )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter][ReqId=0x%04x] FAILED to send message fragments for SendNetworkIdleHint with status=%!status!",
                        Request->RequestId, NdisStatus );
        }
    }
    while( FALSE );

    return NdisStatus;
}

NDIS_STATUS
MbbAdapterSendNetworkIdleHint(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    )
{
    NDIS_STATUS                 NdisStatus;
    LARGE_INTEGER               Timeout;
    MBB_COMMAND                 Command = { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_NETWORK_IDLE_HINT };
    PMBB_REQUEST_CONTEXT        Request = NULL;
    PMBB_REQUEST_MANAGER        RequestManager = NULL;

    do
    {
        if( (RequestManager = MbbAdapterGetRequestManager( Adapter )) == NULL )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter] FAILED to reference RequestManager for SendNetworkIdleHint" );
            NdisStatus = NDIS_STATUS_ADAPTER_NOT_READY;
            break;
        }
        if( (Request = MbbReqMgrCreateRequest(
                            RequestManager,
                            NULL,
                            0,
                            &NdisStatus)) == NULL )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter] FAILED to allocate RequestContext for SendNetworkIdleHint" );
            break;
        }
        Request->OidHandler = MbbNdisGetOidHandlerByCommand( &Command );

        //
        // Ref the request so that the response handler does not free it
        //
        MbbReqMgrRefRequest( Request );

        NdisStatus = MbbReqMgrDispatchRequest(
                        Request,
                        TRUE, // Serialized
                        MbbAdapterDispatchSendNetworkIdleHintRequest,
                        MbbUtilInternalCIDCompletion,
                        MbbUtilInternalCIDResponse
                        );
        //
        // If the request is pending wait for it to complete.
        // If the request didnt complete within the timeout period drop our ref count and fail to the caller.
        // Something is wrong with a device that does not send response for a non-network request
        // If dispatching the request failed then our completion callback will not be invoked,
        // in this case destroy the request.
        //
        if( NdisStatus == NDIS_STATUS_PENDING )
        {
            Timeout.QuadPart = -1 * 10 * MBB_ADAPTER_INITIAL_REQUEST_TIMEOUT_MS;
            NdisStatus = KeWaitForSingleObject(
                            &Request->WaitEvent,
                            Executive,
                            KernelMode,
                            TRUE, // Alertable
                            &Timeout // Timeout
                            );
            if( NdisStatus != STATUS_WAIT_0 )
            {
                NdisStatus = NDIS_STATUS_FAILURE;
            }
            else
            {
                //
                // Only if Send was successful then get the response status.
                //
                if( (NdisStatus = Request->HandlerContext.Command.SendStatus) == NDIS_STATUS_SUCCESS )
                    NdisStatus = Request->HandlerContext.Response.NdisStatus;
            }
        }
        else if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            //
            // It is ok to destroy the request here and deref it later.
            // Destroy will not free the request while there is a pending ref.
            //
            MbbReqMgrDestroyRequest(
                Request->RequestManager,
                Request
                );
        }
    }
    while( FALSE );

    if( RequestManager != NULL )
    {
        MbbReqMgrDeref( RequestManager );
    }
    if( Request != NULL )
    {
        MbbReqMgrDerefRequest( Request );
    }
    return NdisStatus;
}

NDIS_STATUS
MbbAdapterDispatchSendNetworkShutdownHintRequest(
    __in MBB_PROTOCOL_HANDLE    AdapterHandle,
    __in PMBB_REQUEST_CONTEXT   Request
    )
{
    NDIS_STATUS     NdisStatus = NDIS_STATUS_SUCCESS;
    MBB_COMMAND     Command;
    MBB_SUBSCRIBE_EVENT_LIST* MbbDeviceServiceSubscription=NULL;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = NULL;
    ULONG                       BufferSize=0;

    do
    {
        RtlCopyMemory(
            &(Command.ServiceId),
            &MBB_UUID_HOSTSHUTDOWN,
            sizeof(GUID)
            );
        Command.CommandId = MBB_HOSTSHUTDOWN_CID_ONE;



        if( (NdisStatus = MbbUtilSetupCommandMessage(
                                Request,
                                &Command,
                                MBB_COMMAND_TYPE_SET,
                                NULL,
                                0
                                )) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter][ReqId=0x%04x] FAILED to setup command message for SendNetworkShutdownHint with status=%!status!",
                        Request->RequestId, NdisStatus );
            break;
        }
        //
        // Call the wrapper routine to send each fragment.
        // The wrapper will cleanup fragments in case of
        // success or failure.
        //
        NdisStatus = MbbUtilSendMessageFragmentsAndLog( Request );
        if( NdisStatus != NDIS_STATUS_SUCCESS &&
            NdisStatus != NDIS_STATUS_PENDING )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter][ReqId=0x%04x] FAILED to send message fragments for SendNetworkShutdownHint with status=%!status!",
                        Request->RequestId, NdisStatus );
        }
    }
    while( FALSE );

    return NdisStatus;
}

NDIS_STATUS
MbbAdapterSendNetworkShutdownHint(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    )
{
    NDIS_STATUS                 NdisStatus;
    LARGE_INTEGER               Timeout;
    MBB_COMMAND                 Command = { MBB_UUID_HOSTSHUTDOWN_CONSTANT, MBB_HOSTSHUTDOWN_CID_ONE };
    PMBB_REQUEST_CONTEXT        Request = NULL;
    PMBB_REQUEST_MANAGER        RequestManager = NULL;
    BOOLEAN                     HintSupport=FALSE;

    do
    {
        MbbAdapterLock( Adapter );
        HintSupport = Adapter->AdapterFlags.ShutdownNotificationCapable == 1;
        MbbAdapterUnlock( Adapter );

        if (!HintSupport)
        {
            TraceInfo( WMBCLASS_INIT, "[MbbAdapter] device does not support NetworkShutdownHint" );
            NdisStatus=STATUS_SUCCESS;
            break;
        }



        if( (RequestManager = MbbAdapterGetRequestManager( Adapter )) == NULL )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter] FAILED to reference RequestManager for SendNetworkShutdownHint" );
            NdisStatus = NDIS_STATUS_ADAPTER_NOT_READY;
            break;
        }
        if( (Request = MbbReqMgrCreateRequest(
                            RequestManager,
                            NULL,
                            0,
                            &NdisStatus)) == NULL )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter] FAILED to allocate RequestContext for SendNetworkShutdownHint" );
            break;
        }
        Request->OidHandler = MbbNdisGetOidHandlerByCommand( &Command );

        //
        // Ref the request so that the response handler does not free it
        //
        MbbReqMgrRefRequest( Request );

        NdisStatus = MbbReqMgrDispatchRequest(
                        Request,
                        TRUE, // Serialized
                        MbbAdapterDispatchSendNetworkShutdownHintRequest,
                        MbbUtilInternalCIDCompletion,
                        MbbUtilInternalCIDResponse
                        );
        //
        // If the request is pending wait for it to complete.
        // If the request didnt complete within the timeout period drop our ref count and fail to the caller.
        // Something is wrong with a device that does not send response for a non-network request
        // If dispatching the request failed then our completion callback will not be invoked,
        // in this case destroy the request.
        //
        if( NdisStatus == NDIS_STATUS_PENDING )
        {
            Timeout.QuadPart = -1 * 10 * 500000;
            NdisStatus = KeWaitForSingleObject(
                            &Request->WaitEvent,
                            Executive,
                            KernelMode,
                            TRUE, // Alertable
                            &Timeout // Timeout
                            );
            if( NdisStatus != STATUS_WAIT_0 )
            {
                NdisStatus = NDIS_STATUS_FAILURE;
            }
            else
            {
                //
                // Only if Send was successful then get the response status.
                //
                if( (NdisStatus = Request->HandlerContext.Command.SendStatus) == NDIS_STATUS_SUCCESS )
                    NdisStatus = Request->HandlerContext.Response.NdisStatus;
            }
        }
        else if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            //
            // It is ok to destroy the request here and deref it later.
            // Destroy will not free the request while there is a pending ref.
            //
            MbbReqMgrDestroyRequest(
                Request->RequestManager,
                Request
                );
        }
    }
    while( FALSE );

    if( RequestManager != NULL )
    {
        MbbReqMgrDeref( RequestManager );
    }
    if( Request != NULL )
    {
        MbbReqMgrDerefRequest( Request );
    }
    return NdisStatus;
}

NDIS_STATUS
MbbAdapterDispatchSendPreShutdownRequest(
    _In_ MBB_PROTOCOL_HANDLE    AdapterHandle,
    _In_ PMBB_REQUEST_CONTEXT   Request
)
{
    NDIS_STATUS NdisStatus = MbbUtilSetAttributeWithParameter(
        Request,
        NULL,
        0);

    if ((NdisStatus != NDIS_STATUS_SUCCESS) &&
        (NdisStatus != NDIS_STATUS_PENDING))
    {
        TraceError(WMBCLASS_INIT, "[MbbNdis][ReqID=0x%04x] SendPreShutdownRequest Set failed with status=%!status!",
            Request->RequestId, NdisStatus);
    }

    return NdisStatus;
}

NDIS_STATUS
MbbAdapterSendPreShutdown(
    _In_ PMINIPORT_ADAPTER_CONTEXT  Adapter
)
{
    NDIS_STATUS                 NdisStatus;
    LARGE_INTEGER               Timeout;
    MBB_COMMAND                 Command = { MBB_UUID_HOSTSHUTDOWN_CONSTANT, MBB_HOSTSHUTDOWN_CID_PRESHUTDOWN };
    PMBB_REQUEST_CONTEXT        Request = NULL;
    PMBB_REQUEST_MANAGER        RequestManager = NULL;
    BOOLEAN                     IsPreshutdownSupport = FALSE;

    MbbAdapterLock(Adapter);
    IsPreshutdownSupport = (TRUE == Adapter->AdapterFlags.IsPreshutdownCapable);
    MbbAdapterUnlock(Adapter);

    do {
        if (!IsPreshutdownSupport)
        {
            TraceInfo(WMBCLASS_INIT, "[MbbAdapter] device does not support PreShutdown");
            NdisStatus = STATUS_SUCCESS;
            break;
        }

        if ((RequestManager = MbbAdapterGetRequestManager(Adapter)) == NULL)
        {
            TraceError(WMBCLASS_INIT, "[MbbAdapter] FAILED to reference RequestManager for SendPreShutdown");
            NdisStatus = NDIS_STATUS_ADAPTER_NOT_READY;
            break;
        }

        if ((Request = MbbReqMgrCreateRequest(
            RequestManager,
            NULL,
            0,
            &NdisStatus)) == NULL)
        {
            TraceError(WMBCLASS_INIT, "[MbbAdapter] FAILED to allocate RequestContext for SendPreShutdown");
            break;
        }
        Request->OidHandler = MbbNdisGetOidHandlerByCommand(&Command);

        //
        // Ref the request so that the response handler does not free it
        //
        MbbReqMgrRefRequest(Request);

        NdisStatus = MbbReqMgrDispatchRequest(
            Request,
            TRUE, // Serialized
            MbbAdapterDispatchSendPreShutdownRequest,
            MbbUtilInternalCIDCompletion,
            MbbUtilInternalCIDResponse
        );
        //
        // The request is dispatched and queued. We expect to get NDIS_STATUS_PENDING and wait for it to complete.
        //
        if (NdisStatus == NDIS_STATUS_PENDING)
        {
            // A positive value specifies an absolute time, relative to January 1, 1601.
            // Here we use a NEGATIVE value to specify an interval relative to the current time.
            Timeout.QuadPart = -10 * MBB_ADAPTER_PRESHUTDOWN_REQUEST_TIMEOUT_MS; // 100-nanosecond unit;
            NdisStatus = KeWaitForSingleObject(
                &Request->WaitEvent,
                Executive,
                KernelMode,
                TRUE, // Alertable
                &Timeout
            );
            if (NdisStatus != STATUS_WAIT_0)
            {
                NdisStatus = NDIS_STATUS_FAILURE;
            }
            else
            {
                // get the response status only when Send was successful
                if ((NdisStatus = Request->HandlerContext.Command.SendStatus) == NDIS_STATUS_SUCCESS)
                {
                    NdisStatus = Request->HandlerContext.Response.NdisStatus;
                }
            }
        }
        else if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            // Something wrong when dispatching request, destroy it and deref later
            MbbReqMgrDestroyRequest(
                Request->RequestManager,
                Request
            );
        }
    } while (FALSE);

    if (RequestManager != NULL)
    {
        MbbReqMgrDeref(RequestManager);
    }
    if (Request != NULL)
    {
        MbbReqMgrDerefRequest(Request);
    }
    return NdisStatus;
}

VOID
AdapterPauseHandler(
    PSTATE_CHANGE_EVENT             StateChange
    )
{
    PMINIPORT_ADAPTER_CONTEXT Adapter=(PMINIPORT_ADAPTER_CONTEXT)StateChange->Context1;
    BOOLEAN                                     StopSendQueue=FALSE;
    BOOLEAN                                     NoOustandingNBLs=FALSE;


    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Entered");
    if (FastIOPause(StateChange))
    {
        return;
    }

    NdisAcquireSpinLock(&Adapter->Lock);

    ASSERT(Adapter->AdapterState.Started);

    Adapter->AdapterState.Started=FALSE;


    //
    //  add one for this routine running now
    //
    Adapter->AdapterState.PendingActions++;

    //
    //  power changes don't effect the receive control, so we stop it now
    //
    NoOustandingNBLs=TRUE;
    Adapter->AdapterState.PendingActions++;


    if (Adapter->AdapterState.CurrentPowerState == NetDeviceStateD0)
    {
        //
        //  if the device is powered,
        //  stop everything
        //
        StopSendQueue=TRUE;
        Adapter->AdapterState.PendingActions++;
    }
    else
    {
        //
        //  not powered, The send queue is already stopped
        //

    }

    Adapter->AdapterState.RunningEvent=StateChange;

    NdisReleaseSpinLock(&Adapter->Lock);

    if (StopSendQueue)
    {
        MbbSendQCancel(
            &Adapter->SendQueue,
            NDIS_STATUS_PAUSED,
            TRUE
            );

        //
        //  stop the pipes and cancel the io
        //
        MbbBusStopDataPipes(Adapter->BusHandle);

    }

    if (NoOustandingNBLs)
    {
        MbbRecvQCancel(
            &Adapter->ReceiveQueue,
            NDIS_STATUS_PAUSED,
            TRUE
            );
    }



    //
    //  call this to remove the pending action count added above
    //
    DrainCompleteCallback(Adapter);

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Exited");

    return;
}


VOID
AdapterRestartHandler(
    PSTATE_CHANGE_EVENT             StateChange
    )
{
    PMINIPORT_ADAPTER_CONTEXT   Adapter=(PMINIPORT_ADAPTER_CONTEXT)StateChange->Context1;
    NTSTATUS                    Status=STATUS_SUCCESS;

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Entered");
    if (FastIORestart(StateChange))
    {
        return;
    }

    if (Adapter->AdapterState.CurrentPowerState == NetDeviceStateD0)
    {
        Status=MbbBusStartDataPipes(Adapter->BusHandle);

        if (NT_SUCCESS(Status))
        {

            //
            //  powered up, start send queue
            //
            DrainComplete( &(Adapter->SendQueue.QueueDrainObject) );
        }
    }

    //
    //  always enable receive control because power does not touch it
    //
    DrainComplete( &(Adapter->ReceiveQueue.QueueDrainObject) );

    Adapter->AdapterState.Started=TRUE;

    CompleteStateChange(
        &Adapter->AdapterState,
        StateChange
        );

    StateChange=NULL;

    TraceInfo(WMBCLASS_INIT, "Calling NdisMRestartComplete()");

    MbbWriteEvent(
        &RESTART_COMPLETE_EVENT,
        NULL,
        NULL,
        2,
        &Adapter->TraceInstance,
        sizeof(Adapter->TraceInstance),
        &Status,
        sizeof(Status)
        );


    NdisMRestartComplete(Adapter->MiniportAdapterHandle, Status);

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Exited");
}


VOID
AdapterPowerHandler(
    PSTATE_CHANGE_EVENT             StateChange
    )
{
    PMINIPORT_ADAPTER_CONTEXT   Adapter=(PMINIPORT_ADAPTER_CONTEXT)StateChange->Context1;
    BOOLEAN                     StartSendQueue=FALSE;
    BOOLEAN                     StopSendQueue=FALSE;
    NET_DEVICE_POWER_STATE      PreviousPowerState;
    BOOLEAN                     FullPower=FALSE;
    NTSTATUS                    Status=STATUS_SUCCESS;

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Entered");

    NdisAcquireSpinLock(&Adapter->Lock);


    PreviousPowerState=Adapter->AdapterState.CurrentPowerState;
    Adapter->AdapterState.CurrentPowerState = StateChange->Power.NewPower;

    if (StateChange->Power.NewPower == NetDeviceStateD0)
    {
        //
        //  full power
        //
        if (Adapter->AdapterState.Started)
        {
            StartSendQueue=TRUE;
        }
        FullPower=TRUE;

    }
    else
    {
        //
        //  reduced power
        //
        ASSERT(Adapter->AdapterState.PendingActions == 0);

        FullPower=FALSE;

        if (Adapter->AdapterState.Started && MbbBusIsFastIO(Adapter->BusHandle) == FALSE)
        {
            //
            //  started, stop the send queue
            //
            StopSendQueue=TRUE;
            Adapter->AdapterState.PendingActions++;

        }

        Adapter->AdapterState.PendingActions++;

    }

    NdisReleaseSpinLock(&Adapter->Lock);


    if (FullPower)
    {
        //
        //  D0
        //

        if (PreviousPowerState == NetDeviceStateD3)
        {          
            do
            {
                //
                //  device was in d3, open it again
                //
                Status = MbbBusOpen(Adapter->BusHandle, MbbLibraryGetNextTransactionId(), FastIOSendNetBufferListsComplete, FastIOIndicateReceiveNetBufferLists);

                if (NT_ERROR(Status))
                {
                    TraceError(WMBCLASS_INIT, "[Init] MbbBusOpen() with %!STATUS!", Status);
                    break;
                }

                //
                //  opened, make sure the radio is in the right state
                //
                Status = MbbAdapterConfigureRadioState( Adapter ); 
                if (NT_ERROR(Status))
                {
                    TraceError(WMBCLASS_INIT, "D0 power up FAILED to ConfigureRadioState, status=%!STATUS!. Reset data pipe as workaround for surface in order to reset modem", Status);
                    MbbBusResetBulkPipe(Adapter->BusHandle, TRUE);

                    TraceLoggingWrite(
                        g_hLoggingProvider,
                        "MbbPipeReset",
                        TraceLoggingString("PowerHandler", "ResetType"),
                        TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));

                    break;
                }

                // send an indication to service indicating we are out of D3
                MbbAdapterIndicateD3Exit(Adapter, NDIS_DEFAULT_PORT_NUMBER); //always indicate on port 0
            }while(FALSE);

            if(NT_ERROR(Status))
            {
                NdisMRemoveMiniport(
                    Adapter->MiniportAdapterHandle
                    );
            }                  
        }

        if (StartSendQueue && MbbBusIsFastIO(Adapter->BusHandle) == FALSE)
        {
            //
            //  re-enable the send queue
            //

            DrainComplete( &(Adapter->SendQueue.QueueDrainObject) );

            if NT_SUCCESS(Status)
            {

                MbbBusStartDataPipes(Adapter->BusHandle);
            }
        }


        Adapter->LastLowSystemPowerState.TargetSystemState = PowerSystemWorking;
        Adapter->LastLowSystemPowerState.EffectiveSystemState = PowerSystemWorking;

        MbbBusSetNotificationState(Adapter->BusHandle, TRUE);

        MbbAdapterConfigurePacketFilters(Adapter, FALSE);

        MbbAdapterConfigureDeviceServiceSubscription(Adapter, TRUE, 0, 0);

        TraceInfo(WMBCLASS_INIT, "Completing set power request");

        //
        //  done
        //
        MbbReqMgrQueueEvent(
            StateChange->Power.Request->RequestManager,
            StateChange->Power.Request,
            MbbRequestEventSendComplete,
            (PVOID)(NDIS_STATUS_SUCCESS),
            0
            );


        CompleteStateChange(
            &Adapter->AdapterState,
            StateChange
            );

        StateChange=NULL;
    }
    else
    {
        //
        //  low power
        //
        Adapter->AdapterState.RunningEvent=StateChange;

        if (StopSendQueue)
        {

            MbbSendQCancel(
                &Adapter->SendQueue,
                NDIS_STATUS_PAUSED,
                TRUE
                );

            MbbBusStopDataPipes(Adapter->BusHandle);
        }

        if ((PreviousPowerState == NetDeviceStateD0) && (Adapter->AdapterState.CurrentPowerState == NetDeviceStateD3))
        {
            if (Adapter->LastLowSystemPowerState.TargetSystemState > PowerSystemSleeping3)
            {
                // when the system supports hibernate and user requests shutdown, the system will try 
                // hibernate first rather than full shutdown.
                // That is, (TargetSysState: PowerSystemShutdown, EffectiveSysState: PowerSystemHibernate)
                if (Adapter->LastLowSystemPowerState.EffectiveSystemState == PowerSystemHibernate)
                {
                    // If the suspend event is S4, the network device would power off
                    // send preshutdown to make it cleanup in case the device is flashless.
                    // We don't send it for S5 here because S5 is handled by OID request from wwansvc
                    MbbAdapterSendPreShutdown(Adapter);
                }
                //
                //  only send in the case the sleep is greater than S3
                //
                MbbAdapterSendNetworkShutdownHint(Adapter);
            }

            MbbBusSetNotificationState(Adapter->BusHandle, FALSE);

            MbbBusClose(Adapter->BusHandle,MbbLibraryGetNextTransactionId(), FALSE);
        }
        else
        {
            MbbBusSetNotificationState(Adapter->BusHandle, FALSE);
        }

        //
        //  the request will be completed when the drain has completed
        //
        DrainCompleteCallback(Adapter);


    }

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Exited");

    return;


}

VOID
AdapterResetHandler(
    PSTATE_CHANGE_EVENT             StateChange
    )
{
    PMINIPORT_ADAPTER_CONTEXT   Adapter=(PMINIPORT_ADAPTER_CONTEXT)StateChange->Context1;
    BOOLEAN                     StopSendQueue=FALSE;

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Entered");
    if (FastIOReset(StateChange))
    {
        return;
    }
    //
    //  add one for this routing running now
    //
    Adapter->AdapterState.PendingActions++;


    if ((Adapter->AdapterState.CurrentPowerState == NetDeviceStateD0) && Adapter->AdapterState.Started)
    {
        //
        //  if the device is powered and started
        //  stop everything
        //
        StopSendQueue=TRUE;
        Adapter->AdapterState.PendingActions++;
    }
    else
    {
        //
        //  not powered, The send queue is already stopped
        //

    }

    StateChange->Reset.PipeStartStatus=STATUS_SUCCESS;

    if (StopSendQueue)
    {
        MbbSendQCancel(
            &Adapter->SendQueue,
            NDIS_STATUS_PAUSED,
            TRUE
            );

        //
        //  stop and start the pipes to cancel any io
        //
        MbbBusStopDataPipes(Adapter->BusHandle);

        StateChange->Reset.PipeStartStatus = MbbBusResetBulkPipe(Adapter->BusHandle, TRUE);

        TraceLoggingWrite(
            g_hLoggingProvider,
            "MbbPipeReset",
            TraceLoggingString("ResetHandler", "ResetType"),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));

        MbbBusStartDataPipes(Adapter->BusHandle);

    }

    Adapter->AdapterState.RunningEvent=StateChange;

    //
    //  call this to remove the pending action count add above
    //
    DrainCompleteCallback(Adapter);

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Exited");

    return;


}

VOID
AdapterStallClearHandler(
    PSTATE_CHANGE_EVENT             StateChange
    )
{
    PMINIPORT_ADAPTER_CONTEXT   Adapter=(PMINIPORT_ADAPTER_CONTEXT)StateChange->Context1;
    BOOLEAN                     StartSendQueue=FALSE;

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Entered");

    NdisAcquireSpinLock(&Adapter->Lock);
    //
    //  add one for this routine running now
    //
    Adapter->AdapterState.PendingActions++;


    if ((Adapter->AdapterState.CurrentPowerState == NetDeviceStateD0) && Adapter->AdapterState.Started)
    {
        //
        //  if the device is powered and started
        //  stop everything
        //
        StartSendQueue=TRUE;
        Adapter->AdapterState.PendingActions++;
    }
    else
    {
        //
        //  not powered, The send queue is already stopped
        //

    }
    NdisReleaseSpinLock(&Adapter->Lock);

    if(StartSendQueue)
    {
        MbbSendQCancel(
            &Adapter->SendQueue,
            NDIS_STATUS_PAUSED,
            TRUE
            );
    }

    //
    //  stop and start the pipes to cancel any io
    //
    MbbBusStopDataPipes(Adapter->BusHandle);

    MbbBusResetBulkPipe(Adapter->BusHandle, TRUE);

    TraceLoggingWrite(
        g_hLoggingProvider,
        "MbbPipeReset",
        TraceLoggingString("StallClearHandler", "ResetType"),
        TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));

    if (StartSendQueue)
    {
        MbbBusStartDataPipes(Adapter->BusHandle);
        DrainComplete(&(Adapter->SendQueue.QueueDrainObject));
    }

    Adapter->AdapterState.RunningEvent=StateChange;
    //
    //  call this to remove the pending action count add above
    //
    DrainCompleteCallback(Adapter);

    return;


}




NTSTATUS
InitAdapterStateObject(
    __out PADAPTER_STATE      AdapterState,
    __in  NDIS_HANDLE               MiniportAdapterContext,
    MBB_STATE_CHANGE_HANDLER        PauseHandler,
    MBB_STATE_CHANGE_HANDLER        RestartHandler,
    MBB_STATE_CHANGE_HANDLER        PowerHandler,
    MBB_STATE_CHANGE_HANDLER        ResetHandler,
    MBB_STATE_CHANGE_HANDLER        StallClearHandler
    )

{
    ULONG                           i;

    RtlZeroMemory(AdapterState, sizeof(*AdapterState));

    //
    //  device is power to start with
    //
    AdapterState->CurrentPowerState=NetDeviceStateD0;

    AdapterState->Handlers[STATE_CHANGE_TYPE_PAUSE]   = PauseHandler;
    AdapterState->Handlers[STATE_CHANGE_TYPE_RESTART] = RestartHandler;
    AdapterState->Handlers[STATE_CHANGE_TYPE_POWER]   = PowerHandler;
    AdapterState->Handlers[STATE_CHANGE_TYPE_RESET]   = ResetHandler;
    AdapterState->Handlers[STATE_CHANGE_TYPE_STALL_CLEAR]   = StallClearHandler;

    InitializeListHead(&AdapterState->FreeList);



    InitializeListHead(&AdapterState->ListEntry);
    AdapterState->CurrentEvent=NULL;

    KeInitializeEvent(
        &AdapterState->StallClearCompleteEvent,
        NotificationEvent,
        TRUE
    );

    NdisAllocateSpinLock(&AdapterState->Lock);

    for (i=0; i< STATE_CHANGE_EVENT_RESERVE_COUNT; i++)
    {

        PSTATE_CHANGE_EVENT     StateChange=NULL;

        StateChange=ALLOCATE_NONPAGED_POOL(sizeof(*StateChange));

        if (StateChange != NULL)
        {

            NdisAcquireSpinLock(&AdapterState->Lock);

            InsertHeadList(&AdapterState->FreeList, &StateChange->ListEntry);
            StateChange=NULL;

            NdisReleaseSpinLock(&AdapterState->Lock);

        }
        else
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }


    AdapterState->WorkItem=NdisAllocateIoWorkItem(MiniportAdapterContext);

    if (AdapterState->WorkItem == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;

}

VOID
ShutdownAdapterStateObject(
    PADAPTER_STATE      AdapterState
    )

{
    PSTATE_CHANGE_EVENT StateChange=NULL;

    ASSERT(AdapterState->CurrentEvent == NULL);
    ASSERT(IsListEmpty(&AdapterState->ListEntry));

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Entered");

    AdapterState->ShuttingDown=TRUE;

    StateChange=AllocateStateChangeEvent(AdapterState);

    while (StateChange != NULL)
    {
        FREE_POOL(StateChange);
        StateChange=NULL;

        StateChange=AllocateStateChangeEvent(AdapterState);
    }

    if (AdapterState->WorkItem != NULL)
    {
        NdisFreeIoWorkItem(AdapterState->WorkItem);
        AdapterState->WorkItem=NULL;
    }

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Exited");

    return;

}



VOID
AdapterStateWorkitemRoutine(
    PVOID       WorkItemContext,
    NDIS_HANDLE WorkItemHandle
    )

{
    PADAPTER_STATE      AdapterState=(PADAPTER_STATE)WorkItemContext;
    PSTATE_CHANGE_EVENT StateChange=AdapterState->CurrentEvent;


    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Entered");

    if (StateChange->EventType < STATE_CHANGE_MAX && StateChange->EventType >= 0)
    {
        (*AdapterState->Handlers[StateChange->EventType])(StateChange);
    }
    else
    {
         ASSERT(0);
    }

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Exited");

    return;

}


VOID
ProcessStateChangeQueue(
    PADAPTER_STATE      AdapterState
    )

{
    PLIST_ENTRY         ListEntry=NULL;
    PSTATE_CHANGE_EVENT StateChangeEvent=NULL;

    TraceInfo(WMBCLASS_INIT, "%!FUNC!:Entered");

    NdisAcquireSpinLock(&AdapterState->Lock);

    if (AdapterState->CurrentEvent == NULL)
    {
        //
        //  queue is idle
        //
        if (!IsListEmpty(&AdapterState->ListEntry))
        {
            //
            //  more item to run
            //
            ListEntry=RemoveTailList(&AdapterState->ListEntry);

            StateChangeEvent=CONTAINING_RECORD(ListEntry, STATE_CHANGE_EVENT, ListEntry);

            AdapterState->CurrentEvent=StateChangeEvent;

            TraceInfo(WMBCLASS_INIT, "Queuing work item");

            NdisQueueIoWorkItem(AdapterState->WorkItem, AdapterStateWorkitemRoutine, AdapterState);

        }
    }


    NdisReleaseSpinLock(&AdapterState->Lock);

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Exited");

    return;



}

VOID
QueueStateChangeEvent(
    PADAPTER_STATE      AdapterState,
    PSTATE_CHANGE_EVENT StateChange
    )

{

    TraceInfo(WMBCLASS_INIT, "%!FUNC!:Entered");

    NdisAcquireSpinLock(&AdapterState->Lock);

    InsertHeadList(&AdapterState->ListEntry, &StateChange->ListEntry);


    NdisReleaseSpinLock(&AdapterState->Lock);

    ProcessStateChangeQueue(AdapterState);

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Exited");

    return;
}



VOID
CompleteStateChange(
    PADAPTER_STATE      AdapterState,
    PSTATE_CHANGE_EVENT StateChange
    )


{
    TraceInfo(WMBCLASS_INIT, "%!FUNC!:Entered");

    NdisAcquireSpinLock(&AdapterState->Lock);

    ASSERT(AdapterState->CurrentEvent == StateChange);

    AdapterState->CurrentEvent=NULL;

    NdisReleaseSpinLock(&AdapterState->Lock);

    FreeStateChangeEvent(AdapterState, StateChange);

    ProcessStateChangeQueue(AdapterState);

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Exited");

}

PSTATE_CHANGE_EVENT
AllocateStateChangeEvent(
    PADAPTER_STATE      AdapterState
    )

{
    PLIST_ENTRY         ListEntry=NULL;
    PSTATE_CHANGE_EVENT StateChangeEvent=NULL;

    NdisAcquireSpinLock(&AdapterState->Lock);

    if (!IsListEmpty(&AdapterState->FreeList))
    {
        //
        //  more item to run
        //
        ListEntry=RemoveTailList(&AdapterState->FreeList);

        StateChangeEvent=CONTAINING_RECORD(ListEntry, STATE_CHANGE_EVENT, ListEntry);

        RtlZeroMemory(StateChangeEvent, sizeof(*StateChangeEvent));
    }
    else
    {
        if (!AdapterState->ShuttingDown)
        {
            ASSERT(0);
        }
    }

    NdisReleaseSpinLock(&AdapterState->Lock);


    return StateChangeEvent;

}


VOID
FreeStateChangeEvent(
    PADAPTER_STATE      AdapterState,
    PSTATE_CHANGE_EVENT StateChange
    )

{
    NdisAcquireSpinLock(&AdapterState->Lock);

    InsertHeadList(&AdapterState->FreeList, &StateChange->ListEntry);
    StateChange=NULL;

    NdisReleaseSpinLock(&AdapterState->Lock);

    return;

}

VOID
FreeDeviceServiceState(
    __in PMBB_DS_STATE              DeviceServiceState
    )
{
    PMBB_DS             DeviceService;
    ULONG*              CIDList;
    ULONG               i;

    if (DeviceServiceState->ExtSubscribeList != NULL)
        FREE_POOL(DeviceServiceState->ExtSubscribeList);
    
    if (DeviceServiceState->ServicesList != NULL)
    {
        // Free each device service entry
        for (i = 0; i < DeviceServiceState->ServicesCount; i++)
        {
            DeviceService = &(DeviceServiceState->ServicesList[i]);

            // Free the CID list
            if (DeviceService->CIDList != NULL)
                FREE_POOL(DeviceService->CIDList);
        }

        // Free the device services list structure
        FREE_POOL(DeviceServiceState->ServicesList);
    }

    return;
}

_Requires_lock_not_held_(&Adapter->Lock) 
VOID
MbbAdapterSetMultiCarrierCapable(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in BOOLEAN                    IsMultiCarrierSupported
    )
{
    MbbAdapterLock( Adapter );
    Adapter->AdapterFlags.IsMultiCarrier = IsMultiCarrierSupported;
    MbbAdapterUnlock( Adapter );
}

 _Requires_lock_not_held_(&Adapter->Lock) 
BOOLEAN
MbbAdapterIsMultiCarrierCapable(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    )
{
    BOOLEAN IsMultiCarrierSupported;

    MbbAdapterLock( Adapter );
    IsMultiCarrierSupported = (Adapter->AdapterFlags.IsMultiCarrier == 1);
    MbbAdapterUnlock( Adapter );

    return IsMultiCarrierSupported;
}

_Requires_lock_not_held_(&Adapter->Lock) 
VOID
MbbAdapterSetDataClass(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in ULONG                      DataClass
    )
{
    MbbAdapterLock( Adapter );
    Adapter->AdapterDataClass = DataClass;
    MbbAdapterUnlock( Adapter );
}

_Requires_lock_not_held_(&Adapter->Lock) 
VOID
MbbAdapterGetDataClass(
    __in  PMINIPORT_ADAPTER_CONTEXT Adapter,
    __out ULONG*                    DataClass
    )
{
    MbbAdapterLock( Adapter );
    *DataClass = Adapter->AdapterDataClass;
    MbbAdapterUnlock( Adapter );
}

_Requires_lock_not_held_(&Adapter->Lock) 
VOID
MbbAdapterSetSupportedCellularClass(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in MBB_CELLULAR_CLASS         CellularClass
    )
{
    MbbAdapterLock( Adapter );
    Adapter->AdapterSupportedCellularClass = CellularClass;
    MbbAdapterUnlock( Adapter );
}

_Requires_lock_not_held_(&Adapter->Lock) 
MBB_CELLULAR_CLASS
MbbAdapterGetSupportedCellularClass(
    __in  PMINIPORT_ADAPTER_CONTEXT Adapter
    )
{
    MBB_CELLULAR_CLASS CellularClass;

    MbbAdapterLock( Adapter );
    CellularClass = Adapter->AdapterSupportedCellularClass;
    MbbAdapterUnlock( Adapter );
    return CellularClass;
}

_Requires_lock_not_held_(&Adapter->Lock) 
BOOLEAN
MbbAdapterIsMultimodeCapable(
    __in  PMINIPORT_ADAPTER_CONTEXT Adapter
    )
{
    ULONG BitValue;

    MbbAdapterLock( Adapter );
    BitValue = (ULONG)(Adapter->AdapterSupportedCellularClass);
    MbbAdapterUnlock( Adapter );
    //
    // If only a single bit is set then the following expression
    // should be zero. Otherwise this will be set to the "other" bits.
    //
    if( (BitValue & (BitValue - 1)) != 0 )
        return TRUE;
    else
        return FALSE;
}

_Requires_lock_not_held_(&Adapter->Lock) 
VOID
MbbAdapterSetCurrentCellularClass(
    __in  PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in  MBB_CELLULAR_CLASS        CellularClass
    )
{
    MbbAdapterLock( Adapter );
    Adapter->AdapterCurrentCellularClass = CellularClass;
    MbbAdapterUnlock( Adapter );
}

_Requires_lock_not_held_(&Adapter->PortsLock)
VOID
MbbAdapterSetMaxActivatedContexts(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in ULONG                      dwMaxActivatedContexts
    )
{
    ASSERT(dwMaxActivatedContexts >= 1);
    MbbAdapterPortsLock(Adapter);
    Adapter->MaxActivatedContexts = (dwMaxActivatedContexts < MBB_MAX_NUMBER_OF_PORTS) ? dwMaxActivatedContexts: MBB_MAX_NUMBER_OF_PORTS;
    MbbAdapterPortsUnlock(Adapter);
}


_Requires_lock_not_held_(&Adapter->Lock) 
MBB_CELLULAR_CLASS
MbbAdapterGetCurrentCellularClass(
    __in  PMINIPORT_ADAPTER_CONTEXT Adapter
    )
{
    MBB_CELLULAR_CLASS CellularClass;

    MbbAdapterLock( Adapter );
    CellularClass = Adapter->AdapterCurrentCellularClass;
    MbbAdapterUnlock( Adapter );
    return CellularClass;
}

_Requires_lock_not_held_(&Adapter->Lock)
VOID
MbbAdapterSetRequestManager(
    __in  PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in  PMBB_REQUEST_MANAGER      RequestManager
    )
{
    MbbAdapterLock( Adapter );
    Adapter->RequestManager = RequestManager;
    MbbAdapterUnlock( Adapter );
}

_Requires_lock_not_held_(&Adapter->Lock)
PMBB_REQUEST_MANAGER
MbbAdapterGetRequestManager(
    __in  PMINIPORT_ADAPTER_CONTEXT Adapter
    )
/*++
Description
    Synchronizes access to request manager with its lifetime.
    Try to return a valid request manager reference.
    Caller is required to derefernce the request manager.

Return Value
    non-NULL
        Request Manager is successfully returned.
    NULL
        Request Manager is not initialized or could not be referenced.
--*/
{
    PMBB_REQUEST_MANAGER RequestManager = NULL;

    MbbAdapterLock( Adapter );
    if( Adapter->RequestManager &&
        MbbReqMgrRef( Adapter->RequestManager ) )
    {
        RequestManager = Adapter->RequestManager;
    }
    MbbAdapterUnlock( Adapter );

    return RequestManager;
}

_Requires_lock_not_held_(&Adapter->Lock)
NDIS_STATUS
MbbAdapterQueryDeviceId(
    __in    PMINIPORT_ADAPTER_CONTEXT Adapter
    )
{
    NDIS_STATUS                 NdisStatus;
    LARGE_INTEGER               Timeout;
    MBB_COMMAND                 Command = { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_DEVICE_CAPS };
    PMBB_REQUEST_CONTEXT        Request = NULL;
    PMBB_REQUEST_MANAGER        RequestManager = NULL;

    do
    {
        if( (RequestManager = MbbAdapterGetRequestManager( Adapter )) == NULL )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter] FAILED to reference RequestManager for QueryDeviceCaps" );
            NdisStatus = NDIS_STATUS_ADAPTER_NOT_READY;
            break;
        }
        if( (Request = MbbReqMgrCreateRequest(
                            RequestManager,
                            NULL,
                            0,
                            &NdisStatus)) == NULL )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter] FAILED to allocate RequestContext for QueryDeviceCaps" );
            break;
        }
        Request->OidHandler = MbbNdisGetOidHandlerByCommand( &Command );

        //
        // Ref the request so that the response handler does not free it
        //
        MbbReqMgrRefRequest( Request );

        NdisStatus = MbbReqMgrDispatchRequest(
                        Request,
                        TRUE, // Serialized
                        MbbUtilInternalCIDQuery,
                        MbbUtilInternalCIDCompletion,
                        MbbUtilInternalCIDResponse
                        );
        //
        // If the request is pending wait for it to complete.
        // If the request didnt complete within the timeout period drop our ref count and fail to the caller.
        // Something is wrong with a device that does not send response for a non-network request
        // If dispatching the request failed then our completion callback will not be invoked,
        // in this case destroy the request.
        //
        if( NdisStatus == NDIS_STATUS_PENDING )
        {
            Timeout.QuadPart = -1 * 10 * MBB_ADAPTER_INITIAL_REQUEST_TIMEOUT_MS;
            NdisStatus = KeWaitForSingleObject(
                            &Request->WaitEvent,
                            Executive,
                            KernelMode,
                            TRUE, // Alertable
                            &Timeout // Timeout
                            );
            if( NdisStatus != STATUS_WAIT_0 )
            {
                NdisStatus = NDIS_STATUS_FAILURE;
            }
            else
            {
                //
                // Only if Send was successful then get the response status.
                //
                if( (NdisStatus = Request->HandlerContext.Command.SendStatus) == NDIS_STATUS_SUCCESS )
                    NdisStatus = Request->HandlerContext.Response.NdisStatus;
            }
        }
        else if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            //
            // It is ok to destroy the request here and deref it later.
            // Destroy will not free the request while there is a pending ref.
            //
            MbbReqMgrDestroyRequest(
                Request->RequestManager,
                Request
                );
        }

        if( NdisStatus == NDIS_STATUS_SUCCESS )
        {
            PCHAR pDeviceId = (PCHAR)(Request->HandlerContext.Parameters.DeviceCaps.DeviceId);
            CHAR  cOctet[MBB_MAC_ADDRESS_LENGTH];
            ULONG ulOctetIndex;

            RtlZeroMemory(
                cOctet,
                MBB_MAC_ADDRESS_LENGTH
                );

            //
            // XOR the bytes to get a 6-byte MAC
            // Left shift is done for more randomization.
            //
            for(ulOctetIndex = 0;
                pDeviceId[ulOctetIndex*2];
                ulOctetIndex++ )
            {
                cOctet[ulOctetIndex % MBB_MAC_ADDRESS_LENGTH] ^= ( pDeviceId[ulOctetIndex*2] << (ulOctetIndex / MBB_MAC_ADDRESS_LENGTH) );
            }

            //
            // Mask of the multicast and locally administered bit
            //
            cOctet[0] &= ~0x03;

            RtlCopyMemory(
                Adapter->MACAddress,
                cOctet,
                MBB_MAC_ADDRESS_LENGTH
                );
        }
    }
    while( FALSE );

    if( RequestManager != NULL )
    {
        MbbReqMgrDeref( RequestManager );
    }
    if( Request != NULL )
    {
        MbbReqMgrDerefRequest( Request );
    }
    return NdisStatus;
}

_Requires_lock_not_held_(&Adapter->Lock)
VOID
MbbAdapterResetCapabilities(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    )
{
    MbbAdapterLock( Adapter );
    Adapter->AdapterFlags.IsUssdCapable     = FALSE;
    Adapter->AdapterFlags.IsSimAuthCapable  = FALSE;
    Adapter->AdapterFlags.IsAkaAuthCapable  = FALSE;
    Adapter->AdapterFlags.IsAkapAuthCapable = FALSE;
    MbbAdapterUnlock( Adapter );
}

NDIS_STATUS
MbbAdapterQueryMultiCarrierDsCidList(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in const GUID*                Guid,
    __in PMBB_REQUEST_CONTEXT       Request
    )
{
    NDIS_STATUS                 NdisStatus;
    LARGE_INTEGER               Timeout;
    PMBB_REQUEST_CONTEXT        NewRequest = NULL;
    PMBB_REQUEST_MANAGER        RequestManager = NULL;
    PMBB_DS                     MbbDs;
    MBB_COMMAND                 Command = {
                                    MBB_UUID_MULTICARRIER_CONSTANT,
                                    MBB_MULTICARRIER_CID_CURRENT_CID_LIST
                                };

    do
    {
        if( (RequestManager = MbbAdapterGetRequestManager( Adapter )) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbAdapter][ReqId=0x%04x] FAILED to reference RequestManager", Request->RequestId );
            NdisStatus = NDIS_STATUS_ADAPTER_NOT_READY;
            break;
        }
        if( (NewRequest = MbbReqMgrCreateRequest(
                            RequestManager,
                            NULL,
                            0,
                            &NdisStatus)) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbAdapter][ReqId=0x%04x] FAILED to allocate RequestContext", Request->RequestId );
            break;
        }
        NewRequest->OidHandler = MbbNdisGetOidHandlerByCommand( &Command );
        //
        // Cache the context from the previous request since it will be destroyed
        //
        NewRequest->OidContext.OidRequestId             = Request->OidContext.OidRequestId;
        NewRequest->OidContext.OidRequestHandle         = Request->OidContext.OidRequestHandle;
        NewRequest->HandlerContext.Parameters.DeviceCaps= Request->HandlerContext.Parameters.DeviceCaps;

        TraceInfo( WMBCLASS_OID, "[MbbAdapter][ReqId=0x%04x][ReqId=0x%04x] Request created to query CID list for DS=%!GUID!", Request->RequestId, NewRequest->RequestId, Guid );

        MBB_UUID_TO_NET(
            &NewRequest->HandlerContext.Parameters.DeviceCaps.CurrentQueriedDeviceService,
            Guid
            );

        NdisStatus = MbbReqMgrDispatchRequest(
                        NewRequest,
                        (TRUE == Request->OidHandler->IsSerialized),
                        MbbUtilInternalCIDQuery,
                        MbbUtilInternalCIDCompletion,
                        MbbUtilInternalCIDResponse
                        );
        if( NdisStatus != NDIS_STATUS_PENDING &&
            NdisStatus != NDIS_STATUS_INDICATION_REQUIRED )
        {
            //
            // It is ok to destroy the request here and deref it later.
            // Destroy will not free the request while there is a pending ref.
            //
            MbbReqMgrDestroyRequest(
                NewRequest->RequestManager,
                NewRequest
                );
        }
    }
    while( FALSE );

    if( RequestManager != NULL )
    {
        MbbReqMgrDeref( RequestManager );
        RequestManager = NULL;
    }

    return NdisStatus;
}

NDIS_STATUS
MbbAdapterMultiCarrierDeviceServicesToCapabilities(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in PMBB_REQUEST_CONTEXT       Request
    )
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;

    do
    {
        if( Request->HandlerContext.Parameters.DeviceCaps.IsUssdCapsValid == 0 )
        {
            NdisStatus = MbbAdapterQueryMultiCarrierDsCidList(
                            Adapter,
                            &MBB_UUID_USSD,
                            Request
                            );
            if( NDIS_STATUS_SUCCESS != NdisStatus &&
                NDIS_STATUS_PENDING != NdisStatus )
            {
                TraceError( WMBCLASS_OID, "[MbbAdapter][ReqId=0x%04x] FAILED MbbAdapterQueryMultiCarrierDsCidList(USSD)=%!STATUS!", Request->RequestId, NdisStatus );
                break;
            }
        }
        else if( Request->HandlerContext.Parameters.DeviceCaps.IsAuthCapsValid == 0 )
        {
            NdisStatus = MbbAdapterQueryMultiCarrierDsCidList(
                            Adapter,
                            &MBB_UUID_AUTH,
                            Request
                            );
            if( NDIS_STATUS_SUCCESS != NdisStatus &&
                NDIS_STATUS_PENDING != NdisStatus )
            {
                TraceError( WMBCLASS_OID, "[MbbAdapter][ReqId=0x%04x] FAILED MbbAdapterQueryMultiCarrierDsCidList(AUTH)=%!STATUS!", Request->RequestId, NdisStatus );
                break;
            }
        }
    }
    while( FALSE );

    return NdisStatus;
}

NDIS_STATUS
MbbAdapterSingleCarrierDeviceServicesToCapabilities(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in PMBB_REQUEST_CONTEXT       Request
    )
{
    ULONG CidIndex;
    PMBB_DS MbbDs;

    MbbDs = MbbUtilFindDeviceService(
                Adapter,
                (GUID*)&MBB_UUID_USSD
                );
    if( MbbDs != NULL )
    {
        MbbAdapterLock( Adapter );
        Adapter->AdapterFlags.IsUssdCapable = TRUE;
        MbbAdapterUnlock( Adapter );
    }

    MbbDs = MbbUtilFindDeviceService(
                Adapter,
                (GUID*)&MBB_UUID_AUTH
                );
    if( MbbDs != NULL )
    {
        MbbAdapterLock( Adapter );
        for(CidIndex = 0;
            CidIndex < MbbDs->CIDCount;
            CidIndex++ )
        {
            switch( MbbDs->CIDList[CidIndex] )
            {
                case MBB_AUTH_CID_AKA:
                {
                    Adapter->AdapterFlags.IsAkaAuthCapable = TRUE;
                }
                break;

                case MBB_AUTH_CID_AKAP:
                {
                    Adapter->AdapterFlags.IsAkapAuthCapable = TRUE;
                }
                break;

                case MBB_AUTH_CID_SIM:
                {
                    Adapter->AdapterFlags.IsSimAuthCapable = TRUE;
                }
                break;
            }
        }
        MbbAdapterUnlock( Adapter );
    }
    Request->HandlerContext.Parameters.DeviceCaps.IsUssdCapsValid = 1;
    Request->HandlerContext.Parameters.DeviceCaps.IsAuthCapsValid = 1;
    return NDIS_STATUS_SUCCESS;
}

_Requires_lock_not_held_(&Adapter->Lock)
VOID
MbbAdapterSetShutdownNotificationCapabilities(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    )
{
    ULONG CidIndex;
    PMBB_DS MbbDs;

    MbbDs = MbbUtilFindDeviceService(
                Adapter,
                (GUID*)&MBB_UUID_HOSTSHUTDOWN
                );
    if (MbbDs != NULL)
    {
        MbbAdapterLock(Adapter);
        for (CidIndex = 0;
             CidIndex < MbbDs->CIDCount;
             CidIndex++)
        {
            switch (MbbDs->CIDList[CidIndex])
            {
                case MBB_HOSTSHUTDOWN_CID_ONE:
                {
                    Adapter->AdapterFlags.ShutdownNotificationCapable = TRUE;
                }
                break;

                case MBB_HOSTSHUTDOWN_CID_PRESHUTDOWN:
                {
                    Adapter->AdapterFlags.IsPreshutdownCapable = TRUE;
                }
                break;
            }
        }
        MbbAdapterUnlock(Adapter);
    }

    return;
}

_Requires_lock_not_held_(&Adapter->Lock)
VOID
MbbAdapterSetOptionalServiceSupport(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
)
{
    ULONG CidIndex;
    PMBB_DS MbbDs;

    MbbDs = MbbUtilFindDeviceService(
        Adapter,
        (GUID*)&MBB_UUID_BASIC_CONNECT_EXTENSIONS
    );
    if (MbbDs != NULL)
    {
        MbbAdapterLock(Adapter);
        for (CidIndex = 0;
            CidIndex < MbbDs->CIDCount;
            CidIndex++)
        {
            switch (MbbDs->CIDList[CidIndex])
            {
            case MBB_BASICCONNECTEXT_CID_PROVISIONED_CONTEXT_V2:
            {
                Adapter->AdapterFlags.IsProvisionedContextV2Capable = TRUE;
            }
            break;

            case MBB_BASICCONNECTEXT_CID_NETWORK_BLACKLIST:
            {
                Adapter->AdapterFlags.IsNetworkBlacklistCapable = TRUE;
            }
            break;

            case MBB_BASICCONNECTEXT_CID_LTE_ATTACH_CONFIG:
            {
                Adapter->AdapterFlags.IsLTEAttachConfigCapable = TRUE;
            }
            break;

            case MBB_BASICCONNECTEXT_CID_DEVICE_SLOT_MAPPINGS:
            {
                Adapter->AdapterFlags.IsMultiSIMCapable = TRUE;
            }
            break;

            case MBB_BASICCONNECTEXT_CID_DEVICE_CAPS_V2:
            {
                Adapter->AdapterFlags.IsDeviceCapsV2Capable = TRUE;
            }
            break;

            case MBB_BASICCONNECTEXT_CID_PCO:
            {
                Adapter->AdapterFlags.IsPcoCapable = TRUE;
            }
            break;

            case MBB_BASICCONNECTEXT_CID_DEVICE_RESET:
            {
                Adapter->AdapterFlags.IsDeviceResetCapable = TRUE;
            }
            break;

            case MBB_BASICCONNECTEXT_CID_BASE_STATIONS_INFO:
            {
                Adapter->AdapterFlags.IsBaseStationsInfoCapable = TRUE;
            }
            break;

            }
        }
        MbbAdapterUnlock(Adapter);
    }

    MbbDs = MbbUtilFindDeviceService(
        Adapter,
        (GUID*)&MBB_UUID_SARCONTROL
    );
    if (MbbDs != NULL)
    {
        MbbAdapterLock(Adapter);
        for (CidIndex = 0;
            CidIndex < MbbDs->CIDCount;
            CidIndex++)
        {
            switch (MbbDs->CIDList[CidIndex])
            {
            case MBB_SAR_CID_CONFIG:
            {
                Adapter->AdapterFlags.IsSARCapable = TRUE;
            }
            break;
            }
        }
        MbbAdapterUnlock(Adapter);
    }

    MbbDs = MbbUtilFindDeviceService(
        Adapter,
        (GUID*)&MBB_UUID_UICC_LOW_LEVEL
    );
    if (MbbDs != NULL)
    {
        MbbAdapterLock(Adapter);
        for (CidIndex = 0;
            CidIndex < MbbDs->CIDCount;
            CidIndex++)
        {
            switch (MbbDs->CIDList[CidIndex])
            {
            case MBB_UICC_CID_ATR:
            {
                // if this CID is supported, the entire group of MBB_UUID_UICC_LOW_LEVEL CIDs must be supported.
                Adapter->AdapterFlags.IsUiccLowLevelCapable = TRUE;
            }
            break;
            }
        }
        MbbAdapterUnlock(Adapter);
    }

    return;
}

_Requires_lock_not_held_(&Adapter->Lock)
NDIS_STATUS
MbbAdapterFWDeviceServicesToCapabilities(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in PMBB_REQUEST_CONTEXT       Request
    )
/*++
Description:
    Check whether the optional multi-carrier
    device service is implemented by the device. The presence
    of the multi-carrier device service indicates that this
    is a multi-carrier device. This method is called from
    device caps query path.
--*/
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;
    PNDIS_WWAN_DEVICE_CAPS NdisDeviceCaps = Request->HandlerContext.Parameters.DeviceCaps.NdisDeviceCaps;

    do
    {
        //
        //If this is not a multi-carrier device then get
        //get the supported native device services from the
        //device services. If this is a multi-carrier device
        //find the support for USSD and AUTH.
        //
        if( NdisDeviceCaps->DeviceCaps.WwanControlCaps & WWAN_CTRL_CAPS_MODEL_MULTI_CARRIER )
        {
            NdisStatus = MbbAdapterMultiCarrierDeviceServicesToCapabilities( Adapter, Request );
            if( NDIS_STATUS_SUCCESS != NdisStatus &&
                NDIS_STATUS_PENDING != NdisStatus )
            {
                TraceError( WMBCLASS_OID, "[MbbAdapter][ReqId=0x%04x] FAILED MbbAdapterMultiCarrierDeviceServicesToCapabilities()=%!STATUS!", Request->RequestId, NdisStatus );
                break;
            }
        }
        else
        {
            NdisStatus = MbbAdapterSingleCarrierDeviceServicesToCapabilities( Adapter, Request );
            if( NDIS_STATUS_SUCCESS != NdisStatus &&
                NDIS_STATUS_PENDING != NdisStatus )
            {
                TraceError( WMBCLASS_OID, "[MbbAdapter][ReqId=0x%04x] FAILED MbbAdapterSingleCarrierDeviceServicesToCapabilities()=%!STATUS!", Request->RequestId, NdisStatus );
                break;
            }
        }
    }
    while( FALSE );

    return NdisStatus;
}

_Requires_lock_not_held_(&SendQueue->AdapterContext->Lock)
VOID TryQueueStallState(_In_ PMBB_SEND_QUEUE SendQueue)
{
    PSTATE_CHANGE_EVENT StateChange = NULL;

    MbbAdapterLock(SendQueue->AdapterContext);

    if (!SendQueue->AdapterContext->AdapterState.Hung)
    {
        StateChange = AllocateStateChangeEvent(&SendQueue->AdapterContext->AdapterState);

        if (StateChange != NULL)
        {
            SendQueue->AdapterContext->AdapterState.Hung = TRUE;

            StateChange->EventType = STATE_CHANGE_TYPE_STALL_CLEAR;
            StateChange->Context1 = SendQueue->AdapterContext;

            TraceInfo(WMBCLASS_OID, "%!FUNC!: Reset data pipe has been scheduled");
            KeResetEvent(&SendQueue->AdapterContext->AdapterState.StallClearCompleteEvent);
            QueueStateChangeEvent(&SendQueue->AdapterContext->AdapterState, StateChange);

        }
        else
        {
            TraceError(WMBCLASS_OID, "%!FUNC!: Reset data pipe isn't scheduled since AllocateStateChangeEvent failed");
        }
    }
    else
    {
        TraceError(WMBCLASS_OID, "%!FUNC!: Skip reset data pipe since it has already been scheduled, ");
    }

    MbbAdapterUnlock(SendQueue->AdapterContext);
}

VOID WaitStallClearComplete(
    _In_ PADAPTER_STATE AdapterState
)
{
    if (AdapterState->WorkItem != NULL)
    {
        KeWaitForSingleObject(
            &AdapterState->StallClearCompleteEvent,
            Executive,
            KernelMode,
            FALSE, // Alertable
            NULL // Timeout
        );
    }
}