/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        SensorSimulation.h

    Abstract:

        Simulation class declaration.  Derived from the base CSensor object.

        This class simulates a theoretical model camera.  The camera supports
        every modern camera control available and produces a simulation of its
        behavior.  In most cases this simulation is very very basic because we
        have no actual hardware to work with and there producing a visible
        effect in the simulation would be difficult or expensive to do.

    History:

        created 5/8/2014

**************************************************************************/

#pragma once
class CSensorSimulation :
    public CSensor
{
protected:
    const
    KSFILTER_DESCRIPTOR *m_Descriptors;

    LARGE_INTEGER m_StartTime;

    //  State for the various controls.
    RECT m_FocusRect;
    BOOL m_AutoFocusLock;
    BOOL m_AutoExposureLock;
    BOOL m_AutoWhitebalanceLock;
    ULONG m_Flash;
    ULONG m_VideoStabMode;
    CExtendedProperty   *m_WarmStartEnabled;
    CExtendedPhotoMode  m_PhotoMode;
    ULONG m_RequestedHistoryFrames;
    CExtendedProperty   m_MaxFrameRate;
    ULONGLONG m_FaceDetectionFlags;
    ULONG m_FaceDetectionMax;
    ULONG m_FaceDetectionCurrentMax;
    ULONGLONG m_SceneMode;
    CExtendedProperty   m_TorchMode;
    CExtendedProperty   m_OptimizationHint;
    CExtendedMetadata   *m_MetadataInfo;
    PKSCAMERA_PERFRAMESETTING_HEADER m_pPerFrameSettings;
    ULONG m_PFSSize;
    KSCAMERA_EXTENDEDPROP_FOCUSSTATE    m_FocusState;
    ULONGLONG m_VFR;
    ULONGLONG m_VideoHDR;
    ULONGLONG m_VideoStabilization;
    ULONGLONG m_Histogram;
    ULONGLONG   m_OpticalImageStabilization;
    ULONGLONG   m_AdvancedPhoto;

    CWhiteBalanceRoiIspControl  m_RoiWhiteBalance;
    CExposureRoiIspControl      m_RoiExposureMode;
    CFocusRoiIspControl         m_RoiFocusMode;

    VIDCAP_PROPERTY m_Pan;
    VIDCAP_PROPERTY m_Roll;
    VIDCAP_PROPERTY m_Tilt;

    //  Zoom parameters
    FOCAL_LENGTH_PROPERTY   m_FocalLength;  // A description of the zoom limits.
    VIDCAP_PROPERTY m_Zoom;                 // Absolute zoom position / mode.
    VIDCAP_PROPERTY m_ZoomRelative;         // Relative zoom position / mode.

    //  Cached Q16-adjusted bounds.  I do not use the zoom value stored below.
    //
    //  I am using the same m_Zoom value here to keep both the extended and legacy
    //  controls tied together.  It is not necessary to implement both in a real driver.
    //  However the legacy control was implemented first in this simulation and leaving both
    //  in place might have some value.  But supporting both adds the question of whether they
    //  should both present a consistent view of zoom or if they should just save and report
    //  the values set for their own respective properties.
    //
    //  I have chosen to support a consistent view of the zoom factor between the extended and
    //  legacy controls.  Given that the legacy control was implemented first, I have chosen
    //  to keep that storage and rescale the value set by the extended zoom to match the
    //  legacy settings.
    //
    //  But to make the two controls more consistent, I have changed the reported Ocular Focal
    //  Length to 2^16, which matches the implied denominator in Q16 format - the format used
    //  by this extended property.  So as long as lOcularFocalLength is 0x10000, the following
    //  function should not change the reported value at all.  This should simplify any
    //  debugging or tracing.

    CExtendedVidProcSetting m_ExtendedZoom;     //  Must update zoom value on the fly.

    VIDCAP_PROPERTY m_PowerLineFreq;            // Power Line Frequency setting.
    VIDCAP_PROPERTY m_BacklightCompensation;
    VIDCAP_PROPERTY m_Brightness;
    VIDCAP_PROPERTY m_Contrast;
    VIDCAP_PROPERTY m_Hue;

    ULONG       m_IsoResult;
    ULONG       m_EvCompResult;
    ULONG       m_WhiteBalanceResult;
    ULONG       m_ExposureResult;
    ULONG       m_SceneModeResult;
    ULONG       m_FaceDetectionResult;
    ULONG       m_FocusResult;

    ULONGLONG   m_LastFocusMode;
    ULONG       m_LastFocusSetting;

    CRefPtr<CNotifier>  m_FocusNotifier;        // A reference to the CNotifier for any Focus in flight.
    KPassiveTimer      *m_FocusTimer;           // A timer object used to simulate a focus event.
    CRefPtr<CNotifier>  m_FocusRectNotifier;    
    KPassiveTimer      *m_FocusRectTimer;

    static const ULONG METADATA_MAX=4096;

protected:
    //
    //  Deal with scene mode changes.
    //
    NTSTATUS
    UpdateSettings(
        _In_    ULONGLONG ullSceneMode
    );

    //
    //  Put us back to the defaults specified in the DDI.
    //
    virtual
    NTSTATUS
    ProgramDefaults();

public:
    //
    //  This simulation is based solely on the Descriptors.  Yours might
    //  need additional information.
    //
    CSensorSimulation(
        _In_    CCaptureDevice             *Device,
        _In_    const KSFILTER_DESCRIPTOR  *Descriptors
    );

    virtual ~CSensorSimulation();

    //
    //  Parse descriptors to determine what pin simulations are required.
    //  Do memory allocations, etc.
    //
    virtual
    NTSTATUS
    Initialize();

    virtual
    NTSTATUS
    GetFocusState(
        _Out_   KSCAMERA_EXTENDEDPROP_FOCUSSTATE   *FocusState
    );

    //  Lets you know if Variable Photo Sequence is active.
    //  Note: This doesn't tell you if the pin is running.
    virtual
    BOOLEAN
    IsVPSActive()
    {
        return
            KSCAMERA_EXTENDEDPROP_PHOTOMODE_SEQUENCE_SUB_VARIABLE==m_PhotoMode.SubMode() &&
            m_pIspSettings  ;
    }

    //  Callback to handle focus convergence.
    static
    void
    FocusConvergence(
        _In_opt_    PVOID   Context
    );

    //  Callback to handle focus convergence.
    static
    void
    FocusRectConvergence(
        _In_opt_    PVOID   Context
    );

    //  Declare the property controls handled by this sensor.
    DECLARE_PROPERTY( KSPROPERTY_VIDEOCONTROL_MODE_S, VideoControlMode );

    DECLARE_PROPERTY_ASYNC( KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_S, FocusRect );

    DECLARE_PROPERTY( KSPROPERTY_CAMERACONTROL_FLASH_S, Flash );
    DECLARE_PROPERTY_GET( KSPROPERTY_CAMERACONTROL_IMAGE_PIN_CAPABILITY_S, PinDependence );
    DECLARE_PROPERTY( CExtendedProperty, TriggerTime );
    DECLARE_PROPERTY( CExtendedProperty, TorchMode );

    DECLARE_PROPERTY( CExtendedProperty, ExtendedFlash );

    DECLARE_PROPERTY_ASYNC( CExtendedVidProcSetting,    Focus );
    DECLARE_PROPERTY_ASYNC_NOCANCEL( CExtendedProperty, Iso );
    DECLARE_PROPERTY_ASYNC_NOCANCEL( CExtendedVidProcSetting, IsoAdvanced );
    DECLARE_PROPERTY_ASYNC_NOCANCEL( CExtendedEvCompensation, EvCompensation );
    DECLARE_PROPERTY_ASYNC_NOCANCEL( CExtendedVidProcSetting, WhiteBalance );
    DECLARE_PROPERTY_ASYNC_NOCANCEL( CExtendedVidProcSetting, Exposure );
    DECLARE_PROPERTY( CExtendedProperty, PhotoConfirmation );
    DECLARE_PROPERTY_ASYNC_NOCANCEL( CExtendedProperty, SceneMode );
    DECLARE_PROPERTY( CExtendedProperty, FocusPriority );
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
    DECLARE_PROPERTY_ASYNC_NOCANCEL( CExtendedPhotoMode, PhotoMode );
    DECLARE_PROPERTY_ASYNC_NOCANCEL( CExtendedProperty, PhotoMaxFrameRate );
    DECLARE_PROPERTY_ASYNC_NOCANCEL( CExtendedProperty, WarmStart );
    DECLARE_PROPERTY( CExtendedMaxVideoFpsForPhotoRes, MaxVideoFpsForPhotoRes );
    DECLARE_PROPERTY_GET( CExtendedFieldOfView, FieldOfView );
    DECLARE_PROPERTY_GET( CExtendedCameraAngleOffset, CameraAngleOffset );

    DECLARE_PROPERTY_VARSIZE_ASYNC_NOCANCEL( CRoiProperty, Roi );

    DECLARE_PROPERTY( KSPROPERTY_CAMERACONTROL_VIDEOSTABILIZATION_MODE_S, VideoStabMode );

    //  Update the zoom factor over time.
    void
    SmoothZoom();

    //  Apply a zoom factor.
    virtual
    void
    UpdateZoom(void);

    //  Declare legacy controls that are needed.
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

    //  Special-case Per-Frame settings since it requires a variable length structure.
    _Success_(return == 0)
    virtual
    NTSTATUS
    GetPfsCaps(
        _Inout_opt_ KSCAMERA_PERFRAMESETTING_CAP_HEADER *Caps,
        _Inout_     ULONG                               *Size
    );
};

//  Constants...
const LONG  WHITEBALANCE_MIN = 0;
const LONG  WHITEBALANCE_MAX = 15000;
const LONG  WHITEBALANCE_STEP= 1;
const LONG  WHITEBALANCE_DEF = 5000;

const LONG  EXPOSURE_BILOG_MIN  = -10;
const LONG  EXPOSURE_BILOG_MAX  =   9;
const LONG  EXPOSURE_BILOG_STEP =   1;
const LONG  EXPOSURE_BILOG_DEF  =  -5;