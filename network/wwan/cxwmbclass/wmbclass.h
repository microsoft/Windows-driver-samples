//
//    Copyright (C) Microsoft.  All rights reserved.
//
////////////////////////////////////////////////////////////////////////////////
//
//  DEFINES
//
////////////////////////////////////////////////////////////////////////////////
#define WMBCLASS_INTERFACE_TYPE NdisInterfaceInternal;
#define WMBCLASS_CHECK_FOR_HANG_INTERVAL 3;
#define WMBCLASS_MAX_WOL_PATTERN (256)
#define WMBCLASS_MAX_MBIM_WOL_PATTERN (192)


#define MBB_DEFAULT_SESSION_ID      0

#define MBB_REGVAL_NETCFGID             L"NetCfgInstanceId"
#define MBB_REGVAL_OVERRIDE_NAME        L"AllowDriverToOverrideDeviceName"
#define MBB_REGVAL_RADIO_OFF            L"RadioOff"
#define MBB_REGVAL_LAST_IPv4            L"LastIPv4"
#define MBB_REGVAL_LAST_IPv6            L"LastIPv6"
#define MBB_REGVAL_LAST_GWv4            L"LastGWv4"
#define MBB_REGVAL_LAST_GWv6            L"LastGWv6"
#define MBB_REGVAL_LAST_DNSv4           L"LastDNSv4"
#define MBB_REGVAL_LAST_DNSv6           L"LastDNSv6"

#define MBB_LOCK_TAKEN      TRUE
#define MBB_LOCK_NOT_TAKEN  FALSE

#define MBB_PSEUDO_IMEI L"002099001761481"
#define MBB_PSEUDO_ESN  L"12816777215"

#define MBB_MAC_ADDRESS_LENGTH  6


#define MBB_MAX_SERVICE_ACTIVATION_BUFFER 0x10000
#define MBB_MAX_READY_INFO_PHONE_NUMBERS  100
#define MBB_MAX_PROVIDER_LIST_SIZE        1024
#define MBB_MAX_PROVISIONED_CONTEXTS      1024

#define MBB_STATUS_INDICATION_ALREADY_SENT NDIS_STATUS_NOT_RECOGNIZED

#define MBB_MAX_NUMBER_OF_PORTS 11  // including the default port


////////////////////////////////////////////////////////////////////////////////
//
//  TYPEDEFS
//
////////////////////////////////////////////////////////////////////////////////

//
// Forward declaration
//

typedef struct _MBB_PORT         MBB_PORT, *PMBB_PORT;

typedef struct _MINIPORT_DRIVER_CONTEXT
{
    WDFDRIVER               hDriver;
    NDIS_HANDLE             NdisDriverHandle;
    HANDLE                  IpInterfaceNotificationHandle;
    HANDLE                  IpRouteNotificationHandle;
    HANDLE                  IpUnicastAddressNotificationHandle;
    PVOID                   IpWorkItemManagerHandle;
    NDIS_SPIN_LOCK          AdapterListLock;
    LIST_ENTRY              AdapterList;
    REGHANDLE               TraceHandle;
    ULONG                   CurrentTraceInstance;
    REGHANDLE               TraceHandleOpn;
} MINIPORT_DRIVER_CONTEXT,
*PMINIPORT_DRIVER_CONTEXT;

typedef struct _MBB_OID_HANDLER_ENTRY   MBB_OID_HANDLER_ENTRY;
typedef struct _MBB_OID_HANDLER_ENTRY* PMBB_OID_HANDLER_ENTRY;

typedef struct _MBB_REQUEST_CONTEXT   MBB_REQUEST_CONTEXT;
typedef struct _MBB_REQUEST_CONTEXT* PMBB_REQUEST_CONTEXT;

typedef struct _MBB_REQUEST_MANAGER   MBB_REQUEST_MANAGER;
typedef struct _MBB_REQUEST_MANAGER* PMBB_REQUEST_MANAGER;

typedef struct _MINIPORT_ADAPTER_CONTEXT    MINIPORT_ADAPTER_CONTEXT;
typedef struct _MINIPORT_ADAPTER_CONTEXT  *PMINIPORT_ADAPTER_CONTEXT;

typedef
__callback VOID
(*MBB_REQUEST_COMPLETION_CALLBACK)(
    __in    MBB_PROTOCOL_HANDLE     AdapterHandle,
    __in    PMBB_REQUEST_CONTEXT    Request,
    __in    NDIS_STATUS             NdisStatus
    );

