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
#include "BthhfpDevice.h"
#endif // SYSVAD_BTH_BYPASS

#ifdef SYSVAD_USB_SIDEBAND
#include <usbspec.h>
#include <usb.h>
#include <SidebandAudio.h>
#include <USBSidebandAudio.h>
#include "UsbHsMinipairs.h"
#include "UsbHsDevice.h"
#endif // SYSVAD_USB_SIDEBAND


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

#ifdef SYSVAD_USB_SIDEBAND
class UsbHsDevice;     // Forward declaration.
#endif // SYSVAD_USB_SIDEBAND


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

#ifdef SYSVAD_USB_SIDEBAND
        typedef struct _SysvadPowerRelationsDo
        {
            LIST_ENTRY          ListEntry;
            PDEVICE_OBJECT      Pdo;
        }SysVadPowerRelationsDo, *PSysVadPowerRelationsDo;
        LIST_ENTRY              m_PowerRelations;
        FAST_MUTEX              m_PowerRelationsLock;
#endif//SYSVAD_USB_SIDEBAND

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

        STDMETHODIMP_(NTSTATUS) NotifyEndpointPair
        ( 
            _In_ WCHAR              *RenderEndpointTopoName,
            _In_ ULONG              RenderEndpointNameLen,
            _In_ ULONG              RenderPinId,
            _In_ WCHAR              *CaptureEndpointTopoName,
            _In_ ULONG              CaptureEndpointNameLen,
            _In_ ULONG              CapturePinId
        );

#ifdef SYSVAD_BTH_BYPASS
        STDMETHODIMP_(NTSTATUS) InitBthScoBypass();
        
        STDMETHODIMP_(VOID)     CleanupBthScoBypass();
#endif // SYSVAD_BTH_BYPASS

#ifdef SYSVAD_USB_SIDEBAND
        STDMETHODIMP_(NTSTATUS) InitUsbSideband();
        
        STDMETHODIMP_(VOID)     CleanupUsbSideband();

        STDMETHODIMP_(NTSTATUS) AddDeviceAsPowerDependency
        (
            _In_ PDEVICE_OBJECT     pdo
        );

        STDMETHODIMP_(NTSTATUS) RemoveDeviceAsPowerDependency
        (
            _In_ PDEVICE_OBJECT     pdo
        );
#endif // SYSVAD_USB_SIDEBAND


        STDMETHODIMP_(VOID) Cleanup();

#ifdef SYSVAD_USB_SIDEBAND
        STDMETHODIMP_(NTSTATUS) UpdatePowerRelations(_In_ PIRP Irp);
#endif // SYSVAD_USB_SIDEBAND
        
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
        NPAGED_LOOKASIDE_LIST   m_BthHfpWorkTaskPool;           // LookasideList
        size_t                  m_BthHfpWorkTaskPoolElementSize;
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

#ifdef SYSVAD_USB_SIDEBAND
        //=====================================================================
        // USB Sideband Audio support.

    private:
        PVOID                   m_UsbSidebandNotificationHandle;
        FAST_MUTEX              m_UsbSidebandFastMutex;              // To serialize access.
        WDFWORKITEM             m_UsbSidebandWorkItem;               // Async work-item.
        LIST_ENTRY              m_UsbSidebandWorkTasks;              // Work-item's tasks.
        LIST_ENTRY              m_UsbSidebandDevices;                // USB Sideband devices.
        NPAGED_LOOKASIDE_LIST   m_UsbSidebandWorkTaskPool;           // LookasideList
        size_t                  m_UsbSidebandWorkTaskPoolElementSize;
        BOOL                    m_UsbSidebandEnableCleanup;          // Do cleanup if true.

    private:
        static
            DRIVER_NOTIFICATION_CALLBACK_ROUTINE  EvtUsbSidebandInterfaceChange;

        static
            EVT_WDF_WORKITEM                      EvtUsbSidebandInterfaceWorkItem;

    protected:
        UsbHsDevice * UsbSidebandDeviceFind
        (
            _In_ PUNICODE_STRING SymbolicLinkName
        );

        NTSTATUS UsbSidebandInterfaceArrival
        (
            _In_ PUNICODE_STRING SymbolicLinkName
        );

        NTSTATUS UsbSidebandInterfaceRemoval
        (
            _In_ PUNICODE_STRING SymbolicLinkName
        );

#endif // SYSVAD_USB_SIDEBAND


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
        _In_opt_ PCWSTR                                             TemplateReferenceString,
        _In_ ULONG                                                  cPropertyCount,
        _In_reads_opt_(cPropertyCount) const SYSVAD_DEVPROPERTY        *pProperties,
        _Out_ _At_(AudioSymbolicLinkName->Buffer, __drv_allocatesMem(Mem)) PUNICODE_STRING AudioSymbolicLinkName
    );

    NTSTATUS MigrateDeviceInterfaceTemplateParameters
    (
        _In_ PUNICODE_STRING    SymbolicLinkName,
        _In_opt_ PCWSTR         TemplateReferenceString
    );
};

typedef struct _MINIPAIR_UNKNOWN
{
    LIST_ENTRY              ListEntry;
    WCHAR                   Name[MAX_PATH];
    PUNKNOWN                PortInterface;
    PUNKNOWN                MiniportInterface;
    PADAPTERPOWERMANAGEMENT PowerInterface;
    PMINIPORTCHANGE         MiniportChange;
} MINIPAIR_UNKNOWN;

#define MAX_DEVICE_REG_KEY_LENGTH 0x100

//
// Used to implement the singleton pattern.
//
LONG  CAdapterCommon::m_AdapterInstances = 0;



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
#ifdef SYSVAD_USB_SIDEBAND
    ASSERT(IsListEmpty(&m_PowerRelations));
#endif // SYSVAD_USB_SIDEBAND
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

#ifdef SYSVAD_USB_SIDEBAND
    m_UsbSidebandEnableCleanup = FALSE;
#endif // SYSVAD_USB_SIDEBAND


    m_pServiceGroupWave     = NULL;
    m_pDeviceObject         = DeviceObject;
    m_pPhysicalDeviceObject = NULL;
    m_WdfDevice             = NULL;
    m_PowerState            = PowerDeviceD0;
    m_pHW                   = NULL;
    m_pPortClsEtwHelper     = NULL;

    InitializeListHead(&m_SubdeviceCache);

