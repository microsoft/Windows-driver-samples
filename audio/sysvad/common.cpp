/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    common.cpp

Abstract:

    Implementation of the AdapterCommon class.

--*/

#pragma warning (disable : 4127)

#include <initguid.h>
#include <sysvad.h>
#include "hw.h"
#include "savedata.h"
#include "IHVPrivatePropertySet.h"
#include "simple.h"

#ifdef SYSVAD_BTH_BYPASS
#include <limits.h>
#include <bthhfpddi.h>
#include <wdmguid.h>    // guild-arrival/removal
#include <devpkey.h>
#include "bthhfpminipairs.h"

// # of sec before sync request is cancelled.
#define BTH_HFP_SYNC_REQ_TIMEOUT_IN_SEC         60
#define BTH_HFP_NOTIFICATION_MAX_ERROR_COUNT    5

#endif // SYSVAD_BTH_BYPASS

//-----------------------------------------------------------------------------
// CSaveData statics
//-----------------------------------------------------------------------------

PSAVEWORKER_PARAM       CSaveData::m_pWorkItems = NULL;
PDEVICE_OBJECT          CSaveData::m_pDeviceObject = NULL;

//=============================================================================
// Classes
//=============================================================================
#ifdef SYSVAD_BTH_BYPASS
class BthHfpDevice;     // Forward declaration.
#endif // SYSVAD_BTH_BYPASS

///////////////////////////////////////////////////////////////////////////////
// CAdapterCommon
//
class CAdapterCommon :
    public IAdapterCommon,
    public IAdapterPowerManagement,
    public CUnknown
{
    private:
        PSERVICEGROUP           m_pServiceGroupWave;
        PDEVICE_OBJECT          m_pDeviceObject;
        PDEVICE_OBJECT          m_pPhysicalDeviceObject;
        WDFDEVICE               m_WdfDevice;            // Wdf device.
        DEVICE_POWER_STATE      m_PowerState;

        PCSYSVADHW              m_pHW;                  // Virtual SYSVAD HW object
        PPORTCLSETWHELPER       m_pPortClsEtwHelper;

        static LONG             m_AdapterInstances;     // # of adapter objects.

        DWORD                   m_dwIdleRequests;

    public:
        //=====================================================================
        // Default CUnknown
        DECLARE_STD_UNKNOWN();
        DEFINE_STD_CONSTRUCTOR(CAdapterCommon);
        ~CAdapterCommon();

        //=====================================================================
        // Default IAdapterPowerManagement
        IMP_IAdapterPowerManagement;

        //=====================================================================
        // IAdapterCommon methods

        STDMETHODIMP_(NTSTATUS) Init
        (
            _In_  PDEVICE_OBJECT  DeviceObject
        );

        STDMETHODIMP_(PDEVICE_OBJECT)   GetDeviceObject(void);

        STDMETHODIMP_(PDEVICE_OBJECT)   GetPhysicalDeviceObject(void);

        STDMETHODIMP_(WDFDEVICE)        GetWdfDevice(void);

        STDMETHODIMP_(void)     SetWaveServiceGroup
        (
            _In_  PSERVICEGROUP   ServiceGroup
        );

        STDMETHODIMP_(BOOL)     bDevSpecificRead();

        STDMETHODIMP_(void)     bDevSpecificWrite
        (
            _In_  BOOL            bDevSpecific
        );
        STDMETHODIMP_(INT)      iDevSpecificRead();

        STDMETHODIMP_(void)     iDevSpecificWrite
        (
            _In_  INT             iDevSpecific
        );
        STDMETHODIMP_(UINT)     uiDevSpecificRead();

        STDMETHODIMP_(void)     uiDevSpecificWrite
        (
            _In_  UINT            uiDevSpecific
        );

        STDMETHODIMP_(BOOL)     MixerMuteRead
        (
            _In_  ULONG           Index,
            _In_  ULONG           Channel
        );

        STDMETHODIMP_(void)     MixerMuteWrite
        (
            _In_  ULONG           Index,
            _In_  ULONG           Channel,
            _In_  BOOL            Value
        );

        STDMETHODIMP_(ULONG)    MixerMuxRead(void);

        STDMETHODIMP_(void)     MixerMuxWrite
        (
            _In_  ULONG           Index
        );

        STDMETHODIMP_(void)     MixerReset(void);

        STDMETHODIMP_(LONG)     MixerVolumeRead
        (
            _In_  ULONG           Index,
            _In_  ULONG           Channel
        );

        STDMETHODIMP_(void)     MixerVolumeWrite
        (
            _In_  ULONG           Index,
            _In_  ULONG           Channel,
            _In_  LONG            Value
        );

        STDMETHODIMP_(LONG)     MixerPeakMeterRead
        (
            _In_  ULONG           Index,
            _In_  ULONG           Channel
        );

        STDMETHODIMP_(NTSTATUS) WriteEtwEvent
        (
            _In_ EPcMiniportEngineEvent    miniportEventType,
            _In_ ULONGLONG      ullData1,
            _In_ ULONGLONG      ullData2,
            _In_ ULONGLONG      ullData3,
            _In_ ULONGLONG      ullData4
        );

        STDMETHODIMP_(VOID)     SetEtwHelper
        (
            PPORTCLSETWHELPER _pPortClsEtwHelper
        );

        STDMETHODIMP_(NTSTATUS) InstallSubdevice
        (
            _In_opt_        PIRP                                        Irp,
            _In_            PWSTR                                       Name,
            _In_            REFGUID                                     PortClassId,
            _In_            REFGUID                                     MiniportClassId,
            _In_opt_        PFNCREATEMINIPORT                           MiniportCreate,
            _In_            ULONG                                       cPropertyCount,
            _In_reads_opt_(cPropertyCount) const SYSVAD_DEVPROPERTY   * pProperties,
            _In_opt_        PVOID                                       DeviceContext,
            _In_            PENDPOINT_MINIPAIR                          MiniportPair,
            _In_opt_        PRESOURCELIST                               ResourceList,
            _In_            REFGUID                                     PortInterfaceId,
            _Out_opt_       PUNKNOWN                                  * OutPortInterface,
            _Out_opt_       PUNKNOWN                                  * OutPortUnknown,
            _Out_opt_       PUNKNOWN                                  * OutMiniportUnknown
        );

        STDMETHODIMP_(NTSTATUS) UnregisterSubdevice
        (
            _In_opt_ PUNKNOWN               UnknownPort
        );

        STDMETHODIMP_(NTSTATUS) ConnectTopologies
        (
            _In_ PUNKNOWN                   UnknownTopology,
            _In_ PUNKNOWN                   UnknownWave,
            _In_ PHYSICALCONNECTIONTABLE*   PhysicalConnections,
            _In_ ULONG                      PhysicalConnectionCount
        );

        STDMETHODIMP_(NTSTATUS) DisconnectTopologies
        (
            _In_ PUNKNOWN                   UnknownTopology,
            _In_ PUNKNOWN                   UnknownWave,
            _In_ PHYSICALCONNECTIONTABLE*   PhysicalConnections,
            _In_ ULONG                      PhysicalConnectionCount
        );

        STDMETHODIMP_(NTSTATUS) InstallEndpointFilters
        (
            _In_opt_    PIRP                Irp,
            _In_        PENDPOINT_MINIPAIR  MiniportPair,
            _In_opt_    PVOID               DeviceContext,
            _Out_opt_   PUNKNOWN *          UnknownTopology,
            _Out_opt_   PUNKNOWN *          UnknownWave,
            _Out_opt_   PUNKNOWN *          UnknownMiniportTopology,
            _Out_opt_   PUNKNOWN *          UnknownMiniportWave
        );

        STDMETHODIMP_(NTSTATUS) RemoveEndpointFilters
        (
            _In_        PENDPOINT_MINIPAIR  MiniportPair,
            _In_opt_    PUNKNOWN            UnknownTopology,
            _In_opt_    PUNKNOWN            UnknownWave
        );

        STDMETHODIMP_(NTSTATUS) GetFilters
        (
            _In_        PENDPOINT_MINIPAIR  MiniportPair,
            _Out_opt_   PUNKNOWN            *UnknownTopologyPort,
            _Out_opt_   PUNKNOWN            *UnknownTopologyMiniport,
            _Out_opt_   PUNKNOWN            *UnknownWavePort,
            _Out_opt_   PUNKNOWN            *UnknownWaveMiniport
        );

        STDMETHODIMP_(NTSTATUS) SetIdlePowerManagement
        (
            _In_        PENDPOINT_MINIPAIR  MiniportPair,
            _In_        BOOL                bEnabled
        );

#ifdef SYSVAD_BTH_BYPASS
        STDMETHODIMP_(NTSTATUS) InitBthScoBypass();

        STDMETHODIMP_(VOID)     CleanupBthScoBypass();
#endif // SYSVAD_BTH_BYPASS

        STDMETHODIMP_(VOID) Cleanup();

        //=====================================================================
        // friends
        friend NTSTATUS         NewAdapterCommon
        (
            _Out_       PUNKNOWN *              Unknown,
            _In_        REFCLSID,
            _In_opt_    PUNKNOWN                UnknownOuter,
            _When_((PoolType & NonPagedPoolMustSucceed) != 0,
                __drv_reportError("Must succeed pool allocations are forbidden. "
                        "Allocation failures cause a system crash"))
            _In_        POOL_TYPE               PoolType
        );

#ifdef SYSVAD_BTH_BYPASS
        //=====================================================================
        // Bluetooth Hands-free Profile SCO Bypass support.

    private:
        PVOID                   m_BthHfpScoNotificationHandle;
        FAST_MUTEX              m_BthHfpFastMutex;              // To serialize access.
        WDFWORKITEM             m_BthHfpWorkItem;               // Async work-item.
        LIST_ENTRY              m_BthHfpWorkTasks;              // Work-item's tasks.
        LIST_ENTRY              m_BthHfpDevices;                // Bth HFP devices.
        NPAGED_LOOKASIDE_LIST   m_BhtHfpWorkTaskPool;           // LookasideList
        size_t                  m_BhtHfpWorkTaskPoolElementSize;
        BOOL                    m_BthHfpEnableCleanup;          // Do cleanup if true.

    private:
        static
        DRIVER_NOTIFICATION_CALLBACK_ROUTINE  EvtBthHfpScoBypassInterfaceChange;

        static
        EVT_WDF_WORKITEM                      EvtBthHfpScoBypassInterfaceWorkItem;

    protected:
        BthHfpDevice * BthHfpDeviceFind
        (
            _In_ PUNICODE_STRING SymbolicLinkName
        );

        NTSTATUS BthHfpScoInterfaceArrival
        (
            _In_ PUNICODE_STRING SymbolicLinkName
        );

        NTSTATUS BthHfpScoInterfaceRemoval
        (
            _In_ PUNICODE_STRING SymbolicLinkName
        );
#endif // SYSVAD_BTH_BYPASS


    private:

    LIST_ENTRY m_SubdeviceCache;

    NTSTATUS GetCachedSubdevice
    (
        _In_ PWSTR Name,
        _Out_opt_ PUNKNOWN *OutUnknownPort,
        _Out_opt_ PUNKNOWN *OutUnknownMiniport
    );

    NTSTATUS CacheSubdevice
    (
        _In_ PWSTR Name,
        _In_ PUNKNOWN UnknownPort,
        _In_ PUNKNOWN UnknownMiniport
    );

    NTSTATUS RemoveCachedSubdevice
    (
        _In_ PWSTR Name
    );

    VOID EmptySubdeviceCache();

    NTSTATUS CreateAudioInterfaceWithProperties
    (
        _In_ PCWSTR                                                 ReferenceString,
        _In_ ULONG                                                  cPropertyCount,
        _In_reads_opt_(cPropertyCount) const SYSVAD_DEVPROPERTY        *pProperties,
        _Out_ _At_(AudioSymbolicLinkName->Buffer, __drv_allocatesMem(Mem)) PUNICODE_STRING AudioSymbolicLinkName
    );
};

typedef struct _MINIPAIR_UNKNOWN
{
    LIST_ENTRY              ListEntry;
    WCHAR                   Name[MAX_PATH];
    PUNKNOWN                PortInterface;
    PUNKNOWN                MiniportInterface;
    PADAPTERPOWERMANAGEMENT PowerInterface;
} MINIPAIR_UNKNOWN;

//
// Used to implement the singleton pattern.
//
LONG  CAdapterCommon::m_AdapterInstances = 0;


#ifdef SYSVAD_BTH_BYPASS

//=====================================================================
//
// CAdapterCommon: Bluetooth Hands-Free Profile SCO Bypass definitions.
//

