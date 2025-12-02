//
// This class represents a the Bluetooth Hands-Free Profile SCO Bypass device.
// There is one class for each interface (id = symbolic-link-name).
//
#ifdef SYSVAD_BTH_BYPASS

//=====================================================================
//
// CAdapterCommon: Bluetooth Hands-Free Profile SCO Bypass definitions.
//

class CAdapterCommon;
class BthHfpDevice;

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
    ISidebandDeviceCommon,
    public CUnknown
{
private:
    eBthHfpState            m_State;

    IAdapterCommon        * m_Adapter;
    WDFIOTARGET             m_WdfIoTarget;

    LIST_ENTRY              m_ListEntry;
    UNICODE_STRING          m_SymbolicLinkName;
    WCHAR                   m_SpeakerWaveNameBuffer[BTHHFP_INTERFACE_REFSTRING_MAX_LENGTH];
    UNICODE_STRING          m_SpeakerWaveRefString;
    WCHAR                   m_SpeakerTopologyNameBuffer[BTHHFP_INTERFACE_REFSTRING_MAX_LENGTH];
    UNICODE_STRING          m_SpeakerTopologyRefString;
    WCHAR                   m_MicWaveNameBuffer[BTHHFP_INTERFACE_REFSTRING_MAX_LENGTH];
    UNICODE_STRING          m_MicWaveRefString;
    WCHAR                   m_MicTopologyNameBuffer[BTHHFP_INTERFACE_REFSTRING_MAX_LENGTH];
    UNICODE_STRING          m_MicTopologyRefString;

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
    LONG                   m_CodecId;

    PBTHHFP_DESCRIPTOR2     m_Descriptor;
    PKSPROPERTY_VALUES      m_VolumePropValues;
    LONG                    m_SpeakerVolumeLevel;
    LONG                    m_MicVolumeLevel;
    union {
        BOOL                m_ConnectionStatus;
        LONG                m_ConnectionStatusLong;
    }; // unnamed.

    union {
        NTSTATUS            m_StreamStatus;
        LONG                m_StreamStatusLong;
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
    BthHfpEventCallback     m_SpeakerMuteCallback;
    BthHfpEventCallback     m_SpeakerConnectionStatusCallback;
    BthHfpEventCallback     m_SpeakerFormatChangeCallback;
    BthHfpEventCallback     m_MicVolumeCallback;
    BthHfpEventCallback     m_MicMuteCallback;
    BthHfpEventCallback     m_MicConnectionStatusCallback;
    BthHfpEventCallback     m_MicFormatChangeCallback;

public:
    //=====================================================================
    // Default CUnknown
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(BthHfpDevice);
    ~BthHfpDevice();

    NTSTATUS Init
    (
        _In_ IAdapterCommon  *Adapter,
        _In_ PUNICODE_STRING SymbolicLinkName
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
    // ISidebandDeviceCommon functions. 
    //
    STDMETHODIMP_(BOOL)                 IsVolumeSupported
    (
        _In_        eDeviceType             deviceType
    );

    STDMETHODIMP_(PVOID)                GetVolumeSettings
    (
        _In_        eDeviceType             DeviceType,
        _Out_       PULONG                  Size
    );

    STDMETHODIMP_(NTSTATUS)             GetVolume
    (
        _In_        eDeviceType             DeviceType,
        _In_        LONG                    Channel,
        _Out_       LONG                    *pVolume
    );

    STDMETHODIMP_(NTSTATUS)             SetVolume
    (
        _In_        eDeviceType             DeviceType,
        _In_        LONG                    Channel,
        _In_        LONG                    Volume
    );

    STDMETHODIMP_(BOOL)                 IsMuteSupported
    (
        _In_        eDeviceType             deviceType
    );

    STDMETHODIMP_(PVOID)                GetMuteSettings
    (
        _In_        eDeviceType             DeviceType,
        _Out_       PULONG                  Size
    );

    STDMETHODIMP_(LONG)                 GetMute
    (
        _In_        eDeviceType             DeviceType,
        _In_        LONG                    Channel
    );

    STDMETHODIMP_(NTSTATUS)             SetMute
    (
        _In_        eDeviceType             DeviceType,
        _In_        LONG                    Channel,
        _In_        LONG                    Mute
    );

    STDMETHODIMP_(BOOL)                 GetConnectionStatus();

    STDMETHODIMP_(NTSTATUS)             Connect();

    STDMETHODIMP_(NTSTATUS)             Disconnect();

    STDMETHODIMP_(BOOL)                 GetStreamStatus(_In_ eDeviceType deviceType);

    STDMETHODIMP_(NTSTATUS)             StreamOpen(_In_ eDeviceType deviceType);

    STDMETHODIMP_(NTSTATUS)             StreamStart(_In_ eDeviceType deviceType);

    STDMETHODIMP_(NTSTATUS)             StreamSuspend(_In_ eDeviceType deviceType);

    STDMETHODIMP_(NTSTATUS)             StreamClose(_In_ eDeviceType deviceType);

    STDMETHODIMP_(GUID)                 GetContainerId(_In_ eDeviceType deviceType);

    STDMETHODIMP_(VOID)                 SetVolumeHandler
    (
        _In_        eDeviceType             deviceType,
        _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
        _In_opt_    PVOID                   EventHandlerContext
    );

    STDMETHODIMP_(VOID)                 SetMuteHandler
    (
        _In_        eDeviceType             deviceType,
        _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
        _In_opt_    PVOID                   EventHandlerContext
    );

    STDMETHODIMP_(VOID)                 SetConnectionStatusHandler
    (
        _In_        eDeviceType             deviceType,
        _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
        _In_opt_    PVOID                   EventHandlerContext
    );

    STDMETHODIMP_(VOID)                 SetFormatChangeHandler
    (
        _In_        eDeviceType             deviceType,
        _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
        _In_opt_    PVOID                   EventHandlerContext
    );

    STDMETHODIMP_(PPIN_DEVICE_FORMATS_AND_MODES)    GetFormatsAndModes
    (
        _In_        eDeviceType             deviceType
    );

    _IRQL_requires_max_(DISPATCH_LEVEL)
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

    NTSTATUS    SetBthHfpDeviceCapabilities
    (
        _In_    BOOL bSupports16kHzSampling
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

    NTSTATUS    CreateFilterNames(
        _In_ PUNICODE_STRING HfpDeviceSymbolicLinkName
    );

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