#ifdef SYSVAD_USB_SIDEBAND
    InitializeListHead(&m_PowerRelations);
    ExInitializeFastMutex(&m_PowerRelationsLock);
#endif//SYSVAD_USB_SIDEBAND

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

Event type  : eMINIPORT_IHV_DEFINED 
Parameter 1 : Defined and used by IHVs
Parameter 2 : Defined and used by IHVs
Parameter 3 : Defined and used by IHVs
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
    _In_opt_ PCWSTR TemplateReferenceString,
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
    // Migrate optional device interface parameters from the template if it exists
    // This is done first, so that any additional parameters in pProperties will override the defaults.
    //
    if (NULL != TemplateReferenceString)
    {
        ntStatus = MigrateDeviceInterfaceTemplateParameters(AudioSymbolicLinkName, TemplateReferenceString);

        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("MigrateDeviceInterfaceTempalteParameters: MigrateDeviceInterfaceTemplateParameters(...): failed, 0x%x", ntStatus)),
            Done);
    }

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
    _In_opt_        PWSTR                                   TemplateName,
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

    ntStatus = CreateAudioInterfaceWithProperties(Name, TemplateName, cPropertyCount, pProperties, &symbolicLink);
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
        UnknownMiniport->QueryInterface(IID_IMiniportChange, (PVOID *)&(pNewSubdevice->MiniportChange));

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
            SAFE_RELEASE(pRecord->MiniportChange);
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
        SAFE_RELEASE(pRecord->MiniportChange);
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

#ifdef SYSVAD_USB_SIDEBAND
    //
    // This ensures USB Sideband notifications are turned off when port class
    // cleanups and unregisters the static subdevices.
    //
    CleanupUsbSideband();
#endif // SYSVAD_USB_SIDEBAND


    EmptySubdeviceCache();
}

#ifdef SYSVAD_USB_SIDEBAND
//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
CAdapterCommon::UpdatePowerRelations(_In_ PIRP Irp)
{
    PDEVICE_RELATIONS   priorRelations = NULL;
    PDEVICE_RELATIONS   newRelations = NULL;
    ULONG               qprPdosCount = 0;
    ULONG               count = 0;
    size_t              size;
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG               i = 0;
    PLIST_ENTRY         pe = NULL;

    ExAcquireFastMutex(&m_PowerRelationsLock);

    pe = m_PowerRelations.Flink;
    while (pe != &m_PowerRelations)
    {
        pe = pe->Flink;
        qprPdosCount++;
    }

    if (0 == qprPdosCount)
    {
        DPF(D_ERROR, ("CAdapterCommon::UpdatePowerRelations: No PDOs in power relations"));
        // Not an error. Just nothing to do.
        newRelations = (PDEVICE_RELATIONS)(Irp->IoStatus.Information);
        goto Exit;
    }

    count = qprPdosCount;

    priorRelations = (PDEVICE_RELATIONS)Irp->IoStatus.Information;
    if (priorRelations != NULL)
    {
        //
        // Another driver in the stack may have added some entries.
        // Make sure we allocate space for these additional entries.
        //
        count = priorRelations->Count + count;
    }

    //
    // Allocate space for the DEVICE_RELATIONS structure (which includes
    // space for one PDEVICE_OBJECT, and then allocate enough additional
    // space for the extra PDEVICE_OBJECTs we need.
    //
    size = sizeof(DEVICE_RELATIONS) + (count - 1) * sizeof(PDEVICE_OBJECT);
    newRelations = (PDEVICE_RELATIONS)ExAllocatePoolWithTag(PagedPool, size, USBSIDEBANDTEST_POOLTAG015);
    ASSERT(newRelations);
    if (NULL == newRelations)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DPF(D_ERROR, ("CAdapterCommon::UpdatePowerRelations: could not allocate memory"));
        goto Exit;
    }

    //
    // If there was an existing device relations structure, copy
    // the entries to the new structure.
    //
    RtlZeroMemory(newRelations, size);
    if (priorRelations != NULL && priorRelations->Count > 0)
    {
        size = sizeof(DEVICE_RELATIONS) + (priorRelations->Count - 1) * sizeof(PDEVICE_OBJECT);
        RtlCopyMemory(newRelations, priorRelations, size);
    }

    //
    // Add new relations to the DEVICE_RELATIONS structure. Pnp dictates that
    // each PDO in the list be referenced. Pnp manager will deref the PDO.
    //
    pe = m_PowerRelations.Flink;
    while (pe != &m_PowerRelations)
    {
        PSysVadPowerRelationsDo powerDepDo = CONTAINING_RECORD(pe, SysVadPowerRelationsDo, ListEntry);
        pe = pe->Flink;

#pragma prefast(suppress: __WARNING_BUFFER_OVERFLOW, "the access to newRelation->Objects is in-range")
        newRelations->Objects[newRelations->Count] = powerDepDo->Pdo;

        // Add a reference on the PDO before returning it as a dependency.
        // PnP will remove the reference when appropriate as per msdn.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/irp-mn-query-device-relations#operation
        ObReferenceObject(powerDepDo->Pdo);

        //
        // update the count
        //
        newRelations->Count++;
    }

Exit:
    ExReleaseFastMutex(&m_PowerRelationsLock);

    if (!NT_SUCCESS(status))
    {
        //
        // Dereference any previously reported relations before exiting. They
        // are dereferenced here because the PNP manager will see error and not
        // do anything while the driver which added these objects expects the
        // pnp manager to do the dereference. Since this device is changing the
        // status, it must act like the pnp manager.
        //
        if (priorRelations != NULL)
        {
            for (i = 0; i < priorRelations->Count; ++i)
            {
                ObDereferenceObject(priorRelations->Objects[i]);
            }
        }

        ASSERT(newRelations == NULL);
    }

    if (priorRelations != NULL)
    {
        ExFreePool(priorRelations);
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = (ULONG_PTR)newRelations;

    return status;
}
#endif // SYSVAD_USB_SIDEBAND

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
                                    MiniportPair->TemplateTopoName,
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
                                    MiniportPair->TemplateWaveName,
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
        ExFreeToNPagedLookasideList(&This->m_BthHfpWorkTaskPool, task);
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
    bthWorkTask = (BthHfpWorkTask*)ExAllocateFromNPagedLookasideList(&m_BthHfpWorkTaskPool);
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
            ExFreeToNPagedLookasideList(&m_BthHfpWorkTaskPool, bthWorkTask);
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
    bthWorkTask = (BthHfpWorkTask*)ExAllocateFromNPagedLookasideList(&m_BthHfpWorkTaskPool);
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
    m_BthHfpWorkTaskPoolElementSize = sizeof(BthHfpWorkTask);
    ExInitializeNPagedLookasideList(&m_BthHfpWorkTaskPool,
                                    NULL,
                                    NULL,
                                    POOL_NX_ALLOCATION,
                                    m_BthHfpWorkTaskPoolElementSize,
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
    ExDeleteNPagedLookasideList(&m_BthHfpWorkTaskPool);
}
#endif  // SYSVAD_BTH_BYPASS