struct BthHfpWorkItemContext
{
    CAdapterCommon *    Adapter;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME
(
    BthHfpWorkItemContext,
    GetBthHfpWorkItemContext
)

// start/stop the device
enum eBthHfpTaskAction
{
    eBthHfpTaskStart    = 1,
    eBthHfpTaskStop     = 2,
};

struct BthHfpWorkTask
{
    LIST_ENTRY          ListEntry;
    BthHfpDevice      * Device;
    eBthHfpTaskAction   Action;
};


//=====================================================================
//
// Device: Bluetooth Hands-Free Profile SCO Bypass definitions.
//

// BTH HFP device's notification work-item context.
struct BthHfpDeviceNotificationWorkItemContext
{
    BthHfpDevice *  BthHfpDevice;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME
(
    BthHfpDeviceNotificationWorkItemContext,
    GetBthHfpDeviceNotificationWorkItemContext
)

// BTH HFP device's notification request context.
union BthHfpDeviceNotificationBuffer
{
    BOOL        bImmediate;
    LONG        Volume;
    BOOL        BoolStatus;
    NTSTATUS    NtStatus;
};

struct BthHfpDeviceNotificationReqContext
{
    BthHfpDevice                  * BthHfpDevice;
    LONG                            Errors;
    BthHfpDeviceNotificationBuffer  Buffer;
    WDFMEMORY                       MemIn;
    WDFMEMORY                       MemOut;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME
(
    BthHfpDeviceNotificationReqContext,
    GetBthHfpDeviceNotificationReqContext
)

enum eBthHfpState
{
    eBthHfpStateInvalid         = 0,
    eBthHfpStateInitializing    = 1,
    eBthHfpStateRunning         = 2,
    eBthHfpStateStopping        = 3,
    eBthHfpStateStopped         = 4,
    eBthHfpStateFailed          = 5,
};

// To support event notification.
struct BthHfpEventCallback
{
    PFNEVENTNOTIFICATION    Handler;
    PVOID                   Context;
};

//
// This class represents a the Bluetooth Hands-Free Profile SCO Bypass device.
// There is one class for each interface (id = symbolic-link-name).
//
class BthHfpDevice :
    IBthHfpDeviceCommon,
    public CUnknown
{
    private:
        eBthHfpState            m_State;

        CAdapterCommon        * m_Adapter;
        WDFIOTARGET             m_WdfIoTarget;

        LIST_ENTRY              m_ListEntry;
        UNICODE_STRING          m_SymbolicLinkName;

        //
        // The Topo Filter Desc and Topo Pins structures referenced from
        // the Miniports structure are deep copies: they start as copies of the
        // static structures and are modified to allow per-SCO-Device pin
        // categories based on the info obtained from
        // IOCTL_BTHHFP_DEVICE_GET_DESCRIPTOR2
        //
        PENDPOINT_MINIPAIR      m_SpeakerMiniports;
        PENDPOINT_MINIPAIR      m_MicMiniports;

        PUNKNOWN                m_UnknownSpeakerTopology;
        PUNKNOWN                m_UnknownSpeakerWave;
        PUNKNOWN                m_UnknownMicTopology;
        PUNKNOWN                m_UnknownMicWave;

        PBTHHFP_DESCRIPTOR2     m_Descriptor;
        PKSPROPERTY_VALUES      m_VolumePropValues;
        LONG                    m_SpeakerVolumeLevel;
        LONG                    m_MicVolumeLevel;
        union{
          BOOL                  m_ConnectionStatus;
          LONG                  m_ConnectionStatusLong;
        }; // unnamed.

        union{
          NTSTATUS              m_StreamStatus;
          LONG                  m_StreamStatusLong;
        }; // unnamed.

        KEVENT                  m_StreamStatusEvent;

        //
        // Set to TRUE when the HF (remote device) wants to disable the
        // NR + EC of the AG (local system).
        //
        LONG                    m_NRECDisableStatusLong;

        WDFREQUEST              m_StreamReq;
        WDFREQUEST              m_SpeakerVolumeReq;
        WDFREQUEST              m_MicVolumeReq;
        WDFREQUEST              m_ConnectionReq;
        WDFREQUEST              m_NRECDisableStatusReq;
        WDFWORKITEM             m_WorkItem;
        WDFCOLLECTION           m_ReqCollection;
        KSPIN_LOCK              m_Lock;

        LONG                    m_nStreams; // # of open streams.

        BthHfpEventCallback     m_SpeakerVolumeCallback;
        BthHfpEventCallback     m_SpeakerConnectionStatusCallback;
        BthHfpEventCallback     m_MicVolumeCallback;
        BthHfpEventCallback     m_MicConnectionStatusCallback;

    public:
        //=====================================================================
        // Default CUnknown
        DECLARE_STD_UNKNOWN();
        DEFINE_STD_CONSTRUCTOR(BthHfpDevice);
        ~BthHfpDevice();

        NTSTATUS Init
        (
            _In_ CAdapterCommon     * Adapter,
            _In_ PUNICODE_STRING      SymbolicLinkName
        );

    public:
        //=====================================================================
        //
        // Public functions used by CAdapterCommon object.
        //
        VOID Start();
        VOID Stop();

        PLIST_ENTRY GetListEntry()
        {
            return &m_ListEntry;
        }

        PUNICODE_STRING GetSymbolicLinkName()
        {
            return &m_SymbolicLinkName;
        }

        static
        BthHfpDevice *
        GetBthHfpDevice
        (
            _In_ PLIST_ENTRY le
        )
        {
            return CONTAINING_RECORD(le, BthHfpDevice, m_ListEntry);
        }

    public:
        //=====================================================================
        //
        // IBthHfpDeviceCommon functions.
        //
        STDMETHODIMP_(BOOL)                 IsVolumeSupported();

        STDMETHODIMP_(PKSPROPERTY_VALUES)   GetVolumeSettings
        (
            _Out_ PULONG    Size
        );

        STDMETHODIMP_(LONG)                 GetSpeakerVolume();

        STDMETHODIMP_(NTSTATUS)             SetSpeakerVolume
        (
            _In_ ULONG      Volume
        );

        STDMETHODIMP_(LONG)                 GetMicVolume();

        STDMETHODIMP_(NTSTATUS)             SetMicVolume
        (
            _In_ ULONG      Volume
        );

        STDMETHODIMP_(BOOL)                 GetConnectionStatus();

        STDMETHODIMP_(NTSTATUS)             Connect();

        STDMETHODIMP_(NTSTATUS)             Disconnect();

        STDMETHODIMP_(BOOL)                 GetStreamStatus();

        STDMETHODIMP_(NTSTATUS)             StreamOpen();

        STDMETHODIMP_(NTSTATUS)             StreamClose();

        STDMETHODIMP_(GUID)                 GetContainerId();

        STDMETHODIMP_(VOID)                 SetSpeakerVolumeHandler
        (
            _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
            _In_opt_    PVOID                   EventHandlerContext
        );

        STDMETHODIMP_(VOID)                 SetSpeakerConnectionStatusHandler
        (
            _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
            _In_opt_    PVOID                   EventHandlerContext
        );

        STDMETHODIMP_(VOID)                 SetMicVolumeHandler
        (
            _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
            _In_opt_    PVOID                   EventHandlerContext
        );

        STDMETHODIMP_(VOID)                 SetMicConnectionStatusHandler
        (
            _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
            _In_opt_    PVOID                   EventHandlerContext
        );

        STDMETHODIMP_(BOOL)                 IsNRECSupported();

        STDMETHODIMP_(BOOL)                 GetNRECDisableStatus();

    private:
        //=====================================================================
        //
        // Helper functions.
        //
        NTSTATUS    SendIoCtrlSynchronously
        (
            _In_opt_    WDFREQUEST  Request,
            _In_        ULONG       IoControlCode,
            _In_        ULONG       InLength,
            _In_        ULONG       OutLength,
            _When_(InLength > 0 || OutLength > 0, _In_)
            _When_(InLength == 0 && OutLength == 0, _In_opt_)
                        PVOID       Buffer
        );

        NTSTATUS    SendIoCtrlAsynchronously
        (
            _In_        WDFREQUEST      Request,
            _In_        ULONG           IoControlCode,
            _In_opt_    WDFMEMORY       MemIn,
            _In_opt_    WDFMEMORY       MemOut,
            _In_        PFN_WDF_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
            _In_        WDFCONTEXT      Context
        );

        NTSTATUS    GetBthHfpDescriptor
        (
            _Out_ PBTHHFP_DESCRIPTOR2 * Descriptor
        );

        NTSTATUS    EnableBthHfpNrecDisableStatusNotification();

        NTSTATUS    GetBthHfpVolumePropertyValues
        (
            _In_  ULONG                 Length,
            _Out_ PKSPROPERTY_VALUES  * PropValues
        );

        NTSTATUS    SetBthHfpSpeakerVolume
        (
            _In_ LONG  Volume
        );

        NTSTATUS    GetBthHfpSpeakerVolume
        (
            _Out_ LONG  * Volume
        );

        NTSTATUS    EnableBthHfpSpeakerVolumeStatusNotification();

        NTSTATUS    SetBthHfpMicVolume
        (
            _In_ LONG  Volume
        );

        NTSTATUS    GetBthHfpMicVolume
        (
            _Out_ LONG  * Volume
        );

        NTSTATUS    EnableBthHfpMicVolumeStatusNotification();

        NTSTATUS    GetBthHfpConnectionStatus
        (
            _Out_ BOOL * ConnectionStatus
        );

        NTSTATUS    EnableBthHfpConnectionStatusNotification();

        static
        NTSTATUS    CreateCustomEndpointMinipair
        (
            _In_        PENDPOINT_MINIPAIR pBaseMinipair,
            _In_        PUNICODE_STRING FriendlyName,
            _In_        PGUID pCategory,
            _Outptr_    PENDPOINT_MINIPAIR *ppCustomMinipair
        );

        static
        NTSTATUS    UpdateCustomEndpointCategory
        (
            _In_        PPCFILTER_DESCRIPTOR pCustomMinipairTopoFilter,
            _In_        PPCPIN_DESCRIPTOR pCustomMinipairTopoPins,
            _In_        PGUID pCategory
        );

        static
        VOID        DeleteCustomEndpointMinipair
        (
            _In_        PENDPOINT_MINIPAIR CustomMinipair
        );

        NTSTATUS    GetBthHfpCodecId
        (
            _Out_ UCHAR * CodecId
        );

        NTSTATUS    SetBthHfpConnect();

        NTSTATUS    SetBthHfpDisconnect();

        NTSTATUS    SetBthHfpStreamOpen();

        NTSTATUS    SetBthHfpStreamClose();

        NTSTATUS    EnableBthHfpStreamStatusNotification();

        NTSTATUS    StopBthHfpStreamStatusNotification();

        //
        // WDF I/O Target callback.
        //
        static
        EVT_WDF_IO_TARGET_QUERY_REMOVE    EvtBthHfpTargetQueryRemove;

        static
        EVT_WDF_IO_TARGET_REMOVE_CANCELED EvtBthHfpTargetRemoveCanceled;

        static
        EVT_WDF_IO_TARGET_REMOVE_COMPLETE EvtBthHfpTargetRemoveComplete;

        //
        // Status notifications callbacks.
        //
        static
        EVT_WDF_REQUEST_COMPLETION_ROUTINE EvtBthHfpDeviceStreamStatusCompletion;

        static
        EVT_WDF_REQUEST_COMPLETION_ROUTINE EvtBthHfpDeviceNotificationStatusCompletion;

        static
        EVT_WDF_WORKITEM                   EvtBthHfpDeviceNotificationStatusWorkItem;
};
#endif // SYSVAD_BTH_BYPASS

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS SysvadIoSetDeviceInterfacePropertyDataMultiple
(
    _In_ PUNICODE_STRING                                        SymbolicLinkName,
    _In_ ULONG                                                  cPropertyCount,
    _In_reads_opt_(cPropertyCount) const SYSVAD_DEVPROPERTY        *pProperties
)
{
    NTSTATUS ntStatus;

    PAGED_CODE();

    if (pProperties)
    {
        for (ULONG i = 0; i < cPropertyCount; i++)
        {
            ntStatus = IoSetDeviceInterfacePropertyData(
                SymbolicLinkName,
                pProperties[i].PropertyKey,
                LOCALE_NEUTRAL,
                PLUGPLAY_PROPERTY_PERSISTENT,
                pProperties[i].Type,
                pProperties[i].BufferSize,
                pProperties[i].Buffer);

            if (!NT_SUCCESS(ntStatus))
            {
                return ntStatus;
            }
        }
    }

    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
NewAdapterCommon
(
    _Out_       PUNKNOWN *              Unknown,
    _In_        REFCLSID,
    _In_opt_    PUNKNOWN                UnknownOuter,
    _When_((PoolType & NonPagedPoolMustSucceed) != 0,
        __drv_reportError("Must succeed pool allocations are forbidden. "
			    "Allocation failures cause a system crash"))
    _In_        POOL_TYPE               PoolType
)
/*++

Routine Description:

  Creates a new CAdapterCommon

Arguments:

  Unknown -

  UnknownOuter -

  PoolType

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(Unknown);

    NTSTATUS ntStatus;

    //
    // This sample supports only one instance of this object.
    // (b/c of CSaveData's static members and Bluetooth HFP logic).
    //
    if (InterlockedCompareExchange(&CAdapterCommon::m_AdapterInstances, 1, 0) != 0)
    {
        ntStatus = STATUS_DEVICE_BUSY;
        DPF(D_ERROR, ("NewAdapterCommon failed, only one instance is allowed"));
        goto Done;
    }

    //
    // Allocate an adapter object.
    //
    CAdapterCommon *p = new(PoolType, MINADAPTER_POOLTAG) CAdapterCommon(UnknownOuter);
    if (p == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        DPF(D_ERROR, ("NewAdapterCommon failed, 0x%x", ntStatus));
        goto Done;
    }

    //
    // Success.
    //
    *Unknown = PUNKNOWN((PADAPTERCOMMON)(p));
    (*Unknown)->AddRef();
    ntStatus = STATUS_SUCCESS;

Done:
    return ntStatus;
} // NewAdapterCommon

//=============================================================================
#pragma code_seg("PAGE")
CAdapterCommon::~CAdapterCommon
(
    void
)
/*++

Routine Description:

  Destructor for CAdapterCommon.

Arguments:

Return Value:

  void

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::~CAdapterCommon]"));

    if (m_pHW)
    {
        delete m_pHW;
        m_pHW = NULL;
    }

    CSaveData::DestroyWorkItems();

    SAFE_RELEASE(m_pPortClsEtwHelper);
    SAFE_RELEASE(m_pServiceGroupWave);

    if (m_WdfDevice)
    {
        WdfObjectDelete(m_WdfDevice);
        m_WdfDevice = NULL;
    }

    InterlockedDecrement(&CAdapterCommon::m_AdapterInstances);
    ASSERT(CAdapterCommon::m_AdapterInstances == 0);
} // ~CAdapterCommon

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(PDEVICE_OBJECT)
CAdapterCommon::GetDeviceObject
(
    void
)
/*++

Routine Description:

  Returns the deviceobject

Arguments:

Return Value:

  PDEVICE_OBJECT

--*/
{
    PAGED_CODE();

    return m_pDeviceObject;
} // GetDeviceObject

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(PDEVICE_OBJECT)
CAdapterCommon::GetPhysicalDeviceObject
(
    void
)
/*++

Routine Description:

  Returns the PDO.

Arguments:

Return Value:

  PDEVICE_OBJECT

--*/
{
    PAGED_CODE();

    return m_pPhysicalDeviceObject;
} // GetPhysicalDeviceObject

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(WDFDEVICE)
CAdapterCommon::GetWdfDevice
(
    void
)
/*++

Routine Description:

  Returns the associated WDF miniport device. Note that this is NOT an audio
  miniport. The WDF miniport device is the WDF device associated with the
  adapter.

Arguments:

Return Value:

  WDFDEVICE

--*/
{
    PAGED_CODE();

    return m_WdfDevice;
} // GetWdfDevice

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CAdapterCommon::Init
(
    _In_  PDEVICE_OBJECT          DeviceObject
)
/*++

Routine Description:

    Initialize adapter common object.

Arguments:

    DeviceObject - pointer to the device object

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::Init]"));

    ASSERT(DeviceObject);

    NTSTATUS        ntStatus    = STATUS_SUCCESS;

#ifdef SYSVAD_BTH_BYPASS
    m_BthHfpEnableCleanup = FALSE;
#endif // SYSVAD_BTH_BYPASS



    m_pServiceGroupWave     = NULL;
    m_pDeviceObject         = DeviceObject;
    m_pPhysicalDeviceObject = NULL;
    m_WdfDevice             = NULL;
    m_PowerState            = PowerDeviceD0;
    m_pHW                   = NULL;
    m_pPortClsEtwHelper     = NULL;

    InitializeListHead(&m_SubdeviceCache);

    //
    // Get the PDO.
    //
    ntStatus = PcGetPhysicalDeviceObject(DeviceObject, &m_pPhysicalDeviceObject);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("PcGetPhysicalDeviceObject failed, 0x%x", ntStatus)),
        Done);

    //
    // Create a WDF miniport to represent the adapter. Note that WDF miniports
    // are NOT audio miniports. An audio adapter is associated with a single WDF
    // miniport. This driver uses WDF to simplify the handling of the Bluetooth
    // SCO HFP Bypass interface.
    //
    ntStatus = WdfDeviceMiniportCreate( WdfGetDriver(),
                                        WDF_NO_OBJECT_ATTRIBUTES,
                                        DeviceObject,           // FDO
                                        NULL,                   // Next device.
                                        NULL,                   // PDO
                                       &m_WdfDevice);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("WdfDeviceMiniportCreate failed, 0x%x", ntStatus)),
        Done);

    // Initialize HW.
    //
    m_pHW = new (NonPagedPoolNx, SYSVAD_POOLTAG)  CSYSVADHW;
    if (!m_pHW)
    {
        DPF(D_TERSE, ("Insufficient memory for SYSVAD HW"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    IF_FAILED_JUMP(ntStatus, Done);

    m_pHW->MixerReset();

    //
    // Initialize SaveData class.
    //
    CSaveData::SetDeviceObject(DeviceObject);   //device object is needed by CSaveData
    ntStatus = CSaveData::InitializeWorkItems(DeviceObject);
    IF_FAILED_JUMP(ntStatus, Done);

Done:

    return ntStatus;
} // Init

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(void)
CAdapterCommon::MixerReset
(
    void
)
/*++

Routine Description:

  Reset mixer registers from registry.

Arguments:

Return Value:

  void

--*/
{
    PAGED_CODE();

    if (m_pHW)
    {
        m_pHW->MixerReset();
    }
} // MixerReset

//=============================================================================
/* Here are the definitions of the standard miniport events.

Event type	: eMINIPORT_IHV_DEFINED
Parameter 1	: Defined and used by IHVs
Parameter 2	: Defined and used by IHVs
Parameter 3	:Defined and used by IHVs
Parameter 4 :Defined and used by IHVs

Event type: eMINIPORT_BUFFER_COMPLETE
Parameter 1: Current linear buffer position
Parameter 2: the previous WaveRtBufferWritePosition that the drive received
Parameter 3: Data length completed
Parameter 4:0

Event type: eMINIPORT_PIN_STATE
Parameter 1: Current linear buffer position
Parameter 2: the previous WaveRtBufferWritePosition that the drive received
Parameter 3: Pin State 0->KS_STOP, 1->KS_ACQUIRE, 2->KS_PAUSE, 3->KS_RUN
Parameter 4:0

Event type: eMINIPORT_GET_STREAM_POS
Parameter 1: Current linear buffer position
Parameter 2: the previous WaveRtBufferWritePosition that the drive received
Parameter 3: 0
Parameter 4:0


Event type: eMINIPORT_SET_WAVERT_BUFFER_WRITE_POS
Parameter 1: Current linear buffer position
Parameter 2: the previous WaveRtBufferWritePosition that the drive received
Parameter 3: the arget WaveRtBufferWritePosition received from portcls
Parameter 4:0

Event type: eMINIPORT_GET_PRESENTATION_POS
Parameter 1: Current linear buffer position
Parameter 2: the previous WaveRtBufferWritePosition that the drive received
Parameter 3: Presentation position
Parameter 4:0

Event type: eMINIPORT_PROGRAM_DMA
Parameter 1: Current linear buffer position
Parameter 2: the previous WaveRtBufferWritePosition that the drive received
Parameter 3: Starting  WaveRt buffer offset
Parameter 4: Data length

Event type: eMINIPORT_GLITCH_REPORT
Parameter 1: Current linear buffer position
Parameter 2: the previous WaveRtBufferWritePosition that the drive received
Parameter 3: major glitch code: 1:WaveRT buffer is underrun,
                                2:decoder errors,
                                3:receive the same wavert buffer two in a row in event driven mode
Parameter 4: minor code for the glitch cause

Event type: eMINIPORT_LAST_BUFFER_RENDERED
Parameter 1: Current linear buffer position
Parameter 2: the very last WaveRtBufferWritePosition that the driver received
Parameter 3: 0
Parameter 4: 0

*/
#pragma code_seg()
STDMETHODIMP
CAdapterCommon::WriteEtwEvent
(
    _In_ EPcMiniportEngineEvent    miniportEventType,
    _In_ ULONGLONG  ullData1,
    _In_ ULONGLONG  ullData2,
    _In_ ULONGLONG  ullData3,
    _In_ ULONGLONG  ullData4
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (m_pPortClsEtwHelper)
    {
        ntStatus = m_pPortClsEtwHelper->MiniportWriteEtwEvent( miniportEventType, ullData1, ullData2, ullData3, ullData4) ;
    }
    return ntStatus;
} // WriteEtwEvent

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(void)
CAdapterCommon::SetEtwHelper
(
    PPORTCLSETWHELPER _pPortClsEtwHelper
)
{
    PAGED_CODE();

    SAFE_RELEASE(m_pPortClsEtwHelper);

    m_pPortClsEtwHelper = _pPortClsEtwHelper;

    if (m_pPortClsEtwHelper)
    {
        m_pPortClsEtwHelper->AddRef();
    }
} // SetEtwHelper

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP
CAdapterCommon::NonDelegatingQueryInterface
(
    _In_ REFIID                      Interface,
    _COM_Outptr_ PVOID *        Object
)
/*++

Routine Description:

  QueryInterface routine for AdapterCommon

Arguments:

  Interface -

  Object -

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PADAPTERCOMMON(this)));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IAdapterCommon))
    {
        *Object = PVOID(PADAPTERCOMMON(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IAdapterPowerManagement))
    {
        *Object = PVOID(PADAPTERPOWERMANAGEMENT(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
} // NonDelegatingQueryInterface

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(void)
CAdapterCommon::SetWaveServiceGroup
(
    _In_ PSERVICEGROUP            ServiceGroup
)
/*++

Routine Description:


Arguments:

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::SetWaveServiceGroup]"));

    SAFE_RELEASE(m_pServiceGroupWave);

    m_pServiceGroupWave = ServiceGroup;

    if (m_pServiceGroupWave)
    {
        m_pServiceGroupWave->AddRef();
    }
} // SetWaveServiceGroup

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(BOOL)
CAdapterCommon::bDevSpecificRead()
/*++

Routine Description:

  Fetch Device Specific information.

Arguments:

  N/A

Return Value:

    BOOL - Device Specific info

--*/
{
    if (m_pHW)
    {
        return m_pHW->bGetDevSpecific();
    }

    return FALSE;
} // bDevSpecificRead

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(void)
CAdapterCommon::bDevSpecificWrite
(
    _In_  BOOL                    bDevSpecific
)
/*++

Routine Description:

  Store the new value in the Device Specific location.

Arguments:

  bDevSpecific - Value to store

Return Value:

  N/A.

--*/
{
    if (m_pHW)
    {
        m_pHW->bSetDevSpecific(bDevSpecific);
    }
} // DevSpecificWrite

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(INT)
CAdapterCommon::iDevSpecificRead()
/*++

Routine Description:

  Fetch Device Specific information.

Arguments:

  N/A

Return Value:

    INT - Device Specific info

--*/
{
    if (m_pHW)
    {
        return m_pHW->iGetDevSpecific();
    }

    return 0;
} // iDevSpecificRead

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(void)
CAdapterCommon::iDevSpecificWrite
(
    _In_  INT                    iDevSpecific
)
/*++

Routine Description:

  Store the new value in the Device Specific location.

Arguments:

  iDevSpecific - Value to store

Return Value:

  N/A.

--*/
{
    if (m_pHW)
    {
        m_pHW->iSetDevSpecific(iDevSpecific);
    }
} // iDevSpecificWrite

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(UINT)
CAdapterCommon::uiDevSpecificRead()
/*++

Routine Description:

  Fetch Device Specific information.

Arguments:

  N/A

Return Value:

    UINT - Device Specific info

--*/
{
    if (m_pHW)
    {
        return m_pHW->uiGetDevSpecific();
    }

    return 0;
} // uiDevSpecificRead

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(void)
CAdapterCommon::uiDevSpecificWrite
(
    _In_  UINT                    uiDevSpecific
)
/*++

Routine Description:

  Store the new value in the Device Specific location.

Arguments:

  uiDevSpecific - Value to store

Return Value:

  N/A.

--*/
{
    if (m_pHW)
    {
        m_pHW->uiSetDevSpecific(uiDevSpecific);
    }
} // uiDevSpecificWrite

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(BOOL)
CAdapterCommon::MixerMuteRead
(
    _In_  ULONG               Index,
    _In_  ULONG               Channel
)
/*++

Routine Description:

  Store the new value in mixer register array.

Arguments:

  Index - node id

Return Value:

    BOOL - mixer mute setting for this node

--*/
{
    if (m_pHW)
    {
        return m_pHW->GetMixerMute(Index, Channel);
    }

    return 0;
} // MixerMuteRead

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(void)
CAdapterCommon::MixerMuteWrite
(
    _In_  ULONG                   Index,
    _In_  ULONG                   Channel,
    _In_  BOOL                    Value
)
/*++

Routine Description:

  Store the new value in mixer register array.

Arguments:

  Index - node id

  Value - new mute settings

Return Value:

  NT status code.

--*/
{
    if (m_pHW)
    {
        m_pHW->SetMixerMute(Index, Channel, Value);
    }
} // MixerMuteWrite

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(ULONG)
CAdapterCommon::MixerMuxRead()
/*++

Routine Description:

  Return the mux selection

Arguments:

  Index - node id

  Value - new mute settings

Return Value:

  NT status code.

--*/
{
    if (m_pHW)
    {
        return m_pHW->GetMixerMux();
    }

    return 0;
} // MixerMuxRead

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(void)
CAdapterCommon::MixerMuxWrite
(
    _In_  ULONG                   Index
)
/*++

Routine Description:

  Store the new mux selection

Arguments:

  Index - node id

  Value - new mute settings

Return Value:

  NT status code.

--*/
{
    if (m_pHW)
    {
        m_pHW->SetMixerMux(Index);
    }
} // MixerMuxWrite

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(LONG)
CAdapterCommon::MixerVolumeRead
(
    _In_  ULONG                   Index,
    _In_  ULONG                   Channel
)
/*++

Routine Description:

  Return the value in mixer register array.

Arguments:

  Index - node id

  Channel = which channel

Return Value:

    Byte - mixer volume settings for this line

--*/
{
    if (m_pHW)
    {
        return m_pHW->GetMixerVolume(Index, Channel);
    }

    return 0;
} // MixerVolumeRead

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(void)
CAdapterCommon::MixerVolumeWrite
(
    _In_  ULONG                   Index,
    _In_  ULONG                   Channel,
    _In_  LONG                    Value
)
/*++

Routine Description:

  Store the new value in mixer register array.

Arguments:

  Index - node id

  Channel - which channel

  Value - new volume level

Return Value:

    void

--*/
{
    if (m_pHW)
    {
        m_pHW->SetMixerVolume(Index, Channel, Value);
    }
} // MixerVolumeWrite

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(LONG)
CAdapterCommon::MixerPeakMeterRead
(
    _In_  ULONG                   Index,
    _In_  ULONG                   Channel
)
/*++

Routine Description:

  Return the value in mixer register array.

Arguments:

  Index - node id

  Channel = which channel

Return Value:

    Byte - mixer sample peak meter settings for this line

--*/
{
    if (m_pHW)
    {
        return m_pHW->GetMixerPeakMeter(Index, Channel);
    }

    return 0;
} // MixerVolumeRead

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(void)
CAdapterCommon::PowerChangeState
(
    _In_  POWER_STATE             NewState
)
/*++

Routine Description:


Arguments:

  NewState - The requested, new power state for the device.

Return Value:

    void

Note:
  From MSDN:

  To assist the driver, PortCls will pause any active audio streams prior to calling
  this method to place the device in a sleep state. After calling this method, PortCls
  will unpause active audio streams, to wake the device up. Miniports can opt for
  additional notification by utilizing the IPowerNotify interface.

  The miniport driver must perform the requested change to the device's power state
  before it returns from the PowerChangeState call. If the miniport driver needs to
  save or restore any device state before a power-state change, the miniport driver
  should support the IPowerNotify interface, which allows it to receive advance warning
  of any such change. Before returning from a successful PowerChangeState call, the
  miniport driver should cache the new power state.

  While the miniport driver is in one of the sleep states (any state other than
  PowerDeviceD0), it must avoid writing to the hardware. The miniport driver must cache
  any hardware accesses that need to be deferred until the device powers up again. If
  the power state is changing from one of the sleep states to PowerDeviceD0, the
  miniport driver should perform any deferred hardware accesses after it has powered up
  the device. If the power state is changing from PowerDeviceD0 to a sleep state, the
  miniport driver can perform any necessary hardware accesses during the PowerChangeState
  call before it powers down the device.

  While powered down, a miniport driver is never asked to create a miniport driver object
  or stream object. PortCls always places the device in the PowerDeviceD0 state before
  calling the miniport driver's NewStream method.

--*/
{
    DPF_ENTER(("[CAdapterCommon::PowerChangeState]"));

    // Notify all registered miniports of a power state change
    PLIST_ENTRY le = NULL;
    for (le = m_SubdeviceCache.Flink; le != &m_SubdeviceCache; le = le->Flink)
    {
        MINIPAIR_UNKNOWN *pRecord = CONTAINING_RECORD(le, MINIPAIR_UNKNOWN, ListEntry);

        if (pRecord->PowerInterface)
        {
            pRecord->PowerInterface->PowerChangeState(NewState);
        }
    }

    // is this actually a state change??
    //
    if (NewState.DeviceState != m_PowerState)
    {
        // switch on new state
        //
        switch (NewState.DeviceState)
        {
            case PowerDeviceD0:
            case PowerDeviceD1:
            case PowerDeviceD2:
            case PowerDeviceD3:
                m_PowerState = NewState.DeviceState;

                DPF
                (
                    D_VERBOSE,
                    ("Entering D%u", ULONG(m_PowerState) - ULONG(PowerDeviceD0))
                );

                break;

            default:

                DPF(D_VERBOSE, ("Unknown Device Power State"));
                break;
        }
    }
} // PowerStateChange

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::QueryDeviceCapabilities
(
    _Inout_updates_bytes_(sizeof(DEVICE_CAPABILITIES)) PDEVICE_CAPABILITIES    PowerDeviceCaps
)
/*++

Routine Description:

    Called at startup to get the caps for the device.  This structure provides
    the system with the mappings between system power state and device power
    state.  This typically will not need modification by the driver.

Arguments:

  PowerDeviceCaps - The device's capabilities.

Return Value:

  NT status code.

--*/
{
    UNREFERENCED_PARAMETER(PowerDeviceCaps);

    DPF_ENTER(("[CAdapterCommon::QueryDeviceCapabilities]"));

    return (STATUS_SUCCESS);
} // QueryDeviceCapabilities

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::QueryPowerChangeState
(
    _In_  POWER_STATE             NewStateQuery
)
/*++

Routine Description:

  Query to see if the device can change to this power state

Arguments:

  NewStateQuery - The requested, new power state for the device

Return Value:

  NT status code.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    DPF_ENTER(("[CAdapterCommon::QueryPowerChangeState]"));

    // query each miniport for it's power state, we're finished if even one indicates
    // it cannot go to this power state.
    PLIST_ENTRY le = NULL;
    for (le = m_SubdeviceCache.Flink; le != &m_SubdeviceCache && NT_SUCCESS(status); le = le->Flink)
    {
        MINIPAIR_UNKNOWN *pRecord = CONTAINING_RECORD(le, MINIPAIR_UNKNOWN, ListEntry);

        if (pRecord->PowerInterface)
        {
            status = pRecord->PowerInterface->QueryPowerChangeState(NewStateQuery);
        }
    }

    return status;
} // QueryPowerChangeState

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CAdapterCommon::CreateAudioInterfaceWithProperties
(
    _In_ PCWSTR ReferenceString,
    _In_ ULONG cPropertyCount,
    _In_reads_opt_(cPropertyCount) const SYSVAD_DEVPROPERTY *pProperties,
    _Out_ _At_(AudioSymbolicLinkName->Buffer, __drv_allocatesMem(Mem)) PUNICODE_STRING AudioSymbolicLinkName
)
/*++

Routine Description:

Create the audio interface (in disabled mode).

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::CreateAudioInterfaceWithProperties]"));

    NTSTATUS        ntStatus;
    UNICODE_STRING  referenceString;

    RtlInitUnicodeString(&referenceString, ReferenceString);

    //
    // Reset output value.
    //
    RtlZeroMemory(AudioSymbolicLinkName, sizeof(UNICODE_STRING));

    //
    // Register an audio interface if not already present.
    //
    ntStatus = IoRegisterDeviceInterface(
        GetPhysicalDeviceObject(),
        &KSCATEGORY_AUDIO,
        &referenceString,
        AudioSymbolicLinkName);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("CreateAudioInterfaceWithProperties: IoRegisterDeviceInterface(KSCATEGORY_AUDIO): failed, 0x%x", ntStatus)),
        Done);

    //
    // Set properties on the interface
    //
    ntStatus = SysvadIoSetDeviceInterfacePropertyDataMultiple(AudioSymbolicLinkName, cPropertyCount, pProperties);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("CreateAudioInterfaceWithProperties: SysvadIoSetDeviceInterfacePropertyDataMultiple(...): failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:
    if (!NT_SUCCESS(ntStatus))
    {
        RtlFreeUnicodeString(AudioSymbolicLinkName);
        RtlZeroMemory(AudioSymbolicLinkName, sizeof(UNICODE_STRING));
    }
    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::InstallSubdevice
(
    _In_opt_        PIRP                                    Irp,
    _In_            PWSTR                                   Name,
    _In_            REFGUID                                 PortClassId,
    _In_            REFGUID                                 MiniportClassId,
    _In_opt_        PFNCREATEMINIPORT                       MiniportCreate,
    _In_            ULONG                                   cPropertyCount,
    _In_reads_opt_(cPropertyCount) const SYSVAD_DEVPROPERTY * pProperties,
    _In_opt_        PVOID                                   DeviceContext,
    _In_            PENDPOINT_MINIPAIR                      MiniportPair,
    _In_opt_        PRESOURCELIST                           ResourceList,
    _In_            REFGUID                                 PortInterfaceId,
    _Out_opt_       PUNKNOWN                              * OutPortInterface,
    _Out_opt_       PUNKNOWN                              * OutPortUnknown,
    _Out_opt_       PUNKNOWN                              * OutMiniportUnknown
)
{
/*++

Routine Description:

    This function creates and registers a subdevice consisting of a port
    driver, a minport driver and a set of resources bound together.  It will
    also optionally place a pointer to an interface on the port driver in a
    specified location before initializing the port driver.  This is done so
    that a common ISR can have access to the port driver during
    initialization, when the ISR might fire.

Arguments:

    Irp - pointer to the irp object.

    Name - name of the miniport. Passes to PcRegisterSubDevice

    PortClassId - port class id. Passed to PcNewPort.

    MiniportClassId - miniport class id. Passed to PcNewMiniport.

    MiniportCreate - pointer to a miniport creation function. If NULL,
                     PcNewMiniport is used.

    DeviceContext - deviceType specific.

    MiniportPair - endpoint configuration info.

    ResourceList - pointer to the resource list.

    PortInterfaceId - GUID that represents the port interface.

    OutPortInterface - pointer to store the port interface

    OutPortUnknown - pointer to store the unknown port interface.

    OutMiniportUnknown - pointer to store the unknown miniport interface

Return Value:

    NT status code.

--*/
    PAGED_CODE();
    DPF_ENTER(("[InstallSubDevice %S]", Name));

    ASSERT(Name != NULL);
    ASSERT(m_pDeviceObject != NULL);

    NTSTATUS                    ntStatus;
    PPORT                       port            = NULL;
    PUNKNOWN                    miniport        = NULL;
    PADAPTERCOMMON              adapterCommon   = NULL;
    UNICODE_STRING              symbolicLink    = { 0 };

    adapterCommon = PADAPTERCOMMON(this);

    ntStatus = CreateAudioInterfaceWithProperties(Name, cPropertyCount, pProperties, &symbolicLink);
    if (NT_SUCCESS(ntStatus))
    {
        // Currently have no use for the symbolic link
        RtlFreeUnicodeString(&symbolicLink);

        // Create the port driver object
        //
        ntStatus = PcNewPort(&port, PortClassId);
    }

    // Create the miniport object
    //
    if (NT_SUCCESS(ntStatus))
    {
        if (MiniportCreate)
        {
            ntStatus =
                MiniportCreate
                (
                    &miniport,
                    MiniportClassId,
                    NULL,
                    NonPagedPoolNx,
                    adapterCommon,
                    DeviceContext,
                    MiniportPair
                );
        }
        else
        {
            ntStatus =
                PcNewMiniport
                (
                    (PMINIPORT *) &miniport,
                    MiniportClassId
                );
        }
    }

    // Init the port driver and miniport in one go.
    //
    if (NT_SUCCESS(ntStatus))
    {
#pragma warning(push)
        // IPort::Init's annotation on ResourceList requires it to be non-NULL.  However,
        // for dynamic devices, we may no longer have the resource list and this should
        // still succeed.
        //
#pragma warning(disable:6387)
        ntStatus =
            port->Init
            (
                m_pDeviceObject,
                Irp,
                miniport,
                adapterCommon,
                ResourceList
            );
#pragma warning (pop)

        if (NT_SUCCESS(ntStatus))
        {
            // Register the subdevice (port/miniport combination).
            //
            ntStatus =
                PcRegisterSubdevice
                (
                    m_pDeviceObject,
                    Name,
                    port
                );
        }
    }

    // Deposit the port interfaces if it's needed.
    //
    if (NT_SUCCESS(ntStatus))
    {
        if (OutPortUnknown)
        {
            ntStatus =
                port->QueryInterface
                (
                    IID_IUnknown,
                    (PVOID *)OutPortUnknown
                );
        }

        if (OutPortInterface)
        {
            ntStatus =
                port->QueryInterface
                (
                    PortInterfaceId,
                    (PVOID *) OutPortInterface
                );
        }

        if (OutMiniportUnknown)
        {
            ntStatus =
                miniport->QueryInterface
                (
                    IID_IUnknown,
                    (PVOID *)OutMiniportUnknown
                );
        }

    }

    if (port)
    {
        port->Release();
    }

    if (miniport)
    {
        miniport->Release();
    }

    return ntStatus;
} // InstallSubDevice

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::UnregisterSubdevice
(
    _In_opt_   PUNKNOWN     UnknownPort
)
/*++

Routine Description:

  Unregisters and releases the specified subdevice.

Arguments:

  UnknownPort - Wave or topology port interface.

Return Value:

  NTSTATUS

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::UnregisterSubdevice]"));

    ASSERT(m_pDeviceObject != NULL);

    NTSTATUS                ntStatus            = STATUS_SUCCESS;
    PUNREGISTERSUBDEVICE    unregisterSubdevice = NULL;

    if (NULL == UnknownPort)
    {
        return ntStatus;
    }

    //
    // Get the IUnregisterSubdevice interface.
    //
    ntStatus = UnknownPort->QueryInterface(
        IID_IUnregisterSubdevice,
        (PVOID *)&unregisterSubdevice);

    //
    // Unregister the port object.
    //
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = unregisterSubdevice->UnregisterSubdevice(
            m_pDeviceObject,
            UnknownPort);

        //
        // Release the IUnregisterSubdevice interface.
        //
        unregisterSubdevice->Release();
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::ConnectTopologies
(
    _In_ PUNKNOWN                   UnknownTopology,
    _In_ PUNKNOWN                   UnknownWave,
    _In_ PHYSICALCONNECTIONTABLE*   PhysicalConnections,
    _In_ ULONG                      PhysicalConnectionCount
)
/*++

Routine Description:

  Connects the bridge pins between the wave and mixer topologies.

Arguments:

Return Value:

  NTSTATUS

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::ConnectTopologies]"));

    ASSERT(m_pDeviceObject != NULL);

    NTSTATUS        ntStatus            = STATUS_SUCCESS;

    //
    // register wave <=> topology connections
    // This will connect bridge pins of wave and topology
    // miniports.
    //
    for (ULONG i = 0; i < PhysicalConnectionCount && NT_SUCCESS(ntStatus); i++)
    {

        switch(PhysicalConnections[i].eType)
        {
            case CONNECTIONTYPE_TOPOLOGY_OUTPUT:
                ntStatus =
                    PcRegisterPhysicalConnection
                    (
                        m_pDeviceObject,
                        UnknownTopology,
                        PhysicalConnections[i].ulTopology,
                        UnknownWave,
                        PhysicalConnections[i].ulWave
                    );
                if (!NT_SUCCESS(ntStatus))
                {
                    DPF(D_TERSE, ("ConnectTopologies: PcRegisterPhysicalConnection(render) failed, 0x%x", ntStatus));
                }
                break;
            case CONNECTIONTYPE_WAVE_OUTPUT:
                ntStatus =
                    PcRegisterPhysicalConnection
                    (
                        m_pDeviceObject,
                        UnknownWave,
                        PhysicalConnections[i].ulWave,
                        UnknownTopology,
                        PhysicalConnections[i].ulTopology
                    );
                if (!NT_SUCCESS(ntStatus))
                {
                    DPF(D_TERSE, ("ConnectTopologies: PcRegisterPhysicalConnection(capture) failed, 0x%x", ntStatus));
                }
                break;
        }
    }

    //
    // Cleanup in case of error.
    //
    if (!NT_SUCCESS(ntStatus))
    {
        // disconnect all connections on error, ignore error code because not all
        // connections may have been made
        DisconnectTopologies(UnknownTopology, UnknownWave, PhysicalConnections, PhysicalConnectionCount);
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::DisconnectTopologies
(
    _In_ PUNKNOWN                   UnknownTopology,
    _In_ PUNKNOWN                   UnknownWave,
    _In_ PHYSICALCONNECTIONTABLE*   PhysicalConnections,
    _In_ ULONG                      PhysicalConnectionCount
)
/*++

Routine Description:

  Disconnects the bridge pins between the wave and mixer topologies.

Arguments:

Return Value:

  NTSTATUS

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::DisconnectTopologies]"));

    ASSERT(m_pDeviceObject != NULL);

    NTSTATUS                        ntStatus                        = STATUS_SUCCESS;
    NTSTATUS                        ntStatus2                       = STATUS_SUCCESS;
    PUNREGISTERPHYSICALCONNECTION   unregisterPhysicalConnection    = NULL;

    //
    // Get the IUnregisterPhysicalConnection interface
    //
    ntStatus = UnknownTopology->QueryInterface(
        IID_IUnregisterPhysicalConnection,
        (PVOID *)&unregisterPhysicalConnection);

    if (NT_SUCCESS(ntStatus))
    {
        for (ULONG i = 0; i < PhysicalConnectionCount; i++)
        {
            switch(PhysicalConnections[i].eType)
            {
                case CONNECTIONTYPE_TOPOLOGY_OUTPUT:
                    ntStatus =
                        unregisterPhysicalConnection->UnregisterPhysicalConnection(
                            m_pDeviceObject,
                            UnknownTopology,
                            PhysicalConnections[i].ulTopology,
                            UnknownWave,
                            PhysicalConnections[i].ulWave
                        );

                    if (!NT_SUCCESS(ntStatus))
                    {
                        DPF(D_TERSE, ("DisconnectTopologies: UnregisterPhysicalConnection(render) failed, 0x%x", ntStatus));
                    }
                    break;
                case CONNECTIONTYPE_WAVE_OUTPUT:
                    ntStatus =
                        unregisterPhysicalConnection->UnregisterPhysicalConnection(
                            m_pDeviceObject,
                            UnknownWave,
                            PhysicalConnections[i].ulWave,
                            UnknownTopology,
                            PhysicalConnections[i].ulTopology
                        );
                    if (!NT_SUCCESS(ntStatus2))
                    {
                        DPF(D_TERSE, ("DisconnectTopologies: UnregisterPhysicalConnection(capture) failed, 0x%x", ntStatus2));
                    }
                    break;
            }

            // cache and return the first error encountered, as it's likely the most relevent
            if (NT_SUCCESS(ntStatus))
            {
                ntStatus = ntStatus2;
            }
        }
    }

    //
    // Release the IUnregisterPhysicalConnection interface.
    //
    SAFE_RELEASE(unregisterPhysicalConnection);

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CAdapterCommon::GetCachedSubdevice
(
    _In_ PWSTR Name,
    _Out_opt_ PUNKNOWN *OutUnknownPort,
    _Out_opt_ PUNKNOWN *OutUnknownMiniport
)
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::GetCachedSubdevice]"));

    // search list, return interface to device if found, fail if not found
    PLIST_ENTRY le = NULL;
    BOOL bFound = FALSE;

    for (le = m_SubdeviceCache.Flink; le != &m_SubdeviceCache && !bFound; le = le->Flink)
    {
        MINIPAIR_UNKNOWN *pRecord = CONTAINING_RECORD(le, MINIPAIR_UNKNOWN, ListEntry);

        if (0 == wcscmp(Name, pRecord->Name))
        {
            if (OutUnknownPort)
            {
                *OutUnknownPort = pRecord->PortInterface;
                (*OutUnknownPort)->AddRef();
            }

            if (OutUnknownMiniport)
            {
                *OutUnknownMiniport = pRecord->MiniportInterface;
                (*OutUnknownMiniport)->AddRef();
            }

            bFound = TRUE;
        }
    }

    return bFound?STATUS_SUCCESS:STATUS_OBJECT_NAME_NOT_FOUND;
}



//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CAdapterCommon::CacheSubdevice
(
    _In_ PWSTR Name,
    _In_ PUNKNOWN UnknownPort,
    _In_ PUNKNOWN UnknownMiniport
)
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::CacheSubdevice]"));

    // add the item with this name/interface to the list
    NTSTATUS         ntStatus       = STATUS_SUCCESS;
    MINIPAIR_UNKNOWN *pNewSubdevice = NULL;

    pNewSubdevice = new(NonPagedPoolNx, MINADAPTER_POOLTAG) MINIPAIR_UNKNOWN;

    if (!pNewSubdevice)
    {
        DPF(D_TERSE, ("Insufficient memory to cache subdevice"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(ntStatus))
    {
        memset(pNewSubdevice, 0, sizeof(MINIPAIR_UNKNOWN));

        ntStatus = RtlStringCchCopyW(pNewSubdevice->Name, SIZEOF_ARRAY(pNewSubdevice->Name), Name);
    }

    if (NT_SUCCESS(ntStatus))
    {
        pNewSubdevice->PortInterface = UnknownPort;
        pNewSubdevice->PortInterface->AddRef();

        pNewSubdevice->MiniportInterface = UnknownMiniport;
        pNewSubdevice->MiniportInterface->AddRef();

        // cache the IAdapterPowerManagement interface (if available) from the filter. Some endpoints,
        // like FM and cellular, have their own power requirements that we must track. If this fails,
        // it just means this filter doesn't do power management.
        UnknownMiniport->QueryInterface(IID_IAdapterPowerManagement, (PVOID *)&(pNewSubdevice->PowerInterface));

        InsertTailList(&m_SubdeviceCache, &pNewSubdevice->ListEntry);
    }

    if (!NT_SUCCESS(ntStatus))
    {
        if (pNewSubdevice)
        {
            delete pNewSubdevice;
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CAdapterCommon::RemoveCachedSubdevice
(
    _In_ PWSTR Name
)
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::RemoveCachedSubdevice]"));

    // search list, remove the entry from the list

    PLIST_ENTRY le = NULL;
    BOOL bRemoved = FALSE;

    for (le = m_SubdeviceCache.Flink; le != &m_SubdeviceCache && !bRemoved; le = le->Flink)
    {
        MINIPAIR_UNKNOWN *pRecord = CONTAINING_RECORD(le, MINIPAIR_UNKNOWN, ListEntry);

        if (0 == wcscmp(Name, pRecord->Name))
        {
            SAFE_RELEASE(pRecord->PortInterface);
            SAFE_RELEASE(pRecord->MiniportInterface);
            SAFE_RELEASE(pRecord->PowerInterface);
            memset(pRecord->Name, 0, sizeof(pRecord->Name));
            RemoveEntryList(le);
            bRemoved = TRUE;
            delete pRecord;
            break;
        }
    }

    return bRemoved?STATUS_SUCCESS:STATUS_OBJECT_NAME_NOT_FOUND;
}

#pragma code_seg("PAGE")
VOID
CAdapterCommon::EmptySubdeviceCache()
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::EmptySubdeviceCache]"));

    while (!IsListEmpty(&m_SubdeviceCache))
    {
        PLIST_ENTRY le = RemoveHeadList(&m_SubdeviceCache);
        MINIPAIR_UNKNOWN *pRecord = CONTAINING_RECORD(le, MINIPAIR_UNKNOWN, ListEntry);

        SAFE_RELEASE(pRecord->PortInterface);
        SAFE_RELEASE(pRecord->MiniportInterface);
        SAFE_RELEASE(pRecord->PowerInterface);
        memset(pRecord->Name, 0, sizeof(pRecord->Name));

        delete pRecord;
    }
}

#pragma code_seg("PAGE")
VOID
CAdapterCommon::Cleanup()
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::Cleanup]"));

#ifdef SYSVAD_BTH_BYPASS
    //
    // This ensures Bluetooth HFP notifications are turned off when port class
    // cleanups and unregisters the static subdevices.
    //
    CleanupBthScoBypass();
#endif // SYSVAD_BTH_BYPASS

    EmptySubdeviceCache();
}

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::InstallEndpointFilters
(
    _In_opt_    PIRP                Irp,
    _In_        PENDPOINT_MINIPAIR  MiniportPair,
    _In_opt_    PVOID               DeviceContext,
    _Out_opt_   PUNKNOWN *          UnknownTopology,
    _Out_opt_   PUNKNOWN *          UnknownWave,
    _Out_opt_   PUNKNOWN *          UnknownMiniportTopology,
    _Out_opt_   PUNKNOWN *          UnknownMiniportWave
)
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::InstallEndpointFilters]"));

    NTSTATUS            ntStatus            = STATUS_SUCCESS;
    PUNKNOWN            unknownTopology     = NULL;
    PUNKNOWN            unknownWave         = NULL;
    BOOL                bTopologyCreated    = FALSE;
    BOOL                bWaveCreated        = FALSE;
    PUNKNOWN            unknownMiniTopo     = NULL;
    PUNKNOWN            unknownMiniWave     = NULL;

    // Initialize output optional parameters if needed
    if (UnknownTopology)
    {
        *UnknownTopology = NULL;
    }

    if (UnknownWave)
    {
        *UnknownWave = NULL;
    }

    if (UnknownMiniportTopology)
    {
        *UnknownMiniportTopology = NULL;
    }

    if (UnknownMiniportWave)
    {
        *UnknownMiniportWave = NULL;
    }

    ntStatus = GetCachedSubdevice(MiniportPair->TopoName, &unknownTopology, &unknownMiniTopo);
    if (!NT_SUCCESS(ntStatus) || NULL == unknownTopology || NULL == unknownMiniTopo)
    {
        bTopologyCreated = TRUE;

        // Install SYSVAD topology miniport for the render endpoint.
        //
        ntStatus = InstallSubdevice(Irp,
                                    MiniportPair->TopoName, // make sure this name matches with SYSVAD.<TopoName>.szPname in the inf's [Strings] section
                                    CLSID_PortTopology,
                                    CLSID_PortTopology,
                                    MiniportPair->TopoCreateCallback,
                                    MiniportPair->TopoInterfacePropertyCount,
                                    MiniportPair->TopoInterfaceProperties,
                                    DeviceContext,
                                    MiniportPair,
                                    NULL,
                                    IID_IPortTopology,
                                    NULL,
                                    &unknownTopology,
                                    &unknownMiniTopo
                                    );
        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = CacheSubdevice(MiniportPair->TopoName, unknownTopology, unknownMiniTopo);
        }
    }

    ntStatus = GetCachedSubdevice(MiniportPair->WaveName, &unknownWave, &unknownMiniWave);
    if (!NT_SUCCESS(ntStatus) || NULL == unknownWave || NULL == unknownMiniWave)
    {
        bWaveCreated = TRUE;

        // Install SYSVAD wave miniport for the render endpoint.
        //
        ntStatus = InstallSubdevice(Irp,
                                    MiniportPair->WaveName, // make sure this name matches with SYSVAD.<WaveName>.szPname in the inf's [Strings] section
                                    CLSID_PortWaveRT,
                                    CLSID_PortWaveRT,
                                    MiniportPair->WaveCreateCallback,
                                    MiniportPair->WaveInterfacePropertyCount,
                                    MiniportPair->WaveInterfaceProperties,
                                    DeviceContext,
                                    MiniportPair,
                                    NULL,
                                    IID_IPortWaveRT,
                                    NULL,
                                    &unknownWave,
                                    &unknownMiniWave
                                    );

        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = CacheSubdevice(MiniportPair->WaveName, unknownWave, unknownMiniWave);
        }
    }

    if (unknownTopology && unknownWave)
    {
        //
        // register wave <=> topology connections
        // This will connect bridge pins of wave and topology
        // miniports.
        //
        ntStatus = ConnectTopologies(
            unknownTopology,
            unknownWave,
            MiniportPair->PhysicalConnections,
            MiniportPair->PhysicalConnectionCount);
    }

    if (NT_SUCCESS(ntStatus))
    {
        //
        // Set output parameters.
        //
        if (UnknownTopology != NULL && unknownTopology != NULL)
        {
            unknownTopology->AddRef();
            *UnknownTopology = unknownTopology;
        }

        if (UnknownWave != NULL && unknownWave != NULL)
        {
            unknownWave->AddRef();
            *UnknownWave = unknownWave;
        }
        if (UnknownMiniportTopology != NULL && unknownMiniTopo != NULL)
        {
            unknownMiniTopo->AddRef();
            *UnknownMiniportTopology = unknownMiniTopo;
        }

        if (UnknownMiniportWave != NULL && unknownMiniWave != NULL)
        {
            unknownMiniWave->AddRef();
            *UnknownMiniportWave = unknownMiniWave;
        }

    }
    else
    {
        if (bTopologyCreated && unknownTopology != NULL)
        {
            UnregisterSubdevice(unknownTopology);
            RemoveCachedSubdevice(MiniportPair->TopoName);
        }

        if (bWaveCreated && unknownWave != NULL)
        {
            UnregisterSubdevice(unknownWave);
            RemoveCachedSubdevice(MiniportPair->WaveName);
        }
    }

    SAFE_RELEASE(unknownMiniTopo);
    SAFE_RELEASE(unknownTopology);
    SAFE_RELEASE(unknownMiniWave);
    SAFE_RELEASE(unknownWave);

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::RemoveEndpointFilters
(
    _In_        PENDPOINT_MINIPAIR  MiniportPair,
    _In_opt_    PUNKNOWN            UnknownTopology,
    _In_opt_    PUNKNOWN            UnknownWave
)
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::RemoveEndpointFilters]"));

    NTSTATUS    ntStatus   = STATUS_SUCCESS;

    if (UnknownTopology != NULL && UnknownWave != NULL)
    {
        ntStatus = DisconnectTopologies(
            UnknownTopology,
            UnknownWave,
            MiniportPair->PhysicalConnections,
            MiniportPair->PhysicalConnectionCount);

        if (!NT_SUCCESS(ntStatus))
        {
            DPF(D_VERBOSE, ("RemoveEndpointFilters: DisconnectTopologies failed: 0x%x", ntStatus));
        }
    }


    RemoveCachedSubdevice(MiniportPair->WaveName);

    ntStatus = UnregisterSubdevice(UnknownWave);
    if (!NT_SUCCESS(ntStatus))
    {
        DPF(D_VERBOSE, ("RemoveEndpointFilters: UnregisterSubdevice(wave) failed: 0x%x", ntStatus));
    }

    RemoveCachedSubdevice(MiniportPair->TopoName);

    ntStatus = UnregisterSubdevice(UnknownTopology);
    if (!NT_SUCCESS(ntStatus))
    {
        DPF(D_VERBOSE, ("RemoveEndpointFilters: UnregisterSubdevice(topology) failed: 0x%x", ntStatus));
    }

    //
    // All Done.
    //
    ntStatus = STATUS_SUCCESS;

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::GetFilters
(
    _In_        PENDPOINT_MINIPAIR  MiniportPair,
    _Out_opt_   PUNKNOWN *          UnknownTopologyPort,
    _Out_opt_   PUNKNOWN *          UnknownTopologyMiniport,
    _Out_opt_   PUNKNOWN *          UnknownWavePort,
    _Out_opt_   PUNKNOWN *          UnknownWaveMiniport
)
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::GetFilters]"));

    NTSTATUS    ntStatus   = STATUS_SUCCESS;
    PUNKNOWN            unknownTopologyPort     = NULL;
    PUNKNOWN            unknownTopologyMiniport = NULL;
    PUNKNOWN            unknownWavePort         = NULL;
    PUNKNOWN            unknownWaveMiniport     = NULL;

    // if the client requested the topology filter, find it and return it
    if (UnknownTopologyPort != NULL || UnknownTopologyMiniport != NULL)
    {
        ntStatus = GetCachedSubdevice(MiniportPair->TopoName, &unknownTopologyPort, &unknownTopologyMiniport);
        if (NT_SUCCESS(ntStatus))
        {
            if (UnknownTopologyPort)
            {
                *UnknownTopologyPort = unknownTopologyPort;
            }

            if (UnknownTopologyMiniport)
            {
                *UnknownTopologyMiniport = unknownTopologyMiniport;
            }
        }
    }

    // if the client requested the wave filter, find it and return it
    if (NT_SUCCESS(ntStatus) && (UnknownWavePort != NULL || UnknownWaveMiniport != NULL))
    {
        ntStatus = GetCachedSubdevice(MiniportPair->WaveName, &unknownWavePort, &unknownWaveMiniport);
        if (NT_SUCCESS(ntStatus))
        {
            if (UnknownWavePort)
            {
                *UnknownWavePort = unknownWavePort;
            }

            if (UnknownWaveMiniport)
            {
                *UnknownWaveMiniport = unknownWaveMiniport;
            }
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::SetIdlePowerManagement
(
  _In_  PENDPOINT_MINIPAIR  MiniportPair,
  _In_  BOOL bEnabled
)
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::SetIdlePowerManagement]"));

    NTSTATUS      ntStatus   = STATUS_SUCCESS;
    IUnknown      *pUnknown = NULL;
    PPORTCLSPOWER pPortClsPower = NULL;
    // refcounting disable requests. Each miniport is responsible for calling this in pairs,
    // disable on the first request to disable, enable on the last request to enable.

    // make sure that we always call SetIdlePowerManagment using the IPortClsPower
    // from the requesting port, so we don't cache a reference to a port
    // indefinitely, preventing it from ever unloading.
    ntStatus = GetFilters(MiniportPair, NULL, NULL, &pUnknown, NULL);
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus =
            pUnknown->QueryInterface
            (
                IID_IPortClsPower,
                (PVOID*) &pPortClsPower
            );
    }

    if (NT_SUCCESS(ntStatus))
    {
        if (bEnabled)
        {
            m_dwIdleRequests--;

            if (0 == m_dwIdleRequests)
            {
                pPortClsPower->SetIdlePowerManagement(m_pDeviceObject, TRUE);
            }
        }
        else
        {
            if (0 == m_dwIdleRequests)
            {
                pPortClsPower->SetIdlePowerManagement(m_pDeviceObject, FALSE);
            }

            m_dwIdleRequests++;
        }
    }

    SAFE_RELEASE(pUnknown);
    SAFE_RELEASE(pPortClsPower);

    return ntStatus;
}

#ifdef SYSVAD_BTH_BYPASS
//
// CAdapterCommon Bluetooth Hands-Free Profile function implementation.
//

//=============================================================================
#pragma code_seg("PAGE")
VOID
CAdapterCommon::EvtBthHfpScoBypassInterfaceWorkItem
(
    _In_    WDFWORKITEM WorkItem
)
/*++

Routine Description:

  The function handles the arrival or removal of a HFP SCO Bypass interface.

Arguments:

    WorkItem    - WDF work-item object.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[EvtBthHfpScoBypassInterfaceWorkItem]"));

    CAdapterCommon        * This;

    if (WorkItem == NULL)
    {
        return;
    }

    This = GetBthHfpWorkItemContext(WorkItem)->Adapter;
    ASSERT(This != NULL);

    for (;;)
    {
        PLIST_ENTRY         le          = NULL;
        BthHfpWorkTask    * task        = NULL;

        //
        // Retrieve a taask.
        //
        ExAcquireFastMutex(&This->m_BthHfpFastMutex);
        if (!IsListEmpty(&This->m_BthHfpWorkTasks))
        {
            le = RemoveHeadList(&This->m_BthHfpWorkTasks);
            task = CONTAINING_RECORD(le, BthHfpWorkTask, ListEntry);
            InitializeListHead(le);
        }
        ExReleaseFastMutex(&This->m_BthHfpFastMutex);

        if (task == NULL)
        {
            break;
        }

        ASSERT(task->Device != NULL);
        _Analysis_assume_(task->Device != NULL);

        //
        // Process the task.
        //
        switch(task->Action)
        {
        case eBthHfpTaskStart:
            task->Device->Start();
            break;

        case eBthHfpTaskStop:
            task->Device->Stop();
            break;

        default:
            DPF(D_ERROR, ("EvtBthHfpScoBypassInterfaceWorkItem: invalid action %d", task->Action));
            break;
        }

        //
        // Release the ref we took on the device when we inserted the task in the queue.
        // For a stop operation this may be the last reference.
        //
        SAFE_RELEASE(task->Device);

        //
        // Free the task.
        //
        ExFreeToNPagedLookasideList(&This->m_BhtHfpWorkTaskPool, task);
    }
}

//=============================================================================
#pragma code_seg("PAGE")
BthHfpDevice *
CAdapterCommon::BthHfpDeviceFind
(
    _In_ PUNICODE_STRING SymbolicLinkName
)
/*++

Routine Description:

  The function looks for the specified device in the adapter's list.

Arguments:

  SymbolicLinkName - interface's symbolic link.

Return Value:

  BthHfpDevice pointer or NULL.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::BthHfpDeviceFind]"));

    PLIST_ENTRY     le          = NULL;
    BthHfpDevice  * bthDevice   = NULL;

    ExAcquireFastMutex(&m_BthHfpFastMutex);

    for (le = m_BthHfpDevices.Flink; le != &m_BthHfpDevices; le = le->Flink)
    {
        BthHfpDevice  *     tmpBthDevice    = BthHfpDevice::GetBthHfpDevice(le);
        ASSERT(tmpBthDevice != NULL);

        PUNICODE_STRING     unicodeStr      = tmpBthDevice->GetSymbolicLinkName();
        ASSERT(unicodeStr != NULL);

        if (unicodeStr->Length == SymbolicLinkName->Length &&
            0 == wcsncmp(unicodeStr->Buffer, SymbolicLinkName->Buffer, unicodeStr->Length/sizeof(WCHAR)))
        {
            // Found it!
            bthDevice = tmpBthDevice;
            bthDevice->AddRef();
            break;
        }
    }

    ExReleaseFastMutex(&m_BthHfpFastMutex);

    return bthDevice;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CAdapterCommon::BthHfpScoInterfaceArrival
(
    _In_ PUNICODE_STRING SymbolicLinkName
)
/*++

Routine Description:

  The function handles the arrival of a new HFP SCO Bypass interface.

Arguments:

  SymbolicLinkName - new interface's symbolic link.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::BthHfpScoInterfaceArrival]"));

    NTSTATUS            ntStatus        = STATUS_SUCCESS;
    BthHfpDevice      * bthDevice       = NULL;
    BthHfpWorkTask    * bthWorkTask     = NULL;

    DPF(D_VERBOSE, ("BthHfpScoInterfaceArrival: SymbolicLinkName %wZ", SymbolicLinkName));

    //
    // Check if the Bluetooth device is already present.
    // According to the docs it is possible to receive two notifications for the same
    // interface.
    //
    bthDevice = BthHfpDeviceFind(SymbolicLinkName);
    if (bthDevice != NULL)
    {
        DPF(D_VERBOSE, ("BthHfpScoInterfaceArrival: Bluetooth HFP device already present"));
        SAFE_RELEASE(bthDevice);
        ntStatus = STATUS_SUCCESS;
        goto Done;
    }

    //
    // Alloc a new structure for this Bluetooth hands-free device.
    //
    bthDevice = new (NonPagedPoolNx, MINADAPTER_POOLTAG) BthHfpDevice(NULL); // NULL -> OuterUnknown
    if (NULL == bthDevice)
    {
        DPF(D_ERROR, ("BthHfpScoInterfaceArrival: unable to allocate BthHfpDevice, out of memory"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    DPF(D_VERBOSE, ("BthHfpScoInterfaceArrival: created BthHfpDevice 0x%p ", bthDevice));

    //
    // Basic initialization of the Bluetooth Hands-Free Profile interface.
    // The audio miniport creation is done later by the BthHfpDevice.Start()
    // which is invoked asynchronously by a worker thread.
    // BthHfpDevice->Init() must be invoked just after the creation of the object.
    //
    ntStatus = bthDevice->Init(this, SymbolicLinkName);
    IF_FAILED_JUMP(ntStatus, Done);

    //
    // Get and init a work task.
    //
    bthWorkTask = (BthHfpWorkTask*)ExAllocateFromNPagedLookasideList(&m_BhtHfpWorkTaskPool);
    if (NULL == bthWorkTask)
    {
        DPF(D_ERROR, ("BthHfpScoInterfaceArrival: unable to allocate BthHfpWorkTask, out of memory"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    // bthWorkTask->L.Size is set to sizeof(BthHfpWorkTask) in the Look Aside List configuration
#pragma warning(suppress: 6386)
    RtlZeroMemory(bthWorkTask, sizeof(*bthWorkTask));
    bthWorkTask->Action = eBthHfpTaskStart;
    InitializeListHead(&bthWorkTask->ListEntry);
    // Note that bthDevice has one reference at this point.
    bthWorkTask->Device = bthDevice;

    ExAcquireFastMutex(&m_BthHfpFastMutex);

    //
    // Insert this new Bluetooth HFP device in our list.
    //
    InsertTailList(&m_BthHfpDevices, bthDevice->GetListEntry());

    //
    // Add a new task for the worker thread.
    //
    InsertTailList(&m_BthHfpWorkTasks, &bthWorkTask->ListEntry);
    bthDevice->AddRef();    // released when task runs.

    //
    // Schedule a work-item if not already running.
    //
    WdfWorkItemEnqueue(m_BthHfpWorkItem);

    ExReleaseFastMutex(&m_BthHfpFastMutex);

Done:
    if (!NT_SUCCESS(ntStatus))
    {
        // Release the last ref, this will delete the BthHfpDevice
        SAFE_RELEASE(bthDevice);

        if (bthWorkTask != NULL)
        {
            ExFreeToNPagedLookasideList(&m_BhtHfpWorkTaskPool, bthWorkTask);
            bthWorkTask = NULL;
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CAdapterCommon::BthHfpScoInterfaceRemoval
(
    _In_ PUNICODE_STRING SymbolicLinkName
)
/*++

Routine Description:

  The function handles the removal of a HFP SCO Bypass interface.

Arguments:

  SymbolicLinkName - interface's symbolic link to remove.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::BthHfpScoInterfaceRemoval]"));

    NTSTATUS            ntStatus        = STATUS_SUCCESS;
    BthHfpDevice      * bthDevice       = NULL;
    BthHfpWorkTask    * bthWorkTask     = NULL;

    DPF(D_VERBOSE, ("BthHfpScoInterfaceRemoval: SymbolicLinkName %wZ", SymbolicLinkName));

    //
    // Check if the Bluetooth device is present.
    //
    bthDevice = BthHfpDeviceFind(SymbolicLinkName);
    if (bthDevice == NULL)
    {
        // This can happen if the init/start of the BthHfpDevice failed.
        DPF(D_VERBOSE, ("BthHfpScoInterfaceRemoval: Bluetooth HFP device not found"));
        ntStatus = STATUS_SUCCESS;
        goto Done;
    }

    //
    // Init a work task.
    //
    bthWorkTask = (BthHfpWorkTask*)ExAllocateFromNPagedLookasideList(&m_BhtHfpWorkTaskPool);
    if (NULL == bthWorkTask)
    {
        DPF(D_ERROR, ("BthHfpScoInterfaceRemoval: unable to allocate BthHfpWorkTask, out of memory"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    // bthWorkTask->L.Size is set to sizeof(BthHfpWorkTask) in the Look Aside List configuration
#pragma warning(suppress: 6386)
    RtlZeroMemory(bthWorkTask, sizeof(*bthWorkTask));
    bthWorkTask->Action = eBthHfpTaskStop;
    InitializeListHead(&bthWorkTask->ListEntry);
    // Work-item callback will release the reference we got above from BthHfpDeviceFind.
    bthWorkTask->Device = bthDevice;

    ExAcquireFastMutex(&m_BthHfpFastMutex);

    //
    // Remove this Bluetooth device from our list and release the associated reference.
    //
    RemoveEntryList(bthDevice->GetListEntry());
    InitializeListHead(bthDevice->GetListEntry());
    bthDevice->Release();   // This is not the last ref.

    //
    // Add a new task for the worker thread.
    //
    InsertTailList(&m_BthHfpWorkTasks, &bthWorkTask->ListEntry);

    //
    // Schedule a work-item if not already running.
    //
    WdfWorkItemEnqueue(m_BthHfpWorkItem);

    ExReleaseFastMutex(&m_BthHfpFastMutex);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    if (!NT_SUCCESS(ntStatus))
    {
        // Release the ref we got in find.
        SAFE_RELEASE(bthDevice);
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CAdapterCommon::EvtBthHfpScoBypassInterfaceChange(
  _In_          PVOID   NotificationPointer,
  _Inout_opt_   PVOID   Context
  )
/*++

Routine Description:

    This callback is invoked when a new HFP SCO Bypass interface is added or removed.

Arguments:
    NotificationPointer - Interface change notification
    Context - CAdapterCommon ptr.

Return Value:

    NT status code.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[EvtBthHfpScoBypassInterfaceChange]"));

    NTSTATUS                              ntStatus      = STATUS_SUCCESS;
    CAdapterCommon                      * This          = NULL;
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION Notification  = (PDEVICE_INTERFACE_CHANGE_NOTIFICATION) NotificationPointer;

    //
    // Make sure this is the interface class we extect. Any other class guid
    // is an error, but let it go since it is not fatal to the machine.
    //
    if (!IsEqualGUID(Notification->InterfaceClassGuid, GUID_DEVINTERFACE_BLUETOOTH_HFP_SCO_HCIBYPASS))
    {
        DPF(D_VERBOSE, ("EvtBthHfpScoBypassInterfaceChange: bad interface ClassGuid"));
        ASSERTMSG("EvtBthHfpScoBypassInterfaceChange: bad interface ClassGuid ", FALSE);

        goto Done;
    }

    This = (CAdapterCommon *)Context;
    ASSERT(This != NULL);
    _Analysis_assume_(This != NULL);

    //
    // Take action based on the event. Any other event type is an error,
    // but let it go since it is not fatal to the machine.
    //
    if (IsEqualGUID(Notification->Event, GUID_DEVICE_INTERFACE_ARRIVAL))
    {
        ntStatus = This->BthHfpScoInterfaceArrival(Notification->SymbolicLinkName);
    }
    else if (IsEqualGUID(Notification->Event, GUID_DEVICE_INTERFACE_REMOVAL))
    {
        ntStatus = This->BthHfpScoInterfaceRemoval(Notification->SymbolicLinkName);
    }
    else
    {
        DPF(D_VERBOSE, ("EvtBthHfpScoBypassInterfaceChange: bad "
            "GUID_DEVINTERFACE_BLUETOOTH_HFP_SCO_HCIBYPASS event"));
        ASSERTMSG("EvtBthHfpScoBypassInterfaceChange: bad "
            "GUID_DEVINTERFACE_BLUETOOTH_HFP_SCO_HCIBYPASS event ", FALSE);

        goto Done;
    }

Done:
    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CAdapterCommon::InitBthScoBypass()
/*++

Routine Description:

  Initialize the bluetooth bypass environment.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::InitBluetoothBypass]"));

    NTSTATUS                ntStatus = STATUS_SUCCESS;
    WDF_WORKITEM_CONFIG     wiConfig;
    WDF_OBJECT_ATTRIBUTES   attributes;
    BthHfpWorkItemContext * wiContext;

    //
    // Init spin-lock, linked lists, work-item, event, etc.
    // Init all members to default values. This basic init should not fail.
    //
    m_BthHfpWorkItem = NULL;
    m_BthHfpScoNotificationHandle = NULL;
    ExInitializeFastMutex(&m_BthHfpFastMutex);
    InitializeListHead(&m_BthHfpWorkTasks);
    InitializeListHead(&m_BthHfpDevices);
    m_BhtHfpWorkTaskPoolElementSize = sizeof(BthHfpWorkTask);
    ExInitializeNPagedLookasideList(&m_BhtHfpWorkTaskPool,
                                    NULL,
                                    NULL,
                                    POOL_NX_ALLOCATION,
                                    m_BhtHfpWorkTaskPoolElementSize,
                                    MINADAPTER_POOLTAG,
                                    0);
    //
    // Enable Bluetooth HFP SCO-Bypass Cleanup.
    // Do any allocation/initialization that can fail after this point.
    //
    m_BthHfpEnableCleanup = TRUE;

    //
    // Allocate a WDF work-item.
    //
    WDF_WORKITEM_CONFIG_INIT(&wiConfig, EvtBthHfpScoBypassInterfaceWorkItem);
    wiConfig.AutomaticSerialization = FALSE;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, BthHfpWorkItemContext);
    attributes.ParentObject = GetWdfDevice();
    ntStatus = WdfWorkItemCreate( &wiConfig,
                                  &attributes,
                                  &m_BthHfpWorkItem);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("InitBthScoBypass: WdfWorkItemCreate failed: 0x%x", ntStatus)),
        Done);

    wiContext = GetBthHfpWorkItemContext(m_BthHfpWorkItem);
    wiContext->Adapter = this; // weak ref.

    //
    // Register for bluetooth heandsfree profile interface changes.
    //
    ntStatus = IoRegisterPlugPlayNotification (
                    EventCategoryDeviceInterfaceChange,
                    PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                    (PVOID)&GUID_DEVINTERFACE_BLUETOOTH_HFP_SCO_HCIBYPASS,
                    m_pDeviceObject->DriverObject,
                    EvtBthHfpScoBypassInterfaceChange,
                    (PVOID)this,
                    &m_BthHfpScoNotificationHandle);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("InitBthScoBypass: IoRegisterPlugPlayNotification(GUID_DEVINTERFACE_BLUETOOTH_HFP_SCO_HCIBYPASS) failed: 0x%x", ntStatus)),
        Done);

    //
    // Initialization completed.
    //
    ntStatus = STATUS_SUCCESS;

Done:
    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
VOID
CAdapterCommon::CleanupBthScoBypass()
/*++

Routine Description:

  Cleanup the bluetooth bypass environment.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::CleanupBthScoBypass]"));

    //
    // Do nothing if Bluetooth HFP environment was not correctly initialized.
    //
    if (m_BthHfpEnableCleanup == FALSE)
    {
        return;
    }

    //
    // Unregister for bluetooth heandsfree profile interface changes.
    //
    if (m_BthHfpScoNotificationHandle != NULL)
    {
        (void)IoUnregisterPlugPlayNotificationEx(m_BthHfpScoNotificationHandle);
        m_BthHfpScoNotificationHandle = NULL;
    }

    //
    // Wait for the Bluetooth hands-free profile worker thread to be done.
    //
    if (m_BthHfpWorkItem != NULL)
    {
        WdfWorkItemFlush(m_BthHfpWorkItem);
        WdfObjectDelete(m_BthHfpWorkItem);
        m_BthHfpWorkItem = NULL;
    }

    ASSERT(IsListEmpty(&m_BthHfpWorkTasks));

    //
    // Stop and delete all BthHfpDevices. We are the only thread accessing this list,
    // so there is no need to acquire the mutex.
    //
    while (!IsListEmpty(&m_BthHfpDevices))
    {
        BthHfpDevice  * bthDevice   = NULL;
        PLIST_ENTRY     le          = NULL;

        le = RemoveHeadList(&m_BthHfpDevices);

        bthDevice = BthHfpDevice::GetBthHfpDevice(le);
        InitializeListHead(le);

        // bthDevice is invalid after this call.
        bthDevice->Stop();

        // This should be the last reference.
        bthDevice->Release();
    }

    ASSERT(IsListEmpty(&m_BthHfpDevices));

    //
    // General cleanup.
    //
    ExDeleteNPagedLookasideList(&m_BhtHfpWorkTaskPool);
}
#endif  // SYSVAD_BTH_BYPASS

#ifdef SYSVAD_BTH_BYPASS
//
// BthHfpDevice implementation.
//

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP
BthHfpDevice::NonDelegatingQueryInterface
(
    _In_ REFIID                 Interface,
    _COM_Outptr_ PVOID *        Object
)
/*++

Routine Description:

  QueryInterface routine for BthHfpDevice

Arguments:

  Interface -

  Object -

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PBTHHFPDEVICECOMMON(this)));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IBthHfpDeviceCommon))
    {
        *Object = PVOID(PBTHHFPDEVICECOMMON(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
} // NonDelegatingQueryInterface

//=============================================================================
// Dummy stubs to override the default WDF behavior of closing the target
// on query remove. This driver closes and deletes the supporting objects
// when the target removes the BTH HFP SCO Bypass interface.
//

#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::EvtBthHfpTargetQueryRemove
(
    _In_    WDFIOTARGET     IoTarget
)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::EvtBthHfpTargetQueryRemove]"));

    UNREFERENCED_PARAMETER(IoTarget);
    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
VOID
BthHfpDevice::EvtBthHfpTargetRemoveCanceled
(
    _In_    WDFIOTARGET     IoTarget
)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::EvtBthHfpTargetRemoveCanceled]"));

    UNREFERENCED_PARAMETER(IoTarget);
}
#pragma code_seg("PAGE")
VOID
BthHfpDevice::EvtBthHfpTargetRemoveComplete
(
    _In_    WDFIOTARGET     IoTarget
)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::EvtBthHfpTargetRemoveComplete]"));

    UNREFERENCED_PARAMETER(IoTarget);
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::Init
(
    _In_ CAdapterCommon     * Adapter,
    _In_ PUNICODE_STRING      SymbolicLinkName
)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::Init]"));

    NTSTATUS                                ntStatus        = STATUS_SUCCESS;
    BthHfpDeviceNotificationWorkItemContext *wiCtx          = NULL;
    BthHfpDeviceNotificationReqContext      *reqCtx         = NULL;
    WDF_OBJECT_ATTRIBUTES                   attributes;
    WDF_IO_TARGET_OPEN_PARAMS               openParams;
    WDF_WORKITEM_CONFIG                     wiConfig;

    AddRef(); // first ref.

    //
    // Basic init of all the class' members.
    //
    m_State                 = eBthHfpStateInitializing;
    m_Adapter               = Adapter;

    // Static config.
    m_WdfIoTarget           = NULL;
    m_SpeakerMiniports      = NULL;
    m_MicMiniports          = NULL;
    m_UnknownSpeakerTopology = NULL;
    m_UnknownSpeakerWave    = NULL;
    m_UnknownMicTopology    = NULL;
    m_UnknownMicWave        = NULL;
    m_Descriptor            = NULL;
    m_VolumePropValues      = NULL;

    // Notification updates.
    m_SpeakerVolumeLevel    = 0;
    m_MicVolumeLevel        = 0;
    m_ConnectionStatusLong  = FALSE;
    m_StreamStatusLong      = STATUS_INVALID_DEVICE_STATE; // Sco stream is not open.
    m_NRECDisableStatusLong = FALSE;

    m_StreamReq             = NULL;
    m_SpeakerVolumeReq      = NULL;
    m_MicVolumeReq          = NULL;
    m_ConnectionReq         = NULL;
    m_NRECDisableStatusReq  = NULL;

    m_WorkItem              = NULL;
    m_ReqCollection         = NULL;

    m_nStreams              = 0;

    KeInitializeEvent(&m_StreamStatusEvent, NotificationEvent, TRUE);

    InitializeListHead(&m_ListEntry);
    KeInitializeSpinLock(&m_Lock);

    RtlZeroMemory(&m_SymbolicLinkName, sizeof(m_SymbolicLinkName));

    RtlZeroMemory(&m_SpeakerVolumeCallback, sizeof(m_SpeakerVolumeCallback));
    RtlZeroMemory(&m_SpeakerConnectionStatusCallback, sizeof(m_SpeakerConnectionStatusCallback));
    RtlZeroMemory(&m_MicVolumeCallback, sizeof(m_MicVolumeCallback));
    RtlZeroMemory(&m_MicConnectionStatusCallback, sizeof(m_MicConnectionStatusCallback));

    //
    // Allocate a notification WDF work-item.
    //
    WDF_WORKITEM_CONFIG_INIT(&wiConfig, EvtBthHfpDeviceNotificationStatusWorkItem);
    wiConfig.AutomaticSerialization = FALSE;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, BthHfpDeviceNotificationWorkItemContext);
    attributes.ParentObject = Adapter->GetWdfDevice();
    ntStatus = WdfWorkItemCreate( &wiConfig,
                                  &attributes,
                                  &m_WorkItem);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfWorkItemCreate failed: 0x%x", ntStatus)),
        Done);

    wiCtx = GetBthHfpDeviceNotificationWorkItemContext(m_WorkItem);
    wiCtx->BthHfpDevice = this; // weak ref.

    //
    // Allocate a collection to hold notification requests for the notification work-item.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfCollectionCreate(
        &attributes,
        &m_ReqCollection);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfCollectionCreate failed: 0x%x", ntStatus)),
        Done);

    //
    // Open the target interface.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfIoTargetCreate(m_Adapter->GetWdfDevice(),
                                 &attributes,
                                 &m_WdfIoTarget);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfIoTargetCreate failed: 0x%x", ntStatus)),
        Done);

    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
        &openParams,
        SymbolicLinkName,
        STANDARD_RIGHTS_ALL);

    openParams.EvtIoTargetQueryRemove = EvtBthHfpTargetQueryRemove;
    openParams.EvtIoTargetRemoveCanceled = EvtBthHfpTargetRemoveCanceled;
    openParams.EvtIoTargetRemoveComplete = EvtBthHfpTargetRemoveComplete;

    ntStatus = WdfIoTargetOpen(m_WdfIoTarget, &openParams);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfIoTargetOpen(%wZ) failed: 0x%x", SymbolicLinkName, ntStatus)),
        Done);

    //
    // Make a copy of the symbolic link name.
    //
    m_SymbolicLinkName.MaximumLength = SymbolicLinkName->MaximumLength;
    m_SymbolicLinkName.Length = SymbolicLinkName->Length;
    m_SymbolicLinkName.Buffer = (PWSTR) ExAllocatePoolWithTag(NonPagedPoolNx,
                                                              SymbolicLinkName->MaximumLength,
                                                              MINADAPTER_POOLTAG);
    if (m_SymbolicLinkName.Buffer == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: ExAllocatePoolWithTag failed, out of memory")),
        Done);

    RtlCopyUnicodeString(&m_SymbolicLinkName, SymbolicLinkName);

    //
    // Allocate the WDF requests for status notifications.
    //

    //
    // IOCTL_BTHHFP_DEVICE_GET_NRECDISABLE_STATUS_UPDATE
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, BthHfpDeviceNotificationReqContext);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &m_NRECDisableStatusReq);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfRequestCreate(Nrec-disable status) failed, 0x%x", ntStatus)),
        Done);

    // Init context.
    reqCtx = GetBthHfpDeviceNotificationReqContext(m_NRECDisableStatusReq);
    reqCtx->BthHfpDevice = this; // weak ref.

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.bImmediate),
        &reqCtx->MemIn);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.BoolStatus),
        &reqCtx->MemOut);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    //
    // IOCTL_BTHHFP_SPEAKER_GET_VOLUME_STATUS_UPDATE
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, BthHfpDeviceNotificationReqContext);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &m_SpeakerVolumeReq);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfRequestCreate(Speaker-Volume) failed, 0x%x", ntStatus)),
        Done);

    // Init context.
    reqCtx = GetBthHfpDeviceNotificationReqContext(m_SpeakerVolumeReq);
    reqCtx->BthHfpDevice = this; // weak ref.

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.bImmediate),
        &reqCtx->MemIn);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.Volume),
        &reqCtx->MemOut);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    //
    // IOCTL_BTHHFP_MIC_GET_VOLUME_STATUS_UPDATE
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, BthHfpDeviceNotificationReqContext);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &m_MicVolumeReq);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfRequestCreate(Mic-Volume) failed, 0x%x", ntStatus)),
        Done);

    // Init context.
    reqCtx = GetBthHfpDeviceNotificationReqContext(m_MicVolumeReq);
    reqCtx->BthHfpDevice = this; // weak ref.

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.bImmediate),
        &reqCtx->MemIn);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.Volume),
        &reqCtx->MemOut);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    //
    // IOCTL_BTHHFP_DEVICE_GET_CONNECTION_STATUS_UPDATE
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, BthHfpDeviceNotificationReqContext);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &m_ConnectionReq);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfRequestCreate(Connection-Status) failed, 0x%x", ntStatus)),
        Done);

    // Init context.
    reqCtx = GetBthHfpDeviceNotificationReqContext(m_ConnectionReq);
    reqCtx->BthHfpDevice = this; // weak ref.

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.bImmediate),
        &reqCtx->MemIn);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.BoolStatus),
        &reqCtx->MemOut);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    //
    // IOCTL_BTHHFP_STREAM_GET_STATUS_UPDATE
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, BthHfpDeviceNotificationReqContext);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &m_StreamReq);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfRequestCreate(Stream-Status) failed, 0x%x", ntStatus)),
        Done);

    // Init context.
    reqCtx = GetBthHfpDeviceNotificationReqContext(m_StreamReq);
    reqCtx->BthHfpDevice = this; // weak ref.

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.bImmediate),
        &reqCtx->MemIn);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.NtStatus),
        &reqCtx->MemOut);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

   //
   // This remote device is now in running state. No need to use interlock operations
   // b/c at this time this is the only thread accessing this info.
   //
   m_State = eBthHfpStateRunning;

   //
   // Init successful.
   //
   ntStatus = STATUS_SUCCESS;

Done:
    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
BthHfpDevice::~BthHfpDevice
(
    void
)
/*++

Routine Description:

  Destructor for BthHfpDevice.

Arguments:

Return Value:

  void

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::~BthHfpDevice]"));

    ASSERT(m_State != eBthHfpStateRunning);
    ASSERT(IsListEmpty(&m_ListEntry));

    //
    // Release ref to remote stack.
    //
    if (m_WdfIoTarget != NULL)
    {
        WdfObjectDelete(m_WdfIoTarget);
        m_WdfIoTarget = NULL;
    }

    //
    // Free symbolic links.
    //
    if (m_SymbolicLinkName.Buffer != NULL)
    {
        ExFreePoolWithTag(m_SymbolicLinkName.Buffer, MINADAPTER_POOLTAG);
        RtlZeroMemory(&m_SymbolicLinkName, sizeof(m_SymbolicLinkName));
    }

    DeleteCustomEndpointMinipair(m_SpeakerMiniports);
    m_SpeakerMiniports = NULL;

    DeleteCustomEndpointMinipair(m_MicMiniports);
    m_MicMiniports = NULL;

    if (m_Descriptor != NULL)
    {
        ExFreePoolWithTag(m_Descriptor, MINADAPTER_POOLTAG);
        m_Descriptor = NULL;
    }

    if (m_VolumePropValues != NULL)
    {
        ExFreePoolWithTag(m_VolumePropValues, MINADAPTER_POOLTAG);
        m_VolumePropValues = NULL;
    }

    //
    // Free Irps.
    //
    if (m_SpeakerVolumeReq != NULL)
    {
        BthHfpDeviceNotificationReqContext * ctx;

        // Delete the associated memory objects.
        ctx = GetBthHfpDeviceNotificationReqContext(m_SpeakerVolumeReq);
        if (ctx->MemIn != NULL)
        {
            WdfObjectDelete(ctx->MemIn);
            ctx->MemIn = NULL;
        }

        if (ctx->MemOut != NULL)
        {
            WdfObjectDelete(ctx->MemOut);
            ctx->MemOut = NULL;
        }

        // Delete the request.
        WdfObjectDelete(m_SpeakerVolumeReq);
        m_SpeakerVolumeReq = NULL;
    }

    if (m_MicVolumeReq != NULL)
    {
        BthHfpDeviceNotificationReqContext * ctx;

        // Delete the associated memory objects.
        ctx = GetBthHfpDeviceNotificationReqContext(m_MicVolumeReq);
        if (ctx->MemIn != NULL)
        {
            WdfObjectDelete(ctx->MemIn);
            ctx->MemIn = NULL;
        }

        if (ctx->MemOut != NULL)
        {
            WdfObjectDelete(ctx->MemOut);
            ctx->MemOut = NULL;
        }

        // Delete the request.
        WdfObjectDelete(m_MicVolumeReq);
        m_MicVolumeReq = NULL;
    }

    if (m_ConnectionReq != NULL)
    {
        BthHfpDeviceNotificationReqContext * ctx;

        // Delete the associated memory objects.
        ctx = GetBthHfpDeviceNotificationReqContext(m_ConnectionReq);
        if (ctx->MemIn != NULL)
        {
            WdfObjectDelete(ctx->MemIn);
            ctx->MemIn = NULL;
        }

        if (ctx->MemOut != NULL)
        {
            WdfObjectDelete(ctx->MemOut);
            ctx->MemOut = NULL;
        }

        // Delete the request.
        WdfObjectDelete(m_ConnectionReq);
        m_ConnectionReq = NULL;
    }

    if (m_StreamReq != NULL)
    {
        BthHfpDeviceNotificationReqContext * ctx;

        // Delete the associated memory objects.
        ctx = GetBthHfpDeviceNotificationReqContext(m_StreamReq);
        if (ctx->MemIn != NULL)
        {
            WdfObjectDelete(ctx->MemIn);
            ctx->MemIn = NULL;
        }

        if (ctx->MemOut != NULL)
        {
            WdfObjectDelete(ctx->MemOut);
            ctx->MemOut = NULL;
        }

        // Delete the request.
        WdfObjectDelete(m_StreamReq);
        m_StreamReq = NULL;
    }

    //
    // Notification work-item.
    //
    if (m_WorkItem != NULL)
    {
        WdfObjectDelete(m_WorkItem);
        m_WorkItem = NULL;
    }

    //
    // Notification req. collection.
    //
    if (m_ReqCollection != NULL)
    {
        WdfObjectDelete(m_ReqCollection);
        m_ReqCollection = NULL;
    }

    ASSERT(m_UnknownSpeakerTopology == NULL);
    SAFE_RELEASE(m_UnknownSpeakerTopology);

    ASSERT(m_UnknownSpeakerWave == NULL);
    SAFE_RELEASE(m_UnknownSpeakerWave);

    ASSERT(m_UnknownMicTopology == NULL);
    SAFE_RELEASE(m_UnknownMicTopology);

    ASSERT(m_UnknownMicWave == NULL);
    SAFE_RELEASE(m_UnknownMicWave);

    ASSERT(m_nStreams == 0);

    ASSERT(m_SpeakerVolumeCallback.Handler == NULL);
    ASSERT(m_SpeakerConnectionStatusCallback.Handler == NULL);
    ASSERT(m_MicVolumeCallback.Handler == NULL);
    ASSERT(m_MicConnectionStatusCallback.Handler == NULL);
} // ~CAdapterCommon

//
// IBthHfpDeviceCommon implementation.
//

//=============================================================================
#pragma code_seg("PAGE")
BOOL
BthHfpDevice::IsVolumeSupported()
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::IsVolumeSupported]"));

    return m_Descriptor->SupportsVolume;
}

//=============================================================================
#pragma code_seg("PAGE")
PKSPROPERTY_VALUES
BthHfpDevice::GetVolumeSettings
(
    _Out_ PULONG    Size
)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetVolumeSettings]"));

    ASSERT(Size != NULL);

    *Size = m_Descriptor->VolumePropertyValuesSize;

    return m_VolumePropValues;
}

//=============================================================================
#pragma code_seg("PAGE")
LONG
BthHfpDevice::GetSpeakerVolume()
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetSpeakerVolume]"));

    return InterlockedCompareExchange(&m_SpeakerVolumeLevel, 0, 0);
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::SetSpeakerVolume
(
    _In_ ULONG      Volume
)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetSpeakerVolume]"));

    return SetBthHfpSpeakerVolume(Volume);
}

//=============================================================================
#pragma code_seg("PAGE")
LONG
BthHfpDevice::GetMicVolume()
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetMicVolume]"));

    return InterlockedCompareExchange(&m_MicVolumeLevel, 0, 0);
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::SetMicVolume
(
    _In_ ULONG      Volume
)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetMicVolume]"));

    return SetBthHfpMicVolume(Volume);
}

//=============================================================================
#pragma code_seg("PAGE")
BOOL
BthHfpDevice::GetConnectionStatus()
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetConnectionStatus]"));

    return (BOOL)InterlockedCompareExchange(&m_ConnectionStatusLong, 0, 0);
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::GetBthHfpCodecId(_Out_ UCHAR * CodecId)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetBthHfpCodecId]"));

    ASSERT(CodecId != NULL);

#if !defined(NTDDI_WIN10_RS3)
    typedef enum _HFP_BYPASS_CODEC_ID_VERSION {
        REQ_HFP_BYPASS_CODEC_ID_V1 = 1,
    } HFP_BYPASS_CODEC_ID_VERSION, *PHFP_BYPASS_CODEC_ID_VERSION;

    typedef struct _HFP_BYPASS_CODEC_ID_V1 {
        UCHAR CodecId;
    } HFP_BYPASS_CODEC_ID_V1, *PHFP_BYPASS_CODEC_ID_V1;
#endif

    NTSTATUS ntStatus = STATUS_SUCCESS;

    union {
        HFP_BYPASS_CODEC_ID_V1 CodecIdV1;
        HFP_BYPASS_CODEC_ID_VERSION Version;
    } value;

    *CodecId = 0;

    value.Version = REQ_HFP_BYPASS_CODEC_ID_V1;

    //
    // Get the Bth HFP SCO Codec ID.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_DEVICE_GET_CODEC_ID,
        sizeof(value.Version),
        sizeof(value.CodecIdV1),
        &value);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetBthHfpCodecId: SendIoCtrlSynchronously(IOCTL_BTHHFP_DEVICE_GET_CODEC_ID) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *CodecId = value.CodecIdV1.CodecId;
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::Connect()
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::Connect]"));

    return SetBthHfpConnect();
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::Disconnect()
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::Disconnect]"));

    return SetBthHfpDisconnect();
}

//=============================================================================
#pragma code_seg()
BOOL
BthHfpDevice::GetStreamStatus()
{
    DPF_ENTER(("[BthHfpDevice::GetStreamStatus]"));

    NTSTATUS ntStatus;

    ntStatus = (NTSTATUS)InterlockedCompareExchange(&m_StreamStatusLong, 0, 0);

    return NT_SUCCESS(ntStatus) ? TRUE : FALSE;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::StreamOpen()
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::StreamOpen]"));

    NTSTATUS    ntStatus    = STATUS_SUCCESS;
    LONG        nStreams    = 0;

    ASSERT(m_nStreams >= 0);

    nStreams = InterlockedIncrement(&m_nStreams);
    if (nStreams == 1)
    {
        BOOLEAN  streamOpen = FALSE;

        ntStatus = SetBthHfpStreamOpen();
        if (NT_SUCCESS(ntStatus))
        {
            streamOpen = TRUE;
            m_StreamStatus = STATUS_SUCCESS;
            ntStatus = EnableBthHfpStreamStatusNotification();
        }

        //
        // Cleanup if any error.
        //
        if (!NT_SUCCESS(ntStatus))
        {
            nStreams = InterlockedDecrement(&m_nStreams);
            ASSERT(nStreams == 0);
            UNREFERENCED_VAR(nStreams);

            if (streamOpen)
            {
                SetBthHfpStreamClose();
            }

            m_StreamStatus = STATUS_INVALID_DEVICE_STATE;
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::StreamClose()
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::StreamClose]"));

    NTSTATUS    ntStatus    = STATUS_SUCCESS;
    LONG        nStreams    = 0;

    ASSERT(m_nStreams > 0);

    nStreams = InterlockedDecrement(&m_nStreams);
    if (nStreams == 0)
    {
        ntStatus = SetBthHfpStreamClose();

        StopBthHfpStreamStatusNotification();

        m_StreamStatus = STATUS_INVALID_DEVICE_STATE;
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
GUID
BthHfpDevice::GetContainerId()
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetContainerId]"));

    return m_Descriptor->ContainerId;
}

//=============================================================================
#pragma code_seg("PAGE")
VOID
BthHfpDevice::SetSpeakerVolumeHandler
(
    _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
    _In_opt_    PVOID                   EventHandlerContext
)
    {
        PAGED_CODE();
        DPF_ENTER(("[BthHfpDevice::SetSpeakerVolumeHandler]"));

        ASSERT(EventHandler == NULL || m_SpeakerVolumeCallback.Handler == NULL);

        m_SpeakerVolumeCallback.Handler = EventHandler; // weak ref.
        m_SpeakerVolumeCallback.Context = EventHandlerContext;
    }

//=============================================================================
#pragma code_seg("PAGE")
VOID
BthHfpDevice::SetSpeakerConnectionStatusHandler
(
    _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
    _In_opt_    PVOID                   EventHandlerContext
)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetSpeakerConnectionStatusHandler]"));

    ASSERT(EventHandler == NULL || m_SpeakerConnectionStatusCallback.Handler == NULL);

    m_SpeakerConnectionStatusCallback.Handler = EventHandler; // weak ref.
    m_SpeakerConnectionStatusCallback.Context = EventHandlerContext;
}

//=============================================================================
#pragma code_seg("PAGE")
VOID
BthHfpDevice::SetMicVolumeHandler
(
    _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
    _In_opt_    PVOID                   EventHandlerContext
)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetMicVolumeHandler]"));

    ASSERT(EventHandler == NULL || m_MicVolumeCallback.Handler == NULL);

    m_MicVolumeCallback.Handler = EventHandler; // weak ref.
    m_MicVolumeCallback.Context = EventHandlerContext;
}

//=============================================================================
#pragma code_seg("PAGE")
VOID
BthHfpDevice::SetMicConnectionStatusHandler
(
    _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
    _In_opt_    PVOID                   EventHandlerContext
)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetMicConnectionStatusHandler]"));

    ASSERT(EventHandler == NULL || m_MicConnectionStatusCallback.Handler == NULL);

    m_MicConnectionStatusCallback.Handler = EventHandler; // weak ref.
    m_MicConnectionStatusCallback.Context = EventHandlerContext;
}

//=============================================================================
#pragma code_seg("PAGE")
BOOL
BthHfpDevice::IsNRECSupported()
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::IsNRECSupported]"));

    return m_Descriptor->SupportsNREC;
}

//=============================================================================
#pragma code_seg("PAGE")
BOOL
BthHfpDevice::GetNRECDisableStatus()
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetNRECDisableStatus]"));

    // Return TRUE if HF wants to disable the NREC on the AG.
    return (BOOL)InterlockedCompareExchange(&m_NRECDisableStatusLong, 0, 0);
}

//
// Helper functions.
//

//=============================================================================
#pragma code_seg()
NTSTATUS
BthHfpDevice::SendIoCtrlAsynchronously
(
    _In_        WDFREQUEST      Request,
    _In_        ULONG           IoControlCode,
    _In_opt_    WDFMEMORY       MemIn,
    _In_opt_    WDFMEMORY       MemOut,
    _In_        PFN_WDF_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
    _In_        WDFCONTEXT      Context
)
/*++

Routine Description:

  This function aynchronously sends an I/O Ctrl request to the BTH HFP
  SCO Bypass device.

--*/
{
    DPF_ENTER(("[BthHfpDevice::SendIoCtrlAsynchronously]"));

    NTSTATUS    ntStatus    = STATUS_SUCCESS;
    BOOLEAN     fSent       = FALSE;

    //
    // Format and send the request.
    //
    ntStatus = WdfIoTargetFormatRequestForIoctl(
        m_WdfIoTarget,
        Request,
        IoControlCode,
        MemIn,
        NULL,
        MemOut,
        NULL);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("SendIoCtrlAsynchronously: WdfIoTargetFormatRequestForIoctl(0x%x) failed, 0x%x", IoControlCode, ntStatus)),
        Done);