typedef
__callback NDIS_STATUS
(*MBB_REQUEST_DISPATCH_ROUTINE)(
    __in    MBB_PROTOCOL_HANDLE             AdapterHandle,
    __in    PMBB_REQUEST_CONTEXT            Request
    );

typedef
__callback VOID
(*MBB_REQUEST_RESPONSE_HANDLER)(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );
/*++
    Description
        Callback invoked when the response is ready or was failed to retrieve.

    Parameters
    __in PMBB_REQUEST_CONTEXT Request
        In case of command response the request context used to send the encapsulated command.
        In case of status indication arbitrary request context used to retrieve the status indication.

    __in NDIS_STATUS NdisStatus
        Whether retrieving the response was successful.
        This is used to distinguish whether the host was unsuccessful in retrieving a reponse or
        whether the device completed the response with failure status.

    __in MBB_STATUS MbbStatus
        If the host was successful in retrieving the response i.e. NdisStatus is NDIS_STATUS_SUCCESS
        then the status that the device returned.

    __in_bcount_opt(InBufferSize) PUCHAR InBuffer
        If the host was successful in retrieving the response i.e. NdisStatus is NDIS_STATUS_SUCCESS
        and the device responded successfully i.e. MBB_STATUS_SUCCESS( ) then the response buffer.

    __in ULONG InBufferSize
        If the host was successful in retrieving the response i.e. NdisStatus is NDIS_STATUS_SUCCESS
        and the device responded successfully i.e. MBB_STATUS_SUCCESS( ) then the response buffer length.

    Return Value
        None
--*/