#ifdef SYSVAD_USB_SIDEBAND
//
// CAdapterCommon USB Sideband function implementation.
//

//=============================================================================
#pragma code_seg("PAGE")
VOID
CAdapterCommon::EvtUsbSidebandInterfaceWorkItem
(
    _In_    WDFWORKITEM WorkItem
)
/*++

Routine Description:

The function handles the arrival or removal of a USB Sideband interface.

Arguments:

WorkItem    - WDF work-item object.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[EvtUsbSidebandInterfaceWorkItem]"));

    CAdapterCommon        * This;

    if (WorkItem == NULL)
    {
        return;
    }

    This = GetUsbHsWorkItemContext(WorkItem)->Adapter;
    ASSERT(This != NULL);

    for (;;)
    {
        PLIST_ENTRY         le = NULL;
        UsbHsWorkTask    * task = NULL;

        //
        // Retrieve a taask.
        //
        ExAcquireFastMutex(&This->m_UsbSidebandFastMutex);
        if (!IsListEmpty(&This->m_UsbSidebandWorkTasks))
        {
            le = RemoveHeadList(&This->m_UsbSidebandWorkTasks);
            task = CONTAINING_RECORD(le, UsbHsWorkTask, ListEntry);
            InitializeListHead(le);
        }
        ExReleaseFastMutex(&This->m_UsbSidebandFastMutex);

        if (task == NULL)
        {
            break;
        }

        ASSERT(task->Device != NULL);
        _Analysis_assume_(task->Device != NULL);

        //
        // Process the task.
        //
        switch (task->Action)
        {
        case eUsbHsTaskStart:
            task->Device->Start();
            break;

        case eUsbHsTaskStop:
            task->Device->Stop();
            break;

        default:
            DPF(D_ERROR, ("EvtUsbSidebandInterfaceWorkItem: invalid action %d", task->Action));
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
        ExFreeToNPagedLookasideList(&This->m_UsbSidebandWorkTaskPool, task);
    }
}

//=============================================================================
#pragma code_seg("PAGE")
UsbHsDevice *
CAdapterCommon::UsbSidebandDeviceFind
(
    _In_ PUNICODE_STRING SymbolicLinkName
)
/*++

Routine Description:

The function looks for the specified device in the adapter's list.

Arguments:

SymbolicLinkName - interface's symbolic link.

Return Value:

UsbSidebandDevice pointer or NULL.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::UsbSidebandDeviceFind]"));

    PLIST_ENTRY     le = NULL;
    UsbHsDevice  * usbDevice = NULL;

    ExAcquireFastMutex(&m_UsbSidebandFastMutex);

    for (le = m_UsbSidebandDevices.Flink; le != &m_UsbSidebandDevices; le = le->Flink)
    {
        UsbHsDevice  *     tmpUsbHsDevice = UsbHsDevice::GetUsbHsDevice(le);
        ASSERT(tmpUsbHsDevice != NULL);

        PUNICODE_STRING     unicodeStr = tmpUsbHsDevice->GetSymbolicLinkName();
        ASSERT(unicodeStr != NULL);

        if (unicodeStr->Length == SymbolicLinkName->Length &&
            0 == wcsncmp(unicodeStr->Buffer, SymbolicLinkName->Buffer, unicodeStr->Length / sizeof(WCHAR)))
        {
            // Found it!
            usbDevice = tmpUsbHsDevice;
            usbDevice->AddRef();
            break;
        }
    }

    ExReleaseFastMutex(&m_UsbSidebandFastMutex);

    return usbDevice;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CAdapterCommon::UsbSidebandInterfaceArrival
(
    _In_ PUNICODE_STRING SymbolicLinkName
)
/*++

Routine Description:

The function handles the arrival of a new USB Sideband interface.

Arguments:

SymbolicLinkName - new interface's symbolic link.

Return Value:

NT status code.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::UsbSidebandInterfaceArrival]"));

    NTSTATUS            ntStatus = STATUS_SUCCESS;
    UsbHsDevice         *usbHsDevice = NULL;
    UsbHsWorkTask       *usbHsWorkTask = NULL;

    DPF(D_VERBOSE, ("UsbSidebandInterfaceArrival: SymbolicLinkName %wZ", SymbolicLinkName));

    //
    // Check if the USB device is already present.
    // According to the docs it is possible to receive two notifications for the same
    // interface.
    //
    usbHsDevice = UsbSidebandDeviceFind(SymbolicLinkName);
    if (usbHsDevice != NULL)
    {
        DPF(D_VERBOSE, ("UsbSidebandInterfaceArrival: USB device already present"));
        SAFE_RELEASE(usbHsDevice);
        ntStatus = STATUS_SUCCESS;
        goto Done;
    }

    //
    // Alloc a new structure for this USB device.
    //
    usbHsDevice = new (NonPagedPoolNx, MINADAPTER_POOLTAG) UsbHsDevice(NULL); // NULL -> OuterUnknown
    if (NULL == usbHsDevice)
    {
        DPF(D_ERROR, ("UsbSidebandInterfaceArrival: unable to allocate UsbSidebandDevice, out of memory"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    DPF(D_VERBOSE, ("UsbSidebandInterfaceArrival: created UsbSidebandDevice 0x%p ", usbHsDevice));

    //
    // Basic initialization of the USB Sideband interface.
    // The audio miniport creation is done later by the UsbSidebandDevice.Start()
    // which is invoked asynchronously by a worker thread.
    // UsbSidebandDevice->Init() must be invoked just after the creation of the object.
    //
    ntStatus = usbHsDevice->Init(this, SymbolicLinkName);
    IF_FAILED_JUMP(ntStatus, Done);

    //
    // Get and init a work task.
    //
    usbHsWorkTask = (UsbHsWorkTask*)ExAllocateFromNPagedLookasideList(&m_UsbSidebandWorkTaskPool);
    if (NULL == usbHsWorkTask)
    {
        DPF(D_ERROR, ("UsbSidebandInterfaceArrival: unable to allocate UsbSidebandWorkTask, out of memory"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    // usbWorkTask->L.Size is set to sizeof(UsbSidebandWorkTask) in the Look Aside List configuration
#pragma warning(suppress: 6386)
    RtlZeroMemory(usbHsWorkTask, sizeof(*usbHsWorkTask));
    usbHsWorkTask->Action = eUsbHsTaskStart;
    InitializeListHead(&usbHsWorkTask->ListEntry);
    // Note that usbDevice has one reference at this point.
    usbHsWorkTask->Device = usbHsDevice;

    ExAcquireFastMutex(&m_UsbSidebandFastMutex);

    //
    // Insert this new USB Sideband device in our list.
    //
    InsertTailList(&m_UsbSidebandDevices, usbHsDevice->GetListEntry());

    //
    // Add a new task for the worker thread.
    //
    InsertTailList(&m_UsbSidebandWorkTasks, &usbHsWorkTask->ListEntry);
    usbHsDevice->AddRef();    // released when task runs.

                            //
                            // Schedule a work-item if not already running.
                            //
    WdfWorkItemEnqueue(m_UsbSidebandWorkItem);

    ExReleaseFastMutex(&m_UsbSidebandFastMutex);

Done:
    if (!NT_SUCCESS(ntStatus))
    {
        // Release the last ref, this will delete the UsbSidebandDevice 
        SAFE_RELEASE(usbHsDevice);

        if (usbHsWorkTask != NULL)
        {
            ExFreeToNPagedLookasideList(&m_UsbSidebandWorkTaskPool, usbHsWorkTask);
            usbHsWorkTask = NULL;
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CAdapterCommon::UsbSidebandInterfaceRemoval
(
    _In_ PUNICODE_STRING SymbolicLinkName
)
/*++

Routine Description:

The function handles the removal of a USB Sideband interface.

Arguments:

SymbolicLinkName - interface's symbolic link to remove.

Return Value:

NT status code.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::UsbSidebandInterfaceRemoval]"));

    NTSTATUS            ntStatus = STATUS_SUCCESS;
    UsbHsDevice         *usbHsDevice = NULL;
    UsbHsWorkTask       *usbHsWorkTask = NULL;

    DPF(D_VERBOSE, ("UsbSidebandInterfaceRemoval: SymbolicLinkName %wZ", SymbolicLinkName));

    //
    // Check if the USB device is present.
    //
    usbHsDevice = UsbSidebandDeviceFind(SymbolicLinkName);
    if (usbHsDevice == NULL)
    {
        // This can happen if the init/start of the UsbSidebandDevice failed.
        DPF(D_VERBOSE, ("UsbSidebandInterfaceRemoval: USB device not found"));
        ntStatus = STATUS_SUCCESS;
        goto Done;
    }

    //
    // Init a work task.
    //
    usbHsWorkTask = (UsbHsWorkTask*)ExAllocateFromNPagedLookasideList(&m_UsbSidebandWorkTaskPool);
    if (NULL == usbHsWorkTask)
    {
        DPF(D_ERROR, ("UsbSidebandInterfaceRemoval: unable to allocate UsbSidebandWorkTask, out of memory"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    // usbWorkTask->L.Size is set to sizeof(UsbSidebandWorkTask) in the Look Aside List configuration
#pragma warning(suppress: 6386)
    RtlZeroMemory(usbHsWorkTask, sizeof(*usbHsWorkTask));
    usbHsWorkTask->Action = eUsbHsTaskStop;
    InitializeListHead(&usbHsWorkTask->ListEntry);
    // Work-item callback will release the reference we got above from UsbSidebandDeviceFind.
    usbHsWorkTask->Device = usbHsDevice;

    ExAcquireFastMutex(&m_UsbSidebandFastMutex);

    //
    // Remove this USB device from our list and release the associated reference.
    //
    RemoveEntryList(usbHsDevice->GetListEntry());
    InitializeListHead(usbHsDevice->GetListEntry());
    usbHsDevice->Release();   // This is not the last ref.

                            //
                            // Add a new task for the worker thread.
                            //
    InsertTailList(&m_UsbSidebandWorkTasks, &usbHsWorkTask->ListEntry);

    //
    // Schedule a work-item if not already running.
    //
    WdfWorkItemEnqueue(m_UsbSidebandWorkItem);

    ExReleaseFastMutex(&m_UsbSidebandFastMutex);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    if (!NT_SUCCESS(ntStatus))
    {
        // Release the ref we got in find.
        SAFE_RELEASE(usbHsDevice);
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CAdapterCommon::EvtUsbSidebandInterfaceChange(
    _In_          PVOID   NotificationPointer,
    _Inout_opt_   PVOID   Context
)
/*++

Routine Description:

This callback is invoked when a new USB Sideband interface is added or removed.

Arguments:
NotificationPointer - Interface change notification
Context - CAdapterCommon ptr.

Return Value:

NT status code.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[EvtUsbSidebandInterfaceChange]"));

    NTSTATUS                              ntStatus = STATUS_SUCCESS;
    CAdapterCommon                      * This = NULL;
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION Notification = (PDEVICE_INTERFACE_CHANGE_NOTIFICATION)NotificationPointer;

    //
    // Make sure this is the interface class we extect. Any other class guid
    // is an error, but let it go since it is not fatal to the machine.
    //
    if (!IsEqualGUID(Notification->InterfaceClassGuid, GUID_DEVINTERFACE_USB_SIDEBAND_AUDIO_HS_HCIBYPASS))
    {
        DPF(D_VERBOSE, ("EvtUsbSidebandInterfaceChange: bad interface ClassGuid"));
        ASSERTMSG("EvtUsbSidebandInterfaceChange: bad interface ClassGuid ", FALSE);

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
        ntStatus = This->UsbSidebandInterfaceArrival(Notification->SymbolicLinkName);
    }
    else if (IsEqualGUID(Notification->Event, GUID_DEVICE_INTERFACE_REMOVAL))
    {
        ntStatus = This->UsbSidebandInterfaceRemoval(Notification->SymbolicLinkName);
    }
    else
    {
        DPF(D_VERBOSE, ("EvtUsbSidebandInterfaceChange: bad "
            "GUID_DEVINTERFACE_USB_SIDEBAND_AUDIO_HCIBYPASS event"));
        ASSERTMSG("EvtUsbSidebandInterfaceChange: bad "
            "GUID_DEVINTERFACE_USB_SIDEBAND_AUDIO_HCIBYPASS event ", FALSE);

        goto Done;
    }

Done:
    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CAdapterCommon::InitUsbSideband()
/*++

Routine Description:

Initialize the USB Sideband environment.

Return Value:

NT status code.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::InitUsbSideband]"));

    NTSTATUS                ntStatus = STATUS_SUCCESS;
    WDF_WORKITEM_CONFIG     wiConfig;
    WDF_OBJECT_ATTRIBUTES   attributes;
    UsbHsWorkItemContext * wiContext;

    //
    // Init spin-lock, linked lists, work-item, event, etc.
    // Init all members to default values. This basic init should not fail.
    //
    m_UsbSidebandWorkItem = NULL;
    m_UsbSidebandNotificationHandle = NULL;
    ExInitializeFastMutex(&m_UsbSidebandFastMutex);
    InitializeListHead(&m_UsbSidebandWorkTasks);
    InitializeListHead(&m_UsbSidebandDevices);
    m_UsbSidebandWorkTaskPoolElementSize = sizeof(UsbHsWorkTask);
    ExInitializeNPagedLookasideList(&m_UsbSidebandWorkTaskPool,
        NULL,
        NULL,
        POOL_NX_ALLOCATION,
        m_UsbSidebandWorkTaskPoolElementSize,
        MINADAPTER_POOLTAG,
        0);
    //
    // Enable USB Sideband Cleanup.
    // Do any allocation/initialization that can fail after this point.
    //
    m_UsbSidebandEnableCleanup = TRUE;

    //
    // Allocate a WDF work-item.
    //
    WDF_WORKITEM_CONFIG_INIT(&wiConfig, EvtUsbSidebandInterfaceWorkItem);
    wiConfig.AutomaticSerialization = FALSE;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, UsbHsWorkItemContext);
    attributes.ParentObject = GetWdfDevice();
    ntStatus = WdfWorkItemCreate(&wiConfig,
        &attributes,
        &m_UsbSidebandWorkItem);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("InitUsbSideband: WdfWorkItemCreate failed: 0x%x", ntStatus)),
        Done);

    wiContext = GetUsbHsWorkItemContext(m_UsbSidebandWorkItem);
    wiContext->Adapter = this; // weak ref.

                               //
                               // Register for USB Sideband interface changes.
                               //
    ntStatus = IoRegisterPlugPlayNotification(
        EventCategoryDeviceInterfaceChange,
        PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
        (PVOID)&GUID_DEVINTERFACE_USB_SIDEBAND_AUDIO_HS_HCIBYPASS,
        m_pDeviceObject->DriverObject,
        EvtUsbSidebandInterfaceChange,
        (PVOID)this,
        &m_UsbSidebandNotificationHandle);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("InitUsbSideband: IoRegisterPlugPlayNotification(GUID_DEVINTERFACE_USB_SIDEBAND_AUDIO_HCIBYPASS) failed: 0x%x", ntStatus)),
        Done);

    //
    // Initialization completed.
    //
    ntStatus = STATUS_SUCCESS;

Done:
    return ntStatus;
}

//=============================================================================
#pragma code_seg()
NTSTATUS
CAdapterCommon::AddDeviceAsPowerDependency
(
    _In_ PDEVICE_OBJECT pdo
)
{
    NTSTATUS status = STATUS_SUCCESS;

    // allocate SysVadPowerRelationsDo
    PSysVadPowerRelationsDo powerDepDo = (PSysVadPowerRelationsDo)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(SysVadPowerRelationsDo), USBSIDEBANDTEST_POOLTAG014);
    if (NULL == powerDepDo)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DPF(D_ERROR, ("CAdapterCommon::AddDeviceAsPowerDependency could not allocate memory for list entry"));
        goto exit;
    }

    InitializeListHead(&powerDepDo->ListEntry);
    powerDepDo->Pdo = pdo;
    ObReferenceObject(pdo);

    // Add to list
    ExAcquireFastMutex(&m_PowerRelationsLock);

    InsertTailList(&m_PowerRelations, &powerDepDo->ListEntry);

    ExReleaseFastMutex(&m_PowerRelationsLock);

    IoInvalidateDeviceRelations(m_pPhysicalDeviceObject, PowerRelations);

exit:
    return status;
}

//=============================================================================
#pragma code_seg()
NTSTATUS
CAdapterCommon::RemoveDeviceAsPowerDependency
(
    _In_ PDEVICE_OBJECT pdo
)
{
    NTSTATUS status = STATUS_SUCCESS;

    // Find in list
    ExAcquireFastMutex(&m_PowerRelationsLock);

    PLIST_ENTRY pe = m_PowerRelations.Flink;
    while (pe != &m_PowerRelations)
    {
        PSysVadPowerRelationsDo powerDepDo = CONTAINING_RECORD(pe, SysVadPowerRelationsDo, ListEntry);
        pe = pe->Flink;

        if (powerDepDo->Pdo == pdo)
        {
            ObDereferenceObject(powerDepDo->Pdo);
            RemoveEntryList(&powerDepDo->ListEntry);
            ExFreePoolWithTag(powerDepDo, USBSIDEBANDTEST_POOLTAG014);
        }
    }

    ExReleaseFastMutex(&m_PowerRelationsLock);

    IoInvalidateDeviceRelations(m_pPhysicalDeviceObject, PowerRelations);

    return status;
}

//=============================================================================
#pragma code_seg("PAGE")
VOID
CAdapterCommon::CleanupUsbSideband()
/*++

Routine Description:

Cleanup the USB Sideband environment.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::CleanupUsbSideband]"));

    //
    // Do nothing if USB Sideband environment was not correctly initialized.
    //
    if (m_UsbSidebandEnableCleanup == FALSE)
    {
        return;
    }

    //
    // Unregister for USB Sideband interface changes.
    //
    if (m_UsbSidebandNotificationHandle != NULL)
    {
        (void)IoUnregisterPlugPlayNotificationEx(m_UsbSidebandNotificationHandle);
        m_UsbSidebandNotificationHandle = NULL;
    }

    //
    // Wait for the USB Sideband worker thread to be done.
    //
    if (m_UsbSidebandWorkItem != NULL)
    {
        WdfWorkItemFlush(m_UsbSidebandWorkItem);
        WdfObjectDelete(m_UsbSidebandWorkItem);
        m_UsbSidebandWorkItem = NULL;
    }

    ASSERT(IsListEmpty(&m_UsbSidebandWorkTasks));

    //
    // Stop and delete all UsbSidebandDevices. We are the only thread accessing this list, 
    // so there is no need to acquire the mutex.
    //
    while (!IsListEmpty(&m_UsbSidebandDevices))
    {
        UsbHsDevice  * usbHsDevice = NULL;
        PLIST_ENTRY     le = NULL;

        le = RemoveHeadList(&m_UsbSidebandDevices);

        usbHsDevice = UsbHsDevice::GetUsbHsDevice(le);
        InitializeListHead(le);

        // usbDevice is invalid after this call.
        usbHsDevice->Stop();

        // This should be the last reference.
        usbHsDevice->Release();
    }

    ASSERT(IsListEmpty(&m_UsbSidebandDevices));

    //
    // General cleanup.
    //
    ExDeleteNPagedLookasideList(&m_UsbSidebandWorkTaskPool);
}
#endif  // SYSVAD_USB_SIDEBAND


#pragma code_seg("PAGE")
NTSTATUS
CopyRegistryValues(HANDLE _hSourceKey, HANDLE _hDestinationKey)
/*++

Routine Description:

  This method copies the registry values in _hSourceKey to _hDestinationKey.

Return Value:

  NT status code.

--*/
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PKEY_VALUE_FULL_INFORMATION kvFullInfo = NULL;
    ULONG                       ulFullInfoLength = 0;
    ULONG                       ulFullInfoResultLength = 0;
    PWSTR                       pwstrKeyValueName = NULL;
    UNICODE_STRING              strKeyValueName;
    PAGED_CODE();
    // Allocate the KEY_VALUE_FULL_INFORMATION structure
    ulFullInfoLength = sizeof(KEY_VALUE_FULL_INFORMATION) + MAX_DEVICE_REG_KEY_LENGTH;
    kvFullInfo = (PKEY_VALUE_FULL_INFORMATION)ExAllocatePoolWithTag(NonPagedPoolNx, ulFullInfoLength, MINADAPTER_POOLTAG);
    IF_TRUE_ACTION_JUMP(kvFullInfo == NULL, ntStatus = STATUS_INSUFFICIENT_RESOURCES, Exit);

    // Iterate over each value and copy it to the destination
    for (UINT i = 0; NT_SUCCESS(ntStatus); i++)
    {
        // Enumerate the next value
        ntStatus = ZwEnumerateValueKey(_hSourceKey, i, KeyValueFullInformation, kvFullInfo, ulFullInfoLength, &ulFullInfoResultLength);

        // Jump out of this loop if there are no more values
        IF_TRUE_ACTION_JUMP(ntStatus == STATUS_NO_MORE_ENTRIES, ntStatus = STATUS_SUCCESS, Exit);

        // Handle incorrect buffer size
        if (ntStatus == STATUS_BUFFER_TOO_SMALL || ntStatus == STATUS_BUFFER_OVERFLOW)
        {
            // Free and re-allocate the KEY_VALUE_FULL_INFORMATION structure with the correct size
            ExFreePoolWithTag(kvFullInfo, MINADAPTER_POOLTAG);

            ulFullInfoLength = ulFullInfoResultLength;

            kvFullInfo = (PKEY_VALUE_FULL_INFORMATION)ExAllocatePoolWithTag(NonPagedPoolNx, ulFullInfoLength, MINADAPTER_POOLTAG);
            IF_TRUE_ACTION_JUMP(kvFullInfo == NULL, ntStatus = STATUS_INSUFFICIENT_RESOURCES, loop_exit);

            // Try to enumerate the current value again
            ntStatus = ZwEnumerateValueKey(_hSourceKey, i, KeyValueFullInformation, kvFullInfo, ulFullInfoLength, &ulFullInfoResultLength);

            // Jump out of this loop if there are no more values
            IF_TRUE_ACTION_JUMP(ntStatus == STATUS_NO_MORE_ENTRIES, ntStatus = STATUS_SUCCESS, Exit);
            IF_FAILED_JUMP(ntStatus, loop_exit);
        }
        else
        {
            IF_FAILED_JUMP(ntStatus, loop_exit);
        }

        // Allocate the key value name string
        pwstrKeyValueName = (PWSTR)ExAllocatePoolWithTag(NonPagedPoolNx, kvFullInfo->NameLength + sizeof(WCHAR)*2, MINADAPTER_POOLTAG);
        IF_TRUE_ACTION_JUMP(kvFullInfo == NULL, ntStatus = STATUS_INSUFFICIENT_RESOURCES, loop_exit);

        // Copy the key value name from the full information struct
        RtlStringCbCopyNW(pwstrKeyValueName, kvFullInfo->NameLength + sizeof(WCHAR)*2, kvFullInfo->Name, kvFullInfo->NameLength);

        // Make sure the string is null terminated
        pwstrKeyValueName[(kvFullInfo->NameLength) / sizeof(WCHAR)] = 0;

        // Copy the key value name string to a UNICODE string
        RtlInitUnicodeString(&strKeyValueName, pwstrKeyValueName);

        // Write the key value from the source into the destination
        ntStatus = ZwSetValueKey(_hDestinationKey, &strKeyValueName, 0, kvFullInfo->Type, (PVOID)((PUCHAR)kvFullInfo + kvFullInfo->DataOffset), kvFullInfo->DataLength);
        IF_FAILED_JUMP(ntStatus, loop_exit);

    loop_exit:
        // Free the key value name string
        if (pwstrKeyValueName)
        {
            ExFreePoolWithTag(pwstrKeyValueName, MINADAPTER_POOLTAG);
        }

        // Bail if anything failed
        IF_FAILED_JUMP(ntStatus, Exit);
    }

