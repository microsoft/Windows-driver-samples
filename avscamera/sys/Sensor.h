/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        Sensor.h

    Abstract:

        Base Sensor class.  Derive from this class to implement an interface
        to your hardware sensor object.

        This class provides a set of "knobs" to control a model camera.

        This class also controls access to the pin simulations.  Most cameras
        have a limited set of ISP resources and can only instantiate a fixed
        number of pins.  This class grants access to those resources.  Other 
        than that, it's just an interface class.

    History:

        created 5/5/2014

**************************************************************************/

#pragma once

//
//  The following DECLARE_PROPERTY_XXX() macros are used to create consistent
//  function names for each control.
//
#define DECLARE_PROPERTY_GET( type, name )                      \
    virtual                                                     \
    NTSTATUS                                                    \
    Get##name(                                                  \
        _Inout_ type *pProperty                                 \
        );                                                      \

#define DECLARE_PROPERTY_SIZEOF( name )                         \
    virtual                                                     \
    ULONG                                                       \
    SizeOf##name();

#define DECLARE_PROPERTY_SET( type, name )                      \
    virtual                                                     \
    NTSTATUS                                                    \
    Set##name(                                                  \
        _In_    type *pProperty                                 \
        );

#define DECLARE_PROPERTY_SET_ASYNC( type, name )                \
    virtual                                                     \
    NTSTATUS                                                    \
    Set##name##Async(                                           \
        _In_    type        *pProperty,                         \
        _In_    CNotifier   *Notifier                           \
        );

#define DECLARE_PROPERTY_CANCEL( type, name )                   \
    virtual                                                     \
    NTSTATUS                                                    \
    Cancel##name();

#define DECLARE_PROPERTY( type, name )                          \
    DECLARE_PROPERTY_GET( type, name )                          \
    DECLARE_PROPERTY_SET( type, name )

#define DECLARE_PROPERTY_ASYNC_NOCANCEL( type, name )           \
    DECLARE_PROPERTY_GET( type, name )                          \
    DECLARE_PROPERTY_SET_ASYNC( type, name )

#define DECLARE_PROPERTY_ASYNC( type, name )                    \
    DECLARE_PROPERTY_ASYNC_NOCANCEL( type, name )               \
    DECLARE_PROPERTY_CANCEL( type, name )

#define DECLARE_PROPERTY_VARSIZE_ASYNC( type, name )            \
    DECLARE_PROPERTY_ASYNC( type, name )                        \
    DECLARE_PROPERTY_SIZEOF( name )

#define DECLARE_PROPERTY_VARSIZE_ASYNC_NOCANCEL( type, name )   \
    DECLARE_PROPERTY_ASYNC_NOCANCEL( type, name )               \
    DECLARE_PROPERTY_SIZEOF( name )


//
//  CNotifer
//
//  A CNotiver is just a wrapper on a KS event.  It holds the parameters 
//  needed to invoke the KS event successfully and it is referenced counted
//  to make sure the filter does not go away before your sensor is done with
//  the notifier.
//  
//  These objects should be instantiated in the filter and a reference
//  held in your sensor object.
//  
class CNotifier : public CRef
{
private:
    PVOID       m_Object;
    GUID        m_EventSet;
    ULONG       m_EventId;
    ULONG       m_DataSize;
    PVOID       m_Data;
    PFNKSGENERATEEVENTCALLBACK m_Callback;
    PVOID       m_CallbackContext;

public:
    CNotifier(
        _In_        PKSFILTER   Filter,
        _In_        const GUID *EventSet,
        _In_        ULONG       EventId,
        _In_        ULONG       DataSize=0,
        _In_opt_    PVOID       Data=nullptr,
        _In_opt_    PFNKSGENERATEEVENTCALLBACK Callback=nullptr,
        _In_opt_    PVOID       CallbackContext=nullptr
    )
        : m_Object(Filter)
        , m_EventSet(*EventSet)
        , m_EventId(EventId)
        , m_DataSize(DataSize)
        , m_Data(Data)
        , m_Callback(Callback)
        , m_CallbackContext(CallbackContext)
    {}

