/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    Common.h

Abstract:
    
    CAdapterCommon class declaration.

--*/

#ifndef _SYSVAD_COMMON_H_
#define _SYSVAD_COMMON_H_

#define HNSTIME_PER_MILLISECOND 10000

//=============================================================================
// Macros
//=============================================================================

#define UNREFERENCED_VAR(status) \
    status = status 

//-------------------------------------------------------------------------
// Description:
//
// jump to the given label.
//
// Parameters:
//
//      label - [in] label to jump if condition is met
//
#define JUMP(label)                                             \
        goto label;  

//-------------------------------------------------------------------------
// Description:
//
// If the condition evaluates to TRUE, jump to the given label.
//
// Parameters:
//
//      condition - [in] Code that fits in if statement
//      label - [in] label to jump if condition is met
//
#define IF_TRUE_JUMP(condition, label)                          \
    if (condition)                                               \
    {                                                           \
        goto label;                                             \
    }

//-------------------------------------------------------------------------
// Description:
//
// If the condition evaluates to TRUE, perform the given statement
// then jump to the given label.
//
// Parameters:
//
//      condition - [in] Code that fits in if statement
//      action - [in] action to perform in body of if statement
//      label - [in] label to jump if condition is met
//
#define IF_TRUE_ACTION_JUMP(condition, action, label)           \
    if (condition)                                               \
    {                                                           \
        action;                                                 \
        goto label;                                             \
    }
    
//-------------------------------------------------------------------------
// Description:
//
// If the ntStatus is not NT_SUCCESS, perform the given statement then jump to
// the given label.
//
// Parameters:
//
//      ntStatus - [in] Value to check
//      action - [in] action to perform in body of if statement
//      label - [in] label to jump if condition is met
//
#define IF_FAILED_ACTION_JUMP(ntStatus, action, label)          \
        if (!NT_SUCCESS(ntStatus))                              \
        {                                                       \
            action;                                             \
            goto label;                                         \
        }
    
//-------------------------------------------------------------------------
// Description:
//
// If the ntStatus passed is not NT_SUCCESS, jump to the given label.
//
// Parameters:
//
//      ntStatus - [in] Value to check
//      label - [in] label to jump if condition is met
//
#define IF_FAILED_JUMP(ntStatus, label)                         \
    if (!NT_SUCCESS(ntStatus))                                   \
    {                                                           \
        goto label;                                             \
    }

#define SAFE_RELEASE(p) {if (p) { (p)->Release(); (p) = nullptr; } }
#define SAFE_DELETE_PTR_WITH_TAG(ptr, tag) if(ptr) { ExFreePoolWithTag((ptr), tag); (ptr) = NULL; }

// JACKDESC_RGB(r, g, b) 
#define JACKDESC_RGB(r, g, b) \
    ((COLORREF)((r << 16) | (g << 8) | (b)))

// Min/Max defines.
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define MINWAVERT_POOLTAG           'RWNM'
#define MINTOPORT_POOLTAG           'RTNM'
#define MINADAPTER_POOLTAG          'uAyS'
#define USBSIDEBANDTEST_POOLTAG01   '1AyS'
#define USBSIDEBANDTEST_POOLTAG02   '2AyS'
#define USBSIDEBANDTEST_POOLTAG03   '3AyS'
#define USBSIDEBANDTEST_POOLTAG04   '4AyS'
#define USBSIDEBANDTEST_POOLTAG05   '5AyS'
#define USBSIDEBANDTEST_POOLTAG06   '6AyS'
#define USBSIDEBANDTEST_POOLTAG07   '7AyS'
#define USBSIDEBANDTEST_POOLTAG08   '8AyS'
#define USBSIDEBANDTEST_POOLTAG09   '9AyS'
#define USBSIDEBANDTEST_POOLTAG010  'aAyS'
#define USBSIDEBANDTEST_POOLTAG011  'bAyS'
#define USBSIDEBANDTEST_POOLTAG012  'cAyS'
#define USBSIDEBANDTEST_POOLTAG013  'dAyS'
#define USBSIDEBANDTEST_POOLTAG014  'eAyS'
#define USBSIDEBANDTEST_POOLTAG015  'fAyS'
#define USBSIDEBANDTEST_POOLTAG016  'gAyS'
#define A2DPSIDEBANDTEST_POOLTAG01  'hAyS'
#define A2DPSIDEBANDTEST_POOLTAG02  'iAyS'
#define A2DPSIDEBANDTEST_POOLTAG03  'jAyS'
#define A2DPSIDEBANDTEST_POOLTAG04  'kAyS'
#define A2DPSIDEBANDTEST_POOLTAG05  'lAyS'
#define A2DPSIDEBANDTEST_POOLTAG06  'mAyS'
#define A2DPSIDEBANDTEST_POOLTAG07  'nAyS'