Exit:
    // Free the KEY_VALUE_FULL_INFORMATION structure
    if (kvFullInfo)
    {
        ExFreePoolWithTag(kvFullInfo, MINADAPTER_POOLTAG);
    }

    return ntStatus;
}

NTSTATUS
CopyRegistryKey(HANDLE _hSourceKey, HANDLE _hDestinationKey, BOOL _bOverwrite = FALSE)
/*++

Routine Description:

  This method recursively copies the registry values in _hSourceKey to _hDestinationKey.
  Set _bOverwrite to indicate whether the first level values are copied or not.
  Normal use is to set false for the initial call, and then all sub paths will be copied.

Return Value:

  NT status code.

--*/
{
    NTSTATUS                ntStatus = STATUS_UNSUCCESSFUL;
    PKEY_BASIC_INFORMATION  kBasicInfo = NULL;
    ULONG                   ulBasicInfoLength = 0;
    ULONG                   ulBasicInfoResultLength = 0;
    ULONG                   ulDisposition = 0;
    PWSTR                   pwstrKeyName = NULL;
    UNICODE_STRING          strKeyName;
    OBJECT_ATTRIBUTES       hCurrentSourceKeyAttributes;
    OBJECT_ATTRIBUTES       hNewDestinationKeyAttributes;
    HANDLE                  hCurrentSourceKey = NULL;
    HANDLE                  hNewDestinationKey = NULL;
    PAGED_CODE();
    // Validate parameters
    IF_TRUE_ACTION_JUMP(_hSourceKey == nullptr, ntStatus = STATUS_INVALID_PARAMETER, Exit);
    IF_TRUE_ACTION_JUMP(_hDestinationKey == nullptr, ntStatus = STATUS_INVALID_PARAMETER, Exit);

    // Allocate the KEY_BASIC_INFORMATION structure
    ulBasicInfoLength = sizeof(KEY_BASIC_INFORMATION) + MAX_DEVICE_REG_KEY_LENGTH;
    kBasicInfo = (PKEY_BASIC_INFORMATION)ExAllocatePoolWithTag(NonPagedPoolNx, ulBasicInfoLength, MINADAPTER_POOLTAG);
    IF_TRUE_ACTION_JUMP(kBasicInfo == NULL, ntStatus = STATUS_INSUFFICIENT_RESOURCES, Exit);

    ntStatus = STATUS_SUCCESS;
    // Iterate over each key and copy it
    for (UINT i = 0; NT_SUCCESS(ntStatus); i++)
    {
        // Enumerate the next key
        ntStatus = ZwEnumerateKey(_hSourceKey, i, KeyBasicInformation, kBasicInfo, ulBasicInfoLength, &ulBasicInfoResultLength);

        // Jump out of this loop if there are no more keys
        IF_TRUE_ACTION_JUMP(ntStatus == STATUS_NO_MORE_ENTRIES, ntStatus = STATUS_SUCCESS, copy_values);

        // Handle incorrect buffer size
        if (ntStatus == STATUS_BUFFER_TOO_SMALL || ntStatus == STATUS_BUFFER_OVERFLOW)
        {
            // Free and re-allocate the KEY_BASIC_INFORMATION structure with the correct size.
            ExFreePoolWithTag(kBasicInfo, MINADAPTER_POOLTAG);
            ulBasicInfoLength = ulBasicInfoResultLength;
            kBasicInfo = (PKEY_BASIC_INFORMATION)ExAllocatePoolWithTag(NonPagedPoolNx, ulBasicInfoLength, MINADAPTER_POOLTAG);
            IF_TRUE_ACTION_JUMP(kBasicInfo == NULL, ntStatus = STATUS_INSUFFICIENT_RESOURCES, loop_exit);

            // Try to enumerate the current key again.
            ntStatus = ZwEnumerateKey(_hSourceKey, i, KeyBasicInformation, kBasicInfo, ulBasicInfoLength, &ulBasicInfoResultLength);

            // Jump out of this loop if there are no more keys
            IF_TRUE_ACTION_JUMP(ntStatus == STATUS_NO_MORE_ENTRIES, ntStatus = STATUS_SUCCESS, copy_values);
            IF_FAILED_JUMP(ntStatus, loop_exit);
        }
        else
        {
            IF_FAILED_JUMP(ntStatus, loop_exit);
        }

        // Allocate the key name string 
        pwstrKeyName = (PWSTR)ExAllocatePoolWithTag(NonPagedPoolNx, kBasicInfo->NameLength + sizeof(WCHAR), MINADAPTER_POOLTAG);
        IF_TRUE_ACTION_JUMP(kBasicInfo == NULL, ntStatus = STATUS_INSUFFICIENT_RESOURCES, loop_exit);

        // Copy the key name from the basic information struct
        RtlStringCbCopyNW(pwstrKeyName, kBasicInfo->NameLength + sizeof(WCHAR), kBasicInfo->Name, kBasicInfo->NameLength);

        // Make sure the string is null terminated
        pwstrKeyName[(kBasicInfo->NameLength) / sizeof(WCHAR)] = 0;

        // Copy the key name string to a UNICODE string
        RtlInitUnicodeString(&strKeyName, pwstrKeyName);

        // Initialize attributes to open the currently enumerated source key
        InitializeObjectAttributes(&hCurrentSourceKeyAttributes, &strKeyName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, _hSourceKey, NULL);

        // Open the currently enumerated source key
        ntStatus = ZwOpenKey(&hCurrentSourceKey, KEY_READ, &hCurrentSourceKeyAttributes);
        IF_FAILED_ACTION_JUMP(ntStatus, ZwClose(hCurrentSourceKey), loop_exit);

        // Initialize attributes to create the new destination key
        InitializeObjectAttributes(&hNewDestinationKeyAttributes, &strKeyName, OBJ_KERNEL_HANDLE, _hDestinationKey, NULL);

        // Create the key at the destination
        ntStatus = ZwCreateKey(&hNewDestinationKey, KEY_WRITE, &hNewDestinationKeyAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, &ulDisposition);
        IF_FAILED_ACTION_JUMP(ntStatus, ZwClose(hCurrentSourceKey), loop_exit);

        // Now copy the contents of the currently enumerated key to the destination
        ntStatus = CopyRegistryKey(hCurrentSourceKey, hNewDestinationKey, TRUE);
        IF_FAILED_JUMP(ntStatus, loop_exit);

    loop_exit:
        // Free the key name string
        if (pwstrKeyName)
        {
            ExFreePoolWithTag(pwstrKeyName, MINADAPTER_POOLTAG);
        }

        // Close the current source key
        if (hCurrentSourceKey)
        {
            ZwClose(hCurrentSourceKey);
        }

        // Close the new destination key
        if (hNewDestinationKey)
        {
            ZwClose(hNewDestinationKey);
        }

        // Bail if anything failed
        IF_FAILED_JUMP(ntStatus, Exit);
    }

copy_values:
    // Copy the values 
    if (_bOverwrite)
    {
        ntStatus = CopyRegistryValues(_hSourceKey, _hDestinationKey);
        IF_FAILED_JUMP(ntStatus, Exit);
    }

Exit:
    // Free the basic information structure
    if (kBasicInfo)
    {
        ExFreePoolWithTag(kBasicInfo, MINADAPTER_POOLTAG);
    }
    return ntStatus;
}


