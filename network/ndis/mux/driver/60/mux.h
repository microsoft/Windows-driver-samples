/*++
Copyright (c) 1992-2001  Microsoft Corporation

Module Name:

    mux.h

Abstract:

    Data structures, defines and function prototypes for the MUX driver.

Environment:

    Kernel mode only.

Revision History:


--*/

// disable warnings

#if _MSC_VER >= 1200

#pragma warning(push)

#endif

#pragma warning(disable:4200) //  zero-sized array in struct/union

#define MUX_MAJOR_NDIS_VERSION         6
#define MUX_MINOR_NDIS_VERSION         0

#define MUX_MAJOR_DRIVER_VERSION       3
#define MUX_MINOR_DRIVER_VERSION       0

#define MUX_PROT_MAJOR_NDIS_VERSION    6
#define MUX_PROT_MINOR_NDIS_VERSION    0

#define MUX_TAG        'SxuM'
#define WAIT_INFINITE  0

extern LONG               MiniportCount;

#if DBG
//
// Debug levels: lower values indicate higher urgency
//
#define MUX_EXTRA_LOUD       20
#define MUX_VERY_LOUD        10
#define MUX_LOUD             8
#define MUX_INFO             6
#define MUX_WARN             4
#define MUX_ERROR            2
#define MUX_FATAL            0

extern INT                muxDebugLevel;


#define DBGPRINT(lev, Fmt)                                   \
    {                                                        \
        if ((lev) <= muxDebugLevel)                          \
        {                                                    \
            DbgPrint("MUX-IM: ");                            \
            DbgPrint Fmt;                                    \
        }                                                    \
    }
#else

#define DBGPRINT(lev, Fmt)

#endif //DBG



#define ETH_IS_LOCALLY_ADMINISTERED(Address) \
    (BOOLEAN)(((PUCHAR)(Address))[0] & ((UCHAR)0x02))

// forward declarations
typedef struct _ADAPT ADAPT, *PADAPT;
typedef struct _VELAN VELAN, *PVELAN;
typedef struct _MUX_NDIS_REQUEST MUX_NDIS_REQUEST, *PMUX_NDIS_REQUEST;


typedef
VOID
(*PMUX_REQ_COMPLETE_HANDLER) (
    IN PADAPT                           pAdapt,
    IN struct _MUX_NDIS_REQUEST *       pMuxRequest,
    IN NDIS_STATUS                      Status
    );

// This OID specifies the current driver version.
// The high byte is the major version.
// The low byte is the minor version.
#define VELAN_DRIVER_VERSION            ((MUX_MAJOR_DRIVER_VERSION << 8) + \
                                         (MUX_MINOR_DRIVER_VERSION))

// media type, we use ethernet, change if necessary
#define VELAN_MEDIA_TYPE                NdisMedium802_3

// change to your company name instead of using Microsoft
#define VELAN_VENDOR_DESC               "Microsoft"

// Highest byte is the NIC byte plus three vendor bytes, they are normally
// obtained from the NIC
#define VELAN_VENDOR_ID                 0x00FFFFFF

#define VELAN_MAX_MCAST_LIST            32
#define VELAN_MAX_SEND_PKTS             5

#define ETH_MAX_PACKET_SIZE             1514
#define ETH_MIN_PACKET_SIZE             60
#define ETH_HEADER_SIZE                 14


#define VELAN_SUPPORTED_FILTERS ( \
            NDIS_PACKET_TYPE_DIRECTED      | \
            NDIS_PACKET_TYPE_MULTICAST     | \
            NDIS_PACKET_TYPE_BROADCAST     | \
            NDIS_PACKET_TYPE_PROMISCUOUS   | \
            NDIS_PACKET_TYPE_ALL_MULTICAST)

#define MUX_ADAPTER_PACKET_FILTER           \
            NDIS_PACKET_TYPE_PROMISCUOUS

                                         

#define MIN_PACKET_POOL_SIZE            255
#define MAX_PACKET_POOL_SIZE            4096