typedef enum
{
    eSpeakerDevice = 0,
    eSpeakerHpDevice,
    eHdmiRenderDevice,
    eMicInDevice,
    eMicArrayDevice1,
    eMicArrayDevice2,
    eMicArrayDevice3,
    eBthHfpSpeakerDevice,
    eBthHfpMicDevice,
    eCellularDevice,
    eHandsetSpeakerDevice,
    eHandsetMicDevice,
    eSpeakerHsDevice,
    eMicHsDevice,
    eFmRxDevice,
    eSpdifRenderDevice,
    eUsbHsSpeakerDevice,
    eUsbHsMicDevice,
    eA2dpHpSpeakerDevice,
    eMaxDeviceType
    
} eDeviceType;

//
// Signal processing modes and default formats structs.
//
typedef struct _MODE_AND_DEFAULT_FORMAT {
    GUID            Mode;
    KSDATAFORMAT*   DefaultFormat;
} MODE_AND_DEFAULT_FORMAT, *PMODE_AND_DEFAULT_FORMAT;

//
// Enumeration of the various types of pins implemented in this driver.
//
typedef enum
{
    NoPin,
    BridgePin,
    SystemRenderPin,
    OffloadRenderPin,
    RenderLoopbackPin,
    SystemCapturePin,
    KeywordCapturePin,
    TelephonyBidiPin,
} PINTYPE;

//
// PIN_DEVICE_FORMATS_AND_MODES
//
//  Used to specify a pin's type (e.g. system, offload, etc.), formats, and
//  modes. Conceptually serves similar purpose as the PCPIN_DESCRIPTOR to
//  define a pin, but is more specific to driver implementation.
//
//  Arrays of these structures follow the same order as the filter's
//  pin descriptor array so that KS pin IDs can serve as an index.
//
typedef struct _PIN_DEVICE_FORMATS_AND_MODES
{
    PINTYPE                             PinType;

    KSDATAFORMAT_WAVEFORMATEXTENSIBLE * WaveFormats;
    ULONG                               WaveFormatsCount;
    
    MODE_AND_DEFAULT_FORMAT *           ModeAndDefaultFormat;
    ULONG                               ModeAndDefaultFormatCount;

} PIN_DEVICE_FORMATS_AND_MODES, *PPIN_DEVICE_FORMATS_AND_MODES;

typedef struct _SYSVAD_AUDIOPOSTURE_INFO
{
    BOOL OrientationSupported;
} SYSVAD_AUDIOPOSTURE_INFO, *PSYSVAD_AUDIOPOSTURE_INFO;

//
// Parameter module handler function prototypes.
//
static const GUID NULL_GUID = 
{0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}};

static const GUID MODULETYPE_HWEFFECT =
{0x2608993c,0x361e,0x488c,{0xa0,0xbc,0x8f,0x3d,0xe7,0xba,0x12,0xb4}};

//
// Forward declaration.
//
struct AUDIOMODULE_DESCRIPTOR;
typedef AUDIOMODULE_DESCRIPTOR *PAUDIOMODULE_DESCRIPTOR;

typedef struct _AUDIOMODULE_PARAMETER_INFO
{
    USHORT      AccessFlags;        // get/set/basic-support attributes.
    USHORT      Flags;        
    ULONG       Size;
    DWORD       VtType;
    PVOID       ValidSet;
    ULONG       ValidSetCount;
} AUDIOMODULE_PARAMETER_INFO, *PAUDIOMODULE_PARAMETER_INFO;

#define AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION  0x00000001

//
// Module callbacks.
//
typedef
NTSTATUS
(*FN_AUDIOMODULE_INIT_CLASS)
(
    _In_  const AUDIOMODULE_DESCRIPTOR * Module,
    _Inout_opt_ PVOID           Context,
    _In_        size_t          Size,
    _In_        KSAUDIOMODULE_NOTIFICATION * NotificationHeader,
    _In_opt_    PPORTCLSNOTIFICATIONS PortNotifications
);