NTSTATUS CAdapterCommon::MigrateDeviceInterfaceTemplateParameters
(
    _In_ PUNICODE_STRING    SymbolicLinkName,
    _In_opt_ PCWSTR         TemplateReferenceString
)
/*++

Routine Description:

  This method copies all of the properties from the template interface,
  which is specified in the inf, to the actual interface being used which
  may be dynamically generated at run time. This allows for a driver 
  to reuse a single inf entry for multiple audio endpoints. The primary 
  purpose for this is to allow for sideband audio endpoints to dynamically
  generate the reference string at run time, tied to the peripheral connected,
  while still having a simple static inf entry for setting up apo's or other 
  parameters.

  For example, if you have an interface in your inf defined with reference string
  "SpeakerWave". At runtime you could generate "SpeakerWave-1234ABCDE", and specify
  "SpeakerWave" as the template name. When "SpeakerWave-1234ABCDE" is installed
  we will copy all of the parameters that were specified in the inf for "SpeakerWave"
  over to "SpeakerWave-1234ABCDE". You simply need to specify "SpeakerWave" as the
  "TemplateName" in the ENDPOINT_MINIPAIRS.

  By default, the first level of registry keys are not copied. Only the 2nd level and
  deeper are copied. This way the friendly name and other PNP properties will not
  be modified, but the EP and FX properties will be copied.
  
Return Value:

  NT status code.

--*/