typedef UCHAR   MUX_MAC_ADDRESS[6];



//
// Default values:
//
#define MUX_DEFAULT_LINK_SPEED          100000  // in 100s of bits/sec
#define MUX_DEFAULT_LOOKAHEAD_SIZE      512

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT            DriverObject,
    IN PUNICODE_STRING           RegistryPath
    );

DRIVER_DISPATCH PtDispatch;
NTSTATUS
PtDispatch(
    IN PDEVICE_OBJECT            DeviceObject,
    IN PIRP                      Irp
    );

NDIS_STATUS
PtRegisterDevice(
    VOID
    );

NDIS_STATUS
PtDeregisterDevice(
    VOID
   );
//
// Protocol proto-types
//

PROTOCOL_SET_OPTIONS PtSetOptions;

PROTOCOL_OPEN_ADAPTER_COMPLETE_EX PtOpenAdapterComplete;

PROTOCOL_CLOSE_ADAPTER_COMPLETE_EX PtCloseAdapterComplete;

PROTOCOL_OID_REQUEST_COMPLETE PtRequestComplete;

PROTOCOL_STATUS_EX PtStatus;

PROTOCOL_BIND_ADAPTER_EX PtBindAdapter;

PROTOCOL_UNBIND_ADAPTER_EX PtUnbindAdapter;

PROTOCOL_NET_PNP_EVENT PtPNPHandler;

PROTOCOL_RECEIVE_NET_BUFFER_LISTS PtReceiveNBL;

PROTOCOL_SEND_NET_BUFFER_LISTS_COMPLETE PtSendNBLComplete;

VOID
PtQueryAdapterInfo(
    IN  PADAPT                      pAdapt
    );


VOID
PtRequestAdapterSync(
    IN  PADAPT                      pAdapt,
    IN  NDIS_REQUEST_TYPE           RequestType,
    IN  NDIS_OID                    Oid,
    IN  PVOID                       InformationBuffer,
    IN  ULONG                       InformationBufferLength
    );


VOID
PtRequestAdapterAsync(
    IN  PADAPT                      pAdapt,
    IN  NDIS_REQUEST_TYPE           RequestType,
    IN  NDIS_OID                    Oid,
    IN  PVOID                       InformationBuffer,
    IN  ULONG                       InformationBufferLength,
    IN  PMUX_REQ_COMPLETE_HANDLER   pCallback
    );

VOID
PtCompleteForwardedRequest(
    IN PADAPT                       pAdapt,
    IN PMUX_NDIS_REQUEST            pMuxNdisRequest,
    IN NDIS_STATUS                  Status
    );

VOID
PtPostProcessPnPCapabilities(
    IN PVOID                        InformationBuffer,
    IN ULONG                        InformationBufferLength
    );

VOID
PtCompleteBlockingRequest(
    IN PADAPT                       pAdapt,
    IN PMUX_NDIS_REQUEST            pMuxNdisRequest,
    IN NDIS_STATUS                  Status
    );

VOID
PtDiscardCompletedRequest(
    IN PADAPT                       pAdapt,
    IN PMUX_NDIS_REQUEST            pMuxNdisRequest,
    IN NDIS_STATUS                  Status
    );

NDIS_STATUS
PtCreateAndStartVElan(
    IN  PADAPT                      pAdapt,
    IN  PNDIS_STRING                pVElanKey
    );

PVELAN
PtAllocateAndInitializeVElan(
    IN PADAPT                       pAdapt,
    IN PNDIS_STRING                 pVElanKey
    );

VOID
PtDeallocateVElan(
    IN PVELAN                   pVElan
    );

VOID
PtStopVElan(
    IN  PVELAN                      pVElan
    );

VOID
PtUnlinkVElanFromAdapter(
    IN PVELAN                       pVElan
    );

PVELAN
PtFindVElan(
    IN    PADAPT                    pAdapt,
    IN    PNDIS_STRING              pElanKey
    );