    CNotifier(
        _In_        PKSPIN      Pin,
        _In_        const GUID *EventSet,
        _In_        ULONG       EventId,
        _In_        ULONG       DataSize=0,
        _In_opt_    PVOID       Data=nullptr,
        _In_opt_    PFNKSGENERATEEVENTCALLBACK Callback=nullptr,
        _In_opt_    PVOID       CallbackContext=nullptr
    )
        : m_Object(Pin)
        , m_EventSet(*EventSet)
        , m_EventId(EventId)
        , m_DataSize(DataSize)
        , m_Data(Data)
        , m_Callback(Callback)
        , m_CallbackContext(CallbackContext)
    {}

    void
    Set()
    {
        KsGenerateEvents(m_Object, &m_EventSet, m_EventId, m_DataSize, m_Data, m_Callback, m_CallbackContext);
    }
};

class CSensor
{
protected:

    CCaptureDevice *m_Device;

    ULONG           m_PinCount;
    LONG            m_FilterInstanceCount;

    //
    // The AVStream filter object associated with this CCaptureFilter.
    //
    CHardwareSimulation **m_HardwareSimulation;
    CSynthesizer **m_Synthesizer;
    ICapturePin **m_CapturePin;

    PKS_VIDEOINFOHEADER *m_VideoInfoHeader;

    //
    // The number of ISR's that have occurred since capture started.
    //
    ULONG *m_InterruptTime;

    //
    // The last reading of mappings completed.
    //
    ULONG *m_LastMappingsCompleted;

    static const ULONG  INVALID_PIN_INDEX = 0xffffffff;

    ULONG   m_PreviewIndex;
    ULONG   m_StillIndex;
    ULONG   m_VideoIndex;

    //  Access control mutex.
    KMutex  m_SensorMutex;

    ULONG               m_PfsLoopLimit;
    ULONG               m_PfsFrameLimit;
    ISP_FRAME_SETTINGS *m_pIspSettings;

    ISP_FRAME_SETTINGS  m_GlobalIspSettings;

public:

    CSensor(
        _In_    CCaptureDevice *Device,
        _In_    ULONG           PinCount
    );

    //
    // ~CSensor():
    //
    // The h/w Sensor destructor.
    //
    virtual
    ~CSensor( );

    virtual
    NTSTATUS
    Initialize();

    virtual
    NTSTATUS
    ProgramDefaults();

    virtual
    void
    Interrupt(
        _In_    LONG PinIndex
    );

    virtual
    PDEVICE_OBJECT
    GetDeviceObject()
    {
        return m_Device->GetDeviceObject();
    }

    //
    // AcquireHardwareResources():
    //
    // Called to acquire hardware resources for the device based on a given
    // video info header.  This will fail if another object has already
    // acquired hardware resources since we emulate a single capture
    // device.
    //
    virtual
    NTSTATUS
    AcquireHardwareResources(
        _In_    PKSPIN Pin,
        _In_    ICapturePin *CaptureSink,
        _In_    PKS_VIDEOINFOHEADER VideoInfoHeader,
        _Out_   CHardwareSimulation **pSim
    );

    //
    // ReleaseHardwareResources():
    //
    // Called to release hardware resources for the device.
    //
    virtual
    void
    ReleaseHardwareResources(
        _In_    PKSPIN Pin
        //_In_    CHardwareSimulation *pSim
    );

    //
    // Start():
    //
    // Called to start the hardware simulation.  This causes us to simulate
    // interrupts, simulate filling buffers with synthesized data, etc...
    //
    NTSTATUS
    Start (
        _In_    PKSPIN Pin
    );

    //
    // Pause():
    //
    // Called to pause or unpause the hardware simulation.  This will be
    // indentical to a start or stop but it will not reset formats and
    // counters.
    //
    NTSTATUS
    Pause(
        _In_    PKSPIN Pin,
        _In_    BOOLEAN Pausing
    );

    NTSTATUS
    Trigger(
        _In_    ULONG   PinId,
        _In_    LONG    mode
    );

    NTSTATUS
    SetQPC(
        _In_ ULONGLONG QPC
    );

    NTSTATUS
    GetQPC(
        _Out_   PULONGLONG QPC
    );

    NTSTATUS
    SetPinMode(
        _In_    ULONGLONG Flags,
        _In_    ULONG PastBuffers
    );

    NTSTATUS
    SetPhotoFrameRate(
        _In_    ULONGLONG   TimePerFrame
    );

    ULONGLONG
    GetPhotoFrameRate();

    NTSTATUS
    SetFlashStatus(
        _In_    ULONGLONG ulFlashStatus
    );

    bool
    IsPreviewIndex(
        _In_    ULONG   Index
    )
    {
        return m_PreviewIndex == Index ;
    }

    bool
    IsStillIndex(
        _In_    ULONG   Index
    )
    {
        return m_StillIndex == Index ;
    }

    bool
    IsVideoIndex(
        _In_    ULONG   Index
    )
    {
        return m_VideoIndex == Index ;
    }

    bool
    IsValidIndex(
        _In_    ULONG   Index
    )
    {
        return Index < GetPinCount() ;
    }

    inline ULONG GetPreviewIndex()
    {
        return m_PreviewIndex;
    }

    inline ULONG GetStillIndex()
    {
        return m_StillIndex;
    }

    inline ULONG GetVideoIndex()
    {
        return m_VideoIndex;
    }

    //Not thread safe, for User mode notification only.
    LONG
    GetFilterCount()
    {
        return m_FilterInstanceCount;
    }

    NTSTATUS
    AddFilter(PKSFILTER pFilter);

    NTSTATUS
    RemoveFilter(PKSFILTER pFilter);

    ULONG
    GetPinCount()
    {
        return m_PinCount;
    }

    //
    //  GetFourCC
    //
    //  Return the FourCC code for the current pin.
    //
    ULONG
    GetFourCC(
        _In_    ULONG   Index
    )
    {
        return Index<m_PinCount ? m_VideoInfoHeader[Index]->bmiHeader.biCompression : 0;
    }

    //
    // Stop():
    //
    // Called to stop the hardware simulation.  This causes interrupts to
    // stop issuing.  When this call returns, the "fake" hardware has
    // stopped accessing all s/g buffers, etc...
    //
    NTSTATUS
    Stop (
        _In_    PKSPIN
    );

    NTSTATUS
    Reset(
        _In_    PKSPIN
    );

    NTSTATUS
    Reset(
    );


    //
    // ProgramScatterGatherMappings():
    //
    // Called to program the hardware simulation's scatter / gather table.
    // This synchronizes with the "fake" ISR and hardware simulation via
    // a spinlock.
    //
    ULONG
    ProgramScatterGatherMappings (
        _In_    PKSPIN Pin,
        _In_    PKSSTREAM_POINTER *Clone,
        _In_    PUCHAR *Buffer,
        _In_    PKSMAPPING Mappings,
        _In_    ULONG MappingsCount
    );

    //  Lets you know if Variable Photo Sequence is active.
    //  Note: This doesn't tell you if the pin is running.
    virtual
    BOOLEAN
    IsVPSActive()
    {
        //  VPS isn't supported by default.
        return FALSE;
    }

    //  Fetch a pointer to the global ISP_FRAME_SETTINGS
    virtual
    ISP_FRAME_SETTINGS *
    GetGlobalIspSettings();

    //  Program the Per Frame Settings for simulation
    //  We do it before calling start so we can be ready
    //  to program our simulation's hardware.
    //  Note: We make a local copy.
    virtual
    NTSTATUS
    SetPFS(
        _In_    ISP_FRAME_SETTINGS  *pIspSettings,
        _In_    ULONG               FrameLimit,
        _In_    ULONG               LoopLimit
    );

    virtual
    void
    UpdateZoom(void);

protected:
    LONG
    IncrementFilterCount()
    {
        return InterlockedIncrement(&m_FilterInstanceCount);
    }

    LONG
    DecrementFilterCount()
    {
        return InterlockedDecrement(&m_FilterInstanceCount);
    }

public:

    DECLARE_PROPERTY( KSPROPERTY_VIDEOCONTROL_MODE_S, VideoControlMode );

    DECLARE_PROPERTY_GET( KSCAMERA_EXTENDEDPROP_FOCUSSTATE, FocusState );
    DECLARE_PROPERTY_ASYNC( KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_S, FocusRect );
    DECLARE_PROPERTY( KSPROPERTY_CAMERACONTROL_FLASH_S, Flash );
    DECLARE_PROPERTY_GET( KSPROPERTY_CAMERACONTROL_IMAGE_PIN_CAPABILITY_S, PinDependence );

    DECLARE_PROPERTY_ASYNC( CExtendedPhotoMode, PhotoMode );
    DECLARE_PROPERTY_GET( CExtendedProperty, PhotoFrameRate );
    DECLARE_PROPERTY_ASYNC( CExtendedProperty, PhotoMaxFrameRate );
    DECLARE_PROPERTY_ASYNC( CExtendedProperty, WarmStart );
    DECLARE_PROPERTY( CExtendedMaxVideoFpsForPhotoRes, MaxVideoFpsForPhotoRes );
    DECLARE_PROPERTY_GET( CExtendedFieldOfView, FieldOfView );
    DECLARE_PROPERTY_GET( CExtendedCameraAngleOffset, CameraAngleOffset );
    DECLARE_PROPERTY( CExtendedProperty, ExtendedFlash );

    DECLARE_PROPERTY( CExtendedProperty, TriggerTime );
    DECLARE_PROPERTY( CExtendedProperty, TorchMode );

    DECLARE_PROPERTY_ASYNC( CExtendedVidProcSetting, Focus );
    DECLARE_PROPERTY_ASYNC( CExtendedProperty,       Iso );
    DECLARE_PROPERTY_ASYNC( CExtendedVidProcSetting, IsoAdvanced );
    DECLARE_PROPERTY_ASYNC( CExtendedEvCompensation, EvCompensation );
    DECLARE_PROPERTY_ASYNC( CExtendedVidProcSetting, WhiteBalance );
    DECLARE_PROPERTY_ASYNC( CExtendedVidProcSetting, Exposure );
    DECLARE_PROPERTY(       CExtendedProperty,       PhotoConfirmation );
    DECLARE_PROPERTY_ASYNC( CExtendedProperty,       SceneMode );
    DECLARE_PROPERTY(       CExtendedProperty,       FocusPriority );
    DECLARE_PROPERTY_ASYNC( CExtendedProperty, Thumbnail );
    DECLARE_PROPERTY( CExtendedProperty, VideoHDR );
    DECLARE_PROPERTY( CExtendedProperty, VFR );
    DECLARE_PROPERTY( CExtendedVidProcSetting, Zoom );
    DECLARE_PROPERTY( CExtendedProperty, VideoStabilization );
    DECLARE_PROPERTY( CExtendedProperty, Histogram );
    DECLARE_PROPERTY( CExtendedMetadata, Metadata );
    DECLARE_PROPERTY( CExtendedProperty, OpticalImageStabilization );
    DECLARE_PROPERTY( CExtendedProperty, OptimizationHint );
    DECLARE_PROPERTY( CExtendedProperty, AdvancedPhoto );
    DECLARE_PROPERTY( CExtendedVidProcSetting, FaceDetection );

    DECLARE_PROPERTY_VARSIZE_ASYNC( CRoiProperty, Roi );

    DECLARE_PROPERTY( KSPROPERTY_CAMERACONTROL_VIDEOSTABILIZATION_MODE_S, VideoStabMode );

    DECLARE_PROPERTY(KSPROPERTY_CAMERACONTROL_S, Exposure);
    DECLARE_PROPERTY(KSPROPERTY_CAMERACONTROL_S, Focus);
    DECLARE_PROPERTY(KSPROPERTY_CAMERACONTROL_S, Zoom);
    DECLARE_PROPERTY(KSPROPERTY_CAMERACONTROL_S, ZoomRelative);
    DECLARE_PROPERTY(KSPROPERTY_CAMERACONTROL_S, Pan);
    DECLARE_PROPERTY(KSPROPERTY_CAMERACONTROL_S, Roll);
    DECLARE_PROPERTY(KSPROPERTY_CAMERACONTROL_S, Tilt);
    DECLARE_PROPERTY(KSPROPERTY_CAMERACONTROL_FOCAL_LENGTH_S, FocalLength);

    DECLARE_PROPERTY(KSPROPERTY_VIDEOPROCAMP_S, BacklightCompensation);
    DECLARE_PROPERTY(KSPROPERTY_VIDEOPROCAMP_S, Brightness);
    DECLARE_PROPERTY(KSPROPERTY_VIDEOPROCAMP_S, Contrast);
    DECLARE_PROPERTY(KSPROPERTY_VIDEOPROCAMP_S, Hue);
    DECLARE_PROPERTY(KSPROPERTY_VIDEOPROCAMP_S, WhiteBalance);
    DECLARE_PROPERTY(KSPROPERTY_VIDEOPROCAMP_S, PowerlineFreq);

    DECLARE_PROPERTY_GET( CRoiConfig, RoiConfigCaps );

    _Success_(return == 0)
    virtual
    NTSTATUS
    GetPfsCaps(
        _Inout_opt_ KSCAMERA_PERFRAMESETTING_CAP_HEADER *Caps,
        _Inout_     ULONG                               *Size
    );
};