    WdfRequestSetCompletionRoutine(
        Request,
        CompletionRoutine,
        Context);

    fSent = WdfRequestSend(Request, m_WdfIoTarget, NULL);  // no options.
    if (fSent == FALSE)
    {
        ntStatus = WdfRequestGetStatus(Request);
        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = STATUS_INVALID_DEVICE_STATE;
        }

        DPF(D_ERROR, ("SendIoCtrlAsynchronously: WdfRequestSend(0x%x) failed, 0x%x", IoControlCode, ntStatus));
        goto Done;
    }

    //
    // All Done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::SendIoCtrlSynchronously
(
    _In_opt_    WDFREQUEST  Request,
    _In_        ULONG       IoControlCode,
    _In_        ULONG       InLength,
    _In_        ULONG       OutLength,
    _When_(InLength > 0 || OutLength > 0, _In_)
    _When_(InLength == 0 && OutLength == 0, _In_opt_)
                PVOID       Buffer
)
/*++

Routine Description:

  This function inits and synchronously sends an I/O Ctrl request to the BTH HFP
  SCO Bypass device.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SendIoCtrlSynchronously]"));

    NTSTATUS                    ntStatus    = STATUS_SUCCESS;
    PWDF_MEMORY_DESCRIPTOR      memInPtr    = NULL;
    PWDF_MEMORY_DESCRIPTOR      memOutPtr   = NULL;
    WDF_MEMORY_DESCRIPTOR       memIn;
    WDF_MEMORY_DESCRIPTOR       memOut;
    WDF_REQUEST_SEND_OPTIONS    reqOpts;

    //
    // Format and send the request.
    //
    if (InLength)
    {
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memIn,  Buffer, InLength);
        memInPtr = &memIn;
    }

    if (OutLength)
    {
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memOut, Buffer, OutLength);
        memOutPtr = &memOut;
    }

    WDF_REQUEST_SEND_OPTIONS_INIT(
        &reqOpts,
        WDF_REQUEST_SEND_OPTION_TIMEOUT |
         WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);

    reqOpts.Timeout = WDF_REL_TIMEOUT_IN_SEC(BTH_HFP_SYNC_REQ_TIMEOUT_IN_SEC);

    ntStatus = WdfIoTargetSendIoctlSynchronously(
        m_WdfIoTarget,
        Request,
        IoControlCode,
        memInPtr,
        memOutPtr,
        &reqOpts,
        NULL);      // bytes returned.

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_VERBOSE, ("SendIoCtrlSynchronously: WdfIoTargetSendIoctlSynchronously(0x%x) failed, 0x%x", IoControlCode, ntStatus)),
        Done);

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
VOID
BthHfpDevice::EvtBthHfpDeviceNotificationStatusWorkItem
(
    _In_    WDFWORKITEM WorkItem
)
/*++

Routine Description:

  The function processes status notification updates.

Arguments:

  WorkItem    - WDF work-item object.

--*/
{
    DPF_ENTER(("[BthHfpDevice::EvtBthHfpDeviceNotificationStatusWorkItem]"));

    NTSTATUS            ntStatus = STATUS_SUCCESS;
    BthHfpDevice      * This;
    KIRQL               oldIrql;

    if (WorkItem == NULL)
    {
        return;
    }

    This = GetBthHfpDeviceNotificationWorkItemContext(WorkItem)->BthHfpDevice;
    ASSERT(This != NULL);

    for (;;)
    {
        BOOL                                    resend  = TRUE;
        WDFREQUEST                              req     = NULL;
        BthHfpDeviceNotificationReqContext    * reqCtx;
        WDF_REQUEST_COMPLETION_PARAMS           params;

        //
        // Retrieve a task.
        //
        KeAcquireSpinLock(&This->m_Lock, &oldIrql);
        req = (WDFREQUEST) WdfCollectionGetFirstItem(This->m_ReqCollection);
        if (req != NULL)
        {
            WdfCollectionRemove(This->m_ReqCollection, req);
        }
        KeReleaseSpinLock(&This->m_Lock, oldIrql);

        if (req == NULL)
        {
            break;
        }

        //
        // Get request parameters and context.
        //
        WDF_REQUEST_COMPLETION_PARAMS_INIT(&params);
        WdfRequestGetCompletionParams(req, &params);

        reqCtx = GetBthHfpDeviceNotificationReqContext(req);
        ASSERT(reqCtx != NULL);

        //
        // Handle this notification.
        //
        if (NT_SUCCESS(params.IoStatus.Status))
        {
            switch(params.Parameters.Ioctl.IoControlCode)
            {
            case IOCTL_BTHHFP_DEVICE_GET_NRECDISABLE_STATUS_UPDATE:
                {
                    InterlockedExchange(&This->m_NRECDisableStatusLong, (LONG)reqCtx->Buffer.BoolStatus);
                }
                break;

            case IOCTL_BTHHFP_SPEAKER_GET_VOLUME_STATUS_UPDATE:
                {
                    LONG oldVolume;

                    oldVolume = InterlockedExchange(&This->m_SpeakerVolumeLevel, reqCtx->Buffer.Volume);
                    if (reqCtx->Buffer.Volume != oldVolume)
                    {
                        // Notify audio miniport about this change.
                        if (This->m_SpeakerVolumeCallback.Handler != NULL)
                        {
                            This->m_SpeakerVolumeCallback.Handler(
                                This->m_SpeakerVolumeCallback.Context);
                        }
                    }
                }
                break;

            case IOCTL_BTHHFP_MIC_GET_VOLUME_STATUS_UPDATE:
                {
                    LONG oldVolume;

                    oldVolume = InterlockedExchange(&This->m_MicVolumeLevel, reqCtx->Buffer.Volume);
                    if (reqCtx->Buffer.Volume != oldVolume)
                    {
                        // Notify audio miniport about this change.
                        if (This->m_MicVolumeCallback.Handler != NULL)
                        {
                            This->m_MicVolumeCallback.Handler(
                                This->m_MicVolumeCallback.Context);
                        }
                    }
                }
                break;

            case IOCTL_BTHHFP_DEVICE_GET_CONNECTION_STATUS_UPDATE:
                {
                    BOOL oldStatus;

                    oldStatus = (BOOL)InterlockedExchange(&This->m_ConnectionStatusLong, (LONG)reqCtx->Buffer.BoolStatus);
                    if (reqCtx->Buffer.BoolStatus != oldStatus)
                    {
                        // Notify audio miniport about this change.
                        if (This->m_SpeakerConnectionStatusCallback.Handler != NULL)
                        {
                            This->m_SpeakerConnectionStatusCallback.Handler(
                                This->m_SpeakerConnectionStatusCallback.Context);
                        }

                        if (This->m_MicConnectionStatusCallback.Handler != NULL)
                        {
                            This->m_MicConnectionStatusCallback.Handler(
                                This->m_MicConnectionStatusCallback.Context);
                        }
                    }
                }
                break;

            default:
                // This should never happen.
                resend = FALSE;
                DPF(D_ERROR, ("EvtBthHfpDeviceNotificationStatusWorkItem: invalid request ctrl 0x%x",
                    params.Parameters.Ioctl.IoControlCode));
                break;
            }
        }

        if (resend)
        {
            WDF_REQUEST_REUSE_PARAMS    reuseParams;

            WDF_REQUEST_REUSE_PARAMS_INIT(
                &reuseParams,
                WDF_REQUEST_REUSE_NO_FLAGS,
                STATUS_SUCCESS);

            ntStatus = WdfRequestReuse(req, &reuseParams);
            if (!NT_SUCCESS(ntStatus))
            {
                DPF(D_ERROR, ("EvtBthHfpDeviceNotificationStatusWorkItem: WdfRequestReuse failed, 0x%x", ntStatus));
                break;
            }

            // Resend status notification request.
            reqCtx->Buffer.bImmediate = FALSE;

            ntStatus = This->SendIoCtrlAsynchronously(
                req,
                params.Parameters.Ioctl.IoControlCode,
                reqCtx->MemIn,
                reqCtx->MemOut,
                EvtBthHfpDeviceNotificationStatusCompletion,
                This);

            if (!NT_SUCCESS(ntStatus))
            {
                DPF(D_ERROR, ("EvtBthHfpDeviceNotificationStatusWorkItem: SendIoCtrlAsynchronously"
                              "(0x%x) failed, 0x%x",
                              params.Parameters.Ioctl.IoControlCode, ntStatus));
                break;
            }
        }
    }
}

//=============================================================================
#pragma code_seg()
void
BthHfpDevice::EvtBthHfpDeviceNotificationStatusCompletion
(
  _In_  WDFREQUEST Request,
  _In_  WDFIOTARGET Target,
  _In_  PWDF_REQUEST_COMPLETION_PARAMS Params,
  _In_  WDFCONTEXT Context
)
{
    DPF_ENTER(("[BthHfpDevice::EvtBthHfpDeviceNotificationStatusCompletion]"));

    NTSTATUS                                ntStatus    = STATUS_SUCCESS;
    BthHfpDeviceNotificationReqContext    * ctx         = NULL;
    BthHfpDevice                          * This        = NULL;
    KIRQL                                   oldIrql;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);