typedef
NTSTATUS
(*FN_AUDIOMODULE_INIT_INSTANCE)
(
    _In_  const AUDIOMODULE_DESCRIPTOR * Module,
    _In_opt_    PVOID           TemplateContext,
    _Inout_opt_ PVOID           Context,
    _In_        size_t          Size,
    _In_        ULONG           InstanceId
);

typedef
VOID
(*FN_AUDIOMODULE_CLEANUP)
(
    _In_        PVOID           Context
);

typedef
NTSTATUS
(*FN_AUDIOMODULE_HANDLER)
(
    _Inout_opt_                          PVOID   Context,
    _In_reads_bytes_(InBufferCb)         PVOID   InBuffer,
    _In_                                 ULONG   InBufferCb,
    _Out_writes_bytes_opt_(*OutBufferCb) PVOID   OutBuffer,
    _Inout_                              ULONG * OutBufferCb 
);

//
// Module description.
//
struct AUDIOMODULE_DESCRIPTOR
{
    const GUID *                    ClassId; 
    const GUID *                    ProcessingMode;
    WCHAR                           Name[AUDIOMODULE_MAX_NAME_CCH_SIZE];
    ULONG                           InstanceId;
    ULONG                           VersionMajor;
    ULONG                           VersionMinor;
    ULONG                           Flags;
    size_t                          ContextSize;
    FN_AUDIOMODULE_INIT_CLASS       InitClass;
    FN_AUDIOMODULE_INIT_INSTANCE    InitInstance;
    FN_AUDIOMODULE_CLEANUP          Cleanup;
    FN_AUDIOMODULE_HANDLER          Handler;
};

#define AUDIOMODULE_DESCRIPTOR_FLAG_NONE        0x00000000

//
// Audio module instance defintion.
// This sample driver generates an instance id by combinding the
// configuration set # for a class module id with the instance of that 
// configuration. Real driver should use a more robust scheme, such as
// an indirect mapping from/to an instance id to/from a configuration set
// + location in the pipeline + any other info the driver needs.
//
#define AUDIOMODULE_CLASS_CFG_ID_MASK           0xFF
#define AUDIOMODULE_CLASS_CFG_INSTANCE_ID_MASK  0xFFFFFF

#define AUDIOMODULE_INSTANCE_ID(ClassCfgId, ClassCfgInstanceId) \
    ((ULONG(ClassCfgId & AUDIOMODULE_CLASS_CFG_ID_MASK) << 24) | \
     (ULONG(ClassCfgInstanceId & AUDIOMODULE_CLASS_CFG_INSTANCE_ID_MASK)))

#define AUDIOMODULE_GET_CLASSCFGID(InstanceId) \
    (ULONG(InstanceId) >> 24 & AUDIOMODULE_CLASS_CFG_ID_MASK)

//
// Used to track run-time audio module changes.
//
struct AUDIOMODULE
{
    const AUDIOMODULE_DESCRIPTOR *  Descriptor;
    PVOID                           Context;
    ULONG                           InstanceId;
    ULONG                           NextCfgInstanceId;  // used by filter modules
    BOOL                            Enabled;
};

// forward declaration.
typedef struct _ENDPOINT_MINIPAIR *PENDPOINT_MINIPAIR;

// both wave & topology miniport create function prototypes have this form:
typedef HRESULT (*PFNCREATEMINIPORT)(
    _Out_           PUNKNOWN                              * Unknown,
    _In_            REFCLSID,
    _In_opt_        PUNKNOWN                                UnknownOuter,
    _In_            POOL_FLAGS                              PoolFlags, 
    _In_            PUNKNOWN                                UnknownAdapter,
    _In_opt_        PVOID                                   DeviceContext,
    _In_            PENDPOINT_MINIPAIR                      MiniportPair
);

//=============================================================================
//
//=============================================================================
typedef struct _SYSVAD_DEVPROPERTY {
    const DEVPROPKEY   *PropertyKey;
    DEVPROPTYPE Type;
    ULONG BufferSize;
    __field_bcount_opt(BufferSize) PVOID Buffer;
} SYSVAD_DEVPROPERTY, PSYSVAD_DEVPROPERTY;