NDIS_STATUS
PtBootStrapVElans(
    IN  PADAPT                      pAdapt,
    IN  PUNICODE_STRING             InstanceName OPTIONAL
    );

VOID
PtReferenceVElan(
    IN    PVELAN                    pVElan,
    IN    PUCHAR                    String
    );

ULONG
PtDereferenceVElan(
    IN    PVELAN                    pVElan,
    IN    PUCHAR                    String
    );

BOOLEAN
PtReferenceAdapter(
    IN    PADAPT                    pAdapt,
    IN    PUCHAR                    String
    );

ULONG
PtDereferenceAdapter(
    IN    PADAPT                    pAdapt,
    IN    PUCHAR                    String
    );


VOID
PtCloseAdapter(
    IN PADAPT Adapt
    );

//
// Miniport proto-types
//


MINIPORT_SET_OPTIONS MpSetOptions;

MINIPORT_INITIALIZE MPInitialize;

MINIPORT_HALT MPHalt;

MINIPORT_UNLOAD MPUnload;

MINIPORT_PAUSE MPPause;

MINIPORT_RESTART MPRestart;

MINIPORT_OID_REQUEST MPOidRequest;

MINIPORT_SEND_NET_BUFFER_LISTS MPSendNetBufferLists;

MINIPORT_RETURN_NET_BUFFER_LISTS MPReturnNetBufferLists;

MINIPORT_CANCEL_SEND MPCancelSendNetBufferLists;

MINIPORT_DEVICE_PNP_EVENT_NOTIFY MPDevicePnPEvent;

MINIPORT_SHUTDOWN MPAdapterShutdown;

MINIPORT_CANCEL_OID_REQUEST MPCancelOidRequest;

NDIS_STATUS
MPQueryInformation(
    IN    PVELAN                    pVElan,
    IN    PNDIS_OID_REQUEST         NdisRequest
    );

NDIS_STATUS
MPSetInformation(
    IN    PVELAN                    pVElan,
    IN    PNDIS_OID_REQUEST         NdisRequest
    );

NDIS_STATUS
MPMethodRequest(
    IN    PVELAN                    pVElan,
    IN    PNDIS_OID_REQUEST         NdisRequest
    );

NDIS_STATUS
MPSetPacketFilter(
    IN    PVELAN                    pVElan,
    IN    ULONG                     PacketFilter
    );

NDIS_STATUS
MPSetMulticastList(
    IN PVELAN                                       pVElan,
    _In_reads_bytes_(InformationBufferLength) IN PVOID   InformationBuffer,
    IN ULONG                                        InformationBufferLength,
    OUT PULONG                                      pBytesRead,
    OUT PULONG                                      pBytesNeeded
    );

PUCHAR
MacAddrToString(PVOID In
    );

VOID
MPGenerateMacAddr(
    PVELAN                          pVElan
);


NDIS_STATUS
MPForwardOidRequest(
    IN PVELAN                       pVElan,
    IN PNDIS_OID_REQUEST            NdisRequest
    );

//
// Super-structure for NDIS_REQUEST, to allow us to keep context
// about requests sent down to a lower binding.
//
typedef struct _MUX_NDIS_REQUEST
{
    PVELAN                      pVElan;     // Set iff this is a forwarded
                                            // request from a VELAN.
    NDIS_STATUS                 Status;     // Completion status
    NDIS_EVENT                  Event;      // Used to block for completion.
    PMUX_REQ_COMPLETE_HANDLER   pCallback;  // Called on completion of request
    PNDIS_OID_REQUEST           OrigRequest; //Request that originated this request
    NDIS_OID_REQUEST            Request;
    ULONG                       Refcount;   // Refcount
    BOOLEAN                     Cancelled;

} MUX_NDIS_REQUEST, *PMUX_NDIS_REQUEST;


typedef enum _MUX_ADAPTER_BINDING_STATE
{
    MuxAdapterBindingPaused,
    MuxAdapterBindingPausing,
    MuxAdapterBindingRunning
}MUX_ADAPTER_BINDING_STATE, *PMUX_ADAPTER_BINDING_STATE;