    ctx = GetBthHfpDeviceNotificationReqContext(Request);
    This = ctx->BthHfpDevice;
    ASSERT(This != NULL);

    ntStatus = Params->IoStatus.Status;
    if (ntStatus == STATUS_CANCELLED)
    {
        // BTH HFP device is shutting down. Do not re-send this request.
        goto Done;
    }

    //
    // If something is wrong with the HFP interface, do not loop forever.
    //
    if (!NT_SUCCESS(ntStatus))
    {
        if (++ctx->Errors > BTH_HFP_NOTIFICATION_MAX_ERROR_COUNT)
        {
            // Too many errors. Do not re-send this request.
            goto Done;
        }
    }
    else
    {
        // reset the # of errors.
        ctx->Errors = 0;
    }

    //
    // Let the work-item thread process this request.
    //
    KeAcquireSpinLock(&This->m_Lock, &oldIrql);

    ntStatus = WdfCollectionAdd(This->m_ReqCollection, Request);
    if (NT_SUCCESS(ntStatus))
    {
        WdfWorkItemEnqueue(This->m_WorkItem);
    }

    KeReleaseSpinLock(&This->m_Lock, oldIrql);

Done:;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::GetBthHfpDescriptor
(
    _Out_ PBTHHFP_DESCRIPTOR2 * Descriptor
)
/*++

Routine Description:

  This function synchronously gets the remote Bluetooth Hands-Free Profile SCO
  Bypass descriptor.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetBthHfpDescriptor]"));

    NTSTATUS                    ntStatus    = STATUS_SUCCESS;
    WDFREQUEST                  req         = NULL;
    PBTHHFP_DESCRIPTOR2         descriptor  = NULL;
    ULONG                       length      = 0;
    ULONG_PTR                   information = 0;
    WDF_REQUEST_REUSE_PARAMS    reuseParams;
    WDF_OBJECT_ATTRIBUTES       attributes;

    *Descriptor = NULL;

    //
    // Allocate and format a WDF request.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &req);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetBthHfpDescriptor: WdfRequestCreate failed, 0x%x", ntStatus)),
        Done);

    //
    // Get the size of the buffer.
    //
    ntStatus = SendIoCtrlSynchronously(
        req,
        IOCTL_BTHHFP_DEVICE_GET_DESCRIPTOR2,
        NULL,
        NULL,
        NULL);

    if (ntStatus != STATUS_BUFFER_TOO_SMALL)
    {
        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = STATUS_INVALID_DEVICE_STATE;
        }

        DPF(D_ERROR, ("GetBthHfpDescriptor: SendIoCtrlSynchronously(IOCTL_BTHHFP_DEVICE_GET_DESCRIPTOR2): failed, 0x%x", ntStatus));
        goto Done;
    }

    ntStatus = STATUS_SUCCESS;

    information = WdfRequestGetInformation(req);
    if (information == 0 || information > ULONG_MAX)
    {
        ntStatus = STATUS_INVALID_DEVICE_STATE;
        DPF(D_ERROR, ("GetBthHfpDescriptor: IOCTL_BTHHFP_DEVICE_GET_DESCRIPTOR2 buffer too big (%Id): 0x%x", information, ntStatus));
        goto Done;
    }

    length = (ULONG)information;

    //
    // Allocate memory needed to hold the info.
    //
    descriptor  = (PBTHHFP_DESCRIPTOR2) ExAllocatePoolWithTag(NonPagedPoolNx, length, MINADAPTER_POOLTAG);
    if (descriptor == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetBthHfpDescriptor: ExAllocatePoolWithTag failed, out of memory")),
        Done);

    //
    // Get the Bth HFP SCO Bypass descriptor.
    //
    WDF_REQUEST_REUSE_PARAMS_INIT(
        &reuseParams,
        WDF_REQUEST_REUSE_NO_FLAGS,
        STATUS_SUCCESS);

    ntStatus = WdfRequestReuse(req, &reuseParams);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetBthHfpDescriptor: WdfRequestReuse failed, 0x%x", ntStatus)),
        Done);

    ntStatus = SendIoCtrlSynchronously(
        req,
        IOCTL_BTHHFP_DEVICE_GET_DESCRIPTOR2,
        NULL,
        length,
        descriptor);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetBthHfpDescriptor: SendIoCtrlSynchronously(IOCTL_BTHHFP_DEVICE_GET_DESCRIPTOR2) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *Descriptor = descriptor;
    ntStatus = STATUS_SUCCESS;

Done:
    if (!NT_SUCCESS(ntStatus))
    {
        if (descriptor != NULL)
        {
            ExFreePoolWithTag(descriptor, MINADAPTER_POOLTAG);
        }
    }

    if (req != NULL)
    {
        WdfObjectDelete(req);
        req = NULL;
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
NTSTATUS
BthHfpDevice::EnableBthHfpNrecDisableStatusNotification()
/*++

Routine Description:

  This function registers for Bluetooth Hands-Free Profile SCO Bypass NREC-Disable
  status change notification.

--*/
{
    DPF_ENTER(("[BthHfpDevice::EnableBthHfpNrecDisableStatusNotification]"));

    NTSTATUS                                ntStatus    = STATUS_SUCCESS;
    BthHfpDeviceNotificationReqContext    * ctx         = NULL;

    ctx = GetBthHfpDeviceNotificationReqContext(m_NRECDisableStatusReq);

    //
    // This is a notification request.
    //
    ctx->Buffer.bImmediate = FALSE;

    //
    // Get the Bth HFP SCO Bypass NREC-Disable status (async).
    //
    ntStatus = SendIoCtrlAsynchronously(
        m_NRECDisableStatusReq,
        IOCTL_BTHHFP_DEVICE_GET_NRECDISABLE_STATUS_UPDATE,
        ctx->MemIn,
        ctx->MemOut,
        EvtBthHfpDeviceNotificationStatusCompletion,
        this);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("EnableBthHfpNrecDisableStatusNotification: SendIoCtrlAsynchronously(IOCTL_BTHHFP_DEVICE_GET_NRECDISABLE_STATUS_UPDATE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::GetBthHfpVolumePropertyValues
(
    _In_  ULONG                 Length,
    _Out_ PKSPROPERTY_VALUES  * PropValues
)
/*++

Routine Description:

  This function synchronously gets the remote Bluetooth Hands-Free Profile SCO
  Bypass volume values.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetBthHfpVolumePropertyValues]"));

    NTSTATUS            ntStatus    = STATUS_SUCCESS;
    PKSPROPERTY_VALUES  propValues  = NULL;

    *PropValues = NULL;

    //
    // Allocate memory.
    //
    propValues  = (PKSPROPERTY_VALUES) ExAllocatePoolWithTag(NonPagedPoolNx, Length, MINADAPTER_POOLTAG);
    if (propValues == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetBthHfpVolumePropertyValues: ExAllocatePoolWithTag failed, out of memory")),
        Done);

    //
    // Get the Bth HFP SCO Bypass descriptor.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_DEVICE_GET_VOLUMEPROPERTYVALUES,
        0,
        Length,
        propValues);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetBthHfpVolumePropertyValues: SendIoCtrlSynchronously(IOCTL_BTHHFP_DEVICE_GET_VOLUMEPROPERTYVALUES) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *PropValues = propValues;
    ntStatus = STATUS_SUCCESS;

Done:
    if (!NT_SUCCESS(ntStatus))
    {
        if (propValues != NULL)
        {
            ExFreePoolWithTag(propValues, MINADAPTER_POOLTAG);
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::SetBthHfpSpeakerVolume
(
    _In_ LONG  Volume
)
/*++

Routine Description:

  This function synchronously sets the remote Bluetooth Hands-Free Profile SCO
  Bypass speaker volume.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetBthHfpSpeakerVolume]"));

    NTSTATUS        ntStatus    = STATUS_SUCCESS;

    //
    // Get the Bth HFP SCO Bypass speaker volume.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_SPEAKER_SET_VOLUME,
        sizeof(Volume),
        0,
        &Volume);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("SetBthHfpSpeakerVolume: SendIoCtrlSynchronously(IOCTL_BTHHFP_SPEAKER_SET_VOLUME) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::GetBthHfpSpeakerVolume
(
    _Out_ LONG  * Volume
)
/*++

Routine Description:

  This function synchronously gets the remote Bluetooth Hands-Free Profile SCO
  Bypass speaker volume.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetBthHfpSpeakerVolume]"));

    NTSTATUS                        ntStatus    = STATUS_SUCCESS;
    BthHfpDeviceNotificationBuffer  buffer      = {0};

    *Volume = 0;

    buffer.bImmediate = TRUE;

    //
    // Get the Bth HFP SCO Bypass speaker volume.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_SPEAKER_GET_VOLUME_STATUS_UPDATE,
        sizeof(buffer.bImmediate),
        sizeof(buffer.Volume),
        &buffer);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetBthHfpSpeakerVolume: SendIoCtrlSynchronously(IOCTL_BTHHFP_SPEAKER_GET_VOLUME_STATUS_UPDATE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *Volume = buffer.Volume;
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
NTSTATUS
BthHfpDevice::EnableBthHfpSpeakerVolumeStatusNotification()
/*++

Routine Description:

  This function registers for Bluetooth Hands-Free Profile SCO Bypass speaker
  volume change notification.

--*/
{
    DPF_ENTER(("[BthHfpDevice::EnableBthHfpSpeakerVolumeStatusNotification]"));

    NTSTATUS                                ntStatus    = STATUS_SUCCESS;
    BthHfpDeviceNotificationReqContext    * ctx         = NULL;

    ctx = GetBthHfpDeviceNotificationReqContext(m_SpeakerVolumeReq);

    //
    // This is a notification request.
    //
    ctx->Buffer.bImmediate = FALSE;

    //
    // Register for speaker volume updates.
    //
    ntStatus = SendIoCtrlAsynchronously(
        m_SpeakerVolumeReq,
        IOCTL_BTHHFP_SPEAKER_GET_VOLUME_STATUS_UPDATE,
        ctx->MemIn,
        ctx->MemOut,
        EvtBthHfpDeviceNotificationStatusCompletion,
        this);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("EnableBthHfpSpeakerVolumeStatusNotification: SendIoCtrlAsynchronously(IOCTL_BTHHFP_SPEAKER_GET_VOLUME_STATUS_UPDATE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::SetBthHfpMicVolume
(
    _In_ LONG  Volume
)
/*++

Routine Description:

  This function synchronously sets the remote Bluetooth Hands-Free Profile SCO
  Bypass mic volume.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetBthHfpMicVolume]"));

    NTSTATUS        ntStatus    = STATUS_SUCCESS;

    //
    // Get the Bth HFP SCO Bypass mic volume.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_MIC_SET_VOLUME,
        sizeof(Volume),
        0,
        &Volume);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("SetBthHfpMicVolume: SendIoCtrlSynchronously(IOCTL_BTHHFP_MIC_SET_VOLUME) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::GetBthHfpMicVolume
(
    _Out_ LONG  * Volume
)
/*++

Routine Description:

  This function synchronously gets the remote Bluetooth Hands-Free Profile SCO
  Bypass mic volume.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetBthHfpMicVolume]"));

    NTSTATUS                        ntStatus    = STATUS_SUCCESS;
    BthHfpDeviceNotificationBuffer  buffer      = {0};

    *Volume = 0;

    buffer.bImmediate = TRUE;

    //
    // Get the Bth HFP SCO Bypass mic volume.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_MIC_GET_VOLUME_STATUS_UPDATE,
        sizeof(buffer.bImmediate),
        sizeof(buffer.Volume),
        &buffer);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetBthHfpMicVolume: SendIoCtrlSynchronously(IOCTL_BTHHFP_MIC_GET_VOLUME_STATUS_UPDATE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *Volume = buffer.Volume;
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
NTSTATUS
BthHfpDevice::EnableBthHfpMicVolumeStatusNotification()
/*++

Routine Description:

  This function registers for Bluetooth Hands-Free Profile SCO Bypass mic
  volume change notification.

--*/
{
    DPF_ENTER(("[BthHfpDevice::EnableBthHfpMicVolumeStatusNotification]"));

    NTSTATUS                               ntStatus    = STATUS_SUCCESS;
    BthHfpDeviceNotificationReqContext   * ctx         = NULL;

    ctx = GetBthHfpDeviceNotificationReqContext(m_MicVolumeReq);

    //
    // This is a notification request.
    //
    ctx->Buffer.bImmediate = FALSE;

    //
    // Register for mic volume updates.
    //
    ntStatus = SendIoCtrlAsynchronously(
        m_MicVolumeReq,
        IOCTL_BTHHFP_MIC_GET_VOLUME_STATUS_UPDATE,
        ctx->MemIn,
        ctx->MemOut,
        EvtBthHfpDeviceNotificationStatusCompletion,
        this);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("EnableBthHfpMicVolumeStatusNotification: SendIoCtrlAsynchronously(IOCTL_BTHHFP_MIC_GET_VOLUME_STATUS_UPDATE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::GetBthHfpConnectionStatus
(
    _Out_ BOOL * ConnectionStatus
)
/*++

Routine Description:

  This function synchronously gets the remote Bluetooth Hands-Free Profile SCO
  Bypass connection status.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetBthHfpConnectionStatus]"));

    NTSTATUS            ntStatus    = STATUS_SUCCESS;
    BOOL                bValue      = TRUE; // In: bImmediate, Out: value.

    *ConnectionStatus = 0;

    //
    // Get the Bth HFP SCO Bypass connection status.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_DEVICE_GET_CONNECTION_STATUS_UPDATE,
        sizeof(bValue),
        sizeof(bValue),
        &bValue);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetBthHfpConnectionStatus: SendIoCtrlSynchronously(IOCTL_BTHHFP_DEVICE_GET_CONNECTION_STATUS_UPDATE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *ConnectionStatus = bValue;
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
NTSTATUS
BthHfpDevice::EnableBthHfpConnectionStatusNotification()
/*++

Routine Description:

  This function registers for Bluetooth Hands-Free Profile SCO Bypass
  connection status notification.

--*/
{
    DPF_ENTER(("[BthHfpDevice::EnableBthHfpConnectionStatusNotification]"));

    NTSTATUS                               ntStatus    = STATUS_SUCCESS;
    BthHfpDeviceNotificationReqContext   * ctx         = NULL;

    ctx = GetBthHfpDeviceNotificationReqContext(m_ConnectionReq);

    //
    // Make sure this obj is alive while the IRP is active.
    //
    ctx->Buffer.bImmediate = FALSE;

    //
    // Get the Bth HFP SCO Bypass connection status.
    //
    ntStatus = SendIoCtrlAsynchronously(
        m_ConnectionReq,
        IOCTL_BTHHFP_DEVICE_GET_CONNECTION_STATUS_UPDATE,
        ctx->MemIn,
        ctx->MemOut,
        EvtBthHfpDeviceNotificationStatusCompletion,
        this);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("EnableBthHfpConnectionStatusNotification: SendIoCtrlAsynchronously(IOCTL_BTHHFP_DEVICE_GET_CONNECTION_STATUS_UPDATE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::SetBthHfpConnect()
/*++

Routine Description:

  This function synchronously requests a Bluetooth Hands-Free Profile level
  connection to the paired Bluetooth device.

  This request initiates the Service Level Connection establishment procedure
  and completes without waiting for the connection procedure to complete.
  Connection status can be determined using IOCTL_BTHHFP_GET_CONNECTION_STATUS_UPDATE.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetBthHfpConnect]"));

    NTSTATUS        ntStatus    = STATUS_SUCCESS;

    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_DEVICE_REQUEST_CONNECT,
        0,
        0,
        NULL);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("SetBthHfpConnect: SendIoCtrlSynchronously(IOCTL_BTHHFP_DEVICE_REQUEST_CONNECT) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::SetBthHfpDisconnect()
/*++

Routine Description:

  This function synchronously requests a Bluetooth Hands-Free Profile level
  connection to the paired Bluetooth device.

  This request initiates disconnection of the Service Level Connection and
  completes without waiting for the disconnection to complete. Connection
  status can be determined using IOCTL_BTHHFP_GET_CONNECTION_STATUS_UPDATE.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetBthHfpDisconnect]"));

    NTSTATUS        ntStatus    = STATUS_SUCCESS;

    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_DEVICE_REQUEST_DISCONNECT,
        0,
        0,
        NULL);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("SetBthHfpDisconnect: SendIoCtrlSynchronously(IOCTL_BTHHFP_DEVICE_REQUEST_DISCONNECT) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
void
BthHfpDevice::EvtBthHfpDeviceStreamStatusCompletion
(
  _In_  WDFREQUEST Request,
  _In_  WDFIOTARGET Target,
  _In_  PWDF_REQUEST_COMPLETION_PARAMS Params,
  _In_  WDFCONTEXT Context
)
/*++

Routine Description:

  Completion callback for the Bluetooth Hands-Free Profile SCO Bypass
  stream status notification.

--*/
{
    DPF_ENTER(("[BthHfpDevice::EvtBthHfpDeviceStreamStatusCompletion]"));

    NTSTATUS                                ntStatus    = STATUS_SUCCESS;
    BthHfpDeviceNotificationReqContext    * reqCtx      = NULL;
    BthHfpDevice                          * This        = NULL;
    NTSTATUS                                ntResult    = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);

    //
    // Get the SCO stream status.
    //
    reqCtx = GetBthHfpDeviceNotificationReqContext(Request);
    This = reqCtx->BthHfpDevice;
    ASSERT(This != NULL);

    ntStatus = Params->IoStatus.Status;
    if (!NT_SUCCESS(ntStatus))
    {
        ntResult = STATUS_INVALID_DEVICE_STATE;
    }
    else
    {
        ntResult = reqCtx->Buffer.NtStatus;
    }

    InterlockedExchange(&This->m_StreamStatusLong, (LONG)ntResult);

    //
    // Let the stop routine know we are done. Stop routine will
    // re-init the request.
    //
    KeSetEvent(&This->m_StreamStatusEvent, IO_NO_INCREMENT, FALSE);
}

//=============================================================================
#pragma code_seg()
NTSTATUS
BthHfpDevice::EnableBthHfpStreamStatusNotification()
/*++

Routine Description:

  This function registers for Bluetooth Hands-Free Profile SCO Bypass
  stream status notification.

--*/
{
    DPF_ENTER(("[BthHfpDevice::EnableBthHfpStreamStatusNotification]"));

    NTSTATUS                               ntStatus    = STATUS_SUCCESS;
    BthHfpDeviceNotificationReqContext   * ctx         = NULL;

    ASSERT(m_nStreams > 0);

    ctx = GetBthHfpDeviceNotificationReqContext(m_StreamReq);
    ctx->Buffer.bImmediate = FALSE;

    KeClearEvent(&m_StreamStatusEvent);

    //
    // Get the Bth HFP SCO Bypass connection status.
    //
    ntStatus = SendIoCtrlAsynchronously(
        m_StreamReq,
        IOCTL_BTHHFP_STREAM_GET_STATUS_UPDATE,
        ctx->MemIn,
        ctx->MemOut,
        EvtBthHfpDeviceStreamStatusCompletion,
        this);

    if (!NT_SUCCESS(ntStatus))
    {
        KeSetEvent(&m_StreamStatusEvent, IO_NO_INCREMENT, FALSE);
        DPF(D_ERROR, ("EnableBthHfpStreamStatusNotification: SendIoCtrlAsynchronously(IOCTL_BTHHFP_STREAM_GET_STATUS_UPDATE) failed, 0x%x", ntStatus));
        goto Done;
    }

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::StopBthHfpStreamStatusNotification()
/*++

Routine Description:

  This function stops the Bluetooth Hands-Free Profile SCO Bypass
  connection status notification.
  The function waits for the request to be done before returning.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::StopBthHfpStreamStatusNotification]"));

    NTSTATUS                                ntStatus    = STATUS_SUCCESS;
    BthHfpDeviceNotificationReqContext    * reqCtx      = NULL;
    WDF_REQUEST_REUSE_PARAMS                reuseParams;

    WdfRequestCancelSentRequest(m_StreamReq);
    KeWaitForSingleObject(&m_StreamStatusEvent, Executive, KernelMode, FALSE, NULL);

    reqCtx = GetBthHfpDeviceNotificationReqContext(m_StreamReq);
    ASSERT(reqCtx != NULL);
    UNREFERENCED_VAR(reqCtx);

    //
    // Re-init the request for later.
    //
    WDF_REQUEST_REUSE_PARAMS_INIT(
        &reuseParams,
        WDF_REQUEST_REUSE_NO_FLAGS,
        STATUS_SUCCESS);

    ntStatus = WdfRequestReuse(m_StreamReq, &reuseParams);
    if (!NT_SUCCESS(ntStatus))
    {
        DPF(D_ERROR, ("StopBthHfpStreamStatusNotification: WdfRequestReuse failed, 0x%x", ntStatus));
    }

    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::SetBthHfpStreamOpen()
/*++

Routine Description:

  This function synchronously requests an open SCO channel to transmit audio
  data over the air.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetBthHfpStreamOpen]"));

    NTSTATUS        ntStatus    = STATUS_SUCCESS;

    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_STREAM_OPEN,
        0,
        0,
        NULL);

    if (ntStatus == STATUS_DEVICE_BUSY)
    {
        // The stream channel is already open.
        DPF(D_VERBOSE, ("SetBthHfpStreamOpen: the stream channel is already open"));
        ntStatus = STATUS_SUCCESS;
    }

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("SetBthHfpStreamOpen: SendIoCtrlSynchronously(IOCTL_BTHHFP_STREAM_OPEN) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::SetBthHfpStreamClose()
/*++

Routine Description:

  This function synchronously requests to close the SCO channel.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetBthHfpStreamClose]"));

    NTSTATUS        ntStatus    = STATUS_SUCCESS;

    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_STREAM_CLOSE,
        0,
        0,
        NULL);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("SetBthHfpStreamClose: SendIoCtrlSynchronously(IOCTL_BTHHFP_STREAM_CLOSE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
VOID
BthHfpDevice::Start()
/*++

Routine Description:

  Asynchronously called to start the audio device.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::Start]"));

    NTSTATUS            ntStatus            = STATUS_SUCCESS;
    BOOL                connStatus          = FALSE;
    UCHAR               codecId             = 0;
    UINT                bthMiniportsIndex   = 0;

    // CVSD is the narrow band codec for SCO. Wideband codec IDs are any number higher than 1.
    // mSBC is the only required wideband codec, though the controller+headset combination may
    // support other wideband codecs.
    const UCHAR         CODEC_CVSD          = 1;

    //
    // Get bth hfp descriptor
    //
    ntStatus = GetBthHfpDescriptor(&m_Descriptor);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: GetBthHfpDescriptor: failed to retrieve BTHHFP_DESCRIPTOR2, 0x%x", ntStatus)),
        Done);

    //
    // Get valume settings.
    //
    if (m_Descriptor->SupportsVolume)
    {
        PKSPROPERTY_VALUES  volumePropValues    = NULL;
        LONG                volume              = 0;

        // Volume settings.
        ntStatus = GetBthHfpVolumePropertyValues(
            m_Descriptor->VolumePropertyValuesSize,
            &volumePropValues);

        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("Start: GetBthHfpVolumePropertyValues: failed to retrieve KSPROPERTY_VALUES, 0x%x", ntStatus)),
            Done);

        m_VolumePropValues = volumePropValues;

        // Speaker volume.
        ntStatus = GetBthHfpSpeakerVolume(&volume);
        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("Start: GetBthHfpSpeakerVolume: failed, 0x%x", ntStatus)),
            Done);

        m_SpeakerVolumeLevel = volume;

        // Mic volume.
        ntStatus = GetBthHfpMicVolume(&volume);
        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("Start: GetBthHfpMicVolume: failed, 0x%x", ntStatus)),
            Done);

        m_MicVolumeLevel = volume;
    }

    //
    // Get connection status.
    //
    ntStatus = GetBthHfpConnectionStatus(&connStatus);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: GetBthHfpConnectionStatus: failed, 0x%x", ntStatus)),
        Done);

    m_ConnectionStatus = connStatus;

    //
    // Get codec id (if non-zero, connection supports Wideband Speech)
    //
    ntStatus = GetBthHfpCodecId(&codecId);
    if (ntStatus == STATUS_INVALID_DEVICE_REQUEST)
    {
        // GetBthHfpCodecId fails with STATUS_INVALID_DEVICE_REQUEST if the system doesn't
        // support Wideband Speech (currently only Mobile supports this call)
        ntStatus = STATUS_SUCCESS;
    }
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: GetBthHfpCodecId: failed, 0x%x", ntStatus)),
        Done);

    if (codecId > CODEC_CVSD)
    {
        // Use the miniport tables that support 16kHz
        bthMiniportsIndex = 1;
    }

    //
    // Customize the topology/wave descriptors for this instance
    //
    ntStatus = CreateCustomEndpointMinipair(
        g_BthHfpRenderEndpoints[bthMiniportsIndex],
        &m_Descriptor->FriendlyName,
        &m_Descriptor->OutputPinCategory,
        &m_SpeakerMiniports);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: CreateCustomEndpointMinipair for Render: failed, 0x%x", ntStatus)),
        Done);

    ntStatus = CreateCustomEndpointMinipair(
        g_BthHfpCaptureEndpoints[bthMiniportsIndex],
        &m_Descriptor->FriendlyName,
        &m_Descriptor->InputPinCategory,
        &m_MicMiniports);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: CreateCustomEndpointMinipair for Capture: failed, 0x%x", ntStatus)),
        Done);

    ASSERT(m_SpeakerMiniports != NULL);
    ASSERT(m_MicMiniports != NULL);
    _Analysis_assume_(m_SpeakerMiniports != NULL);
    _Analysis_assume_(m_MicMiniports != NULL);

    //
    // Register topology and wave filters.
    //
    ntStatus = m_Adapter->InstallEndpointFilters(
        NULL,
        m_SpeakerMiniports,
        PBTHHFPDEVICECOMMON(this),
        &m_UnknownSpeakerTopology,
        &m_UnknownSpeakerWave, NULL, NULL
        );

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: InstallEndpointRenderFilters (Bth HFP SCO-Bypass): failed, 0x%x", ntStatus)),
        Done);

    ntStatus = m_Adapter->InstallEndpointFilters(
        NULL,
        m_MicMiniports,
        PBTHHFPDEVICECOMMON(this),
        &m_UnknownMicTopology,
        &m_UnknownMicWave, NULL, NULL
        );

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: InstallEndpointCaptureFilters (Bth HFP SCO-Bypass): failed, 0x%x", ntStatus)),
        Done);

    //
    // Pend status notifications.
    //

    // NREC disable AudioGateway (AG) status.
    ntStatus = EnableBthHfpNrecDisableStatusNotification();
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: EnableBthHfpNrecDisableStatusNotification: failed, 0x%x", ntStatus)),
        Done);

    // Volume speaker status.
    ntStatus = EnableBthHfpSpeakerVolumeStatusNotification();
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: EnableBthHfpSpeakerVolumeStatusNotification: failed, 0x%x", ntStatus)),
        Done);

    // Volume mic status.
    ntStatus = EnableBthHfpMicVolumeStatusNotification();
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: EnableBthHfpMicVolumeStatusNotification: failed, 0x%x", ntStatus)),
        Done);

    // Connection status.
    ntStatus = EnableBthHfpConnectionStatusNotification();
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: EnableBthHfpConnectionStatusNotification: failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:;
    if (!NT_SUCCESS(ntStatus))
    {
        InterlockedExchange((PLONG)&m_State, eBthHfpStateFailed);
    }
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::CreateCustomEndpointMinipair
(
    _In_        PENDPOINT_MINIPAIR pBaseMinipair,
    _In_        PUNICODE_STRING FriendlyName,
    _In_        PGUID pCategory,
    _Outptr_    PENDPOINT_MINIPAIR *ppCustomMinipair
)
{
    NTSTATUS ntStatus;
    PENDPOINT_MINIPAIR  pNewMinipair = NULL;
    SYSVAD_DEVPROPERTY* pProperties = NULL;
    PPCFILTER_DESCRIPTOR pNewTopoFilterDesc = NULL;
    PPCPIN_DESCRIPTOR   pNewTopoPins = NULL;
    ULONG cProperties;
    ULONG cTopoPins;

    PAGED_CODE();

    //
    // This routine will add one more property to whatever the base minipair describes for the topo filter interface properties
    // It will also allocate and set up custom filter and pin descriptors to allow changing the KSNODETYPE for the hfp device
    //
    cTopoPins = pBaseMinipair->TopoDescriptor->PinCount;
    cProperties = pBaseMinipair->TopoInterfacePropertyCount + 1;
    pProperties = (SYSVAD_DEVPROPERTY*)ExAllocatePoolWithTag(NonPagedPoolNx, cProperties * sizeof(SYSVAD_DEVPROPERTY), SYSVAD_POOLTAG);
    pNewMinipair = (ENDPOINT_MINIPAIR*)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(ENDPOINT_MINIPAIR), SYSVAD_POOLTAG);
    pNewTopoFilterDesc = (PCFILTER_DESCRIPTOR*)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(PCFILTER_DESCRIPTOR), SYSVAD_POOLTAG);
    pNewTopoPins = (PCPIN_DESCRIPTOR*)ExAllocatePoolWithTag(NonPagedPoolNx, cTopoPins * sizeof(PCPIN_DESCRIPTOR), SYSVAD_POOLTAG);

    if ((pProperties != NULL) && (pNewMinipair != NULL) && (pNewTopoFilterDesc != NULL) && (pNewTopoPins != NULL))
    {
        SYSVAD_DEVPROPERTY *pLastProperty;

        // Copy base minipair properties to new property list
        if (pBaseMinipair->TopoInterfacePropertyCount > 0)
        {
            RtlCopyMemory(pProperties, pBaseMinipair->TopoInterfaceProperties, (cProperties - 1) * sizeof(SYSVAD_DEVPROPERTY));
        }

        // Add friendly name property to the list
        NT_ASSERT(FriendlyName->Length + sizeof(UNICODE_NULL) <= FriendlyName->MaximumLength);  // Assuming NULL terminated string
        pLastProperty = &pProperties[cProperties - 1];
        pLastProperty->PropertyKey = &DEVPKEY_DeviceInterface_FriendlyName;
        pLastProperty->Type = DEVPROP_TYPE_STRING_INDIRECT;
        pLastProperty->BufferSize = FriendlyName->Length + sizeof(UNICODE_NULL);
        pLastProperty->Buffer = FriendlyName->Buffer;

        // Copy base minipair structure
        RtlCopyMemory(pNewMinipair, pBaseMinipair, sizeof(ENDPOINT_MINIPAIR));

        RtlCopyMemory(pNewTopoFilterDesc, pBaseMinipair->TopoDescriptor, sizeof(PCFILTER_DESCRIPTOR));
        RtlCopyMemory(pNewTopoPins, pBaseMinipair->TopoDescriptor->Pins, cTopoPins * sizeof(PCPIN_DESCRIPTOR));

        pNewTopoFilterDesc->Pins = pNewTopoPins;
        pNewMinipair->TopoDescriptor = pNewTopoFilterDesc;

        // Update it to point to new property list
        pNewMinipair->TopoInterfacePropertyCount = cProperties;
        pNewMinipair->TopoInterfaceProperties = pProperties;

        ntStatus = UpdateCustomEndpointCategory(pNewTopoFilterDesc, pNewTopoPins, pCategory);
        if (!NT_SUCCESS(ntStatus))
        {
            DPF(D_ERROR, ("UpdateCustomEndpointCategory: failed, 0x%x", ntStatus));
        }
        else
        {
            *ppCustomMinipair = pNewMinipair;

            pProperties = NULL;
            pNewMinipair = NULL;
            pNewTopoFilterDesc = NULL;
            pNewTopoPins = NULL;

            ntStatus = STATUS_SUCCESS;
        }
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (pProperties != NULL)
    {
        ExFreePoolWithTag(pProperties, SYSVAD_POOLTAG);
    }
    if (pNewMinipair != NULL)
    {
        ExFreePoolWithTag(pNewMinipair, SYSVAD_POOLTAG);
    }
    if (pNewTopoFilterDesc != NULL)
    {
        ExFreePoolWithTag(pNewTopoFilterDesc, SYSVAD_POOLTAG);
    }
    if (pNewTopoPins != NULL)
    {
        ExFreePoolWithTag(pNewTopoPins, SYSVAD_POOLTAG);
    }

    return ntStatus;
}
//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::UpdateCustomEndpointCategory
(
    _In_        PPCFILTER_DESCRIPTOR pCustomMinipairTopoFilter,
    _In_        PPCPIN_DESCRIPTOR pCustomMinipairTopoPins,
    _In_        PGUID pCategory
)
{
    NTSTATUS ntStatus = STATUS_NOT_FOUND;
    ULONG cPinCount = 0;
    BOOL FoundCategoryAudio = FALSE;
    BOOL FoundNodeType = FALSE;

    PAGED_CODE();

    cPinCount = pCustomMinipairTopoFilter->PinCount;

    // Find the right pin: There should be two pins, one with Category KSCATEGORY_AUDIO,
    // and one with a KSNODETYPE_* Category. We need to modify the KSNODETYPE category.
    for (ULONG i = 0; i < cPinCount; ++i)
    {
        if (IsEqualGUID(*pCustomMinipairTopoPins[i].KsPinDescriptor.Category, KSCATEGORY_AUDIO))
        {
            ASSERT(FoundCategoryAudio == FALSE);
            if (FoundCategoryAudio)
            {
                ntStatus = STATUS_INVALID_DEVICE_STATE;
                DPF(D_ERROR, ("UpdateCustomEndpointCategory: KSCATEGORY_AUDIO found more than once, 0x%x", ntStatus));
                break;
            }

            FoundCategoryAudio = TRUE;
            continue;
        }

        ASSERT(FoundNodeType == FALSE);
        if (FoundNodeType)
        {
            ntStatus = STATUS_INVALID_DEVICE_STATE;
            DPF(D_ERROR, ("UpdateCustomEndpointCategory: Found more than one applicable Pin, 0x%x", ntStatus));
            break;
        }

        pCustomMinipairTopoPins[i].KsPinDescriptor.Category = pCategory;
        FoundNodeType = TRUE;
        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}
//=============================================================================
#pragma code_seg("PAGE")
VOID
BthHfpDevice::DeleteCustomEndpointMinipair
(
    _In_        PENDPOINT_MINIPAIR pCustomMinipair
)
{
    PAGED_CODE();

    if (pCustomMinipair != NULL)
    {
        if (pCustomMinipair->TopoInterfaceProperties != NULL)
        {
            ExFreePoolWithTag(const_cast<SYSVAD_DEVPROPERTY*>(pCustomMinipair->TopoInterfaceProperties), SYSVAD_POOLTAG);
            pCustomMinipair->TopoInterfaceProperties = NULL;
        }
        if (pCustomMinipair->TopoDescriptor != NULL)
        {
            if (pCustomMinipair->TopoDescriptor->Pins != NULL)
            {
                ExFreePoolWithTag((PVOID)pCustomMinipair->TopoDescriptor->Pins, SYSVAD_POOLTAG);
                pCustomMinipair->TopoDescriptor->Pins = NULL;
            }
            ExFreePoolWithTag(pCustomMinipair->TopoDescriptor, SYSVAD_POOLTAG);
            pCustomMinipair->TopoDescriptor = NULL;
        }
        ExFreePoolWithTag(pCustomMinipair, SYSVAD_POOLTAG);
    }
}

//=============================================================================
#pragma code_seg("PAGE")
VOID
BthHfpDevice::Stop()
/*++

Routine Description:

  Asynchronously called to stop the audio device.
  After returning from this function, there are no more async notifications
  pending (volume, connection, etc.).

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::Stop]"));

    NTSTATUS        ntStatus    = STATUS_SUCCESS;
    eBthHfpState    state       = eBthHfpStateInvalid;

    state = (eBthHfpState) InterlockedExchange((PLONG)&m_State, eBthHfpStateStopping);
    ASSERT(state == eBthHfpStateRunning || state == eBthHfpStateFailed);
    UNREFERENCED_VAR(state);

    //
    // Stop async notifications.
    //
    WdfIoTargetPurge(m_WdfIoTarget, WdfIoTargetPurgeIoAndWait);

    //
    // Wait for work-item.
    //
    WdfWorkItemFlush(m_WorkItem);

    //
    // Remove the topology and wave render filters.
    //
    if (m_UnknownSpeakerTopology || m_UnknownSpeakerWave)
    {
        ntStatus = m_Adapter->RemoveEndpointFilters(
            m_SpeakerMiniports,
            m_UnknownSpeakerTopology,
            m_UnknownSpeakerWave);

        if (!NT_SUCCESS(ntStatus))
        {
            DPF(D_ERROR, ("RemoveEndpointFilters (Bth HFP SCO-Bypass Speaker): failed, 0x%x", ntStatus));
        }
    }

    //
    // Remove the topology and wave capture filters.
    //
    if (m_UnknownMicTopology || m_UnknownMicWave)
    {
        ntStatus = m_Adapter->RemoveEndpointFilters(
            m_MicMiniports,
            m_UnknownMicTopology,
            m_UnknownMicWave);

        if (!NT_SUCCESS(ntStatus))
        {
            DPF(D_ERROR, ("RemoveEndpointFilters (Bth HFP SCO-Bypass Capture): failed, 0x%x", ntStatus));
        }
    }

    //
    // Release port/miniport pointers.
    //
    SAFE_RELEASE(m_UnknownSpeakerTopology);
    SAFE_RELEASE(m_UnknownSpeakerWave);
    SAFE_RELEASE(m_UnknownMicTopology);
    SAFE_RELEASE(m_UnknownMicWave);

    //
    // The device is in the stopped state.
    //
    InterlockedExchange((PLONG)&m_State, eBthHfpStateStopped);

    DeleteCustomEndpointMinipair(m_SpeakerMiniports);
    m_SpeakerMiniports = NULL;

    DeleteCustomEndpointMinipair(m_MicMiniports);
    m_MicMiniports = NULL;

}

#endif // SYSVAD_BTH_BYPASS

//
// The following are copied from:
// \Program Files\Windows Kits\10\Include\10.0.15063.0\km\stdunk.h
//

#ifdef _NEW_DELETE_OPERATORS_

// Note: Since VS2015 Update 2 overloaded operator new and operator delete may not
// be declared inline (Level 1 (/W1) on-by-default, warning C4595).
// See https://msdn.microsoft.com/en-us/library/mt656697.aspx
//
// To mitigate this issue, add "#define _NEW_DELETE_OPERATORS_" before "#include <stdunk.h>"
// and implement non-inline operator new and operator delete locally.

/*****************************************************************************
 * ::new()
 *****************************************************************************
 * New function for creating objects with a specified allocation tag.
 */
// PVOID operator new
// (
//     size_t          iSize,
//     _When_((poolType & NonPagedPoolMustSucceed) != 0,
//        __drv_reportError("Must succeed pool allocations are forbidden. "
//              "Allocation failures cause a system crash"))
//     POOL_TYPE       poolType
// )
// {
//     PVOID result = ExAllocatePoolWithTag(poolType,iSize,'wNcP');
//
//     if (result)
//     {
//         RtlZeroMemory(result,iSize);
//     }
//
//     return result;
// }

/*****************************************************************************
 * ::new()
 *****************************************************************************
 * New function for creating objects with a specified allocation tag.
 */
PVOID operator new
(
    size_t          iSize,
    _When_((poolType & NonPagedPoolMustSucceed) != 0,
       __drv_reportError("Must succeed pool allocations are forbidden. "
             "Allocation failures cause a system crash"))
    POOL_TYPE       poolType,
    ULONG           tag
)
{
    PVOID result = ExAllocatePoolWithTag(poolType,iSize,tag);

    if (result)
    {
        RtlZeroMemory(result,iSize);
    }

    return result;
}

/*****************************************************************************
 * ::delete()
 *****************************************************************************
 * Delete function.
 */
// void __cdecl operator delete
// (
//     PVOID pVoid
// )
// {
//     if (pVoid)
//     {
//         ExFreePool(pVoid);
//     }
// }

/*****************************************************************************
 * ::delete()
 *****************************************************************************
 * Delete function.
 */
// void __cdecl operator delete
// (
//     PVOID pVoid,
//     ULONG tag
// )
// {
//     if (pVoid)
//     {
//         ExFreePoolWithTag(pVoid,tag);
//     }
// }

void __cdecl operator delete
(
    _Pre_maybenull_ __drv_freesMem(Mem) PVOID pVoid,
    _In_ size_t cbSize
)
{
    UNREFERENCED_PARAMETER(cbSize);

    if (pVoid)
    {
        ExFreePool(pVoid);
    }
}

void __cdecl operator delete[]
(
    _Pre_maybenull_ __drv_freesMem(Mem) PVOID pVoid
)
{
    if (pVoid)
    {
        ExFreePool(pVoid);
    }
}

// void __cdecl operator delete[]
// (
//     _Pre_maybenull_ __drv_freesMem(Mem) PVOID pVoid,
//     _In_ size_t cbSize
// )
// {
//     UNREFERENCED_PARAMETER(cbSize);

//     if (pVoid)
//     {
//         ExFreePool(pVoid);
//     }
// }

#endif //!_NEW_DELETE_OPERATORS_
