//
// This class represents a the Bluetooth Hands-Free Profile USB Headset device.
// There is one class for each interface (id = symbolic-link-name).
//
#ifdef SYSVAD_USB_SIDEBAND


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
// Device: Bluetooth Hands-Free Profile USB Headset definitions.
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
    SIDEBANDAUDIO_VOLUME_PARAMS         Volume;
    SIDEBANDAUDIO_MUTE_PARAMS           Mute;
    SIDEBANDAUDIO_SIDETONE_PARAMS       Sidetone;
    SIDEBANDAUDIO_STREAM_STATUS_PARAMS  streamStatus;
};

struct UsbHsDeviceNotificationReqContext
{
    UsbHsDevice                     *UsbHsDevice;
    LONG                            Errors;
    UsbHsDeviceNotificationBuffer   Buffer;
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

typedef struct _USBHSDEVICE_EP_TRANSPORT_RESOURCES
{
    PUSB_INTERFACE_DESCRIPTOR                       pUsbInterfaceDescriptor;
    PUSBD_ENDPOINT_OFFLOAD_INFORMATION              pUsbOffloadInformation;
    PUSBD_ENDPOINT_OFFLOAD_INFORMATION              pSyncUsbOffloadInformation;
    PSIDEBANDAUDIO_EP_USBAUDIO_TRANSPORT_RESOURCES  pUsbAudioTransportResources;
    PSIDEBANDAUDIO_EP_USBAUDIO_TRANSPORT_RESOURCES  pSyncUsbAudioTransportResources;
    PUSB_ENDPOINT_DESCRIPTOR                        pUsbEndpointDescriptor;
    PUSB_ENDPOINT_DESCRIPTOR                        pSyncUsbEndpointDescriptor;
}USBHSDEVICE_EP_TRANSPORT_RESOURCES, *PUSBHSDEVICE_EP_TRANSPORT_RESOURCES;

#endif // SYSVAD_USB_SIDEBAND
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
        WCHAR                   m_SpeakerWaveNameBuffer[USBHS_INTERFACE_REFSTRING_MAX_LENGTH];
        UNICODE_STRING          m_SpeakerWaveRefString;
        WCHAR                   m_SpeakerTopologyNameBuffer[USBHS_INTERFACE_REFSTRING_MAX_LENGTH];
        UNICODE_STRING          m_SpeakerTopologyRefString;
        WCHAR                   m_MicWaveNameBuffer[USBHS_INTERFACE_REFSTRING_MAX_LENGTH];
        UNICODE_STRING          m_MicWaveRefString;
        WCHAR                   m_MicTopologyNameBuffer[USBHS_INTERFACE_REFSTRING_MAX_LENGTH];
        UNICODE_STRING          m_MicTopologyRefString;
        
        //
        // The Topo Filter Desc and Topo Pins structures referenced from
        // the Miniports structure are deep copies: they start as copies of the 
        // static structures and are modified to allow USB-Device pin 
        // categories based on the info obtained from 
        // IOCTL_SBAUD_GET_ENDPOINT_DESCRIPTOR
        //
        PENDPOINT_MINIPAIR                      m_SpeakerMiniports;
        PENDPOINT_MINIPAIR                      m_MicMiniports;
        
        PUNKNOWN                                m_UnknownSpeakerTopology;
        PUNKNOWN                                m_UnknownSpeakerWave;
        PUNKNOWN                                m_UnknownMicTopology;
        PUNKNOWN                                m_UnknownMicWave;        

        PSIDEBANDAUDIO_DEVICE_DESCRIPTOR        m_Descriptor;

        //
        // Using new Endpoint descriptor structure
        // Old: SIDEBANDAUDIO_ENDPOINT_DESCRIPTOR - IOCTL_SBAUD_GET_ENDPOINT_DESCRIPTOR
        // Use SIDEBANDAUDIO_ENDPOINT_DESCRIPTOR for 20H2 and older OS.
        //
        // New: SIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2 - IOCTL_SBAUD_GET_ENDPOINT_DESCRIPTOR2
        //
        // The new struct includes interface properties
        // that need to be added to the KS Filter interface
        //
        // USB Audio class driver supports both old and new IOCTLs
        //
        PSIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2     m_pSpeakerDescriptor;
        PSIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2     m_pMicDescriptor;
        PKSPROPERTY_DESCRIPTION                 m_SpeakerVolumePropValues;
        LONG                                    m_SpeakerVolumeLevel;
        PKSPROPERTY_DESCRIPTION                 m_MicVolumePropValues;
        LONG                                    m_MicVolumeLevel;

        PKSPROPERTY_DESCRIPTION                 m_SpeakerMutePropValues;
        LONG                                    m_SpeakerMute;
        PKSPROPERTY_DESCRIPTION                 m_MicMutePropValues;
        LONG                                    m_MicMute;


        ULONG                                   m_SpeakerEpIndex;
        ULONG                                   m_MicEpIndex;
        
        union{
          NTSTATUS                              m_SpeakerStreamStatus;
          LONG                                  m_SpeakerStreamStatusLong;
        }; // unnamed.
        union{
          NTSTATUS                              m_MicStreamStatus;
          LONG                                  m_MicStreamStatusLong;
        }; // unnamed.

        KEVENT                                  m_SpeakerStreamStatusEvent;
        KEVENT                                  m_MicStreamStatusEvent;

        PSIDEBANDAUDIO_SUPPORTED_FORMATS        m_pSpeakerSupportedFormatsIntersection;
        ULONG                                   m_SpeakerSelectedFormat;
        PSIDEBANDAUDIO_SUPPORTED_FORMATS        m_pMicSupportedFormatsIntersection;
        ULONG                                   m_MicSelectedFormat;

        USBHSDEVICE_EP_TRANSPORT_RESOURCES      m_SpeakerTransportResources;
        USBHSDEVICE_EP_TRANSPORT_RESOURCES      m_MicTransportResources;

        //
        // Set to TRUE when the HF (remote device) wants to disable the 
        // NR + EC of the AG (local system).
        //
        
        WDFREQUEST              m_SpeakerStreamReq;
        WDFREQUEST              m_MicStreamReq;
        WDFREQUEST              m_SpeakerVolumeReq;
        WDFREQUEST              m_MicVolumeReq;
        WDFREQUEST              m_SpeakerMuteReq;
        WDFREQUEST              m_MicMuteReq;
        WDFWORKITEM             m_WorkItem;
        WDFCOLLECTION           m_ReqCollection;
        KSPIN_LOCK              m_Lock;

        LONG                    m_nSpeakerStreamsOpen; // # of open streams.
        LONG                    m_nMicStreamsOpen; // # of open streams.

        LONG                    m_nSpeakerStreamsStart; // # of Started streams.
        LONG                    m_nMicStreamsStart; // # of Started streams.

        UsbHsEventCallback     m_SpeakerVolumeCallback;
        UsbHsEventCallback     m_MicVolumeCallback;

        UsbHsEventCallback     m_SpeakerMuteCallback;
        UsbHsEventCallback     m_MicMuteCallback;

        UsbHsEventCallback     m_SpeakerConnectionStatusCallback;
        UsbHsEventCallback     m_MicConnectionStatusCallback;

        UsbHsEventCallback     m_SpeakerFormatChangeCallback;
        UsbHsEventCallback     m_MicFormatChangeCallback;

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
        
        NTSTATUS    GetUsbHsDescriptor
        (
            _Out_ PSIDEBANDAUDIO_DEVICE_DESCRIPTOR *Descriptor
        );
        
        NTSTATUS    GetUsbHsEndpointDescriptor
        (
            _In_    ULONG                                   EpIndex,
            _Out_   PSIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2     *EpDescriptor
        );

        NTSTATUS    GetUsbHsVolumePropertyValues
        (
            _In_  ULONG                     EpIndex,
            _In_  ULONG                     Length,
            _Out_ PKSPROPERTY_DESCRIPTION   *PropValues
        );
        
        NTSTATUS    SetUsbHsSpeakerVolume
        (
            _In_ LONG   Channel,
            _In_ LONG   Volume  
        );
        
        NTSTATUS    GetUsbHsSpeakerVolume
        (
            _In_  LONG  Channel,
            _Out_ LONG  *Volume    
        );
        
        NTSTATUS    EnableUsbHsSpeakerVolumeStatusNotification();
        
        NTSTATUS    SetUsbHsMicVolume
        (
            _In_ LONG  Channel,
            _In_ LONG  Volume  
        );
        
        NTSTATUS    GetUsbHsMicVolume
        (
            _In_  LONG  Channel,
            _Out_ LONG  * Volume    
        );
        
        NTSTATUS    EnableUsbHsMicVolumeStatusNotification();

        NTSTATUS    GetUsbHsMutePropertyValues
        (
            _In_  ULONG                     EpIndex,
            _In_ ULONG                      Length,
            _Out_ PKSPROPERTY_DESCRIPTION   *PropValues
        );
        
        NTSTATUS    SetUsbHsSpeakerMute
        (
            _In_ LONG   Channel,
            _In_ LONG   Mute
        );
        
        NTSTATUS    GetUsbHsSpeakerMute
        (
            _In_  LONG  Channel,
            _Out_ LONG  *Mute
        );
        
        NTSTATUS    EnableUsbHsSpeakerMuteStatusNotification();
        
        NTSTATUS    SetUsbHsMicMute
        (
            _In_ LONG   Channel,
            _In_ LONG   Mute
        );
        
        NTSTATUS    GetUsbHsMicMute
        (
            _In_  LONG  Channel,
            _Out_ LONG  *Mute
        );
        
        NTSTATUS    EnableUsbHsMicMuteStatusNotification();

        NTSTATUS    GetUsbHsEndpointFormatsIntersection
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

        NTSTATUS    FreeTransportResources
        (
            _In_    PUSBHSDEVICE_EP_TRANSPORT_RESOURCES     pTransportResources
        );

        NTSTATUS    GetTransportResources
        (
            _In_    ULONG                                   EpIndex,
            _In_    PSIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2     pEndpointDescriptor,
            _Out_   PUSBHSDEVICE_EP_TRANSPORT_RESOURCES     pTransportResources
        );

        NTSTATUS    SetTransportResources
        (
            _In_    PSIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2     pEndpointDescriptor,
            _In_    ULONG                                   EpIndex
        );

        NTSTATUS    GetSiop
        (
            _In_        const GUID  *ParamSet,
            _In_        ULONG       SiopTypeId,
            _In_        ULONG       EpIndex,
            _In_        ULONG       PoolTag,
            _Outptr_    PBYTE       *ppOutputBuffer
        );
        
        static
        NTSTATUS    CreateCustomEndpointMinipair
        (
            _In_        PENDPOINT_MINIPAIR  pBaseMinipair,
            _In_        PUNICODE_STRING     FriendlyName,
            _In_        PGUID               pCategory,
            _Outptr_    PENDPOINT_MINIPAIR  *ppCustomMinipair
        );

        static
        NTSTATUS    UpdateCustomEndpointCategory
        (
            _In_        PPCFILTER_DESCRIPTOR    pCustomMinipairTopoFilter,
            _In_        PPCPIN_DESCRIPTOR       pCustomMinipairTopoPins,
            _In_        PGUID                   pCategory
        );

        static
        VOID        DeleteCustomEndpointMinipair
        (
            _In_        PENDPOINT_MINIPAIR CustomMinipair
        );

        NTSTATUS    SetSidebandClaimed(_In_ BOOL bClaimed);
        
        NTSTATUS    SpeakerStreamOpen();

        NTSTATUS    MicStreamOpen();

        NTSTATUS    SpeakerStreamStart();

        NTSTATUS    MicStreamStart();

        NTSTATUS    SpeakerStreamSuspend();

        NTSTATUS    MicStreamSuspend();

        NTSTATUS    SpeakerStreamClose();

        NTSTATUS    MicStreamClose();

        NTSTATUS    SetUsbHsStreamOpen(ULONG EpIndex, PKSDATAFORMAT_WAVEFORMATEXTENSIBLE pStreamFormat, PSIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2 pEndpointDescriptor);

        NTSTATUS    SetUsbHsStreamStart(ULONG EpIndex);

        NTSTATUS    SetUsbHsStreamSuspend(ULONG EpIndex);

        NTSTATUS    SetUsbHsStreamClose(ULONG EpIndex);

        NTSTATUS    EnableUsbHsSpeakerStreamStatusNotification();
        
        NTSTATUS    StopUsbHsSpeakerStreamStatusNotification();
        
        NTSTATUS    EnableUsbHsMicStreamStatusNotification();
        
        NTSTATUS    StopUsbHsMicStreamStatusNotification();

        NTSTATUS    CreateFilterNames(
            _In_ PUNICODE_STRING UsbHsDeviceSymbolicLinkName
        );


#if 0
        NTSTATUS    GetSpeakerTransportResources();
        NTSTATUS    FreeSpeakerTransportResources();
        NTSTATUS    GetMicTransportResources();
        NTSTATUS    FreeMicTransportResources();
#endif
        
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