#define MUX_BINDING_ACTIVE          0x00000001
#define MUX_BINDING_CLOSING         0x00000002

//
// The ADAPT object represents a binding to a lower adapter by
// the protocol edge of this driver. Based on the configured
// Upper bindings, zero or more virtual miniport devices (VELANs)
// are created above this binding.
//
typedef struct _ADAPT
{
    // Chain adapters. Access to this is protected by the global lock.
    LIST_ENTRY                  Link;

    // References to this adapter.
    ULONG                       RefCount;

    // Handle to the lower adapter, used in NDIS calls referring
    // to this adapter.
    NDIS_HANDLE                 BindingHandle;

    // List of all the virtual ELANs created on this lower binding
    LIST_ENTRY                  VElanList;

    // Length of above list.
    ULONG                       VElanCount;

    // String used to access configuration for this binding.
    NDIS_STRING                 ConfigString;

    // Open Status. Used by bind/halt for Open/Close Adapter status.
    NDIS_STATUS                 Status;

    NDIS_EVENT                  Event;

    //
    // Packet filter set to the underlying adapter. This is
    // a combination (union) of filter bits set on all
    // attached VELAN miniports.
    //
    ULONG                       PacketFilter;

    // Power state of the underlying adapter
    NDIS_DEVICE_POWER_STATE     PtDevicePowerState;

    //
    // NDIS Medium ofr VELAN taken from the miniport below
    //
    NDIS_MEDIUM                 Medium ;

    //
    // BindParameters passed to protocol giving it information on 
    // the miniport below
    //
    NDIS_BIND_PARAMETERS        BindParameters;
    NDIS_PNP_CAPABILITIES       PowerManagementCapabilities;
    NDIS_RECEIVE_SCALE_CAPABILITIES RcvScaleCapabilities;
    NDIS_LINK_STATE             LastIndicatedLinkState;
    MUX_ADAPTER_BINDING_STATE   BindingState;
    
    ULONG                       OutstandingSends;
    PNDIS_EVENT                 PauseEvent;
    NDIS_SPIN_LOCK              Lock;
#ifndef WIN9X
    //
    // Read/Write lock: allows multiple readers but only a single
    // writer. Used to protect the VELAN list and fields (e.g. packet
    // filter) shared on an ADAPT by multiple VELANs. Code that
    // needs to traverse the VELAN list safely acquires a READ lock.
    // Code that needs to safely modify the VELAN list or shared
    // fields acquires a WRITE lock (which also excludes READers).
    //
    // See macros MUX_ACQUIRE_ADAPT_xxx/MUX_RELEASE_ADAPT_xxx below.
    //
    // TBD - if we want to support this on Win9X, reimplement this!
    //
    NDIS_RW_LOCK                ReadWriteLock;
#endif // WIN9X

    ULONG                       OutstandingRequests;
    PNDIS_EVENT                 CloseEvent;
    ULONG                       Flags;
} ADAPT, *PADAPT;