#define ENDPOINT_NO_FLAGS                       0x00000000
#define ENDPOINT_OFFLOAD_SUPPORTED              0x00000001
#define ENDPOINT_LOOPBACK_SUPPORTED             0x00000002
#define ENDPOINT_SOUNDDETECTOR_SUPPORTED        0x00000004
#define ENDPOINT_CELLULAR_PROVIDER1             0x00000008
#define ENDPOINT_CELLULAR_PROVIDER2             0x00000010

//
// Endpoint miniport pair (wave/topology) descriptor.
//
typedef struct _ENDPOINT_MINIPAIR 
{
    eDeviceType                     DeviceType;

    // Topology miniport.
    PWSTR                           TopoName;               // make sure this or the template name matches with SYSVAD.<TopoName>.szPname in the inf's [Strings] section
    PWSTR                           TemplateTopoName;       // optional template name
    PFNCREATEMINIPORT               TopoCreateCallback;
    PCFILTER_DESCRIPTOR*            TopoDescriptor;
    ULONG                           TopoInterfacePropertyCount;
    const SYSVAD_DEVPROPERTY*       TopoInterfaceProperties;

    // Wave RT miniport.
    PWSTR                           WaveName;               // make sure this or the template name matches with SYSVAD.<WaveName>.szPname in the inf's [Strings] section
    PWSTR                           TemplateWaveName;       // optional template name
    PFNCREATEMINIPORT               WaveCreateCallback;
    PCFILTER_DESCRIPTOR*            WaveDescriptor;
    ULONG                           WaveInterfacePropertyCount;
    const SYSVAD_DEVPROPERTY*       WaveInterfaceProperties;

    USHORT                          DeviceMaxChannels;
    PIN_DEVICE_FORMATS_AND_MODES*   PinDeviceFormatsAndModes;
    ULONG                           PinDeviceFormatsAndModesCount;

    // Miniport physical connections.
    PHYSICALCONNECTIONTABLE*        PhysicalConnections;
    ULONG                           PhysicalConnectionCount;

    // General endpoint flags (one of more ENDPOINT_<flag-type>, see above)
    ULONG                           DeviceFlags;

    // Static module list description.
    AUDIOMODULE_DESCRIPTOR *        ModuleList;
    ULONG                           ModuleListCount;
    const GUID *                    ModuleNotificationDeviceId;
} ENDPOINT_MINIPAIR;

//=============================================================================
// Defines
//=============================================================================

DEFINE_GUID(IID_IAdapterCommon,
0x7eda2950, 0xbf9f, 0x11d0, 0x87, 0x1f, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);

// {4E1E5697-4D04-4438-A913-B0EEE03732BA}
DEFINE_GUID(IID_IMiniportChange, 
0x4e1e5697, 0x4d04, 0x4438, 0xa9, 0x13, 0xb0, 0xee, 0xe0, 0x37, 0x32, 0xba);

//=============================================================================
// Interfaces
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// IID_IMiniportChange
//
DECLARE_INTERFACE_(IMiniportChange, IUnknown)
{
    STDMETHOD_(NTSTATUS, NotifyEndpointPair) 
    ( 
        THIS_
        _In_ WCHAR              *RenderEndpointTopoName,
        _In_ ULONG              RenderEndpointNameLen,
        _In_ ULONG              RenderPinId,
        _In_ WCHAR              *CaptureEndpointTopoName,
        _In_ ULONG              CaptureEndpointNameLen,
        _In_ ULONG              CapturePinId
    ) PURE;
};