{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    HANDLE              hDeviceInterfaceParametersKey(NULL);
    HANDLE              hTemplateDeviceInterfaceParametersKey(NULL);
    UNICODE_STRING      TemplateSymbolicLinkName;
    UNICODE_STRING      referenceString;

    RtlInitUnicodeString(&TemplateSymbolicLinkName, NULL);
    RtlInitUnicodeString(&referenceString, TemplateReferenceString);

    //
    // Register an audio interface if not already present for the template interface, so we can access
    // the registry path. If it's already registered, this simply returns the symbolic link name. 
    // No need to unregister it (there is no mechanism to), and we'll never make it active.
    //
    ntStatus = IoRegisterDeviceInterface(
        GetPhysicalDeviceObject(),
        &KSCATEGORY_AUDIO,
        &referenceString,
        &TemplateSymbolicLinkName);

    // Open the template device interface's registry key path
    ntStatus = IoOpenDeviceInterfaceRegistryKey(&TemplateSymbolicLinkName, GENERIC_READ, &hTemplateDeviceInterfaceParametersKey);
    IF_FAILED_JUMP(ntStatus, Exit);

    // Open the new device interface's registry key path that we plan to activate
    ntStatus = IoOpenDeviceInterfaceRegistryKey(SymbolicLinkName, GENERIC_WRITE, &hDeviceInterfaceParametersKey);
    IF_FAILED_JUMP(ntStatus, Exit);

    // Copy the template device parameters key to the device interface key
    ntStatus = CopyRegistryKey(hTemplateDeviceInterfaceParametersKey, hDeviceInterfaceParametersKey);
    IF_FAILED_JUMP(ntStatus, Exit);

Exit:
    RtlFreeUnicodeString(&TemplateSymbolicLinkName);

    if (hTemplateDeviceInterfaceParametersKey)
    {
        ZwClose(hTemplateDeviceInterfaceParametersKey);
    }

    if (hDeviceInterfaceParametersKey)
    {
        ZwClose(hDeviceInterfaceParametersKey);
    }

    return ntStatus;
}

#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::NotifyEndpointPair
( 
    _In_ WCHAR              *RenderEndpointTopoName,
    _In_ ULONG              RenderEndpointNameLen,
    _In_ ULONG              RenderPinId,
    _In_ WCHAR              *CaptureEndpointTopoName,
    _In_ ULONG              CaptureEndpointNameLen,
    _In_ ULONG              CapturePinId
)
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;

    PAGED_CODE ();

    PLIST_ENTRY le = NULL;
    BOOL bRemoved = FALSE;

    // notify each subdevice which implements IMiniportChange
    for (le = m_SubdeviceCache.Flink; le != &m_SubdeviceCache && !bRemoved; le = le->Flink)
    {
        MINIPAIR_UNKNOWN *pRecord = CONTAINING_RECORD(le, MINIPAIR_UNKNOWN, ListEntry);

        if(pRecord->MiniportChange)
        {
            pRecord->MiniportChange->NotifyEndpointPair(
                RenderEndpointTopoName,
                RenderEndpointNameLen,
                RenderPinId,
                CaptureEndpointTopoName,
                CaptureEndpointNameLen,
                CapturePinId
            );
        }
    }

    return ntStatus;
}