//
// VELAN object represents a virtual ELAN instance and its
// corresponding virtual miniport adapter.
//
typedef struct _VELAN
{
    // Link into parent adapter's VELAN list.
    LIST_ENTRY                  Link;

    // link inot global VELAN list
    LIST_ENTRY                  GlobalLink;

    // References to this VELAN.
    ULONG                       RefCount;

    // Parent ADAPT.
    PADAPT                      pAdapt;

    // Copy of BindingHandle from ADAPT.
    NDIS_HANDLE                 BindingHandle;

    // Adapter handle for NDIS up-calls related to this virtual miniport.
    NDIS_HANDLE                 MiniportAdapterHandle;

    // Virtual miniport's power state.
    NDIS_DEVICE_POWER_STATE     MPDevicePowerState;

    // Has our Halt entry point been called?
    BOOLEAN                     MiniportHalting;

    // Do we need to indicate receive complete?
    BOOLEAN                     IndicateRcvComplete;

    // Do we need to indicate status complete?
    BOOLEAN                     IndicateStatusComplete;

    // Synchronization fields
    BOOLEAN                     MiniportInitPending;
    NDIS_EVENT                  MiniportInitEvent;

    // Uncompleted Sends/Requests to the adapter below.
    ULONG                       OutstandingSends;

    // Count outstanding indications, including received
    // packets, passed up to protocols on this VELAN.
    ULONG                       OutstandingReceives;

    // A request block that is used to forward a request presented
    // to the virtual miniport, to the lower binding. Since NDIS
    // serializes requests to a miniport, we only need one of these
    // per VELAN.
    //
    MUX_NDIS_REQUEST            Request;        
    // Have we queued a request because the lower binding is
    // at a low power state?
    BOOLEAN                     QueuedRequest;

    // Have we started to deinitialize this VELAN?
    BOOLEAN                     DeInitializing;

    // configuration
    UCHAR                       PermanentAddress[ETH_LENGTH_OF_ADDRESS];
    UCHAR                       CurrentAddress[ETH_LENGTH_OF_ADDRESS];

    NDIS_STRING                 CfgDeviceName;  // used as the unique
                                                // ID for the VELAN
    ULONG                       VElanNumber;    // logical Elan number


    //
    //  ----- Buffer Management: Header buffers and Protocol buffers ----
    //

    // Some standard miniport parameters (OID values).
    ULONG                       PacketFilter;
    ULONG                       LookAhead;
    ULONG64                     LinkSpeed;

    ULONG                       MaxBusySends;
    ULONG                       MaxBusyRecvs;

    // Packet counts
    ULONG64                     GoodTransmits;
    ULONG64                     GoodReceives;
    ULONG                       NumTxSinceLastAdjust;

    // Count of transmit errors
    ULONG                       TxAbortExcessCollisions;
    ULONG                       TxLateCollisions;
    ULONG                       TxDmaUnderrun;
    ULONG                       TxLostCRS;
    ULONG                       TxOKButDeferred;
    ULONG                       OneRetry;
    ULONG                       MoreThanOneRetry;
    ULONG                       TotalRetries;
    ULONG                       TransmitFailuresOther;

    // Count of receive errors
    ULONG                       RcvCrcErrors;
    ULONG                       RcvAlignmentErrors;
    ULONG                       RcvResourceErrors;
    ULONG                       RcvDmaOverrunErrors;
    ULONG                       RcvCdtFrames;
    ULONG                       RcvRuntErrors;
    ULONG                       RegNumTcb;

    // Multicast list
    MUX_MAC_ADDRESS             McastAddrs[VELAN_MAX_MCAST_LIST];
    ULONG                       McastAddrCount;
    

    NDIS_STATUS                 LastIndicatedStatus;
    NDIS_STATUS                 LatestUnIndicateStatus;
    NDIS_SPIN_LOCK              Lock;

    //  Miniport Pause/Restart functionality
    BOOLEAN                     Paused;
    NDIS_SPIN_LOCK              PauseLock;
    NDIS_LINK_STATE             LatestUnIndicateLinkState;
    NDIS_LINK_STATE             LastIndicatedLinkState;

#if IEEE_VLAN_SUPPORT
    ULONG                       VlanId;
    ULONG                       RcvFormatErrors;
    ULONG                       RcvVlanIdErrors;
    BOOLEAN                     RestoreLookaheadSize;
    NPAGED_LOOKASIDE_LIST       TagLookaside;    
#endif

    NET_IFINDEX                 IfIndex;
} VELAN, *PVELAN;


#define MUX_ACQUIRE_SPIN_LOCK(_pLock, DispatchLevel)     \
    {                                                    \
        if (DispatchLevel)                               \
        {                                                \
            NdisDprAcquireSpinLock(_pLock);              \
        }                                                \
        else                                             \
        {                                                \
            NdisAcquireSpinLock(_pLock);                 \
        }                                                \
    }