typedef NDIS_STATUS
(*MBB_OID_SET_HANDLER)(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

typedef NDIS_STATUS
(*MBB_OID_QUERY_HANDLER)(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

typedef VOID
(*MBB_OID_COMPLETION_HANDLER)(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus
    );

typedef NDIS_STATUS
(*MBB_OID_RESPONSE_HANDLER)(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

struct _MBB_OID_HANDLER_ENTRY
{
    NDIS_OID                    Oid;
    ULONG                       IsSettable:1;
    ULONG                       IsQueryable:1;
    ULONG                       IsSerialized:1;
    ULONG                       IsIndicationRequired:1;
    ULONG                       CompleteRequestOnSendComplete:1;
    //
    // Valid for InBuffers i.e. Sets and Methods
    //
    UCHAR                       NdisObjectHeaderType;
    UCHAR                       NdisObjectHeaderRevision;
    USHORT                      NdisObjectHeaderSize;

    MBB_OID_SET_HANDLER         SetHandler;
    ULONG                       MinSetInBufferSize;

    MBB_OID_QUERY_HANDLER       QueryHandler;
    ULONG                       MinQueryInBufferSize;
    ULONG                       MinQueryOutBufferSize;
    //
    // Valid, if a Response CID is associated with this OID
    //
    MBB_COMMAND                 ResponseCommand;
    ULONG                       ResponseBufferLength;
    MBB_OID_RESPONSE_HANDLER    ResponseHandler;
    //
    // Optional completion handler for cleanup on send failure.
    //
    MBB_OID_COMPLETION_HANDLER  CompletionHandler;
};

typedef enum
{
    //
    // Dispatch the request. Request may be queued behind others.
    //
    MbbRequestEventDispatch = 0,
    //
    // Request is current, start processing the request.
    //
    MbbRequestEventStart,
    //
    // Request was cancelled
    //
    MbbRequestEventCancel,
    //
    // Bus layer indicated (un)succesful sending of all fragments
    //
    MbbRequestEventSendComplete,
    //
    // A response matching the TransactionId of the request was received.
    //
    MbbRequestEventResponseReceived,
    //
    // Same as MbbRequestStateResponseReceived but with MBB_STATUS_MORE_DATA.
    //
    MbbRequestEventResponseReceivedMoreData,
    //
    //
    //
    MbbRequestEventMaximum

} MBB_REQUEST_EVENT;

typedef enum
{
    //
    // Initial state, request is created and ready to be dispatched.
    // If the request is cancelled in this state the request owner
    // is not notified about the cancellation since the request
    // is not dispatched yet and does not have a completion routine.
    //
    MbbRequestStateReady = 0,
    //
    // Request is queued for dispatch as requests are serialized.
    // The request may be started immediately if its the current request
    // or may have to wait for other requests to complete. When the request
    // is cancelled in this state the completion callback is called with failure.
    //
    MbbRequestStateDispatching,
    //
    // Initial state, when not queued.
    // Dispatch routine has sent all the fragments to the bus layer
    // which is still sending them to the device.
    //
    MbbRequestStateSendPending,
    //
    // Bus layer completed sending all fragments to the device.
    // The FSM notifies the CompletionCallback.
    //
    MbbRequestStateSendComplete,
    //
    // The host received response from the device matching the TransactionId
    // of this request. The rquest will be completed to higher layers.
    // The FSM notifies the ResponseAvailable callback.
    //
    MbbRequestStateResponseReceived,
    //
    // Same as MbbRequestStateResponseReceived but with MBB_STATUS_MORE_DATA.
    // More MbbRequestEventResponseReceived or MbbRequestEventResponseReceivedMoreData
    // are expected.
    //
    MbbRequestStateResponseReceivedMoreData,
    //
    // The request is cancelled.
    // The device didnt respond within a defined amount of time.
    // The request will complete with failure status.
    // The FSM notifies the CompletionCallback / ResponseAvailable callback.
    //
    MbbRequestStateCancelled,
    //
    // Catch invalid state transitions.
    //
    MbbRequestStateInvalid,
    //
    //
    //
    MbbRequestStateMaximum

} MBB_REQUEST_STATE;

typedef struct
{
    ULONG   InUse:1;
    ULONG   Allocated:1;

} MBB_EVENT_FLAGS;

typedef struct _MBB_EVENT_ENTRY
{
    LIST_ENTRY              EventLink;
    MBB_EVENT_FLAGS         EventFlags;
    MBB_REQUEST_EVENT       Event;
    PMBB_REQUEST_CONTEXT    Request;
    PVOID                   EventData;
    ULONG                   EventDataLength;

} MBB_EVENT_ENTRY,
*PMBB_EVENT_ENTRY;

typedef struct _MBB_REASSEMBLE_CONTEXT
{
    PUCHAR      FragmentBuffer;
    ULONG       FragmentBufferLength;
    ULONG       FragmentLength;
    PUCHAR      Buffer;
    ULONG       BufferLength;
    ULONG       BufferOffset;
    ULONG       DataLength;
    ULONG       TransactionId;
    ULONG       FragmentCount;
    ULONG       NextFragment;
    NDIS_STATUS NdisStatus;
    MBB_STATUS  MbbStatus;
    MBB_COMMAND Command;

} MBB_REASSEMBLE_CONTEXT,
*PMBB_REASSEMBLE_CONTEXT;

typedef
__callback VOID
(*MBB_DRAIN_COMPLETE)(
    __in    PVOID                   Context
    );


typedef struct _DRAIN_OBJECT {

    NDIS_SPIN_LOCK              Lock;
    ULONG                       Count;
    BOOL                        Draining;
    BOOL                        DrainComplete;

    MBB_DRAIN_COMPLETE          DrainCompleteCallback;
    PVOID                       Context;

} DRAIN_OBJECT, *PDRAIN_OBJECT;

VOID
InitDrainObject(
    PDRAIN_OBJECT               DrainObject,
    MBB_DRAIN_COMPLETE          DrainCompleteCallback,
    PVOID                       Context
    );

VOID
StartDrain(
    PDRAIN_OBJECT               DrainObject
    );

VOID
DrainComplete(
    PDRAIN_OBJECT               DrainObject
    );

BOOLEAN
DrainAddRef(
    PDRAIN_OBJECT               DrainObject
    );

VOID
DrainRelease(
    PDRAIN_OBJECT               DrainObject
    );

//
// MBB_NDIS_OID_STATE
//
// Oids have multi-states to correctly complete them
// without race condition between the dispatch thread
// and the completion thread. Note that request manager
// can call a completion routine if the request was
// cancelled or dispatch failed. Thus the oids completion
// handler may be called without the dispatch handler
// ever being called. All the different possibilities or
// states are tracked through the oid state.
//
typedef enum
{
    MbbNdisOidStateNone = 0,
    //
    // The oid is pending with request manager
    // to be dispatched. The oid can get cancelled
    // without the Oid Dispatch routine ever being
    // called. If the Oid Completion routine runs
    // at this state then it should 
    //
    MbbNdisOidStatePending,
    //
    // Oid Dispatch routine is currently running.
    // If the Oid completion routine runs during
    // this state then it will defer the oid
    // completion to the dispatch handler.
    //
    MbbNdisOidStateDispatching,
    //
    // Oid dispatch routine has completed. If the
    // Oid completion routine runs after this point
    // then it will complete the Oid.
    //
    MbbNdisOidStateDispatched,
    //
    // Oid completion routine has run. If the Dispatch
    // routine finds this state then it means that the
    // Oid completion routine has deferred the Oid 
    // competion. The Dispatch routine will complete Oid.
    //
    MbbNdisOidStateComplete,
    MbbNdisOidStateMax

} MBB_NDIS_OID_STATE;

struct _MBB_REQUEST_CONTEXT
{
    //
    // Request Management
    //

    LIST_ENTRY                      ManagerLink;
    ULONG                           RequestId;
    ULONG                           TransactionId;
    GUID                            ActivityId;
    
    // Controls the lifetime of the request.
    // On creation the RefCount is set to 1.
    // Destroy drops this initial RefCount.
    // When the RefCount drops to zero request is freed.
    ULONG                           ReferenceCount;
    // Tracks the request.
    // RequestManager->Lock is used to synchronize 
    // access to the fields of the request.
    PMBB_REQUEST_MANAGER            RequestManager;
    // Event to wait for request to get into desired state - Send Complete,
    // Response received
    KEVENT                          WaitEvent;

    PMBB_OID_HANDLER_ENTRY          OidHandler;

    //
    // State Machine
    //

    ULONGLONG                       DispatchTime;
    MBB_REQUEST_STATE               State;
    MBB_REQUEST_STATE               LastState;
    MBB_REQUEST_EVENT               LastEvent;

    //
    // Pre-allocated resources for forward progress.
    //

    MBB_EVENT_ENTRY                 EventEntries[MbbRequestEventMaximum];

    //
    // Per sub-component contexts
    //

    // Valid if request was created for ndis oid.
    struct
    {
        PNDIS_OID_REQUEST               OidRequest;
        PVOID                           OidRequestId;
        NDIS_HANDLE                     OidRequestHandle;
        NDIS_STATUS                     OidStatus;
        BOOLEAN                         IsSetOid;
        // synchronized with interlock
        MBB_NDIS_OID_STATE              OidState;

    } OidContext;

    struct
    {
        BOOLEAN                         IsSerialized;
        BOOLEAN                         IsQueued;
        LIST_ENTRY                      QueueLink;
        LIST_ENTRY                      CancelLink;
        LIST_ENTRY                      TimeoutLink;
        MBB_REQUEST_DISPATCH_ROUTINE    DispatchRoutine;
        MBB_REQUEST_COMPLETION_CALLBACK CompletionCallback;
        MBB_REQUEST_RESPONSE_HANDLER    ResponseHandler;

    } ReqMgrContext;

    struct
    {
        PVOID                           DataToFreeOnCompletion;
        PVOID                           DataToFreeOnResponse;
        BOOLEAN                         IsIndication;
        //
        // Send Command
        //
        struct
        {
            // Static fields
            ULONG                       FragmentLength;
            ULONG                       FragmentCount;
            PUCHAR                      Data;
            ULONG                       DataLength;
            MBB_COMMAND                 Command;
            MBB_COMMAND_TYPE            CommandType;
            // Dynamic fields
            BOOLEAN                     IsProcessing;
            ULONG                       NextFragmentIndex;
            ULONG                       FragmentSentCount;
            NDIS_STATUS                 SendStatus;

        } Command;
        //
        // Get Response
        //
        MBB_REASSEMBLE_CONTEXT          Response;
        //
        // Parameters
        //
        union {
            struct {
                WCHAR                   DeviceId[WWAN_DEVICEID_LEN];
                GUID                    CurrentQueriedDeviceService;
                ULONG                   IsUssdCapsValid:1;
                ULONG                   IsAuthCapsValid:1;
                ULONG                   NdisDeviceCapsSize;
                PNDIS_WWAN_DEVICE_CAPS  NdisDeviceCaps;
            } DeviceCaps;

            struct {
                ULONG                   IsWwanEmergencyModeValid:1;
                ULONG                   IsCdmaShortMsgSizeValid:1;
                ULONG                   IsQueryReadyInfo:1;
                ULONG                   IsFirstCid:1;
                ULONG                   NdisReadyInfoSize;
                PNDIS_WWAN_READY_INFO   NdisReadyInfo;
                WWAN_EMERGENCY_MODE     WwanEmergencyMode;
                UCHAR                   CdmaShortMsgSize;
            } SubscriberReadyInfo;

            struct {
                ULONG                   IsWwanAvailableDataClassValid:1;
                ULONG                   IsQuery:1;
                ULONG                   IsFirstCid:1;
                ULONG                   NdisPacketServiceStateSize;
                PNDIS_WWAN_PACKET_SERVICE_STATE   NdisPacketServiceState;
                ULONG                   AvailableDataClass;
            } PacketServiceState;

            struct {
                BOOLEAN Activate;
                ULONG   ConnectionId;
                ULONG   SessionId;
            } Connect;

            struct {
                MBB_CONNECTION_STATE    ConnectionState;
                ULONG                   SessionId;
            } IpAddress;
            struct {
                // The radio state set that was requested
                WWAN_RADIO  SetAction;
            } RadioState;
            struct {
                // Info about the external request
                PMBB_SUBSCRIBE_EVENT_LIST      ExtList;  // Memory managed via DataToFreeOnResponse
                ULONG       ExtSize;
            } EventSubscribe;
            struct {
                GUID        DeviceServiceGuid;
                ULONG       SessionId;
                MBB_DSS_LINK_STATE  LinkState;
            } DssSession;
            
            struct {
                // used for sync device services subscription changes
                BOOLEAN     FullPower;
                ULONG       MediaSpecificWakeUpEvents;
                ULONG       WakeUpFlags;
            } SyncDeviceServiceSubription;
            struct {
                BOOLEAN          Set;
                NDIS_PORT_NUMBER PortNumber;
            } SetPacketFilter;           
           struct {
           PMBB_PORT Port;
            } NdisPortContext;
           struct {
            ULONG SessionId;
            } ContextSessionId;                  
        }                               Parameters;

    } HandlerContext;
};

typedef enum
{
    MbbTimerTypeRequest = 0,
    MbbTimerTypeFragment,
    MbbTimerTypeMaximum

} MBB_TIMER_TYPE;

typedef struct _MBB_TIMER_CONTEXT
{
    MBB_TIMER_TYPE          TimerType;
    ULONGLONG               TimerArmTime;
    NDIS_HANDLE             TimerHandle;
    ULONG                   TimerDelayInMS;
    ULONG                   TimerPeriodInMS;
    ULONG                   TimerToleranceInMS;
    PNDIS_TIMER_FUNCTION    TimerFunction;
    PMBB_REQUEST_MANAGER    RequestManager;

} MBB_TIMER_CONTEXT,
*PMBB_TIMER_CONTEXT;

typedef struct _MBB_REQUEST_MANAGER
{
    PMINIPORT_ADAPTER_CONTEXT   AdapterContext;
    LIST_ENTRY                  AllocatedRequestList;
    LIST_ENTRY                  PendingRequestQueue;
    MBB_TIMER_CONTEXT           TimerContexts[MbbTimerTypeMaximum];
    PMBB_REQUEST_CONTEXT        CurrentRequest;
    NDIS_SPIN_LOCK              Spinlock;
    KEVENT                      NoAllocatedRequestEvent;
    ULONG                       RequestIdCounter;
    DRAIN_OBJECT                DrainObject;
    KEVENT                      DrainCompleteEvent;
    //
    // Pre-allocated resources for fragmentation & reassembly
    //
    struct
    {
        ULONG                   ControlFragmentLength;
        ULONG                   BulkFragmentLength;
        PVOID                   BufferManager;
        BOOLEAN                 ReassembleInUse;
        MBB_REASSEMBLE_CONTEXT  Reassemble;

    } Fragmentation;
    //
    // Work Item
    //
    PVOID                       WorkItemManagerHandle;
    //
    // Request allocator
    //
    PVOID                       RequestAllocatorHandle;
    // BOOLEAN to indicate whether the Request Manager is currently closed and Draining all the 
    //  outstanding requests in the Queue. Used in the Surprise Removal Path
    BOOLEAN                     IsClosed;

} MBB_REQUEST_MANAGER,
*PMBB_REQUEST_MANAGER;

typedef enum
{
    MbbDataPathTelmetryStatusNotReport = 0,
    MbbDataPathTelmetryStatusReportSuccess,
    MbbDataPathTelmetryStatusReportHang
} MBB_DATA_PATH_TELMETRY_STATUS;

typedef struct _MBB_SEND_QUEUE
{
    //
    // Queue state
    //
    NDIS_SPIN_LOCK              Lock;
    BOOLEAN                     ProcessingQueue;
    DRAIN_OBJECT                QueueDrainObject;
    //
    // Lookaside lists
    //
    NPAGED_LOOKASIDE_LIST      NblLookasideList;
    NPAGED_LOOKASIDE_LIST      NtbLookasideList;
    NPAGED_LOOKASIDE_LIST      NbLookasideList;
    //
    // Track NTBs
    //
    ULONG                       ConcurrentSends;
    ULONG                       MaxConcurrentSends;
    LIST_ENTRY                  NtbQueue;
    ULONG                       NtbSequence;
    //
    // Track NBLs
    //
    LIST_ENTRY                  NblTrackList;
    LIST_ENTRY                  NblDispatchQueue;
    KEVENT                      NblQueueEmptyEvent;
    //
    // Handles
    //
    PMINIPORT_ADAPTER_CONTEXT   AdapterContext;
    MBB_BUS_HANDLE              BusHandle;
    PCHAR                       PaddingBuffer;

#if DBG
    //
    // Testing
    //
    PCHAR                       ScratchBuffer;
    ULONG                       ScratchLength;
#endif

    //
    // NBL pool for DSS
    //
    NDIS_HANDLE                 NblPool;
    MBB_DATA_PATH_TELMETRY_STATUS LastDataPathTelemetryStatus;

} MBB_SEND_QUEUE,
*PMBB_SEND_QUEUE;

typedef struct _MBB_RECEIVE_QUEUE
{
    //
    // Queue State
    //
    BOOLEAN                 LookasideList;
    NDIS_SPIN_LOCK          Lock;
    KEVENT                  QueueEmptyEvent;
    DRAIN_OBJECT            QueueDrainObject;
    //
    // Track Receives
    //
    LIST_ENTRY              ReceivedQueue;
    //
    // Resources
    //
    NDIS_HANDLE             NblPool;
    NPAGED_LOOKASIDE_LIST   ReceiveLookasideList;

} MBB_RECEIVE_QUEUE,
*PMBB_RECEIVE_QUEUE;

typedef enum _STATE_CHANGE_TYPE {

    STATE_CHANGE_TYPE_PAUSE = 0,
    STATE_CHANGE_TYPE_RESTART = 1,
    STATE_CHANGE_TYPE_POWER = 2,
    STATE_CHANGE_TYPE_RESET = 3,
    STATE_CHANGE_TYPE_STALL_CLEAR = 4,
    STATE_CHANGE_MAX =5

} STATE_CHANGE_TYPE, *PSTATE_CHANGE_TYPE;

#define STATE_CHANGE_EVENT_RESERVE_COUNT  (10)

typedef struct _STATE_CHANGE_EVENT {

    LIST_ENTRY                  ListEntry;
    STATE_CHANGE_TYPE           EventType;
    PVOID                       Context1;
    PVOID                       Context2;

    union  {
        struct {
            PMBB_REQUEST_CONTEXT        Request;
            NET_DEVICE_POWER_STATE      NewPower;
        } Power;
        struct {
            NTSTATUS                    PipeStartStatus;
        } Reset;
    };

} STATE_CHANGE_EVENT, *PSTATE_CHANGE_EVENT;

typedef
__callback VOID
(*MBB_STATE_CHANGE_HANDLER)(
    PSTATE_CHANGE_EVENT             StateChange
    );



typedef struct _ADAPTER_STATE {

    BOOLEAN                     ShuttingDown;

    BOOLEAN                     Started;

    NET_DEVICE_POWER_STATE      CurrentPowerState;

    BOOLEAN                     Hung;

    PSTATE_CHANGE_EVENT         RunningEvent;

    ULONG                       PendingActions;

    NDIS_SPIN_LOCK              Lock;
    NDIS_HANDLE                 WorkItem;
    LIST_ENTRY                  ListEntry;
    PSTATE_CHANGE_EVENT         CurrentEvent;

    LIST_ENTRY                  FreeList;

    MBB_STATE_CHANGE_HANDLER    Handlers[STATE_CHANGE_MAX];

    KEVENT                      StallClearCompleteEvent;

} ADAPTER_STATE, *PADAPTER_STATE;

// Per-device service state
typedef struct _MBB_DS
{
    // Device service ID (from device)
    GUID                    DeviceServiceId;

    // DSS state (from device)
    WWAN_DEVICE_SERVICE_SESSION_CAPABILITY DSSCapability;
    ULONG                   MaxDSSInstances;
    
    // Number of CIDs supported (from device)
    ULONG                   CIDCount;
    
    // Pointer to the CID list (from device)
    PULONG                  CIDList;

    
} MBB_DS, *PMBB_DS;

// The state for all device services
typedef struct _MBB_DS_STATE
{
    // Number of supported services (from device)
    ULONG                   ServicesCount;

    // Pointer to list of device services (from device)
    PMBB_DS                 ServicesList;

    // Maximum number of DSS sessions (from device)
    ULONG                   MaxDSSSessions;

    // Subscribe OID settings
    ULONG                   ExtSubscribeListBufferSize;
    PMBB_SUBSCRIBE_EVENT_LIST ExtSubscribeList;
    
} MBB_DS_STATE, *PMBB_DS_STATE;

typedef union
{
    ULONG       Value;
    struct
    {
        ULONG   IsMultiCarrier:1;
        ULONG   IsUssdCapable:1;
        ULONG   IsSimAuthCapable:1;
        ULONG   IsAkaAuthCapable:1;
        ULONG   IsAkapAuthCapable:1;
        ULONG   IsShowIMSI:1;
        ULONG   ShutdownNotificationCapable:1;
        ULONG   IsPreshutdownCapable:1;
        ULONG   IsProvisionedContextV2Capable : 1;
        ULONG   IsNetworkBlacklistCapable : 1;
        ULONG   IsSARCapable : 1;
        ULONG   IsLTEAttachConfigCapable : 1;
        ULONG   IsMultiSIMCapable : 1;
        ULONG   IsUiccLowLevelCapable : 1;
        ULONG   IsDeviceCapsV2Capable : 1;
        ULONG   IsPcoCapable : 1;
        ULONG   IsDeviceResetCapable : 1;
        ULONG   IsBaseStationsInfoCapable : 1;
    };
} ADAPTER_FLAGS;

typedef struct _POWER_FILTER_LOOKUP {

    ULONG               PatternId;
    BOOLEAN             InUse;
    ULONG               MaskSize;
    PUCHAR              Mask;
    PUCHAR              Pattern;
    NDIS_PORT_NUMBER    PortNumber;
} POWER_FILTER_LOOKUP, *PPOWER_FILTER_LOOKUP;

typedef struct _SESSIONID_PORTNUMBER_ENTRY
{
    // Indicates whether the session Id is in use or not
    BOOLEAN             InUse;

    // Valid only when InUse is set. Corresponds
    // to the NDIS PORT number for this session
    NDIS_PORT_NUMBER    PortNumber;
    
} SESSIONID_PORTNUMBER_ENTRY, *PSESSIONID_PORTNUMBER_ENTRY;


typedef struct _MINIPORT_ADAPTER_CONTEXT
{
    //
    // NDIS information
    //
    NDIS_SPIN_LOCK          Lock;
    ULONG                   Reference;
    NDIS_HANDLE             MiniportAdapterHandle;
    ULONG                   TraceInstance;

    NET_IFINDEX             IfIndex;
    NET_LUID                NetLuid;
    BOOLEAN                 OverrideDeviceName;
    GUID                    NetCfgId;
    LIST_ENTRY              DriverLink;
    UCHAR                   MACAddress[MBB_MAC_ADDRESS_LENGTH];
    //
    // Bus information
    //
    PDEVICE_OBJECT          Pdo;
    PDEVICE_OBJECT          Fdo;
    PDEVICE_OBJECT          NextDeviceObject;
    ANSI_STRING             FriendlyName;
    MBB_BUS_HANDLE          BusHandle;
    MBB_BUS_PARAMETERS      BusParams;
    //
    // Adapter State
    //
    ADAPTER_STATE           AdapterState;
    //
    // Request processing
    //
    MBB_SEND_QUEUE          SendQueue;
    MBB_RECEIVE_QUEUE       ReceiveQueue;
    MBB_REQUEST_MANAGER     RequestManagerAllocation;
    PMBB_REQUEST_MANAGER    RequestManager;
    //
    // Stats
    //
    ULONG64                 MaxLinkSpeed;
    ULONG64                 CurrentLinkSpeed;

    ULONGLONG               UplinkSpeed;
    ULONGLONG               DownlinkSpeed;

    ULONGLONG               GenXmitFramesOk;
    ULONGLONG               GenRcvFramesOk;
    NDIS_STATISTICS_INFO    Stats;
  
    // Radio state: TRUE = OFF, FALSE = ON
    BOOLEAN                 RadioOff;

    BOOLEAN                 AvailableDataClassValid;
    ULONG                   AvailableDataClass;
    ULONG                   AdapterDataClass;
    ADAPTER_FLAGS           AdapterFlags;
    // Used for multi-mode adapters
    MBB_CELLULAR_CLASS      AdapterSupportedCellularClass;
    MBB_CELLULAR_CLASS      AdapterCurrentCellularClass;

    PPOWER_FILTER_LOOKUP    PowerFilterTable;

    NDIS_MINIPORT_SYSPOWER_NOTIFY   LastLowSystemPowerState;

    //
    // Device services related state
    //
    MBB_DS_STATE            DeviceServiceState;

    //
    // Protects Port specific information i.e
    // 1) NumberofPorts
    // 2) PortList.
    // 3) MaxActivatedContexts
    // Separate lock  is required to protect Port specific 
    // information to avoid contention for Adapter lock.
    //
    
    NDIS_SPIN_LOCK          PortsLock;
    
    /**
       * List to hold the ports created on this adapter
       */
    MBB_PORT*               PortList[MBB_MAX_NUMBER_OF_PORTS];
    ULONG                   NumberOfPorts;

    // Indicates how many PDP contexts can this adapter(device) hold.
    // This should come from OID_WWAN_DEVICE_CAPS
    ULONG                   MaxActivatedContexts;

    NDIS_SPIN_LOCK          SessionIdPortTableLock;
    
    SESSIONID_PORTNUMBER_ENTRY    SessionIdPortTable[MBB_MAX_NUMBER_OF_PORTS];

    BOOLEAN                 SurpriseRemoved;
} MINIPORT_ADAPTER_CONTEXT,
*PMINIPORT_ADAPTER_CONTEXT;

extern MINIPORT_DRIVER_CONTEXT GlobalControl;

// MBB services natively supported by Windows via OIDs
static GUID NativeMbnServices[]  = 
{ 
    MBB_UUID_BASIC_CONNECT_CONSTANT,
    MBB_UUID_SMS_CONSTANT,
    MBB_UUID_USSD_CONSTANT,
    MBB_UUID_AUTH_CONSTANT,
    MBB_UUID_DSS_CONSTANT,
    MBB_UUID_BASIC_CONNECT_EXT_CONSTANT
};


////////////////////////////////////////////////////////////////////////////////
//
//  PROTOTYPES
//
////////////////////////////////////////////////////////////////////////////////
//
// Driver routines
//
_Acquires_lock_( Driver->AdapterListLock )
__drv_raisesIRQL(DISPATCH_LEVEL)
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_savesIRQLGlobal( NdisSpinLock, Driver )
VOID
MbbDriverLockAdapterList(
    __in PMINIPORT_DRIVER_CONTEXT Driver
    );

_Releases_lock_( Driver->AdapterListLock )
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_minIRQL(DISPATCH_LEVEL)
__drv_restoresIRQLGlobal( NdisSpinLock, Driver )
VOID
MbbDriverUnlockAdapterList(
    __in PMINIPORT_DRIVER_CONTEXT Driver
    );

VOID
MbbDriverAddAdapter(
    __in PMINIPORT_DRIVER_CONTEXT   Driver,
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    );

VOID
MbbDriverRemoveAdapter(
    __in PMINIPORT_DRIVER_CONTEXT   Driver,
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    );

PMINIPORT_ADAPTER_CONTEXT
MbbDriverFindAdapterByNetLuid(
    __in PMINIPORT_DRIVER_CONTEXT   Driver,
    __in PNET_LUID                  NetLuid
    );
//
// NDIS Miniport routines
//
MINIPORT_OID_REQUEST                MbbNdisMiniportOidRequest;
MINIPORT_CANCEL_OID_REQUEST         MbbNdisMiniportCancelOidRequest;
MINIPORT_SEND_NET_BUFFER_LISTS      MbbNdisMiniportSendNetBufferLists;
MINIPORT_CANCEL_SEND                MbbNdisMiniportCancelSend;
MINIPORT_RETURN_NET_BUFFER_LISTS    MbbNdisMiniportReturnNetBufferLists;

MBB_BUS_HANDLE
MbbNdisGetBusHandle(
    __in    MBB_PROTOCOL_HANDLE AdapterHandle
    );

VOID
MbbNdisReceiveCallback(
    __in    MBB_PROTOCOL_HANDLE     ProtocolHandle,
    __in    MBB_RECEIVE_CONTEXT     ReceiveContext,
    __in    PMDL                    Mdl
    );

VOID
DrainCompleteCallback(
    PVOID       Context
    );

PMBB_OID_HANDLER_ENTRY
MbbNdisGetOidHandlerByCommand(
    __in PMBB_COMMAND   Command
    );

// Dss Send complete handler
VOID
MbbNdisDeviceServiceSessionSendComplete(
    __in    MBB_REQUEST_HANDLE      RequestHandle,
    __in    NDIS_STATUS             NdisStatus
    );

// Dss Receive handler
NDIS_STATUS
MbbNdisDeviceServiceSessionReceive(
    __in PMINIPORT_ADAPTER_CONTEXT          AdapterContext,
    __in ULONG                              SessionId,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );
