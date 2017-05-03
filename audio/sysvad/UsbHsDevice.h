//
// This class represents a the Bluetooth Hands-Free Profile SCO Bypass device.
// There is one class for each interface (id = symbolic-link-name).
//
//#ifdef SYSVAD_USB_SIDEBAND


//=====================================================================
//
//  USB Sideband Headset class definition.
//

class CAdapterCommon;
class UsbHsDevice;

struct UsbHsWorkItemContext
{
    CAdapterCommon *    Adapter;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME
(
    UsbHsWorkItemContext,
    GetUsbHsWorkItemContext
)

// start/stop the device
enum eUsbHsTaskAction
{
    eUsbHsTaskStart    = 1,
    eUsbHsTaskStop     = 2,
};

struct UsbHsWorkTask
{
    LIST_ENTRY          ListEntry;
    UsbHsDevice      * Device;
    eUsbHsTaskAction   Action;
};


//=====================================================================
//
// Device: Bluetooth Hands-Free Profile SCO Bypass definitions.
//

// BTH HFP device's notification work-item context.
struct UsbHsDeviceNotificationWorkItemContext
{
    UsbHsDevice *  UsbHsDevice;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME
(
    UsbHsDeviceNotificationWorkItemContext,
    GetUsbHsDeviceNotificationWorkItemContext
)

// USB HS device's notification request context.
union UsbHsDeviceNotificationBuffer
{
    BOOL        bImmediate;
    LONG        Volume;
    BOOL        BoolStatus;
    NTSTATUS    NtStatus;
};

struct UsbHsDeviceNotificationReqContext
{
    UsbHsDevice                  * UsbHsDevice;
    LONG                            Errors;
    UsbHsDeviceNotificationBuffer  Buffer;
    WDFMEMORY                       MemIn;
    WDFMEMORY                       MemOut;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME
(
    UsbHsDeviceNotificationReqContext,
    GetUsbHsDeviceNotificationReqContext
)

enum eUsbHsState
{
    eUsbHsStateInvalid         = 0,
    eUsbHsStateInitializing    = 1,
    eUsbHsStateRunning         = 2,
    eUsbHsStateStopping        = 3,
    eUsbHsStateStopped         = 4,
    eUsbHsStateFailed          = 5,
};

// To support event notification.
struct UsbHsEventCallback
{
    PFNEVENTNOTIFICATION    Handler;
    PVOID                   Context;
};

//#endif // SYSVAD_USB_SIDEBAND
class UsbHsDevice : 
    ISidebandDeviceCommon,
    public CUnknown    
{
    private:
        eUsbHsState            m_State;
            
        IAdapterCommon        * m_Adapter;
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

        UsbHsEventCallback     m_SpeakerVolumeCallback;
        UsbHsEventCallback     m_SpeakerConnectionStatusCallback;
        UsbHsEventCallback     m_MicVolumeCallback;
        UsbHsEventCallback     m_MicConnectionStatusCallback;

    public:
        //=====================================================================
        // Default CUnknown
        DECLARE_STD_UNKNOWN();
        DEFINE_STD_CONSTRUCTOR(UsbHsDevice);
        ~UsbHsDevice();

        NTSTATUS Init
        (
            _In_ IAdapterCommon     * Adapter, 
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
        UsbHsDevice * 
        GetUsbHsDevice
        (
            _In_ PLIST_ENTRY le
        )
        {
            return CONTAINING_RECORD(le, UsbHsDevice, m_ListEntry);
        }

    public:
        //=====================================================================
        //
        // ISidebandDeviceCommon functions. 
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
        
        NTSTATUS    GetUsbHsDescriptor
        (
            _Out_ PBTHHFP_DESCRIPTOR2 * Descriptor
        );
        
        NTSTATUS    EnableUsbHsNrecDisableStatusNotification();
        
        NTSTATUS    GetUsbHsVolumePropertyValues
        (
            _In_  ULONG                 Length,
            _Out_ PKSPROPERTY_VALUES  * PropValues
        );
        
        NTSTATUS    SetUsbHsSpeakerVolume
        (
            _In_ LONG  Volume  
        );
        
        NTSTATUS    GetUsbHsSpeakerVolume
        (
            _Out_ LONG  * Volume    
        );
        
        NTSTATUS    EnableUsbHsSpeakerVolumeStatusNotification();
        
        NTSTATUS    SetUsbHsMicVolume
        (
            _In_ LONG  Volume  
        );
        
        NTSTATUS    GetUsbHsMicVolume
        (
            _Out_ LONG  * Volume    
        );
        
        NTSTATUS    EnableUsbHsMicVolumeStatusNotification();
        
        NTSTATUS    GetUsbHsConnectionStatus
        (
            _Out_ BOOL * ConnectionStatus    
        );
        
        NTSTATUS    EnableUsbHsConnectionStatusNotification();
        
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
        
        NTSTATUS    GetUsbHsCodecId
        (
            _Out_ UCHAR * CodecId
        );

        NTSTATUS    SetUsbHsConnect();
        
        NTSTATUS    SetUsbHsDisconnect();

        NTSTATUS    SetUsbHsStreamOpen();

        NTSTATUS    SetUsbHsStreamClose();
        
        NTSTATUS    EnableUsbHsStreamStatusNotification();
        
        NTSTATUS    StopUsbHsStreamStatusNotification();
        
        //
        // WDF I/O Target callback.
        //
        static
        EVT_WDF_IO_TARGET_QUERY_REMOVE    EvtUsbHsTargetQueryRemove;

        static
        EVT_WDF_IO_TARGET_REMOVE_CANCELED EvtUsbHsTargetRemoveCanceled;

        static
        EVT_WDF_IO_TARGET_REMOVE_COMPLETE EvtUsbHsTargetRemoveComplete;

        //
        // Status notifications callbacks.
        //
        static 
        EVT_WDF_REQUEST_COMPLETION_ROUTINE EvtUsbHsDeviceStreamStatusCompletion;

        static 
        EVT_WDF_REQUEST_COMPLETION_ROUTINE EvtUsbHsDeviceNotificationStatusCompletion;
        
        static
        EVT_WDF_WORKITEM                   EvtUsbHsDeviceNotificationStatusWorkItem;
};