#define MUX_RELEASE_SPIN_LOCK(_pLock, DispatchLevel)     \
    {                                                    \
        if (DispatchLevel)                               \
        {                                                \
            NdisDprReleaseSpinLock(_pLock);              \
        }                                                \
        else                                             \
        {                                                \
            NdisReleaseSpinLock(_pLock);                 \
        }                                                \
    }



#if IEEE_VLAN_SUPPORT

#define TPID                            0x0081    
//
// Define tag_header structure
//
typedef struct _VLAN_TAG_HEADER
{
    UCHAR       TagInfo[2];    
} VLAN_TAG_HEADER, *PVLAN_TAG_HEADER;


//
// Macro definitions for VLAN support
// 
#define VLAN_TAG_HEADER_SIZE        4 

#define VLANID_DEFAULT              0 
#define VLAN_ID_MAX                 0xfff
#define VLAN_ID_MIN                 0x0

#define USER_PRIORITY_MASK          0xe0
#define CANONICAL_FORMAT_ID_MASK    0x10
#define HIGH_VLAN_ID_MASK           0x0F

//
// Get information for tag headre
// 
#define GET_CANONICAL_FORMAT_ID_FROM_TAG(_pTagHeader)         \
    ((_pTagHeader)->TagInfo[0] & CANONICAL_FORMAT_ID_MASK)

#define GET_USER_PRIORITY_FROM_TAG(_pTagHeader)               \
    ((_pTagHeader)->TagInfo[0] & USER_PRIORITY_MASK)

#define GET_VLAN_ID_FROM_TAG(_pTagHeader)                     \
    (ULONG)(((USHORT)((_pTagHeader)->TagInfo[0] & HIGH_VLAN_ID_MASK) << 8) |(USHORT)((_pTagHeader)->TagInfo[1]))

//
// Clear the tag header struct
// 
#define INITIALIZE_TAG_HEADER_TO_ZERO(_pTagHeader) \
{                                                  \
     (_pTagHeader)->TagInfo[0] = 0;                  \
     (_pTagHeader)->TagInfo[1] = 0;                  \
}
     
//
// Set VLAN information to tag header
// Before we called all the set macro, first we need to initialize pTagHeader  to be 0
//
#define SET_CANONICAL_FORMAT_ID_TO_TAG(_pTagHeader, _CanonicalFormatId)     \
    (_pTagHeader)->TagInfo[0] |= ((UCHAR)(_CanonicalFormatId) << 4)
     
#define SET_USER_PRIORITY_TO_TAG(_pTagHeader, _UserPriority)                \
    (_pTagHeader)->TagInfo[0] |= ((UCHAR)(_UserPriority) << 5)
     
#define SET_VLAN_ID_TO_TAG(_pTagHeader, _VlanId)                            \
    {                                                                       \
        (_pTagHeader)->TagInfo[0] |= (((UCHAR)((_VlanId) >> 8)) & 0x0f);    \
        (_pTagHeader)->TagInfo[1] |= (UCHAR)(_VlanId);                      \
    }


//
// Copy tagging information in the indicated frame to per packet info
// 
#define COPY_TAG_INFO_FROM_HEADER_TO_PACKET_INFO(_Ieee8021qInfo, _pTagHeader)                                   \
{                                                                                                               \
    (_Ieee8021qInfo).TagHeader.UserPriority = ((_pTagHeader->TagInfo[0] & USER_PRIORITY_MASK) >> 5);              \
    (_Ieee8021qInfo).TagHeader.CanonicalFormatId = ((_pTagHeader->TagInfo[0] & CANONICAL_FORMAT_ID_MASK) >> 4);   \
    (_Ieee8021qInfo).TagHeader.VlanId = (((USHORT)(_pTagHeader->TagInfo[0] & HIGH_VLAN_ID_MASK) << 8)| (USHORT)(_pTagHeader->TagInfo[1]));                                                                \
}