///////////////////////////////////////////////////////////////////////////////
// IAdapterCommon
//
DECLARE_INTERFACE_(IAdapterCommon, IUnknown)
{
    STDMETHOD_(NTSTATUS,        Init) 
    ( 
        THIS_
        _In_  PDEVICE_OBJECT      DeviceObject 
    ) PURE;

    STDMETHOD_(PDEVICE_OBJECT,  GetDeviceObject)
    (
        THIS
    ) PURE;
    
    STDMETHOD_(PDEVICE_OBJECT,  GetPhysicalDeviceObject)
    (
        THIS
    ) PURE;

    STDMETHOD_(WDFDEVICE,       GetWdfDevice)
    (
        THIS
    ) PURE;

    STDMETHOD_(VOID,            SetWaveServiceGroup) 
    ( 
        THIS_
        _In_ PSERVICEGROUP        ServiceGroup 
    ) PURE;

    STDMETHOD_(BOOL,            bDevSpecificRead)
    (
        THIS_
    ) PURE;

    STDMETHOD_(VOID,            bDevSpecificWrite)
    (
        THIS_
        _In_  BOOL                bDevSpecific
    );

    STDMETHOD_(INT,             iDevSpecificRead)
    (
        THIS_
    ) PURE;

    STDMETHOD_(VOID,            iDevSpecificWrite)
    (
        THIS_
        _In_  INT                 iDevSpecific
    );

    STDMETHOD_(UINT,            uiDevSpecificRead)
    (
        THIS_
    ) PURE;

    STDMETHOD_(VOID,            uiDevSpecificWrite)
    (
        THIS_
        _In_  UINT                uiDevSpecific
    );

    STDMETHOD_(BOOL,            MixerMuteRead)
    (
        THIS_
        _In_  ULONG               Index,
        _In_  ULONG               Channel
    ) PURE;

    STDMETHOD_(VOID,            MixerMuteWrite)
    (
        THIS_
        _In_  ULONG               Index,
        _In_  ULONG               Channel,
        _In_  BOOL                Value
    );

    STDMETHOD_(ULONG,           MixerMuxRead)
    (
        THIS
    );

    STDMETHOD_(VOID,            MixerMuxWrite)
    (
        THIS_
        _In_  ULONG               Index
    );

    STDMETHOD_(LONG,            MixerVolumeRead) 
    ( 
        THIS_
        _In_  ULONG               Index,
        _In_  ULONG               Channel
    ) PURE;

    STDMETHOD_(VOID,            MixerVolumeWrite) 
    ( 
        THIS_
        _In_  ULONG               Index,
        _In_  ULONG               Channel,
        _In_  LONG                Value 
    ) PURE;
    
    STDMETHOD_(LONG,            MixerPeakMeterRead) 
    ( 
        THIS_
        _In_  ULONG               Index,
        _In_  ULONG               Channel
    ) PURE;

    STDMETHOD_(VOID,            MixerReset) 
    ( 
        THIS 
    ) PURE;

    STDMETHOD_(NTSTATUS,        WriteEtwEvent) 
    ( 
        THIS_ 
        _In_ EPcMiniportEngineEvent    miniportEventType,
        _In_ ULONGLONG  ullData1,
        _In_ ULONGLONG  ullData2,
        _In_ ULONGLONG  ullData3,
        _In_ ULONGLONG  ullData4
    ) PURE;

    STDMETHOD_(VOID,            SetEtwHelper) 
    ( 
        THIS_
        PPORTCLSETWHELPER _pPortClsEtwHelper
    ) PURE;
    
    STDMETHOD_(NTSTATUS,        InstallSubdevice)
    ( 
        _In_opt_        PIRP                                        Irp,
        _In_            PWSTR                                       Name,
        _In_opt_        PWSTR                                       TemplateName,
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

    STDMETHOD_(NTSTATUS,        UnregisterSubdevice)
    (
        THIS_
        _In_opt_   PUNKNOWN     UnknownPort
    );

    STDMETHOD_(NTSTATUS,        ConnectTopologies)
    (
        THIS_
        _In_ PUNKNOWN                   UnknownTopology,
        _In_ PUNKNOWN                   UnknownWave,
        _In_ PHYSICALCONNECTIONTABLE*   PhysicalConnections,
        _In_ ULONG                      PhysicalConnectionCount
    );

    STDMETHOD_(NTSTATUS,        DisconnectTopologies)
    (
        THIS_
        _In_ PUNKNOWN                   UnknownTopology,
        _In_ PUNKNOWN                   UnknownWave,
        _In_ PHYSICALCONNECTIONTABLE*   PhysicalConnections,
        _In_ ULONG                      PhysicalConnectionCount
    );

    STDMETHOD_(NTSTATUS,        InstallEndpointFilters)
    (
        THIS_
        _In_opt_    PIRP                Irp, 
        _In_        PENDPOINT_MINIPAIR  MiniportPair,
        _In_opt_    PVOID               DeviceContext,
        _Out_opt_   PUNKNOWN *          UnknownTopology,
        _Out_opt_   PUNKNOWN *          UnknownWave,
        _Out_opt_   PUNKNOWN *          UnknownMiniportTopology,
        _Out_opt_   PUNKNOWN *          UnknownMiniportWave
    );

    STDMETHOD_(NTSTATUS,        RemoveEndpointFilters)
    (
        THIS_
        _In_        PENDPOINT_MINIPAIR  MiniportPair,
        _In_opt_    PUNKNOWN            UnknownTopology,
        _In_opt_    PUNKNOWN            UnknownWave
    );

    STDMETHOD_(NTSTATUS,        GetFilters)
    (
        THIS_
        _In_        PENDPOINT_MINIPAIR  MiniportPair,
        _Out_opt_   PUNKNOWN            *UnknownTopologyPort,
        _Out_opt_   PUNKNOWN            *UnknownTopologyMiniport,
        _Out_opt_   PUNKNOWN            *UnknownWavePort,
        _Out_opt_   PUNKNOWN            *UnknownWaveMiniport
    );

    STDMETHOD_(NTSTATUS,        SetIdlePowerManagement)
    (
        THIS_
        _In_        PENDPOINT_MINIPAIR  MiniportPair,
        _In_        BOOL                bEnable
    );

#ifdef SYSVAD_BTH_BYPASS
    STDMETHOD_(NTSTATUS,        InitBthScoBypass)();
       
#endif // SYSVAD_BTH_BYPASS

#ifdef SYSVAD_USB_SIDEBAND
    STDMETHOD_(NTSTATUS,        InitUsbSideband)();

    STDMETHOD_(NTSTATUS,        AddDeviceAsPowerDependency)
    (
        _In_ PDEVICE_OBJECT pdo
    );

    STDMETHOD_(NTSTATUS,        RemoveDeviceAsPowerDependency)
    (
        _In_ PDEVICE_OBJECT pdo
    );

#endif // SYSVAD_USB_SIDEBAND

#ifdef SYSVAD_A2DP_SIDEBAND
    STDMETHOD_(NTSTATUS,        InitA2dpSideband)();

#endif // SYSVAD_A2DP_SIDEBAND

    STDMETHOD_(VOID, Cleanup)();

#ifdef SYSVAD_USB_SIDEBAND
    STDMETHOD_(NTSTATUS, UpdatePowerRelations)(_In_ PIRP Irp);
#endif // SYSVAD_USB_SIDEBAND

    STDMETHOD_(NTSTATUS, NotifyEndpointPair) 
    ( 
        THIS_
        _In_ WCHAR  *RenderEndpointTopoName,
        _In_ ULONG  RenderEndpointNameLen,
        _In_ ULONG  RenderPinId,
        _In_ WCHAR  *CaptureEndpointTopoName,
        _In_ ULONG  CaptureEndpointNameLen,
        _In_ ULONG  CapturePinId
    ) PURE;
};

typedef IAdapterCommon *PADAPTERCOMMON;
typedef IMiniportChange *PMINIPORTCHANGE;

//=============================================================================
// Function Prototypes
//=============================================================================
NTSTATUS
NewAdapterCommon
( 
    _Out_       PUNKNOWN *              Unknown,
    _In_        REFCLSID,
    _In_opt_    PUNKNOWN                UnknownOuter,
    _In_        POOL_FLAGS              PoolFlags
);


#if defined(SYSVAD_BTH_BYPASS) || defined(SYSVAD_USB_SIDEBAND)

// Event callback definition.
typedef VOID (*PFNEVENTNOTIFICATION)(
    _In_opt_    PVOID   Context
);

DEFINE_GUID(IID_IBthHfpDeviceCommon,
    0x576b824a, 0x5248, 0x47b1, 0x82, 0xc5, 0xe4, 0x7b, 0xa7, 0xe2, 0xaf, 0x2b);

DEFINE_GUID(IID_IUsbHsDeviceCommon,
    0xb57b5547, 0x63f, 0x4a58, 0xb3, 0x7, 0x90, 0xb2, 0xfc, 0x6c, 0x32, 0xdb);

DEFINE_GUID(IID_IA2dpHpDeviceCommon,
    0xe1a6f128, 0x905, 0x4c18, 0xb4, 0x84, 0xc2, 0xca, 0x45, 0xb1, 0x57, 0x6d);



//=============================================================================
// Interfaces
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// ISidebandDeviceCommon
//
DECLARE_INTERFACE_(ISidebandDeviceCommon, IUnknown)
{
    STDMETHOD_(BOOL,                IsVolumeSupported)
    (
        THIS_
        _In_        eDeviceType             deviceType
    ) PURE;
    
    STDMETHOD_(PVOID,               GetVolumeSettings)
    (
        THIS_
        _In_        eDeviceType             deviceType,
        _Out_       PULONG                  Size 
    ) PURE;

    STDMETHOD_(NTSTATUS,            GetVolume)
    (
        THIS_
        _In_        eDeviceType             deviceType,
        _In_        LONG                    Channel,
        _Out_       LONG                    *pVolume
    ) PURE;

    STDMETHOD_(NTSTATUS,            SetVolume)
    (
        THIS_
        _In_        eDeviceType             deviceType,
        _In_        LONG                    Channel,
        _In_        LONG                    Volume
    ) PURE;

    STDMETHOD_(BOOL,                IsMuteSupported)
    (
        THIS_
        _In_        eDeviceType             deviceType
    ) PURE;
    
    STDMETHOD_(PVOID,               GetMuteSettings)
    (
        THIS_
        _In_        eDeviceType             deviceType,
        _Out_       PULONG                  Size 
    ) PURE;

    STDMETHOD_(LONG,                GetMute)
    (
        THIS_
        _In_        eDeviceType             deviceType,
        _In_        LONG                    Channel
    ) PURE;

    STDMETHOD_(NTSTATUS,            SetMute)
    (
        THIS_
        _In_        eDeviceType             deviceType,
        _In_        LONG                    Channel,
        _In_        LONG                    Mute
    ) PURE;

    STDMETHOD_(BOOL,                GetConnectionStatus)
    (
        THIS_
    ) PURE;
    
    STDMETHOD_(NTSTATUS,            Connect)
    (
        THIS_
    ) PURE;

    STDMETHOD_(NTSTATUS,            Disconnect)
    (
        THIS_
    ) PURE;
    
    STDMETHOD_(BOOL,                GetStreamStatus)
    (
        THIS_
        _In_        eDeviceType             deviceType
    ) PURE;

    STDMETHOD_(NTSTATUS,            StreamOpen)
    (
        THIS_
        _In_        eDeviceType             deviceType
    ) PURE;

    STDMETHOD_(NTSTATUS,            StreamStart)
    (
        THIS_
        _In_        eDeviceType             deviceType
    ) PURE;

    STDMETHOD_(NTSTATUS,            StreamSuspend)
    (
        THIS_
        _In_        eDeviceType             deviceType
    ) PURE;

    STDMETHOD_(NTSTATUS,            StreamClose)
    (
        THIS_
        _In_        eDeviceType             deviceType
    ) PURE;

    STDMETHOD_(GUID,                GetContainerId)
    (
        THIS_
        _In_        eDeviceType             deviceType
    ) PURE;

    STDMETHOD_(VOID,                SetVolumeHandler)
    (
        THIS_
        _In_        eDeviceType             deviceType,
        _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
        _In_opt_    PVOID                   EventHandlerContext
    ) PURE;

    STDMETHOD_(VOID,                SetMuteHandler)
    (
        THIS_
        _In_        eDeviceType             deviceType,
        _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
        _In_opt_    PVOID                   EventHandlerContext
    ) PURE;
    
    STDMETHOD_(VOID,                SetConnectionStatusHandler)
    (
        THIS_
        _In_        eDeviceType             deviceType,
        _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
        _In_opt_    PVOID                   EventHandlerContext
    ) PURE;

    STDMETHOD_(VOID,                SetFormatChangeHandler)
    (
        THIS_
        _In_        eDeviceType             deviceType,
        _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
        _In_opt_    PVOID                   EventHandlerContext
    ) PURE;

    STDMETHOD_(PPIN_DEVICE_FORMATS_AND_MODES, GetFormatsAndModes)
    (
        THIS_
        _In_        eDeviceType             deviceType
    ) PURE;

    _IRQL_requires_max_(DISPATCH_LEVEL)
    STDMETHOD_(BOOL,                IsNRECSupported)
    (
        THIS_
    ) PURE;
    
    STDMETHOD_(BOOL,                GetNRECDisableStatus)
    (
        THIS_
    ) PURE;
};
typedef ISidebandDeviceCommon *PSIDEBANDDEVICECOMMON;

#endif // SYSVAD_BTH_BYPASS

#endif  //_SYSVAD_COMMON_H_


