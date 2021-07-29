//
// This class represents a the Bluetooth A2DP Headphone device.
// There is one class for each interface (id = symbolic-link-name).
//
#ifdef SYSVAD_A2DP_SIDEBAND


//=====================================================================
//
//  A2DP Sideband Headphone class definition.
//

class CAdapterCommon;
class A2dpHpDevice;

struct A2dpHpWorkItemContext
{
    CAdapterCommon *    Adapter;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME
(
    A2dpHpWorkItemContext,
    GetA2dpHpWorkItemContext
)

// start/stop the device
enum eA2dpHpTaskAction
{
    eA2dpHpTaskStart    = 1,
    eA2dpHpTaskStop     = 2,
};

struct A2dpHpWorkTask
{
    LIST_ENTRY          ListEntry;
    A2dpHpDevice      * Device;
    eA2dpHpTaskAction   Action;
};


//=====================================================================
//
// Device: Bluetooth A2DP Headphone definitions.
//

// BTH A2DP device's notification work-item context.
struct A2dpHpDeviceNotificationWorkItemContext
{
    A2dpHpDevice *  A2dpHpDevice;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME
(
    A2dpHpDeviceNotificationWorkItemContext,
    GetA2dpHpDeviceNotificationWorkItemContext
)

// A2DP HP device's notification request context.
union A2dpHpDeviceNotificationBuffer
{
    SIDEBANDAUDIO_VOLUME_PARAMS          Volume;
    SIDEBANDAUDIO_MUTE_PARAMS            Mute;
    SIDEBANDAUDIO_STREAM_STATUS_PARAMS   StreamStatus;
    SIDEBANDAUDIO_CONNECTION_PARAMS      ConnectionStatus;
    SIDEBANDAUDIO_SIOP_REQUEST_PARAM     SiopUpdate;
};

struct A2dpHpDeviceNotificationReqContext
{
    A2dpHpDevice                    *A2dpHpDevice;
    LONG                            Errors;
    A2dpHpDeviceNotificationBuffer  Buffer;
    WDFMEMORY                       MemIn;
    WDFMEMORY                       MemOut;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME
(
    A2dpHpDeviceNotificationReqContext,
    GetA2dpHpDeviceNotificationReqContext
)

enum eA2dpHpState
{
    eA2dpHpStateInvalid         = 0,
    eA2dpHpStateInitializing    = 1,
    eA2dpHpStateRunning         = 2,
    eA2dpHpStateStopping        = 3,
    eA2dpHpStateStopped         = 4,
    eA2dpHpStateFailed          = 5,
};

// To support event notification.
struct A2dpHpEventCallback
{
    PFNEVENTNOTIFICATION    Handler;
    PVOID                   Context;
};

typedef struct _AVDTP_CATEGORY_HEADER
{
    UCHAR SvcCategory;
    UCHAR Losc;     //Length Of Service Capability
} AVDTP_CATEGORY_HEADER;

//
// Contains codec information specific for the Subband Codec.
// Structure is defined in section 4.3.2 of the Bluetooth A2DP 1.3.2 specification.
//
typedef struct _A2DP_SBC_MEDIA_HEADER
{
    AVDTP_CATEGORY_HEADER CategoryCodecHeader;

    UCHAR Rfa : 4;
    UCHAR MediaType : 4;
    UCHAR CodecType;

    UCHAR ChannelMode : 4;
    UCHAR SamplingFrequency : 4;

    UCHAR AllocMethod : 2;
    UCHAR Subbands : 2;
    UCHAR BlockLength : 4;

    UCHAR MinBitpoolVal;
    UCHAR MaxBitpoolVal;

    // return the appropriate Losc value for this structure
    static unsigned char CorrectLosc()
    {
        return static_cast<unsigned char>((sizeof(_A2DP_SBC_MEDIA_HEADER) - sizeof(AVDTP_CATEGORY_HEADER)));
    }
} A2DP_SBC_MEDIA_HEADER;

//
// Contains codec information specific for the Advanced Audio Codec (AAC)
// Structure is defined in section 4.5.2 of the Bluetooth A2DP 1.3.2 specification.
//
typedef struct _A2DP_AAC_MEDIA_HEADER
{
    AVDTP_CATEGORY_HEADER CategoryCodecHeader;

    UCHAR Rfa : 4;
    UCHAR MediaType : 4;
    UCHAR CodecType;

    // Octet0
    UCHAR ObjectType;

    // Octet1
    UCHAR SamplingFrequencyUpper8Bits;

    // Octet2
    UCHAR RFA : 2;
    UCHAR Channels : 2;
    UCHAR SamplingFrequencyLower4Bits : 4;

    // Octet3
    UCHAR BitRateUpper7Bits : 7;
    UCHAR VariableBitRate : 1;

    // Octet4
    UCHAR BitRateMiddle8Bits;

    // Octet5
    UCHAR BitRateLower8Bits;

    // return the appropriate Losc value for this structure
    static UCHAR CorrectLosc()
    {
        return static_cast<UCHAR>((sizeof(A2DP_AAC_MEDIA_HEADER) - sizeof(AVDTP_CATEGORY_HEADER)));
    }
} A2DP_AAC_MEDIA_HEADER;

typedef union
{
    A2DP_SBC_MEDIA_HEADER SbcHeader;
    A2DP_AAC_MEDIA_HEADER AacHeader;
}A2DPHPDEVICE_CODEC_CAPABILITIES, *PA2DPHPDEVICE_CODEC_CAPABILITIES;

typedef struct _A2DPHPDEVICE_EP_TRANSPORT_RESOURCES
{
    ULONG                       A2dpHwTransportResource;
}A2DPHPDEVICE_EP_TRANSPORT_RESOURCES, *PA2DPHPDEVICE_EP_TRANSPORT_RESOURCES;

#endif // SYSVAD_A2DP_SIDEBAND
class A2dpHpDevice : 
    ISidebandDeviceCommon,
    public CUnknown    
{
    private:
        eA2dpHpState            m_State;
            
        IAdapterCommon        * m_Adapter;
        WDFIOTARGET             m_WdfIoTarget;
        
        LIST_ENTRY              m_ListEntry;
        UNICODE_STRING          m_SymbolicLinkName;
        WCHAR                   m_SpeakerWaveNameBuffer[A2DPHP_INTERFACE_REFSTRING_MAX_LENGTH];
        UNICODE_STRING          m_SpeakerWaveRefString;
        WCHAR                   m_SpeakerTopologyNameBuffer[A2DPHP_INTERFACE_REFSTRING_MAX_LENGTH];
        UNICODE_STRING          m_SpeakerTopologyRefString;
        
        //
        // The Topo Filter Desc and Topo Pins structures referenced from
        // the Miniports structure are deep copies: they start as copies of the 
        // static structures and are modified to allow A2DP-Device pin 
        // categories based on the info obtained from 
        // IOCTL_SBAUD_GET_ENDPOINT_DESCRIPTOR
        //
        PENDPOINT_MINIPAIR                      m_SpeakerMiniports;
        
        PUNKNOWN                                m_UnknownSpeakerTopology;
        PUNKNOWN                                m_UnknownSpeakerWave;

        PSIDEBANDAUDIO_DEVICE_DESCRIPTOR        m_Descriptor;
        PSIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2     m_pSpeakerDescriptor;
        PKSPROPERTY_DESCRIPTION                 m_SpeakerVolumePropValues;
        LONG                                    m_SpeakerVolumeLevel;
        PA2DPHPDEVICE_CODEC_CAPABILITIES        m_ConnectedCodecCaps;
        FAST_MUTEX                              m_ConnectedCodecLock;
        union {
            NTSTATUS                            m_ConnectionStatus;
            LONG                                m_ConnectionStatusLong;
        }; // unnamed.

        PKSPROPERTY_DESCRIPTION                 m_SpeakerMutePropValues;
        LONG                                    m_SpeakerMute;


        ULONG                                   m_SpeakerEpIndex;
        
        union{
          NTSTATUS                              m_SpeakerStreamStatus;
          LONG                                  m_SpeakerStreamStatusLong;
        }; // unnamed.

        KEVENT                                  m_SpeakerStreamStatusEvent;

        PSIDEBANDAUDIO_SUPPORTED_FORMATS        m_pSpeakerSupportedFormatsIntersection;
        ULONG                                   m_SpeakerSelectedFormat;

        A2DPHPDEVICE_EP_TRANSPORT_RESOURCES     m_SpeakerTransportResources;

        WDFREQUEST              m_SpeakerStreamReq;
        WDFREQUEST              m_SpeakerVolumeReq;
        WDFREQUEST              m_SpeakerMuteReq;
        WDFREQUEST              m_ConnectionReq;
        WDFREQUEST              m_SiopUpdateReq;
        WDFWORKITEM             m_WorkItem;
        WDFCOLLECTION           m_ReqCollection;
        KSPIN_LOCK              m_Lock;

        LONG                    m_nSpeakerStreams; // # of open streams.
        LONG                    m_nSpeakerStartedStreams;

        A2dpHpEventCallback     m_SpeakerVolumeCallback;

        A2dpHpEventCallback     m_SpeakerMuteCallback;

        A2dpHpEventCallback     m_SpeakerConnectionStatusCallback;

        A2dpHpEventCallback     m_SpeakerFormatChangeCallback;

    public:
        //=====================================================================
        // Default CUnknown
        DECLARE_STD_UNKNOWN();
        DEFINE_STD_CONSTRUCTOR(A2dpHpDevice);
        ~A2dpHpDevice();

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
        A2dpHpDevice * 
        GetA2dpHpDevice
        (
            _In_ PLIST_ENTRY le
        )
        {
            return CONTAINING_RECORD(le, A2dpHpDevice, m_ListEntry);
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
            _In_        eDeviceType             deviceType,
            _Out_       PULONG                  Size 
        );
    
        STDMETHODIMP_(NTSTATUS)             GetVolume
        (
            _In_        eDeviceType             deviceType,
            _In_        LONG                    Channel,
            _Out_       LONG                    *pVolume
        );
     
        STDMETHODIMP_(NTSTATUS)             SetVolume
        (
            _In_        eDeviceType             deviceType,
            _In_        LONG                    Channel,
            _In_        LONG                    Volume
        );

        STDMETHODIMP_(BOOL)                 IsMuteSupported
        (
            _In_        eDeviceType             deviceType
        );
        
        STDMETHODIMP_(PVOID)                GetMuteSettings
        (
            _In_        eDeviceType             deviceType,
            _Out_       PULONG                  Size 
        );
    
        STDMETHODIMP_(LONG)                 GetMute
        (
            _In_        eDeviceType             deviceType,
            _In_        LONG                    Channel
        );
     
        STDMETHODIMP_(NTSTATUS)             SetMute
        (
            _In_        eDeviceType             deviceType,
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

        NTSTATUS    SendIoCtrlSynchronously
        (
            _In_opt_    WDFREQUEST  Request,
            _In_        ULONG       IoControlCode,
            _In_        ULONG       InLength,
            _In_        ULONG       OutLength,
            _When_(InLength > 0, _In_)
            _When_(InLength == 0, _In_opt_)
                        PVOID       InBuffer,
            _When_(OutLength > 0, _In_)
            _When_(OutLength == 0, _In_opt_)
                        PVOID       OutBuffer
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

        NTSTATUS    SetA2dpHpDeviceCapabilities
        (
        );
        
        NTSTATUS    GetA2dpHpDescriptor
        (
            _Out_ PSIDEBANDAUDIO_DEVICE_DESCRIPTOR *Descriptor
        );
        
        NTSTATUS    GetA2dpHpEndpointDescriptor
        (
            _In_    ULONG                                   EpIndex,
            _Out_   PSIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2     *EpDescriptor
        );

        NTSTATUS    GetA2dpHpVolumePropertyValues
        (
            _In_  ULONG                     EpIndex,
            _In_  ULONG                     Length,
            _Out_ PKSPROPERTY_DESCRIPTION   *PropValues
        );
        
        NTSTATUS    SetA2dpHpSpeakerVolume
        (
            _In_ LONG   Channel,
            _In_ LONG   Volume  
        );
        
        NTSTATUS    GetA2dpHpSpeakerVolume
        (
            _In_  LONG  Channel,
            _Out_ LONG  *Volume    
        );
        
        NTSTATUS    EnableA2dpHpSpeakerVolumeStatusNotification();
        
        NTSTATUS    GetA2dpHpMutePropertyValues
        (
            _In_  ULONG                     EpIndex,
            _In_  ULONG                     Length,
            _Out_ PKSPROPERTY_DESCRIPTION   *PropValues
        );
        
        NTSTATUS    SetA2dpHpSpeakerMute
        (
            _In_ LONG   Channel,
            _In_ LONG   Mute
        );
        
        NTSTATUS    GetA2dpHpSpeakerMute
        (
            _In_  LONG  Channel,
            _Out_ LONG  *Mute
        );
        
        NTSTATUS    EnableA2dpHpSpeakerMuteStatusNotification();

        NTSTATUS    GetA2dpHpConnectionStatus
        (
            _Out_ BOOL *ConnectionStatus
        );

        NTSTATUS    EnableA2dpHpConnectionStatusNotification();


        NTSTATUS    GetA2dpHpEndpointFormatsIntersection
        (
            _In_    PSIDEBANDAUDIO_SUPPORTED_FORMATS        pDeviceFormats,
            _Out_   PSIDEBANDAUDIO_SUPPORTED_FORMATS        *ppSupportedFormatsIntersection
        );

        NTSTATUS    VerifyEndpointFormatCompatibility
        (
            _In_    PSIDEBANDAUDIO_SUPPORTED_FORMATS        EpSupportedFormats,
            _In_    PKSDATAFORMAT_WAVEFORMATEXTENSIBLE      pHardwareSupportedFormats,
            _In_    ULONG                                   NumHardwareSupportedFormats,
            _Out_   PULONG                                  pHardwareFormatIndex
        );

        NTSTATUS    GetA2dpHpEndpointTransportResources
        (
            _In_    ULONG                                   EpIndex,
            _In_    BOOL                                    bFeedback,
            _Out_   PA2DPHPDEVICE_EP_TRANSPORT_RESOURCES    pTransportResources
        );
        
        static
        NTSTATUS    CreateCustomEndpointMinipair
        (
            _In_                                            PENDPOINT_MINIPAIR pBaseMinipair,
            _In_                                            PUNICODE_STRING FriendlyName,
            _In_                                            PGUID pCategory,
            _In_                                            ULONG customFilterInterfacePropertyCount,
            _In_count_(customFilterInterfacePropertyCount)  DEVPROPERTY* customFilterInterfaceProperties,
            _Outptr_                                        PENDPOINT_MINIPAIR* ppCustomMinipair
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

        NTSTATUS    GetA2dpHpCodecCaps
        (
            _Outptr_    PA2DPHPDEVICE_CODEC_CAPABILITIES* ppCodecCaps
        );

        NTSTATUS    UpdateA2dpHpCodecCaps
        (
            _In_        PA2DPHPDEVICE_CODEC_CAPABILITIES pCodecCaps
        );

        NTSTATUS    SetSidebandClaimed(_In_ BOOL bClaimed);
        
        NTSTATUS    SpeakerStreamOpen();

        NTSTATUS    SpeakerStreamStart();

        NTSTATUS    SpeakerStreamSuspend();

        NTSTATUS    SpeakerStreamClose();

        NTSTATUS    SetA2dpHpStreamOpen(ULONG EpIndex, PKSDATAFORMAT_WAVEFORMATEXTENSIBLE pStreamFormat, PSIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2 pEndpointDescriptor);

        NTSTATUS    SetA2dpHpStreamStart();

        NTSTATUS    SetA2dpHpStreamSuspend();

        NTSTATUS    SetA2dpHpStreamClose();

        NTSTATUS    EnableA2dpHpSpeakerStreamStatusNotification();

        NTSTATUS    EnableA2dpHpSpeakerSiopNotification();
        
        NTSTATUS    StopA2dpHpSpeakerStreamStatusNotification();
        
        NTSTATUS    CreateFilterNames(
            _In_ PUNICODE_STRING A2dpHpDeviceSymbolicLinkName
        );
        
        //
        // WDF I/O Target callback.
        //
        static
        EVT_WDF_IO_TARGET_QUERY_REMOVE    EvtA2dpHpTargetQueryRemove;

        static
        EVT_WDF_IO_TARGET_REMOVE_CANCELED EvtA2dpHpTargetRemoveCanceled;

        static
        EVT_WDF_IO_TARGET_REMOVE_COMPLETE EvtA2dpHpTargetRemoveComplete;

        //
        // Status notifications callbacks.
        //
        static 
        EVT_WDF_REQUEST_COMPLETION_ROUTINE EvtA2dpHpDeviceStreamStatusCompletion;

        static 
        EVT_WDF_REQUEST_COMPLETION_ROUTINE EvtA2dpHpDeviceNotificationStatusCompletion;
        
        static
        EVT_WDF_WORKITEM                   EvtA2dpHpDeviceNotificationStatusWorkItem;
};