//
// Flags used by VELAN supports
//
#define MUX_RETREAT_DATA          0x00000001

//
// Flags used by VELAN on Receive code path
//
#define MUX_ADVANCE_DATA          0x00000001

//
// Every NBL that is indicated up to a protocol needs to advance the buffer
// in case the VLAN tag is present. It should be restored before returning the 
// packet to the miniport. This structure is used for that purpose
//
typedef struct _RECV_NBL_ENTRY
{
    ULONG               Flags;
    VLAN_TAG_HEADER     TagHeader;

    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT)
    UCHAR               Pad[];
} RECV_NBL_ENTRY, *PRECV_NBL_ENTRY;

//
// This structure is used to save context in the NET_BUFFER on the send path,
// if the ethernet header and VLAN tag is allocated by MUX
//
typedef struct _IM_SEND_NB_ENTRY
{
    PMDL            CurrentMdl;
    PMDL            PrevMdl;
    ULONG           CurrentMdlOffset;
    PNET_BUFFER     NextNetBuffer;
} IM_SEND_NB_ENTRY, *PIM_SEND_NB_ENTRY;

#endif //IEEE_VLAN_SUPPORT

typedef struct _IM_NBL_ENTRY 
{
    NDIS_HANDLE  PreviousSourceHandle;
    PVELAN       pVElan;
#if IEEE_VLAN_SUPPORT    
    ULONG        Flags;
    PNET_BUFFER  MdlAllocatedNetBuffers;
#endif
    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT)
    UCHAR        Pad[];
} IM_NBL_ENTRY, *PIM_NBL_ENTRY;

//
// Macro definitions for others.
//

//
// Is a given power state a low-power state?
//
#define MUX_IS_LOW_POWER_STATE(_PwrState)                       \
            ((_PwrState) > NdisDeviceStateD0)

#define MUX_INIT_ADAPT_RW_LOCK(_pAdapt) \
            NdisInitializeReadWriteLock(&(_pAdapt)->ReadWriteLock)


#define MUX_ACQUIRE_ADAPT_READ_LOCK(_pAdapt, _pLockState)       \
            NdisAcquireReadWriteLock(&(_pAdapt)->ReadWriteLock, \
                                     FALSE,                     \
                                     _pLockState)

#define MUX_RELEASE_ADAPT_READ_LOCK(_pAdapt, _pLockState)       \
            NdisReleaseReadWriteLock(&(_pAdapt)->ReadWriteLock, \
                                     _pLockState)

#define MUX_ACQUIRE_ADAPT_WRITE_LOCK(_pAdapt, _pLockState)      \
            NdisAcquireReadWriteLock(&(_pAdapt)->ReadWriteLock, \
                                     TRUE,                      \
                                     _pLockState)

#define MUX_RELEASE_ADAPT_WRITE_LOCK(_pAdapt, _pLockState)      \
            NdisReleaseReadWriteLock(&(_pAdapt)->ReadWriteLock, \
                                     _pLockState)

#define MUX_INCR_PENDING_RECEIVES(_pVElan)                      \
            NdisInterlockedIncrement((PLONG)&pVElan->OutstandingReceives)

#define MUX_DECR_PENDING_RECEIVES(_pVElan)                      \
            NdisInterlockedDecrement((PLONG)&pVElan->OutstandingReceives)

#define MUX_INCR_PENDING_SENDS(_pVElan)                         \
            NdisInterlockedIncrement((PLONG)&pVElan->OutstandingSends)

#define MUX_DECR_PENDING_SENDS(_pVElan)                         \
            NdisInterlockedDecrement((PLONG)&pVElan->OutstandingSends)

#define MUX_DECR_MULTIPLE_PENDING_RECEIVES(_pVElan, _NumReceives) \
            InterlockedExchangeAdd((PLONG)&_pVElan->OutstandingReceives, \
                                    0 - (LONG) _NumReceives)




#define MUX_INCR_STATISTICS(_pUlongVal)                         \
            NdisInterlockedIncrement((PLONG)_pUlongVal)

