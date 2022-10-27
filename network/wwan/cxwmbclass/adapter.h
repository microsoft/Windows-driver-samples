//
//    Copyright (C) Microsoft.  All rights reserved.
//

typedef struct _WWAN_IP_ADDRESS_STATE WWAN_IP_ADDRESS_STATE;
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
    );

_Releases_lock_( Adapter->Lock )
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_minIRQL(DISPATCH_LEVEL)
__drv_restoresIRQLGlobal( NdisSpinLock, Adapter )
VOID
MbbAdapterUnlock(
    __in PMINIPORT_ADAPTER_CONTEXT Adapter
    );

_Acquires_lock_( Adapter->PortsLock )
__drv_raisesIRQL(DISPATCH_LEVEL)
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_savesIRQLGlobal( NdisSpinLock, Adapter )
VOID
MbbAdapterPortsLock(
    __in PMINIPORT_ADAPTER_CONTEXT Adapter
    );

_Releases_lock_( Adapter->PortsLock )
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_minIRQL(DISPATCH_LEVEL)
__drv_restoresIRQLGlobal( NdisSpinLock, Adapter )
VOID
MbbAdapterPortsUnlock(
    __in PMINIPORT_ADAPTER_CONTEXT Adapter
    );

_Acquires_lock_( Adapter->SessionIdPortTableLock )
__drv_raisesIRQL(DISPATCH_LEVEL)
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_savesIRQLGlobal( NdisSpinLock, Adapter )
VOID
MbbAdapterSessionIdPortTableLock(
    __in PMINIPORT_ADAPTER_CONTEXT Adapter
    );

_Releases_lock_( Adapter->SessionIdPortTableLock )
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_minIRQL(DISPATCH_LEVEL)
__drv_restoresIRQLGlobal( NdisSpinLock, Adapter )
VOID
MbbAdapterSessionIdPortTableUnlock(
    __in PMINIPORT_ADAPTER_CONTEXT Adapter
    );


BOOLEAN
MbbAdapterIsReady(
    __in PMINIPORT_ADAPTER_CONTEXT Adapter
    );

VOID
MbbAdapterConnectionChange(
    __in    MBB_PROTOCOL_HANDLE     ProtocolHandle,
    __in    PMBB_CONNECTION_STATE   ConnectionState,
    __in    NDIS_PORT_NUMBER        PortNumber
    );

VOID
MbbAdapterSetConnectionState(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in PMBB_CONNECTION_STATE      ConnectionState,
    __in NDIS_PORT_NUMBER           PortNumber
    );

VOID
MbbAdapterSetLinkState(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in PMBB_CONNECTION_STATE      ConnectionState,
    __in NDIS_PORT_NUMBER           PortNumber
    );

VOID
MbbAdapterSetPortState(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in PMBB_CONNECTION_STATE      ConnectionState,
    __in NDIS_PORT_NUMBER           PortNumber
    );

VOID
MbbPortSetIpAddressState(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in PWWAN_IP_ADDRESS_STATE     WwanIpAddressState,
    __in NDIS_PORT_NUMBER           PortNumber
    );

BOOLEAN
MbbPortIsConnected(
    __in PMBB_PORT Port
    );

VOID
MbbAdapterRef(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    );

VOID
MbbAdapterDeref(
    __in PMINIPORT_ADAPTER_CONTEXT Adapter
    );

NDIS_STATUS
MbbAdapterPerformInitialRequests(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    );

NTSTATUS
InitAdapterStateObject(
    __out PADAPTER_STATE            AdapterState,
    __in  NDIS_HANDLE               MiniportAdapterContext,
    MBB_STATE_CHANGE_HANDLER        PauseHandler,
    MBB_STATE_CHANGE_HANDLER        RestartHandler,
    MBB_STATE_CHANGE_HANDLER        PowerHandler,
    MBB_STATE_CHANGE_HANDLER        ResetHandler,
    MBB_STATE_CHANGE_HANDLER        StallClearHandler
    );

VOID
ShutdownAdapterStateObject(
    PADAPTER_STATE      AdapterState
    );

VOID
QueueStateChangeEvent(
    PADAPTER_STATE      AdapterState,
    PSTATE_CHANGE_EVENT StateChange
    );

VOID
CompleteStateChange(
    PADAPTER_STATE      AdapterState,
    PSTATE_CHANGE_EVENT StateChange
    );


PSTATE_CHANGE_EVENT
AllocateStateChangeEvent(
    PADAPTER_STATE      AdapterState
    );

VOID
FreeStateChangeEvent(
    PADAPTER_STATE      AdapterState,
    PSTATE_CHANGE_EVENT StateChange
    );

VOID
FreeDeviceServiceState(
    __in PMBB_DS_STATE              DeviceServiceState
    );

_Requires_lock_not_held_(&Adapter->Lock) 
VOID
MbbAdapterSetMultiCarrierCapable(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in BOOLEAN                    IsMultiCarrierSupported
    );

_Requires_lock_not_held_(&Adapter->Lock) 
BOOLEAN
MbbAdapterIsMultiCarrierCapable(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    );

_Requires_lock_not_held_(&Adapter->Lock) 
VOID
MbbAdapterSetDataClass(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in ULONG                      DataClass
    );

_Requires_lock_not_held_(&Adapter->Lock) 
VOID
MbbAdapterGetDataClass(
    __in  PMINIPORT_ADAPTER_CONTEXT Adapter,
    __out ULONG*                    DataClass
    );

_Requires_lock_not_held_(&Adapter->Lock) 
VOID
MbbAdapterSetSupportedCellularClass(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in MBB_CELLULAR_CLASS         CellularClass
    );

_Requires_lock_not_held_(&Adapter->Lock) 
MBB_CELLULAR_CLASS
MbbAdapterGetSupportedCellularClass(
    __in  PMINIPORT_ADAPTER_CONTEXT Adapter
    );

_Requires_lock_not_held_(&Adapter->Lock) 
BOOLEAN
MbbAdapterIsMultimodeCapable(
    __in  PMINIPORT_ADAPTER_CONTEXT Adapter
    );

_Requires_lock_not_held_(&Adapter->Lock) 
VOID
MbbAdapterSetCurrentCellularClass(
    __in  PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in  MBB_CELLULAR_CLASS        CellularClass
    );

_Requires_lock_not_held_(&Adapter->Lock) 
MBB_CELLULAR_CLASS
MbbAdapterGetCurrentCellularClass(
    __in  PMINIPORT_ADAPTER_CONTEXT Adapter
    );

_Requires_lock_not_held_(&Adapter->PortsLock) 
VOID
MbbAdapterSetMaxActivatedContexts(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in DWORD                      dwMaxActivatedContexts
    );


NDIS_STATUS
MbbAdapterConfigurePacketFilters(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in BOOLEAN                    Set
    );

NDIS_STATUS
MbbAdapterConfigurePacketFiltersOnSession(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in BOOLEAN                    Set,
    __in NDIS_PORT_NUMBER           PortNumber
    );

NDIS_STATUS
MbbAdapterConfigureDeviceServiceSubscription(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in BOOLEAN                    FullPower,
    __in ULONG                      MediaSpecificWakeUpEvents,
    __in ULONG                      WakeUpEvents
    );

NDIS_STATUS
MbbAdapterSendNetworkIdleHint(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    );

_Requires_lock_not_held_(&Adapter->Lock)
VOID
MbbAdapterSetRequestManager(
    __in  PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in  PMBB_REQUEST_MANAGER      RequestManager
    );

_Requires_lock_not_held_(&Adapter->Lock)
PMBB_REQUEST_MANAGER
MbbAdapterGetRequestManager(
    __in  PMINIPORT_ADAPTER_CONTEXT Adapter
    );

_Requires_lock_not_held_(&Adapter->Lock)
NDIS_STATUS
MbbAdapterQueryDeviceId(
    __in    PMINIPORT_ADAPTER_CONTEXT Adapter
    );

_Requires_lock_not_held_(&Adapter->Lock)
NDIS_STATUS
MbbAdapterFWDeviceServicesToCapabilities(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in PMBB_REQUEST_CONTEXT       Request
    );

_Requires_lock_not_held_(&Adapter->Lock)
VOID
MbbAdapterResetCapabilities(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    );


VOID
AdapterStallClearHandler(
    PSTATE_CHANGE_EVENT             StateChange
    );


NDIS_STATUS
MbbAdapterSendNetworkShutdownHint(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    );

NDIS_STATUS
MbbAdapterSendPreShutdown(
    _In_ PMINIPORT_ADAPTER_CONTEXT  Adapter
);

_Requires_lock_not_held_(&Adapter->Lock)
VOID
MbbAdapterSetShutdownNotificationCapabilities(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    );

VOID
MbbAdapterIndicateD3Exit(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in NDIS_PORT_NUMBER           PortNumber
    );


_Requires_lock_not_held_(&Adapter->Lock)
VOID
MbbAdapterSetOptionalServiceSupport(
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    );

_Requires_lock_not_held_(&SendQueue->AdapterContext->Lock)
VOID TryQueueStallState(
    _In_ PMBB_SEND_QUEUE SendQueue
);

VOID WaitStallClearComplete(
    _In_ PADAPTER_STATE AdapterState
);