#define MUX_INCR_STATISTICS64(_pUlong64Val)                     \
{                                                               \
    PLARGE_INTEGER      _pLargeInt = (PLARGE_INTEGER)_pUlong64Val;\
    if (NdisInterlockedIncrement((PLONG)&_pLargeInt->LowPart) == 0)    \
    {                                                           \
        NdisInterlockedIncrement(&_pLargeInt->HighPart);        \
    }                                                           \
}

#define ASSERT_AT_PASSIVE()                                     \
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL)
    
#define ASSERT_AT_DISPATCH()                                     \
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL)



//
// Simple Mutual Exclusion constructs used in preference to
// using KeXXX calls since we don't have Mutex calls in NDIS.
// These can only be called at passive IRQL.
//

typedef struct _MUX_MUTEX
{
    NDIS_MUTEX              Mutex;
    ULONG                   ModuleAndLine;  // useful for debugging

} MUX_MUTEX, *PMUX_MUTEX;

#define MUX_INIT_MUTEX(_pMutex)                                 \
{                                                               \
    NDIS_INIT_MUTEX(&(_pMutex)->Mutex);                         \
    (_pMutex)->ModuleAndLine = 0;                               \
}

#define MUX_ACQUIRE_MUTEX(_pMutex)                              \
{                                                               \
    NDIS_WAIT_FOR_MUTEX(&(_pMutex)->Mutex);                     \
    (_pMutex)->ModuleAndLine = (MODULE_NUMBER << 16) | __LINE__;\
}

#define MUX_RELEASE_MUTEX(_pMutex)                              \
{                                                               \
    (_pMutex)->ModuleAndLine = 0;                               \
    NDIS_RELEASE_MUTEX(&(_pMutex)->Mutex);                      \
}


//
// Global variables
//
extern NDIS_HANDLE           ProtHandle, DriverHandle;
extern NDIS_MEDIUM           MediumArray[1];
extern NDIS_SPIN_LOCK        GlobalLock;
extern MUX_MUTEX             GlobalMutex;
extern LIST_ENTRY            AdapterList;
extern LIST_ENTRY            VElanList;
extern ULONG                 NextVElanNumber;

//
// Module numbers for debugging
//
#define MODULE_MUX          'X'
#define MODULE_PROT         'P'
#define MODULE_MINI         'M'
#define MODULE_MUX_TEST     'T'


#ifdef IEEE_VLAN_SUPPORT

PMDL
MuxAllocateMdl(
    IN OUT PULONG               BufferSize
    );

NDIS_STATUS 
MPHandleSendTaggingNB(
    IN PVELAN           pVElan,
    IN PNET_BUFFER_LIST NetBufferList
    );

VOID 
MPRestoreSendNBL(
    IN PVELAN               pVElan,
    IN PNET_BUFFER_LIST     NetBufferList,
    IN PNET_BUFFER          LastNetBuffer,
    IN PNET_BUFFER          MdlAllocatedNetBuffers
    );

NDIS_STATUS 
PtHandleReceiveTaggingNB(
    IN PVELAN                   pVElan,
    IN PNET_BUFFER_LIST         NetBufferList,
    IN PNDIS_NET_BUFFER_LIST_8021Q_INFO  NdisPacket8021qInfo    
    );

NDIS_STATUS 
PtStripVlanTagNB(
    IN PNET_BUFFER_LIST         NetBufferList,
    OUT PNDIS_NET_BUFFER_LIST_8021Q_INFO NdisPacket8021qInfo,
    OUT PRECV_NBL_ENTRY         RecvContext    
    );

NDIS_STATUS 
PtRestoreReceiveNBL(
    IN PNET_BUFFER_LIST         NetBufferList
    );

#define MuxRecognizedVlanId(_pVElan, _VlanId)        ((_pVElan)->VlanId == (_VlanId))

#endif



#if _MSC_VER >= 1200

#pragma warning(pop)

#else

#endif



