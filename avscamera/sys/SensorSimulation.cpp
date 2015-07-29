/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        SensorSimulation.cpp

    Abstract:

        Simulation class implementation.  Derived from the base CSensor object.

        This class simulates a theoretical model camera.  The camera supports
        every modern camera control available and produces a simulation of its
        behavior.  In most cases this simulation is very very basic because we
        have no actual hardware to work with and there producing a visible
        effect in the simulation would be difficult or expensive to do.

    History:

        created 5/8/2014

**************************************************************************/

#include "Common.h"

/**************************************************************************

    PAGEABLE CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

//
//  The following macro is used to define a legacy control's range and 
//  default value.  We use the macro simply to minimize the verbosity of
//  our declarations.
//
#define DEFINE_LEGACY_CTRL_CAPS( Control, Minimum, Maximum, SteppingDelta, DefValue )   \
/*--------------------------------------------------------------                        \
    This structure defines what the control is capable of.                              \
--------------------------------------------------------------*/                        \
KSPROPERTY_STEPPING_LONG Control##RangeAndStep[] =                                      \
{                                                                                       \
    {                                                                                   \
        SteppingDelta,  /* SteppingDelta */                                             \
        0,              /* Reserved      */                                             \
        Minimum,        /* Minimum       */                                             \
        Maximum         /* Maximum       */                                             \
    }                                                                                   \
};                                                                                      \
                                                                                        \
/*  This is the default control value.  */                                              \
const LONG Control##Default = DefValue;                                                 \
                                                                                        \
KSPROPERTY_MEMBERSLIST Control##MembersList[] =                                         \
{                                                                                       \
    {                                                                                   \
        {                                                                               \
            KSPROPERTY_MEMBER_STEPPEDRANGES,                                            \
            sizeof (Control##RangeAndStep),                                             \
            SIZEOF_ARRAY(Control##RangeAndStep),                                        \
            0                                                                           \
        },                                                                              \
        (PVOID)Control##RangeAndStep,                                                   \
    },                                                                                  \
    {                                                                                   \
        {                                                                               \
            KSPROPERTY_MEMBER_VALUES,                                                   \
            sizeof (Control##Default),                                                  \
            1,                                                                          \
            KSPROPERTY_MEMBER_FLAG_DEFAULT                                              \
        },                                                                              \
        (PVOID)&Control##Default                                                        \
    }                                                                                   \
};                                                                                      \
                                                                                        \
KSPROPERTY_VALUES Control##Values =                                                     \
{                                                                                       \
    {                                                                                   \
        STATICGUIDOF(KSPROPTYPESETID_General),                                          \
        VT_I4,                                                                          \
        0                                                                               \
    },                                                                                  \
    SIZEOF_ARRAY(Control##MembersList),                                                 \
    Control##MembersList                                                                \
};

DEFINE_LEGACY_CTRL_CAPS( ZoomRelative, -2, +2, 1, 0 )
DEFINE_LEGACY_CTRL_CAPS( Zoom, FOCALLENGTH_OPTICAL_MIN, FOCALLENGTH_OPTICAL_MAX, 1, FOCALLENGTH_OCULAR )
DEFINE_LEGACY_CTRL_CAPS( Pan,  -180, 180, 1, 0 )
DEFINE_LEGACY_CTRL_CAPS( Roll, -180, 180, 1, 0 )
DEFINE_LEGACY_CTRL_CAPS( Tilt, -180, 180, 1, 0 )
DEFINE_LEGACY_CTRL_CAPS( BacklightCompensation, 0, 1, 1, 0 )
DEFINE_LEGACY_CTRL_CAPS( Brightness, -10000, 10000, 1, 750 )
DEFINE_LEGACY_CTRL_CAPS( Contrast,   -10000, 10000, 1, 100 )
DEFINE_LEGACY_CTRL_CAPS( Hue,        -18000, 18000, 1, 0   )
DEFINE_LEGACY_CTRL_CAPS( PLF, POWERLINEFREQ_DISABLED, POWERLINEFREQ_60HZ, 1, POWERLINEFREQ_DEFAULT )
DEFINE_LEGACY_CTRL_CAPS( WhiteBalance, WHITEBALANCE_MIN, WHITEBALANCE_MAX, WHITEBALANCE_STEP, WHITEBALANCE_DEF )
DEFINE_LEGACY_CTRL_CAPS( Exposure, EXPOSURE_BILOG_MIN, EXPOSURE_BILOG_MAX, EXPOSURE_BILOG_STEP, EXPOSURE_BILOG_DEF)
DEFINE_LEGACY_CTRL_CAPS( Focus, 1, 1200, 1, 100 );

void
CSensorSimulation::
FocusConvergence(
    _In_opt_    PVOID   Context
)
/*++

Routine Description:

    A timer callback that simulates focus convergence.

    Update the focus state to focused.  
    
    Update the position to a random location that is in bounds if we are in 
    an auto or continuous auto mode.  The new location is probably not 
    realistic; but it is important to simulate the change in location for
    testing.

    Notice that we do no kick off the timer again on continuous auto focus.
    A camera that is performing continuous auto focus in software would 
    probably do something there.

Arguments:

    Context -
        In our case, this is a pointer to our simulation object.

Return Value:

    None

--*/
{
    PAGED_CODE();

    if( Context )
    {
        CSensorSimulation *Sensor = (CSensorSimulation *) Context;
        KScopedMutex lock( Sensor->m_SensorMutex );

        if( Sensor->m_FocusNotifier )
        {
            Sensor->m_FocusNotifier->Set();
            Sensor->m_FocusNotifier = nullptr;
        }

        //  Let's just fake a focus change if we get here from somewhere else.
        //  This only applies to auto focus modes and only if we're not focused.
        if( (Sensor->m_GlobalIspSettings.FocusMode &
                (KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO | KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS)) &&
                Sensor->m_FocusState != KSCAMERA_EXTENDEDPROP_FOCUSSTATE_FOCUSED )
        {
            Sensor->m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul =
                GetRandom( Sensor->m_GlobalIspSettings.FocusSetting.Min,
                           Sensor->m_GlobalIspSettings.FocusSetting.Max );
        }
        else if( Sensor->m_GlobalIspSettings.FocusMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL )
        {
            switch( Sensor->m_GlobalIspSettings.FocusMode & KSCAMERA_EXTENDEDPROP_FOCUS_DISTANCE_MASK )
            {
            case KSCAMERA_EXTENDEDPROP_FOCUS_DISTANCE_INFINITY:
                Sensor->m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul =
                    Sensor->m_GlobalIspSettings.FocusSetting.Max;
                break;

            case KSCAMERA_EXTENDEDPROP_FOCUS_DISTANCE_HYPERFOCAL:
                Sensor->m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul =
                    Sensor->m_GlobalIspSettings.FocusSetting.Max - Sensor->m_GlobalIspSettings.FocusSetting.Step;
                break;

            case KSCAMERA_EXTENDEDPROP_FOCUS_DISTANCE_NEAREST:
                Sensor->m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul =
                    Sensor->m_GlobalIspSettings.FocusSetting.Min;
                break;

            }
        }

        //  TODO:
        //  If we're continuous focus, we should probably kick off another focus change here...
        //  ... but that tends to mess with testing.

        //  We always want to transition to here.
        Sensor->m_FocusState = KSCAMERA_EXTENDEDPROP_FOCUSSTATE_FOCUSED;
    }
}

void
CSensorSimulation::
FocusRectConvergence(
    _In_opt_    PVOID   Context
)
/*++

Routine Description:

    A timer callback that simulates FocusRect convergence.

Arguments:

    Context -
        In our case, this is a pointer to our simulation object.

Return Value:

    None

--*/
{
    PAGED_CODE();

    if( Context )
    {
        CSensorSimulation *Sensor = (CSensorSimulation *) Context;
        KScopedMutex lock( Sensor->m_SensorMutex );

        if( Sensor->m_FocusRectNotifier )
        {
            Sensor->m_FocusRectNotifier->Set();
            Sensor->m_FocusRectNotifier = nullptr;
        }

        //  Not much else to do here.
    }
}

CSensorSimulation::CSensorSimulation(
    _In_    CCaptureDevice             *Device,
    _In_    const KSFILTER_DESCRIPTOR  *Descriptors
)
    : CSensor( Device, Descriptors->PinDescriptorsCount )
    , m_Descriptors( Descriptors )
    , m_MetadataInfo( nullptr )
    , m_FocusTimer( nullptr )
    , m_FocusRectTimer( nullptr )

{
    PAGED_CODE();

    KeQuerySystemTime (&m_StartTime);
}


CSensorSimulation::~CSensorSimulation()
{
    PAGED_CODE();

    delete m_FocusTimer;
    delete m_FocusRectTimer;
    delete [] m_MetadataInfo;
    delete [] (PBYTE) m_pPerFrameSettings;
    delete [] m_WarmStartEnabled;
}

NTSTATUS
CSensorSimulation::
ProgramDefaults()
/*++

Routine Description:

    Reset our simulation back to the DDI-specified defaults.

Arguments:

    None

Return Value:

    Success / Failure

--*/
{
    PAGED_CODE();

    DBG_ENTER("()");

    m_FocusNotifier = nullptr;
    m_pPerFrameSettings = nullptr;


    m_FaceDetectionMax = MAX_FACES;
    m_FaceDetectionFlags = KSCAMERA_EXTENDEDPROP_FACEDETECTION_OFF;
    m_FaceDetectionCurrentMax = 0;
    m_AutoFocusLock = false;
    m_AutoExposureLock = false;
    m_AutoWhitebalanceLock = false;
    m_Flash = 0;
    m_VideoStabMode = 0;
    m_VFR = KSCAMERA_EXTENDEDPROP_VFR_ON;
    m_VideoHDR = KSCAMERA_EXTENDEDPROP_VIDEOHDR_OFF;
    m_VideoStabilization = KSCAMERA_EXTENDEDPROP_VIDEOSTABILIZATION_OFF;
    m_Histogram = KSCAMERA_EXTENDEDPROP_HISTOGRAM_OFF;
    m_OpticalImageStabilization = KSCAMERA_EXTENDEDPROP_OIS_AUTO;
    m_AdvancedPhoto = KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_OFF;

    m_FocusRect.left = 0;
    m_FocusRect.top = 0;
    m_FocusRect.right = 0;
    m_FocusRect.bottom = 0;

    m_FocusState = KSCAMERA_EXTENDEDPROP_FOCUSSTATE_UNINITIALIZED;

    m_PhotoMode = CExtendedPhotoMode();
    m_PhotoMode.Capability = KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL | KSCAMERA_EXTENDEDPROP_PHOTOMODE_SEQUENCE;
    m_PhotoMode.PinId = GetStillIndex();
    m_PhotoMode.MaxHistoryFrames() = IMAGE_CAPTURE_PIN_MAXIMUM_HISTORY_FRAMES;
    m_PhotoMode.RequestedHistoryFrames() = 0;
    m_PhotoMode.SubMode() = KSCAMERA_EXTENDEDPROP_PHOTOMODE_SEQUENCE_SUB_NONE;

    m_MaxFrameRate = CExtendedProperty();   // assume nothing.
    SetQPC( 0 );
    m_TorchMode.Flags = KSCAMERA_EXTENDEDPROP_VIDEOTORCH_OFF;
    m_TorchMode = 50UL;

    m_OptimizationHint.Flags = KSCAMERA_EXTENDEDPROP_OPTIMIZATION_PHOTO;

    m_SceneMode = KSCAMERA_EXTENDEDPROP_SCENEMODE_AUTO;

    //  Wipe the global settings to zeros for now.
    RtlFillBytes(&m_GlobalIspSettings, sizeof(m_GlobalIspSettings), 0);

    m_GlobalIspSettings.FlashMode = KSCAMERA_EXTENDEDPROP_FLASH_OFF;
    m_GlobalIspSettings.FlashValue = 50;

    m_GlobalIspSettings.ISOMode = KSCAMERA_EXTENDEDPROP_ISO_AUTO;
    m_GlobalIspSettings.ISOValue = 200;

    m_GlobalIspSettings.EVCompensation.Mode = 0;
    m_GlobalIspSettings.EVCompensation.Min = -2;
    m_GlobalIspSettings.EVCompensation.Max = 2;
    m_GlobalIspSettings.EVCompensation.Value = 0;
    m_GlobalIspSettings.EVCompensation.Mode = KSCAMERA_EXTENDEDPROP_EVCOMP_FULLSTEP;

    m_GlobalIspSettings.WhiteBalanceMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
    m_GlobalIspSettings.WhiteBalanceSetting.Max  = WHITEBALANCE_MAX;
    m_GlobalIspSettings.WhiteBalanceSetting.Min  = WHITEBALANCE_MIN;
    m_GlobalIspSettings.WhiteBalanceSetting.Step = WHITEBALANCE_STEP;
    m_GlobalIspSettings.WhiteBalanceSetting.Reserved = 0;
    m_GlobalIspSettings.WhiteBalanceSetting.VideoProc.Value.ul = WhiteBalanceDefault;
    m_GlobalIspSettings.WhiteBalanceSetting.Mode = KSCAMERA_EXTENDEDPROP_WHITEBALANCE_TEMPERATURE;

    m_GlobalIspSettings.ExposureMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
    m_GlobalIspSettings.ExposureSetting.Max = (MAX_EXPOSURE_TIME>LONG_MAX) ? LONG_MAX : (LONG)(LONG_MAX & MAX_EXPOSURE_TIME);
    m_GlobalIspSettings.ExposureSetting.Min = (LONG)MIN_EXPOSURE_TIME;
    m_GlobalIspSettings.ExposureSetting.Step = 1;
    m_GlobalIspSettings.ExposureSetting.Reserved = 0;
    m_GlobalIspSettings.ExposureSetting.VideoProc.Value.ll = DEF_EXPOSURE_TIME;
    m_GlobalIspSettings.ExposureSetting.Mode = 0;

    m_GlobalIspSettings.FocusMode = KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS | KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_FULLRANGE;
    m_GlobalIspSettings.FocusSetting.Max = 1200;
    m_GlobalIspSettings.FocusSetting.Min = 1;
    m_GlobalIspSettings.FocusSetting.Step = 1;
    m_GlobalIspSettings.FocusSetting.Reserved = 0;
    m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul = 100;
    m_GlobalIspSettings.FocusSetting.Mode = 0;
    m_GlobalIspSettings.FocusPriority = KSCAMERA_EXTENDEDPROP_FOCUSPRIORITY_ON;

    m_GlobalIspSettings.bPhotoConfirmation = TRUE;

    //  Defaults to fall back to if we get a cancel.
    m_LastFocusMode = KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS | KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_FULLRANGE;
    m_LastFocusSetting = 100;


    //  Specifies the dynamic range of our fictious camera's lens system
    //  Note: Magnification = Lobjective / Locular

    //  The following values set only the minimum and maximum for the rational number expressing the zoom.

    //  This specifies the denominator of the rational value used to specify magnification.
    m_FocalLength.lOcularFocalLength = FOCALLENGTH_OCULAR;
    //  If lObjectiveFocalLengthMin < lOcularFocalLength, then camera allows Zoom out: aka, wide-field view.
    m_FocalLength.lObjectiveFocalLengthMin = FOCALLENGTH_OPTICAL_MIN;
    //  If lObjectiveFocalLengthMax > lOcularFocalLength, then camera allows Zoom in: aka, Magnified, narrow-field view.
    m_FocalLength.lObjectiveFocalLengthMax = FOCALLENGTH_OPTICAL_MAX;

    //  Cache values for the new extended zoom property.
    m_ExtendedZoom.Flags = KSCAMERA_EXTENDEDPROP_ZOOM_DIRECT;
    m_ExtendedZoom.Min() = (LONG)(TO_Q16(FOCALLENGTH_OPTICAL_MIN) / FOCALLENGTH_OCULAR);
    m_ExtendedZoom.Max() = (LONG)(TO_Q16(FOCALLENGTH_OPTICAL_MAX) / FOCALLENGTH_OCULAR);
    m_ExtendedZoom.Step() = 1;   // This avoids the rounding issue.
    m_ExtendedZoom = ZoomDefault;
    //TO_Q16(ZoomRangeAndStep[0].SteppingDelta) / FOCALLENGTH_OCULAR;

    //  Set up our current zoom.
    m_Zoom.Capabilities =
        KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL |
        KSPROPERTY_CAMERACONTROL_FLAGS_AUTO |
        KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE |
        KSPROPERTY_CAMERACONTROL_FLAGS_RELATIVE;
    m_Zoom.Value = ZoomDefault;
    m_Zoom.Flags = KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL | KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE;

    //  Set up our relative zoom.
    m_ZoomRelative.Capabilities =
        KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL |
        KSPROPERTY_CAMERACONTROL_FLAGS_AUTO |
        KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE;
    m_ZoomRelative.Value = ZoomRelativeDefault;
    m_ZoomRelative.Flags = KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL | KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE;

    m_Pan.Capabilities =
        KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL |
        KSPROPERTY_CAMERACONTROL_FLAGS_AUTO |
        KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE;
    m_Pan.Value = PanDefault;
    m_Pan.Flags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO | KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE;

    m_Roll.Capabilities =
        KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL |
        KSPROPERTY_CAMERACONTROL_FLAGS_AUTO |
        KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE;
    m_Roll.Value = RollDefault;
    m_Roll.Flags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO | KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE;

    m_Tilt.Capabilities =
        KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL |
        KSPROPERTY_CAMERACONTROL_FLAGS_AUTO |
        KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE;
    m_Tilt.Value = TiltDefault;
    m_Tilt.Flags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO | KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE;

    m_BacklightCompensation.Capabilities =
        KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL |
        KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_BacklightCompensation.Value = BacklightCompensationDefault;
    m_BacklightCompensation.Flags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;

    m_Brightness.Capabilities =
        KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL |
        KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_Brightness.Value = BrightnessDefault;
    m_Brightness.Flags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;

    m_Contrast.Capabilities =
        KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL |
        KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_Contrast.Value = ContrastDefault;
    m_Contrast.Flags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;

    m_Hue.Capabilities =
        KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL |
        KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_Hue.Value = HueDefault;
    m_Hue.Flags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;

    //  Set up our Power Line Frequency defaults.
    m_PowerLineFreq.Capabilities = KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    m_PowerLineFreq.Value = POWERLINEFREQ_DEFAULT;
    m_PowerLineFreq.Flags = KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;

    m_IsoResult         = STATUS_SUCCESS;
    m_EvCompResult      = STATUS_SUCCESS;
    m_WhiteBalanceResult= STATUS_SUCCESS;
    m_ExposureResult    = STATUS_SUCCESS;
    m_SceneModeResult   = STATUS_SUCCESS;
    m_FaceDetectionResult   = STATUS_SUCCESS;
    m_FocusResult       = STATUS_SUCCESS;

    for( ULONG PinIndex=0; PinIndex<GetPinCount(); PinIndex++ )
    {
        // Reset the metadata control.
        m_MetadataInfo[PinIndex] = CExtendedMetadata(PinIndex);

        //  Reset the warmstart control.
        m_WarmStartEnabled[PinIndex] = CExtendedProperty(KSCAMERA_EXTENDEDPROP_WARMSTART_MODE_DISABLED);
        m_WarmStartEnabled[PinIndex].PinId = PinIndex;
    }

    DBG_LEAVE("()");
    return STATUS_SUCCESS;
}

NTSTATUS
CSensorSimulation::
Initialize()
/*++

Routine Description:

    Perform memory allocations for the simulation.  Inspect the descriptors
    for pins and allocate appropriate simulation objects based on the pin's
    type.  

Arguments:

    None

Return Value:

    Success / Failure

--*/
{
    PAGED_CODE();

    DBG_ENTER("()");
    NTSTATUS    Status = STATUS_SUCCESS;
    IFFAILED_EXIT( Status = CSensor::Initialize() );

    IFNULL_EXIT( m_FocusTimer = new (NonPagedPoolNx, 'emiT') KPassiveTimer( m_Device->GetDeviceObject() ) );
    IFNULL_EXIT( m_FocusRectTimer = new (NonPagedPoolNx, 'emiT') KPassiveTimer( m_Device->GetDeviceObject() ) );
    IFNULL_EXIT( m_MetadataInfo = new (PagedPool, 'ateM') CExtendedMetadata[GetPinCount()] );
    IFNULL_EXIT( m_WarmStartEnabled = new (PagedPool, 'mraW' ) CExtendedProperty[GetPinCount()] );

    if( NT_SUCCESS( Status ) )
    {
        for( ULONG PinIndex=0;
                NT_SUCCESS(Status) && PinIndex<m_Descriptors->PinDescriptorsCount;
                PinIndex++ )
        {
            const KSPIN_DESCRIPTOR_EX *PinDescriptors = m_Descriptors->PinDescriptors;
            CHardwareSimulation *pSim=nullptr;

            NT_ASSERT( m_Descriptors->PinDescriptorSize == sizeof( *PinDescriptors ) );

            //  Video pin type
            if( IsEqualGUID( *PinDescriptors[PinIndex].PinDescriptor.Category, PIN_CATEGORY_CAPTURE ) )
            {
                pSim = new (NonPagedPoolNx, 'ediV') CVideoHardwareSimulation( this, PinIndex );
                m_VideoIndex = PinIndex;
            }
            else if( IsEqualGUID( *PinDescriptors[PinIndex].PinDescriptor.Category, PIN_CATEGORY_PREVIEW ) )
            {
                pSim = new (NonPagedPoolNx, 'verP') CPreviewHardwareSimulation( this, PinIndex );
                m_PreviewIndex = PinIndex;      // must set to enable photo confirmation...
            }
            else if( IsEqualGUID( *PinDescriptors[PinIndex].PinDescriptor.Category, PINNAME_IMAGE ) )
            {
                pSim = new (NonPagedPoolNx, 'litS') CImageHardwareSimulation( this, PinIndex );
                m_StillIndex = PinIndex;
            }
            else
            {
                Status = STATUS_INTERNAL_ERROR;
                NT_ASSERT(FALSE);
            }

            if( pSim )
            {
                if( pSim->Initialize() )
                {
                    m_HardwareSimulation[PinIndex] = pSim;
                }
                else
                {
                    //
                    // If we couldn't create the hardware simulation, fail.
                    //
                    delete pSim;
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

done:
    DBG_ENTER("()=0x%08X", Status);
    return Status;
}

/**************************************************************************

    Sensor-level control simulations

    1. Query capabilities and state of the device.
    2. Issue a command or set state on the device.

**************************************************************************/

//  Get for KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_PROPERTY_ID
NTSTATUS
CSensorSimulation::
GetFocusRect(
    _Inout_ PKSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_S pRoi
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    pRoi->FocusRect = m_FocusRect;
    pRoi->AutoFocusLock = m_AutoFocusLock;
    pRoi->AutoExposureLock = m_AutoExposureLock;
    pRoi->AutoWhitebalanceLock = m_AutoWhitebalanceLock;
    pRoi->Capabilities = KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_FLAGS_AUTO | KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_FLAGS_MANUAL |
                         KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_FLAGS_ASYNC | KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_CONFIG_FOCUS |
                         KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_CONFIG_EXPOSURE;
    return STATUS_SUCCESS;
}

//  Set for KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_PROPERTY_ID
NTSTATUS
CSensorSimulation::
SetFocusRectAsync(
    _In_    PKSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_S pRoi,
    _In_    CNotifier  *Notifier            // Use to notify the framework that the control is done.
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    if( m_FocusRectNotifier )
    {
        //  The last operation is still pending.
        return STATUS_INVALID_DEVICE_STATE;
    }

    m_FocusRectNotifier = Notifier;

    m_FocusRect = pRoi->FocusRect;
    m_AutoFocusLock = pRoi->AutoFocusLock;
    m_AutoExposureLock = pRoi->AutoExposureLock;
    m_AutoWhitebalanceLock = pRoi->AutoWhitebalanceLock;

    if(m_AutoFocusLock)
    {
        m_GlobalIspSettings.FocusMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul = 100;
    }
    if(m_AutoExposureLock)
    {
        m_GlobalIspSettings.ExposureMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureSetting.VideoProc.Value.ll = DEF_EXPOSURE_TIME;
        DBG_TRACE("ExposureMode=KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO, ExposureTime=%lld00ns",
                  m_GlobalIspSettings.ExposureSetting.VideoProc.Value.ll);
    }
    if(m_AutoWhitebalanceLock)
    {
        m_GlobalIspSettings.WhiteBalanceMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.WhiteBalanceSetting.VideoProc.Value.ul = WhiteBalanceDefault;
        m_GlobalIspSettings.WhiteBalanceSetting.Mode = KSCAMERA_EXTENDEDPROP_WHITEBALANCE_TEMPERATURE;
    }

    //  Fire a one-shot timer to transition the focus state.
    m_FocusTimer->Set(
        -2500000LL,   //  250ms
        FocusRectConvergence,
        this );

    return STATUS_SUCCESS;
}

//  Cancel an outstanding KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_PROPERTY_ID Set
NTSTATUS
CSensorSimulation::
CancelFocusRect()
{
    PAGED_CODE();

    //  Per spec, this control cannot be cancelled.  So we block until complete.
    //  Even if we didn't block here, the CNotifier object is reference counted,
    //  so the CCaptureFilter would block until the operation completes.
    if( m_FocusRectNotifier )
    {
        m_FocusRectTimer->Wait();
    }
    return STATUS_UNSUCCESSFUL;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_FACEDETECTION.
NTSTATUS
CSensorSimulation::
GetFaceDetection(
    _Inout_ CExtendedVidProcSetting *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    *pProperty = CExtendedVidProcSetting( m_FaceDetectionFlags );

    pProperty->Capability =
        KSCAMERA_EXTENDEDPROP_FACEDETECTION_OFF     |
        KSCAMERA_EXTENDEDPROP_FACEDETECTION_PREVIEW |
        KSCAMERA_EXTENDEDPROP_FACEDETECTION_VIDEO   |
        KSCAMERA_EXTENDEDPROP_FACEDETECTION_PHOTO   |
        KSCAMERA_EXTENDEDPROP_FACEDETECTION_BLINK   |
        KSCAMERA_EXTENDEDPROP_FACEDETECTION_SMILE ;

    pProperty->Max() = m_FaceDetectionMax;
    pProperty->Min() = 1;
    pProperty->Step() = 1;
    *pProperty = m_FaceDetectionCurrentMax;
    pProperty->Result = m_FaceDetectionResult;

    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_FACEDETECTION.
NTSTATUS
CSensorSimulation::
SetFaceDetection(
    _In_    CExtendedVidProcSetting *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    m_FaceDetectionCurrentMax =
        ( KSCAMERA_EXTENDEDPROP_FACEDETECTION_MASK & pProperty->Flags ) ?
        pProperty->GetULONG() : 0;
    m_FaceDetectionFlags = pProperty->Flags;
    m_FaceDetectionResult = STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSSTATE
NTSTATUS
CSensorSimulation::
GetFocusState(
    _Out_   KSCAMERA_EXTENDEDPROP_FOCUSSTATE   *FocusState
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    *FocusState = m_FocusState;

    DBG_LEAVE("(%s)", FocusStateDbgTxt(m_FocusState));
    return STATUS_SUCCESS;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSMODE
NTSTATUS
CSensorSimulation::
GetFocus(
    _Inout_ CExtendedVidProcSetting *pSetting
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    //  Mode
    pSetting->Flags = m_GlobalIspSettings.FocusMode;

    //  capabilities.
    pSetting->Capability =  KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL         |
                            KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO        |
                            KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK        |
                            KSCAMERA_EXTENDEDPROP_FOCUS_UNLOCK              |
                            KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL      |
                            KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS          |
                            KSCAMERA_EXTENDEDPROP_CAPS_CANCELLABLE          |
                            KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_MACRO         |
                            KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_NORMAL        |
                            KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_FULLRANGE     |
                            KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUSLOCK      |
                            KSCAMERA_EXTENDEDPROP_FOCUS_REGIONBASED         |
                            KSCAMERA_EXTENDEDPROP_FOCUS_DRIVERFALLBACK_OFF  |
                            KSCAMERA_EXTENDEDPROP_FOCUS_DISTANCE_INFINITY   |
                            KSCAMERA_EXTENDEDPROP_FOCUS_DISTANCE_HYPERFOCAL |
                            KSCAMERA_EXTENDEDPROP_FOCUS_DISTANCE_NEAREST
                            ;

    pSetting->Result = m_FocusResult;

    //  min/max/step/etc.
    pSetting->m_Setting = m_GlobalIspSettings.FocusSetting;

    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSMODE
NTSTATUS
CSensorSimulation::
SetFocusAsync(
    _In_    CExtendedVidProcSetting *pSetting,
    _In_    CNotifier               *Notifier           // Use to notify the framework that the control is done.
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    //  Signal immediately if the we are continuous focus.
    bool    bSignalImmediately = false;
    ULONGLONG   Flags = pSetting->Flags;

    //  Cache our current focus settings in case we get a cancel request.
    m_LastFocusMode = m_GlobalIspSettings.FocusMode;
    m_LastFocusSetting = m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul;

    //  If we've been asked to unlock, remove any lock flags.
    if( Flags & KSCAMERA_EXTENDEDPROP_FOCUS_UNLOCK )
    {
        //  Mark us as signalling immediately if Focus was not locked.
        bSignalImmediately =
            ( m_GlobalIspSettings.FocusMode &
              ( KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK |
                KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUSLOCK ) ) == 0;

        //  Change our operation so we go back to the previous state.
        Flags = m_GlobalIspSettings.FocusMode &
                ~( KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK |
                   KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUSLOCK );
    }

    //  Handle the stand-alone lock case by setting the appropriate flags.
    if( ( Flags &
            (KSCAMERA_EXTENDEDPROP_FOCUS_MODE_MASK |
             KSCAMERA_EXTENDEDPROP_FOCUS_MODE_ADVANCED_MASK) ) ==
            KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK )
    {
        if( m_GlobalIspSettings.FocusMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO )
        {
            Flags =
                m_GlobalIspSettings.FocusMode | KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK;
        }
        if( m_GlobalIspSettings.FocusMode & KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS )
        {
            Flags =
                m_GlobalIspSettings.FocusMode |= KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUSLOCK;
        }
    }

    //  Record the Focus mode.
    DBG_TRACE("Recording FocusMode = 0x%016llX", Flags);
    m_GlobalIspSettings.FocusMode = Flags;

    if(m_GlobalIspSettings.FocusMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL)
    {
        m_GlobalIspSettings.FocusSetting.VideoProc.Value.l = pSetting->GetLONG();
    }

    if( bSignalImmediately )
        //  Unlock, but nothing really to do.
    {
        Notifier->Set();
        m_FocusNotifier = nullptr;
    }
    else if( Flags & KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS )
        //  Continuous case: Signal immediately.
        //  Then prime the timer to switch the state to focused.
    {
        m_FocusNotifier = nullptr;
        m_FocusState = KSCAMERA_EXTENDEDPROP_FOCUSSTATE_SEARCHING;
        Notifier->Set();

        //  Fire a one-shot timer to transition the focus state.
        m_FocusTimer->Set(
            -4000000LL,   //  400ms
            FocusConvergence,
            this );
    }
    else if( Flags & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO )
        //  Normal [auto?] focus operation.
        //  Autofocus should be SLOW.
    {
        m_FocusNotifier = Notifier;
        m_FocusState = KSCAMERA_EXTENDEDPROP_FOCUSSTATE_SEARCHING;

        //  Fire a one-shot timer to transition the focus state.
        m_FocusTimer->Set(
            -4000000LL,   //  400ms
            FocusConvergence,
            this );
    }
    else
        //  Handle Manual, lock & unlock cases FAST ~1 frame time.
    {
        m_FocusNotifier = Notifier;
        m_FocusState = KSCAMERA_EXTENDEDPROP_FOCUSSTATE_SEARCHING;

        //  Fire a one-shot timer to transition the focus state.
        m_FocusTimer->Set(
            -330000LL,  //  33ms
            FocusConvergence,
            this );
    }

    m_FocusResult = STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

//  Cancel oustanding set KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSMODE
NTSTATUS
CSensorSimulation::
CancelFocus()
{
    PAGED_CODE();

    //  Cancel our timer to prevent a race.
    m_FocusTimer->Cancel();

    //  Acquire the lock after the cancel to avoid a potential deadlock.
    KScopedMutex    lock( m_SensorMutex );

    //  Now check to see if an operation was still outstanding.
    //  We could just check the status from the timer's Cancel(), but I 
    //  prefer checking to see if the handler cleared the notifier instead.
    if( m_FocusNotifier )
    {
        m_FocusState = KSCAMERA_EXTENDEDPROP_FOCUSSTATE_LOST;

        //  Cancel behavior isn't well-defined in the spec.  Do something reasonable.
        //  In our case, we'll put the focus mode and value back to the last setting.
        m_GlobalIspSettings.FocusMode = m_LastFocusMode;
        m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul = m_LastFocusSetting;

        m_FocusNotifier->Set();
        m_FocusNotifier = nullptr;
        m_FocusResult = (ULONG) STATUS_CANCELLED;
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_FLASHMODE
NTSTATUS
CSensorSimulation::
GetExtendedFlash(
    _Inout_ CExtendedProperty   *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    *pProperty = CExtendedProperty( m_GlobalIspSettings.FlashMode );
    *pProperty = m_GlobalIspSettings.FlashValue;
    pProperty->Capability = KSCAMERA_EXTENDEDPROP_FLASH_ON | KSCAMERA_EXTENDEDPROP_FLASH_ON_ADJUSTABLEPOWER |
                            KSCAMERA_EXTENDEDPROP_FLASH_AUTO | KSCAMERA_EXTENDEDPROP_FLASH_AUTO_ADJUSTABLEPOWER |
                            KSCAMERA_EXTENDEDPROP_FLASH_REDEYEREDUCTION | KSCAMERA_EXTENDEDPROP_FLASH_SINGLEFLASH |
                            KSCAMERA_EXTENDEDPROP_FLASH_MULTIFLASHSUPPORTED |
                            KSCAMERA_EXTENDEDPROP_FLASH_ASSISTANT_ON | KSCAMERA_EXTENDEDPROP_FLASH_ASSISTANT_OFF |
                            KSCAMERA_EXTENDEDPROP_FLASH_ASSISTANT_AUTO
                            ;

    DBG_TRACE("FlashMode=0x%016llX, FlashValue=%d",
              m_GlobalIspSettings.FlashMode,
              m_GlobalIspSettings.FlashValue );

    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_FLASHMODE
NTSTATUS
CSensorSimulation::
SetExtendedFlash(
    _In_    CExtendedProperty   *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    if(pProperty->Flags & (KSCAMERA_EXTENDEDPROP_FLASH_ON_ADJUSTABLEPOWER |
                           KSCAMERA_EXTENDEDPROP_FLASH_AUTO_ADJUSTABLEPOWER) )
    {
        m_GlobalIspSettings.FlashValue = pProperty->m_Value.Value.ul;
    }
    else
    {
        m_GlobalIspSettings.FlashValue = 50;
    }

    m_GlobalIspSettings.FlashMode = pProperty->Flags;
    NTSTATUS Status = SetFlashStatus(pProperty->Flags);

    DBG_TRACE("FlashMode=0x%016llX, FlashValue=%d, Status=0x%08X",
              m_GlobalIspSettings.FlashMode,
              m_GlobalIspSettings.FlashValue,
              Status );

    pProperty->Result = (ULONG) Status;
    return Status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_ISO
NTSTATUS
CSensorSimulation::
GetIso(
    _Inout_ CExtendedProperty   *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    pProperty->Flags = IsoModeValue2Preset( 
                           m_GlobalIspSettings.ISOMode, 
                           m_GlobalIspSettings.ISOValue );
    pProperty->Result = STATUS_SUCCESS;
    pProperty->Capability = KSCAMERA_EXTENDEDPROP_ISO_AUTO |
                            KSCAMERA_EXTENDEDPROP_ISO_50 | KSCAMERA_EXTENDEDPROP_ISO_80 |
                            KSCAMERA_EXTENDEDPROP_ISO_100 | KSCAMERA_EXTENDEDPROP_ISO_200 |
                            KSCAMERA_EXTENDEDPROP_ISO_400 | KSCAMERA_EXTENDEDPROP_ISO_800 |
                            KSCAMERA_EXTENDEDPROP_ISO_1600 | KSCAMERA_EXTENDEDPROP_ISO_3200 |
                            KSCAMERA_EXTENDEDPROP_ISO_6400 | KSCAMERA_EXTENDEDPROP_ISO_12800 |
                            KSCAMERA_EXTENDEDPROP_ISO_25600 |
                            KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL;
    pProperty->Result = m_IsoResult;
    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_ISO
NTSTATUS
CSensorSimulation::
SetIsoAsync(
    _In_    CExtendedProperty   *pProperty,
    _In_    CNotifier           *Notifier            // Use to notify the framework that the control is done.
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    m_GlobalIspSettings.ISOMode = (ULONG)pProperty->Flags;
    m_GlobalIspSettings.ISOValue = 0;

    //  This could be done in a separate work item after the scene mode has been programmed...
    m_IsoResult = STATUS_SUCCESS;
    Notifier->Set();

    return STATUS_SUCCESS;
}

#define KSCAMERA_EXTENDEDPROP_ISO_ADVANCED_MASK (KSCAMERA_EXTENDEDPROP_ISO_AUTO | KSCAMERA_EXTENDEDPROP_ISO_MANUAL)

//  Create a set of default settings...
static
const
KSCAMERA_EXTENDEDPROP_VIDEOPROCSETTING g_IsoAdvancedSettings =
{
    0,          // Mode - Unused
    50,         // Min
    25600,      // Max
    1           // Step (advertised doesn't have to be actual)
};

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_ISO_ADVANCED.
NTSTATUS
CSensorSimulation::
GetIsoAdvanced(
    _Inout_ CExtendedVidProcSetting   *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    //  Initialize the property with defaults + (AUTO | MANUAL)
    pProperty->Flags = 
            m_GlobalIspSettings.ISOMode & 
                KSCAMERA_EXTENDEDPROP_ISO_ADVANCED_MASK;
    pProperty->Result = STATUS_SUCCESS;

    //  Advertise basic caps...
    pProperty->Capability =
        KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL |
        KSCAMERA_EXTENDEDPROP_ISO_AUTO          |
        KSCAMERA_EXTENDEDPROP_ISO_MANUAL ;

    //  Advertise the manual caps...
    *pProperty = g_IsoAdvancedSettings;

    //  If ISO is MANUAL, set the value.
    if( m_GlobalIspSettings.ISOMode == KSCAMERA_EXTENDEDPROP_ISO_MANUAL )
    {
        *pProperty = m_GlobalIspSettings.ISOValue;
    }
    else if( !(m_GlobalIspSettings.ISOMode == KSCAMERA_EXTENDEDPROP_ISO_AUTO) )
    {
        //  Try converting any legacy presets to a manual value.
        pProperty->Flags = KSCAMERA_EXTENDEDPROP_ISO_MANUAL;
        //  Convert the ISO setting
        *pProperty = IsoPreset2Value( m_GlobalIspSettings.ISOMode );
        //  If we weren't able to report a valid number, just report something reasonable...
        if( pProperty->GetLONG() > pProperty->Max() )
        {
            *pProperty = pProperty->Min();
        }
    }
    pProperty->Result = m_IsoResult;

    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_ISO_ADVANCED.
NTSTATUS
CSensorSimulation::
SetIsoAdvancedAsync(
    _In_    CExtendedVidProcSetting *pProperty,
    _In_    CNotifier               *Notifier            // Use to notify the framework that the control is done.
)
{
    PAGED_CODE();

    KScopedMutex    lock( m_SensorMutex );

    //  Just save the parameters
    if( pProperty->Flags & KSCAMERA_EXTENDEDPROP_ISO_MANUAL )
    {
        m_GlobalIspSettings.ISOMode = pProperty->Flags;
        m_GlobalIspSettings.ISOValue= pProperty->GetULONG();
    }
    else if( pProperty->Flags & KSCAMERA_EXTENDEDPROP_ISO_AUTO )
    {
        m_GlobalIspSettings.ISOMode = pProperty->Flags;
    }

    //  Set regardless - it takes effect immediately for us.
    m_IsoResult = STATUS_SUCCESS;
    Notifier->Set();

    return STATUS_SUCCESS;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_EVCOMPENSATION
NTSTATUS
CSensorSimulation::
GetEvCompensation(
    _Inout_ CExtendedEvCompensation *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    *pProperty = CExtendedEvCompensation( m_GlobalIspSettings.EVCompensation.Mode );

    // Return our cached copy; but update the value from the global setting.
    *pProperty = m_GlobalIspSettings.EVCompensation;
    pProperty->Capability = KSCAMERA_EXTENDEDPROP_EVCOMP_SIXTHSTEP | KSCAMERA_EXTENDEDPROP_EVCOMP_QUARTERSTEP |
                            KSCAMERA_EXTENDEDPROP_EVCOMP_THIRDSTEP | KSCAMERA_EXTENDEDPROP_EVCOMP_HALFSTEP |
                            KSCAMERA_EXTENDEDPROP_EVCOMP_FULLSTEP | KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL;
    pProperty->Result = m_EvCompResult;
    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_EVCOMPENSATION
NTSTATUS
CSensorSimulation::
SetEvCompensationAsync(
    _In_    CExtendedEvCompensation *pProperty,
    _In_    CNotifier               *Notifier           // Use to notify the framework that the control is done.
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    //  Update the Globals
    m_GlobalIspSettings.EVCompensation.Mode = (ULONG) pProperty->Flags;
    m_GlobalIspSettings.EVCompensation.Value = pProperty->Value();

    //  Update the current setting in case we're queried for it later.
    m_GlobalIspSettings.EVCompensation.Value = pProperty->Value();

    //  This could be done in a separate work item after the device has been programmed...
    m_EvCompResult = STATUS_SUCCESS;
    Notifier->Set();

    return STATUS_SUCCESS;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_WHITEBALANCEMODE
NTSTATUS
CSensorSimulation::
GetWhiteBalance(
    _Inout_ CExtendedVidProcSetting *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    *pProperty = CExtendedVidProcSetting( m_GlobalIspSettings.WhiteBalanceMode );

    // Return our cached copy; but update the value from the global setting.
    *pProperty = m_GlobalIspSettings.WhiteBalanceSetting;
    pProperty->Capability = KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL | KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO |
                            KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK | KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL;
    pProperty->Result = m_WhiteBalanceResult;
    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_WHITEBALANCEMODE
NTSTATUS
CSensorSimulation::
SetWhiteBalanceAsync(
    _In_    CExtendedVidProcSetting *pProperty,
    _In_    CNotifier               *Notifier           // Use to notify the framework that the control is done.
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    m_GlobalIspSettings.WhiteBalanceMode = pProperty->Flags;
    m_GlobalIspSettings.WhiteBalanceSetting.Mode = pProperty->Mode();
    m_GlobalIspSettings.WhiteBalanceSetting.VideoProc.Value.ul = pProperty->GetULONG();

    //  This could be done in a separate work item after the device has been programmed...
    m_WhiteBalanceResult = STATUS_SUCCESS;
    Notifier->Set();

    return STATUS_SUCCESS;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_EXPOSURE
NTSTATUS
CSensorSimulation::
GetExposure(
    _Inout_ CExtendedVidProcSetting *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    *pProperty = CExtendedVidProcSetting( m_GlobalIspSettings.ExposureMode );

    // Return our cached copy; but update the value from the global setting.
    *pProperty = m_GlobalIspSettings.ExposureSetting;
    pProperty->Capability = KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL | KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO |
                            KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK | KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL;
    pProperty->Result = m_ExposureResult;
    DBG_TRACE("ExposureMode=0x%016llX, ExposureTime=%lld00ns",
              pProperty->Flags, pProperty->GetLONGLONG());
    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_EXPOSURE
NTSTATUS
CSensorSimulation::
SetExposureAsync(
    _In_    CExtendedVidProcSetting *pProperty,
    _In_    CNotifier               *Notifier           // Use to notify the framework that the control is done.
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    DBG_TRACE("ExposureMode=0x%016llX, ExposureTime=%lld00ns",
              pProperty->Flags, pProperty->GetLONGLONG());
    if( pProperty->Flags & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL )
    {
        m_GlobalIspSettings.ExposureSetting.VideoProc.Value.ull = pProperty->GetULONGLONG();
    }
    m_GlobalIspSettings.ExposureMode = pProperty->Flags;

    //  This could be done in a separate work item after the device has been programmed...
    m_ExposureResult = STATUS_SUCCESS;
    Notifier->Set();

    return STATUS_SUCCESS;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOCONFIRMATION
NTSTATUS
CSensorSimulation::
GetPhotoConfirmation(
    _Inout_ CExtendedProperty *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    *pProperty = CExtendedProperty( m_GlobalIspSettings.bPhotoConfirmation );
    DBG_TRACE( "Is %s", m_GlobalIspSettings.bPhotoConfirmation ? "On" : "Off" );
    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOCONFIRMATION
NTSTATUS
CSensorSimulation::
SetPhotoConfirmation(
    _In_    CExtendedProperty *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    // Return our cached copy; but update the value from the global setting.
    m_GlobalIspSettings.bPhotoConfirmation =
        BOOLEAN( (pProperty->Flags & KSCAMERA_EXTENDEDPROP_PHOTOCONFIRMATION_ON)
                 == KSCAMERA_EXTENDEDPROP_PHOTOCONFIRMATION_ON);
    DBG_TRACE( "Is %s", m_GlobalIspSettings.bPhotoConfirmation ? "On" : "Off" );
    return STATUS_SUCCESS;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_SCENEMODE
NTSTATUS
CSensorSimulation::
GetSceneMode(
    _Inout_ CExtendedProperty   *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    *pProperty = CExtendedProperty( m_SceneMode );
    pProperty->Capability = KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL | KSCAMERA_EXTENDEDPROP_SCENEMODE_MACRO |
                            KSCAMERA_EXTENDEDPROP_SCENEMODE_PORTRAIT | KSCAMERA_EXTENDEDPROP_SCENEMODE_SPORT |
                            KSCAMERA_EXTENDEDPROP_SCENEMODE_SNOW | KSCAMERA_EXTENDEDPROP_SCENEMODE_NIGHT |
                            KSCAMERA_EXTENDEDPROP_SCENEMODE_BEACH | KSCAMERA_EXTENDEDPROP_SCENEMODE_SUNSET |
                            KSCAMERA_EXTENDEDPROP_SCENEMODE_CANDLELIGHT | KSCAMERA_EXTENDEDPROP_SCENEMODE_LANDSCAPE |
                            KSCAMERA_EXTENDEDPROP_SCENEMODE_NIGHTPORTRAIT | KSCAMERA_EXTENDEDPROP_SCENEMODE_BACKLIT;
    pProperty->Result = m_SceneModeResult;
    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_SCENEMODE
NTSTATUS
CSensorSimulation::
SetSceneModeAsync(
    _In_    CExtendedProperty       *pProperty,
    _In_    CNotifier               *Notifier           // Use to notify the framework that the control is done.
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    m_SceneMode = (ULONG)pProperty->Flags;

    UpdateSettings( m_SceneMode );

    //  This could be done in a separate work item after the scene mode has been programmed...
    m_SceneModeResult = STATUS_SUCCESS;
    Notifier->Set();

    return STATUS_SUCCESS;
}

//  Apply scene mode changes.
NTSTATUS
CSensorSimulation::
UpdateSettings(
    _In_    ULONGLONG ullSceneMode
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;
    KScopedMutex    lock( m_SensorMutex );

    if( KSCAMERA_EXTENDEDPROP_SCENEMODE_AUTO==ullSceneMode )
    {
        //
        //  If the mode was auto, let's randomly replace it with another valid value.
        //
        static
        ULONGLONG ValidModes[] =
        {
            KSCAMERA_EXTENDEDPROP_SCENEMODE_MACRO,
            KSCAMERA_EXTENDEDPROP_SCENEMODE_PORTRAIT, KSCAMERA_EXTENDEDPROP_SCENEMODE_SPORT,
            KSCAMERA_EXTENDEDPROP_SCENEMODE_SNOW, KSCAMERA_EXTENDEDPROP_SCENEMODE_NIGHT,
            KSCAMERA_EXTENDEDPROP_SCENEMODE_BEACH, KSCAMERA_EXTENDEDPROP_SCENEMODE_SUNSET,
            KSCAMERA_EXTENDEDPROP_SCENEMODE_CANDLELIGHT, KSCAMERA_EXTENDEDPROP_SCENEMODE_LANDSCAPE,
            KSCAMERA_EXTENDEDPROP_SCENEMODE_NIGHTPORTRAIT, KSCAMERA_EXTENDEDPROP_SCENEMODE_BACKLIT
        };

        ullSceneMode =
            ValidModes[ (ULONG) (1 << GetRandom( (ULONG)0, SIZEOF_ARRAY(ValidModes)-1 )) ];
    }

    switch(ullSceneMode)
    {
    case KSCAMERA_EXTENDEDPROP_SCENEMODE_MACRO:
        m_GlobalIspSettings.ISOMode = KSCAMERA_EXTENDEDPROP_ISO_AUTO;
        m_GlobalIspSettings.EVCompensation.Value = 0;
        m_GlobalIspSettings.EVCompensation.Mode = KSCAMERA_EXTENDEDPROP_EVCOMP_FULLSTEP;
        m_GlobalIspSettings.WhiteBalanceSetting.VideoProc.Value.ul = WhiteBalanceDefault;
        m_GlobalIspSettings.WhiteBalanceSetting.Mode = KSCAMERA_EXTENDEDPROP_WHITEBALANCE_TEMPERATURE;
        m_GlobalIspSettings.WhiteBalanceMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureSetting.VideoProc.Value.ul = DEF_EXPOSURE_TIME;
        m_GlobalIspSettings.ExposureSetting.Mode = 0;
        m_GlobalIspSettings.FocusMode = KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS | KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_FULLRANGE;
        m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul = 100;
        break;
    case KSCAMERA_EXTENDEDPROP_SCENEMODE_PORTRAIT:
        m_GlobalIspSettings.ISOMode = KSCAMERA_EXTENDEDPROP_ISO_AUTO;
        m_GlobalIspSettings.EVCompensation.Value = 0;
        m_GlobalIspSettings.EVCompensation.Mode = KSCAMERA_EXTENDEDPROP_EVCOMP_FULLSTEP;
        m_GlobalIspSettings.WhiteBalanceSetting.VideoProc.Value.ul = WhiteBalanceDefault;
        m_GlobalIspSettings.WhiteBalanceSetting.Mode = KSCAMERA_EXTENDEDPROP_WHITEBALANCE_TEMPERATURE;
        m_GlobalIspSettings.WhiteBalanceMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureSetting.VideoProc.Value.ul = DEF_EXPOSURE_TIME;
        m_GlobalIspSettings.ExposureSetting.Mode = 0;
        m_GlobalIspSettings.FocusMode = KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS | KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_FULLRANGE;
        m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul = 100;
        break;
    case KSCAMERA_EXTENDEDPROP_SCENEMODE_SPORT:
        m_GlobalIspSettings.ISOMode = KSCAMERA_EXTENDEDPROP_ISO_AUTO;
        m_GlobalIspSettings.EVCompensation.Value = 0;
        m_GlobalIspSettings.EVCompensation.Mode = KSCAMERA_EXTENDEDPROP_EVCOMP_FULLSTEP;
        m_GlobalIspSettings.WhiteBalanceSetting.VideoProc.Value.ul = WhiteBalanceDefault;
        m_GlobalIspSettings.WhiteBalanceSetting.Mode = KSCAMERA_EXTENDEDPROP_WHITEBALANCE_TEMPERATURE;
        m_GlobalIspSettings.WhiteBalanceMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureSetting.VideoProc.Value.ul = DEF_EXPOSURE_TIME;
        m_GlobalIspSettings.ExposureSetting.Mode = 0;
        m_GlobalIspSettings.FocusMode = KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS | KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_FULLRANGE;
        m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul = 100;
        break;
    case KSCAMERA_EXTENDEDPROP_SCENEMODE_SNOW:
        m_GlobalIspSettings.ISOMode = KSCAMERA_EXTENDEDPROP_ISO_AUTO;
        m_GlobalIspSettings.EVCompensation.Value = 0;
        m_GlobalIspSettings.EVCompensation.Mode = KSCAMERA_EXTENDEDPROP_EVCOMP_FULLSTEP;
        m_GlobalIspSettings.WhiteBalanceSetting.VideoProc.Value.ul = WhiteBalanceDefault;
        m_GlobalIspSettings.WhiteBalanceSetting.Mode = KSCAMERA_EXTENDEDPROP_WHITEBALANCE_TEMPERATURE;
        m_GlobalIspSettings.WhiteBalanceMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureSetting.VideoProc.Value.ul = DEF_EXPOSURE_TIME;
        m_GlobalIspSettings.ExposureSetting.Mode = 0;
        m_GlobalIspSettings.FocusMode = KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS | KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_FULLRANGE;
        m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul = 100;
        break;
    case KSCAMERA_EXTENDEDPROP_SCENEMODE_NIGHT:
        m_GlobalIspSettings.ISOMode = KSCAMERA_EXTENDEDPROP_ISO_AUTO;
        m_GlobalIspSettings.EVCompensation.Value = 0;
        m_GlobalIspSettings.EVCompensation.Mode = KSCAMERA_EXTENDEDPROP_EVCOMP_FULLSTEP;
        m_GlobalIspSettings.WhiteBalanceSetting.VideoProc.Value.ul = WhiteBalanceDefault;
        m_GlobalIspSettings.WhiteBalanceSetting.Mode = KSCAMERA_EXTENDEDPROP_WHITEBALANCE_TEMPERATURE;
        m_GlobalIspSettings.WhiteBalanceMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureSetting.VideoProc.Value.ul = DEF_EXPOSURE_TIME;
        m_GlobalIspSettings.ExposureSetting.Mode = 0;
        m_GlobalIspSettings.FocusMode = KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS | KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_FULLRANGE;
        m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul = 100;
        break;
    case KSCAMERA_EXTENDEDPROP_SCENEMODE_BEACH:
        m_GlobalIspSettings.ISOMode = KSCAMERA_EXTENDEDPROP_ISO_AUTO;
        m_GlobalIspSettings.EVCompensation.Value = 0;
        m_GlobalIspSettings.EVCompensation.Mode = KSCAMERA_EXTENDEDPROP_EVCOMP_FULLSTEP;
        m_GlobalIspSettings.WhiteBalanceSetting.VideoProc.Value.ul = WhiteBalanceDefault;
        m_GlobalIspSettings.WhiteBalanceSetting.Mode = KSCAMERA_EXTENDEDPROP_WHITEBALANCE_TEMPERATURE;
        m_GlobalIspSettings.WhiteBalanceMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureSetting.VideoProc.Value.ul = DEF_EXPOSURE_TIME;
        m_GlobalIspSettings.ExposureSetting.Mode = 0;
        m_GlobalIspSettings.FocusMode = KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS | KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_FULLRANGE;
        m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul = 100;
        break;
    case KSCAMERA_EXTENDEDPROP_SCENEMODE_SUNSET:
        m_GlobalIspSettings.ISOMode = KSCAMERA_EXTENDEDPROP_ISO_AUTO;
        m_GlobalIspSettings.EVCompensation.Value = 0;
        m_GlobalIspSettings.EVCompensation.Mode = KSCAMERA_EXTENDEDPROP_EVCOMP_FULLSTEP;
        m_GlobalIspSettings.WhiteBalanceSetting.VideoProc.Value.ul = WhiteBalanceDefault;
        m_GlobalIspSettings.WhiteBalanceSetting.Mode = KSCAMERA_EXTENDEDPROP_WHITEBALANCE_TEMPERATURE;
        m_GlobalIspSettings.WhiteBalanceMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureSetting.VideoProc.Value.ul = DEF_EXPOSURE_TIME;
        m_GlobalIspSettings.ExposureSetting.Mode = 0;
        m_GlobalIspSettings.FocusMode = KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS | KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_FULLRANGE;
        m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul = 100;
        break;
    case KSCAMERA_EXTENDEDPROP_SCENEMODE_CANDLELIGHT:
        m_GlobalIspSettings.ISOMode = KSCAMERA_EXTENDEDPROP_ISO_AUTO;
        m_GlobalIspSettings.EVCompensation.Value = 0;
        m_GlobalIspSettings.EVCompensation.Mode = KSCAMERA_EXTENDEDPROP_EVCOMP_FULLSTEP;
        m_GlobalIspSettings.WhiteBalanceSetting.VideoProc.Value.ul = WhiteBalanceDefault;
        m_GlobalIspSettings.WhiteBalanceSetting.Mode = KSCAMERA_EXTENDEDPROP_WHITEBALANCE_TEMPERATURE;
        m_GlobalIspSettings.WhiteBalanceMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureSetting.VideoProc.Value.ul = DEF_EXPOSURE_TIME;
        m_GlobalIspSettings.ExposureSetting.Mode = 0;
        m_GlobalIspSettings.FocusMode = KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS | KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_FULLRANGE;
        m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul = 100;
        break;
    case KSCAMERA_EXTENDEDPROP_SCENEMODE_LANDSCAPE:
        m_GlobalIspSettings.ISOMode = KSCAMERA_EXTENDEDPROP_ISO_AUTO;
        m_GlobalIspSettings.EVCompensation.Value = 0;
        m_GlobalIspSettings.EVCompensation.Mode = KSCAMERA_EXTENDEDPROP_EVCOMP_FULLSTEP;
        m_GlobalIspSettings.WhiteBalanceSetting.VideoProc.Value.ul = WhiteBalanceDefault;
        m_GlobalIspSettings.WhiteBalanceSetting.Mode = KSCAMERA_EXTENDEDPROP_WHITEBALANCE_TEMPERATURE;
        m_GlobalIspSettings.WhiteBalanceMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureSetting.VideoProc.Value.ul = DEF_EXPOSURE_TIME;
        m_GlobalIspSettings.ExposureSetting.Mode = 0;
        m_GlobalIspSettings.FocusMode = KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS | KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_FULLRANGE;
        m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul = 100;
        break;
    case KSCAMERA_EXTENDEDPROP_SCENEMODE_NIGHTPORTRAIT:
        m_GlobalIspSettings.ISOMode = KSCAMERA_EXTENDEDPROP_ISO_AUTO;
        m_GlobalIspSettings.EVCompensation.Value = 0;
        m_GlobalIspSettings.EVCompensation.Mode = KSCAMERA_EXTENDEDPROP_EVCOMP_FULLSTEP;
        m_GlobalIspSettings.WhiteBalanceSetting.VideoProc.Value.ul = WhiteBalanceDefault;
        m_GlobalIspSettings.WhiteBalanceSetting.Mode = KSCAMERA_EXTENDEDPROP_WHITEBALANCE_TEMPERATURE;
        m_GlobalIspSettings.WhiteBalanceMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureSetting.VideoProc.Value.ul = DEF_EXPOSURE_TIME;
        m_GlobalIspSettings.ExposureSetting.Mode = 0;
        m_GlobalIspSettings.FocusMode = KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS | KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_FULLRANGE;
        m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul = 100;
        break;
    case KSCAMERA_EXTENDEDPROP_SCENEMODE_BACKLIT:
        m_GlobalIspSettings.ISOMode = KSCAMERA_EXTENDEDPROP_ISO_AUTO;
        m_GlobalIspSettings.EVCompensation.Value = 0;
        m_GlobalIspSettings.EVCompensation.Mode = KSCAMERA_EXTENDEDPROP_EVCOMP_FULLSTEP;
        m_GlobalIspSettings.WhiteBalanceSetting.VideoProc.Value.ul = WhiteBalanceDefault;
        m_GlobalIspSettings.WhiteBalanceSetting.Mode = KSCAMERA_EXTENDEDPROP_WHITEBALANCE_TEMPERATURE;
        m_GlobalIspSettings.WhiteBalanceMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureMode = KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
        m_GlobalIspSettings.ExposureSetting.VideoProc.Value.ul = DEF_EXPOSURE_TIME;
        m_GlobalIspSettings.ExposureSetting.Mode = 0;
        m_GlobalIspSettings.FocusMode = KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS | KSCAMERA_EXTENDEDPROP_FOCUS_RANGE_FULLRANGE;
        m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul = 100;
        break;
    default:
        break;
    }
    DBG_TRACE("ExposureMode=0x%016llX, ExposureTime=%lld00ns",
              m_GlobalIspSettings.ExposureMode, m_GlobalIspSettings.ExposureSetting.VideoProc.Value.ll);
    return status;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSPRIORITY
NTSTATUS
CSensorSimulation::
GetFocusPriority(
    _Inout_ CExtendedProperty *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    *pProperty = CExtendedProperty(m_GlobalIspSettings.FocusPriority);
    DBG_TRACE("Is %s", m_GlobalIspSettings.FocusPriority ? "On" : "Off");
    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSPRIORITY
NTSTATUS
CSensorSimulation::
SetFocusPriority(
    _In_    CExtendedProperty *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    m_GlobalIspSettings.FocusPriority = pProperty->Flags;
    return STATUS_SUCCESS;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_VIDEOHDR.
NTSTATUS
CSensorSimulation::
GetVideoHDR(
    _Inout_ CExtendedProperty *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    pProperty->Flags = m_VideoHDR;
    pProperty->Capability = KSCAMERA_EXTENDEDPROP_VIDEOHDR_OFF |
                            KSCAMERA_EXTENDEDPROP_VIDEOHDR_ON  |
                            KSCAMERA_EXTENDEDPROP_VIDEOHDR_AUTO;
    pProperty->PinId = m_VideoIndex;
    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_VIDEOHDR.
NTSTATUS
CSensorSimulation::
SetVideoHDR(
    _In_    CExtendedProperty *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    m_VideoHDR = pProperty->Flags;
    return STATUS_SUCCESS;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_VFR.
NTSTATUS
CSensorSimulation::
GetVFR(
    _Inout_ CExtendedProperty *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    *pProperty = CExtendedProperty( m_VFR );
    pProperty->PinId = m_VideoIndex;
    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_VFR.
NTSTATUS
CSensorSimulation::
SetVFR(
    _In_    CExtendedProperty *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    m_VFR = pProperty->Flags;
    return STATUS_SUCCESS;
}

//  KSPROPERTY_CAMERACONTROL_EXTENDED_ZOOM Get handler
NTSTATUS
CSensorSimulation::
GetZoom(
    _Inout_ CExtendedVidProcSetting *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    *pProperty = CExtendedVidProcSetting(m_ExtendedZoom);
    pProperty->Capability = KSCAMERA_EXTENDEDPROP_ZOOM_DEFAULT |
                            KSCAMERA_EXTENDEDPROP_ZOOM_DIRECT |
                            KSCAMERA_EXTENDEDPROP_ZOOM_SMOOTH ;

    //  Update the zoom factor.
    //
    //  I am using the same pFilter->m_Zoom value here just to keep the extended and legacy
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
    //
    DBG_TRACE("Legacy Zoom Value = %d", m_Zoom.Value);

    // To ensure we clear out the high order value in the VideoProc
    // union, we'll assign the LONG to LONGLONG field.
    pProperty->m_Setting.VideoProc.Value.ll = (LONG) (TO_Q16(m_Zoom.Value) / m_FocalLength.lOcularFocalLength);

    DBG_TRACE("Extended Zoom Value = %d", pProperty->GetLONG());

    return STATUS_SUCCESS;
}

//  KSPROPERTY_CAMERACONTROL_EXTENDED_ZOOM Set handler
NTSTATUS
CSensorSimulation::
SetZoom(
    _In_    CExtendedVidProcSetting *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    LONG Zoom = pProperty->GetLONG();
    DBG_TRACE("Extended Zoom Value = %d", Zoom);

    m_ExtendedZoom = *pProperty;
    DBG_TRACE("Zoom Flags = 0x%016llX", m_ExtendedZoom.Flags);

    //  Take us directly to the position if we're doing a direct zoom.
    //  Depends on IHVs, KSCAMERA_EXTENDEDPROP_ZOOM_DEFAULT may be either direct zoom or
    //  smooth zoom. Here direct zoom will be performed when DEFAULT is specified.
    if( pProperty->Flags != KSCAMERA_EXTENDEDPROP_ZOOM_SMOOTH )
    {
        //  Convert back to legacy notation...
        //
        //  I am using the same pFilter->m_Zoom value here just to keep the extended and legacy
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
        //
        m_Zoom.Value = FROM_Q16(Zoom, m_FocalLength.lOcularFocalLength);
        DBG_TRACE("Legacy Zoom Value = %d", m_Zoom.Value);
    }

    //  It's always success; this is a sync control, so this isn't really used.
    m_ExtendedZoom.Result = ntStatus;

    DBG_LEAVE("()=0x%08X", ntStatus);
    return ntStatus;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_VIDEOSTABILIZATION.
NTSTATUS
CSensorSimulation::
GetVideoStabilization(
    _Inout_    CExtendedProperty *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    *pProperty = CExtendedProperty(m_VideoStabilization);
    pProperty->Capability = KSCAMERA_EXTENDEDPROP_VIDEOSTABILIZATION_OFF |
                            KSCAMERA_EXTENDEDPROP_VIDEOSTABILIZATION_ON |
                            KSCAMERA_EXTENDEDPROP_VIDEOSTABILIZATION_AUTO;
    pProperty->PinId = m_VideoIndex;

    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_VIDEOSTABILIZATION.
NTSTATUS
CSensorSimulation::
SetVideoStabilization(
    _In_    CExtendedProperty *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    m_VideoStabilization = pProperty->Flags;
    return STATUS_SUCCESS;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_OIS.
NTSTATUS
CSensorSimulation::
GetOpticalImageStabilization(
    _Inout_    CExtendedProperty *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    *pProperty = CExtendedProperty(m_OpticalImageStabilization);
    pProperty->Capability = KSCAMERA_EXTENDEDPROP_OIS_OFF |
                            KSCAMERA_EXTENDEDPROP_OIS_ON  |
                            KSCAMERA_EXTENDEDPROP_OIS_AUTO;

    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_OIS.
NTSTATUS
CSensorSimulation::
SetOpticalImageStabilization(
    _In_    CExtendedProperty *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    m_OpticalImageStabilization = pProperty->Flags;
    return STATUS_SUCCESS;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_ADVANCEDPHOTO.
NTSTATUS
CSensorSimulation::
GetAdvancedPhoto(
    _Inout_    CExtendedProperty *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    *pProperty = CExtendedProperty(m_AdvancedPhoto);
    pProperty->PinId = GetStillIndex();
    pProperty->Capability = KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_OFF          |
                            KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_AUTO         |
                            KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_HDR          |
                            KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_FNF          |
                            KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_ULTRALOWLIGHT;
    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_ADVANCEDPHOTO.
NTSTATUS
CSensorSimulation::
SetAdvancedPhoto(
    _In_    CExtendedProperty *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    m_AdvancedPhoto = pProperty->Flags;
    return STATUS_SUCCESS;
}

//  Get [legacy] KSPROPERTY_CAMERACONTROL_FOCUS
NTSTATUS
CSensorSimulation::
GetFocus(
    _Inout_ PKSPROPERTY_CAMERACONTROL_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    pKsCameraControl->Capabilities = KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL |
                                     KSPROPERTY_CAMERACONTROL_FLAGS_AUTO |
                                     KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE;
    pKsCameraControl->Flags = KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE;
    if (m_GlobalIspSettings.FocusMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO)
    {
        pKsCameraControl->Flags |= KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    }
    else if (m_GlobalIspSettings.FocusMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL)
    {
        pKsCameraControl->Flags |= KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL;
    }
    pKsCameraControl->Value = m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul;

    return STATUS_SUCCESS;
}

//  Set [legacy] KSPROPERTY_CAMERACONTROL_FOCUS
NTSTATUS
CSensorSimulation::
SetFocus(
    _In_    PKSPROPERTY_CAMERACONTROL_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL)
    {
        //  Convert relative values to absolute.
        if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_RELATIVE)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
        }

        if (NT_SUCCESS(ntStatus))
        {
            //  We report these changes in the next metadata payload.
            m_GlobalIspSettings.FocusSetting.VideoProc.Value.ul = pKsCameraControl->Value;
            m_GlobalIspSettings.FocusMode =
                KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL;

            m_FocusNotifier = nullptr;
            m_FocusState = KSCAMERA_EXTENDEDPROP_FOCUSSTATE_SEARCHING;

            //  Fire a one-shot timer to transition the focus state.
            m_FocusTimer->Set(
                -330000LL,  //  33ms
                FocusConvergence,
                this);
        }

    }
    else if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_AUTO)
    {
        m_GlobalIspSettings.FocusMode = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO | KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK;

        m_FocusNotifier = nullptr;
        m_FocusState = KSCAMERA_EXTENDEDPROP_FOCUSSTATE_SEARCHING;

        //  Fire a one-shot timer to transition the focus state.
        m_FocusTimer->Set(
            -4000000LL,   //  400ms
            FocusConvergence,
            this );
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

//  Get [legacy] KSPROPERTY_CAMERACONTROL_EXPOSURE
NTSTATUS
CSensorSimulation::
GetExposure(
    _Inout_ PKSPROPERTY_CAMERACONTROL_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    pKsCameraControl->Capabilities = KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL |
                                     KSPROPERTY_CAMERACONTROL_FLAGS_AUTO |
                                     KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE;
    pKsCameraControl->Flags = KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE;
    if (m_GlobalIspSettings.ExposureMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO)
    {
        pKsCameraControl->Flags |= KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    }
    else if (m_GlobalIspSettings.ExposureMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL)
    {
        pKsCameraControl->Flags |= KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL;
    }

    pKsCameraControl->Value = Exposure100nsToBinaryLog(m_GlobalIspSettings.ExposureSetting.VideoProc.Value.ll);

    DBG_TRACE("ExposureMode=0x%016llX, ExposureTime=%lld00ns",
              m_GlobalIspSettings.ExposureMode, m_GlobalIspSettings.ExposureSetting.VideoProc.Value.ll);

    return STATUS_SUCCESS;
}

//  Set [legacy] KSPROPERTY_CAMERACONTROL_EXPOSURE
NTSTATUS
CSensorSimulation::
SetExposure(
    _In_    PKSPROPERTY_CAMERACONTROL_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL)
    {
        //  Convert relative values to absolute.
        if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_RELATIVE || !ExposureBinaryLogIsValid(pKsCameraControl->Value))
        {
            ntStatus = STATUS_INVALID_PARAMETER;
        }

        if (NT_SUCCESS(ntStatus))
        {
            //  We report these changes in the next metadata payload
            m_GlobalIspSettings.ExposureSetting.VideoProc.Value.ll = ExposureBinaryLogTo100ns(pKsCameraControl->Value);
            m_GlobalIspSettings.ExposureMode =
                KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL;
        }
    }
    else if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_AUTO)
    {
        m_GlobalIspSettings.ExposureMode = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO | KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK;

    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    DBG_TRACE("ExposureMode=0x%016llX, ExposureTime=%lld00ns",
              m_GlobalIspSettings.ExposureMode, m_GlobalIspSettings.ExposureSetting.VideoProc.Value.ll);

    return ntStatus;
}

//  Update the zoom factor over time.
void
CSensorSimulation::
SmoothZoom()
{
    PAGED_CODE();

#define ZOOM_SPEED (LONG) (TO_Q16(1) * 0.07f)
#define ZOOM_SPEED_VIDEO_RECORDING (LONG) (TO_Q16(1) * 0.02f)
#define ZOOM_EPSILON (LONG)(TO_Q16(1) * 0.005f)

    LONG    currentZoom = (LONG)
                          (TO_Q16(m_Zoom.Value) /
                           m_FocalLength.lOcularFocalLength);
    LONG    targetZoom = m_ExtendedZoom.GetLONG();

    LONG    ZoomDelta = abs(currentZoom - targetZoom);

    // Jump to target if we are very close
    if( ZoomDelta <= ZOOM_EPSILON )
    {
        currentZoom = targetZoom;
    }
    else
    {
        // Zoom direction
        bool    bZoomIn = (targetZoom > currentZoom);
        LONG    OpModeZoomSpeed = ZOOM_SPEED;

        // User lower zoom speed for video.
        if( INVALID_PIN_INDEX != m_VideoIndex &&
                PinRunning == m_HardwareSimulation[m_VideoIndex]->GetState() )
        {
            OpModeZoomSpeed = ZOOM_SPEED_VIDEO_RECORDING;
        }

        // Calculate zoom step that is relative to current zoom factor.
        // the bigger the current zoom factor the bigger the step.
        LONG    ZoomStep;
        if( bZoomIn )
        {
            ZoomStep = MULT_Q16(OpModeZoomSpeed, currentZoom);
        }
        else
        {
            ZoomStep = currentZoom - DIV_Q16(currentZoom, (TO_Q16(1) + OpModeZoomSpeed));
        }

        if( ZoomDelta < 2 * ZoomStep)
        {
            // decrease step when close to target
            ZoomStep = MULT_Q16(ZoomDelta, ((LONG)(TO_Q16(1) * 0.4f)));
        }

        if (bZoomIn)
        {
            currentZoom += ZoomStep;
        }
        else
        {
            currentZoom -= ZoomStep;
        }
    }

    //  Convert back to the zoom factor.
    m_Zoom.Value = FROM_Q16(currentZoom, m_FocalLength.lOcularFocalLength);

    if (!NT_SUCCESS(BoundsCheckSigned(m_Zoom.Value, ZoomRangeAndStep[0])))
    {
        m_Zoom.Value = FROM_Q16(targetZoom, m_FocalLength.lOcularFocalLength);
    }

    DBG_LEAVE(" m_Zoom.Value=%d, FocusMode=0x%016llX", m_Zoom.Value, m_GlobalIspSettings.FocusMode);
}


//  Adjust the zoom for each preview frame if the zoom is in motion.
//  This function is used by both the legacy relative zoom control
//  and the extended property smooth zoom control.
void
CSensorSimulation::
UpdateZoom(void)
{
    PAGED_CODE();

    //  Legacy zoom relative adjustment.
    LONG Value = m_Zoom.Value + m_ZoomRelative.Value;

    if (NT_SUCCESS(BoundsCheckSigned(Value, ZoomRangeAndStep[0])))
    {
        m_Zoom.Value = Value;
    }
    else
    {
        m_ZoomRelative.Value = 0;
    }

    //  Extended property zoom / smooth zoom adjustment.
    SmoothZoom();
}

//  Get [legacy] KSPROPERTY_CAMERACONTROL_ZOOM
NTSTATUS
CSensorSimulation::
GetZoom(
    _Inout_ PKSPROPERTY_CAMERACONTROL_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    pKsCameraControl->Capabilities = m_Zoom.Capabilities;
    pKsCameraControl->Flags = m_Zoom.Flags;
    pKsCameraControl->Value = m_Zoom.Value;

    return STATUS_SUCCESS;
}

//  Set [legacy] KSPROPERTY_CAMERACONTROL_ZOOM
NTSTATUS
CSensorSimulation::
SetZoom(
    _In_    PKSPROPERTY_CAMERACONTROL_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL)
    {
        //  Convert relative values to absolute.
        if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_RELATIVE)
        {
            pKsCameraControl->Value += m_Zoom.Value;
        }

        //  Bounds check
        ntStatus = BoundsCheckSigned(pKsCameraControl->Value, ZoomRangeAndStep[0]);
        if (NT_SUCCESS(ntStatus))
        {
            //  We report these changes in the next metadata payload.
            m_Zoom.Value = pKsCameraControl->Value;
            m_Zoom.Flags =
                KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL |
                KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE;
        }
    }
    else if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_AUTO)
    {
        m_Zoom.Flags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

//  Get [legacy] KSPROPERTY_CAMERACONTROL_ZOOM_RELATIVE
NTSTATUS
CSensorSimulation::
GetZoomRelative(
    _Inout_ PKSPROPERTY_CAMERACONTROL_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    pKsCameraControl->Capabilities = m_ZoomRelative.Capabilities;
    pKsCameraControl->Flags = m_ZoomRelative.Flags;
    pKsCameraControl->Value = m_ZoomRelative.Value;

    return STATUS_SUCCESS;
}

//  Set [legacy] KSPROPERTY_CAMERACONTROL_ZOOM_RELATIVE
NTSTATUS
CSensorSimulation::
SetZoomRelative(
    _In_    PKSPROPERTY_CAMERACONTROL_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL)
    {
        //  Convert relative values to absolute.
        if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_RELATIVE)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
        }
        else
        {
            //  Bounds check
            ntStatus = BoundsCheckSigned(pKsCameraControl->Value, ZoomRelativeRangeAndStep[0]);
        }

        if (NT_SUCCESS(ntStatus))
        {
            //  We report these changes in the next metadata payload.
            m_ZoomRelative.Value = pKsCameraControl->Value;
            m_ZoomRelative.Flags =
                KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL |
                KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE;
        }
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

//  Get KSPROPERTY_CAMERACONTROL_PAN
NTSTATUS
CSensorSimulation::
GetPan(
    _Inout_ PKSPROPERTY_CAMERACONTROL_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    pKsCameraControl->Capabilities = m_Pan.Capabilities;
    pKsCameraControl->Flags = m_Pan.Flags;
    pKsCameraControl->Value = m_Pan.Value;

    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_PAN
NTSTATUS
CSensorSimulation::
SetPan(
    _In_    PKSPROPERTY_CAMERACONTROL_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL)
    {
        //  Convert relative values to absolute.
        if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_RELATIVE)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
        }

        if (NT_SUCCESS(ntStatus))
        {
            //  Bounds check
            ntStatus = BoundsCheckSigned(pKsCameraControl->Value, PanRangeAndStep[0]);
            if (NT_SUCCESS(ntStatus))
            {
                //  We report these changes in the next metadata payload.
                m_Pan.Value = pKsCameraControl->Value;
                m_Pan.Flags =
                    KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL |
                    KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE;
            }
        }
    }
    else if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_AUTO)
    {
        m_Pan.Flags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

//  Get KSPROPERTY_CAMERACONTROL_ROLL
NTSTATUS
CSensorSimulation::
GetRoll(
    _Inout_ PKSPROPERTY_CAMERACONTROL_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    pKsCameraControl->Capabilities = m_Roll.Capabilities;
    pKsCameraControl->Flags = m_Roll.Flags;
    pKsCameraControl->Value = m_Roll.Value;

    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_ROLL
NTSTATUS
CSensorSimulation::
SetRoll(
    _In_    PKSPROPERTY_CAMERACONTROL_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL)
    {
        //  Convert relative values to absolute.
        if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_RELATIVE)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
        }
        if (NT_SUCCESS(ntStatus))
        {
            //  Bounds check
            ntStatus = BoundsCheckSigned(pKsCameraControl->Value, RollRangeAndStep[0]);
            if (NT_SUCCESS(ntStatus))
            {
                //  We report these changes in the next metadata payload.
                m_Roll.Value = pKsCameraControl->Value;
                m_Roll.Flags =
                    KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL |
                    KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE;
            }
        }
    }
    else if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_AUTO)
    {
        m_Roll.Flags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

//  Get KSPROPERTY_CAMERACONTROL_TILT
NTSTATUS
CSensorSimulation::
GetTilt(
    _Inout_ PKSPROPERTY_CAMERACONTROL_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    pKsCameraControl->Capabilities = m_Tilt.Capabilities;
    pKsCameraControl->Flags = m_Tilt.Flags;
    pKsCameraControl->Value = m_Tilt.Value;

    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_TILT
NTSTATUS
CSensorSimulation::
SetTilt(
    _In_    PKSPROPERTY_CAMERACONTROL_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL)
    {
        //  Convert relative values to absolute.
        if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_RELATIVE)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
        }
        if (NT_SUCCESS(ntStatus))
        {
            //  Bounds check
            ntStatus = BoundsCheckSigned(pKsCameraControl->Value, TiltRangeAndStep[0]);
            if (NT_SUCCESS(ntStatus))
            {
                //  We report these changes in the next metadata payload.
                m_Tilt.Value = pKsCameraControl->Value;
                m_Tilt.Flags =
                    KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL |
                    KSPROPERTY_CAMERACONTROL_FLAGS_ABSOLUTE;
            }
        }
    }
    else if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_AUTO)
    {
        m_Tilt.Flags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

//  Get KSPROPERTY_CAMERACONTROL_FOCAL_LENGTH
NTSTATUS
CSensorSimulation::
GetFocalLength(
    _Inout_ PKSPROPERTY_CAMERACONTROL_FOCAL_LENGTH_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    pKsCameraControl->lOcularFocalLength = m_FocalLength.lOcularFocalLength;
    pKsCameraControl->lObjectiveFocalLengthMax = m_FocalLength.lObjectiveFocalLengthMax;
    pKsCameraControl->lObjectiveFocalLengthMin = m_FocalLength.lObjectiveFocalLengthMin;

    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_FOCAL_LENGTH
NTSTATUS
CSensorSimulation::
SetFocalLength(
    _In_    PKSPROPERTY_CAMERACONTROL_FOCAL_LENGTH_S pKsCameraControl
)
{
    PAGED_CODE();
    return STATUS_NOT_FOUND;
}

//  Get KSPROPERTY_VIDEOPROCAMP_BACKLIGHT_COMPENSATION
NTSTATUS
CSensorSimulation::
GetBacklightCompensation(
    _Inout_ PKSPROPERTY_VIDEOPROCAMP_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    pKsCameraControl->Capabilities = m_BacklightCompensation.Capabilities;
    pKsCameraControl->Flags = m_BacklightCompensation.Flags;
    pKsCameraControl->Value = m_BacklightCompensation.Value;

    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_VIDEOPROCAMP_BACKLIGHT_COMPENSATION
NTSTATUS
CSensorSimulation::
SetBacklightCompensation(
    _In_    PKSPROPERTY_VIDEOPROCAMP_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL)
    {
        //  Bounds check
        ntStatus = BoundsCheckSigned(pKsCameraControl->Value, BacklightCompensationRangeAndStep[0]);
        if (NT_SUCCESS(ntStatus))
        {
            //  We report these changes in the next metadata payload.
            m_BacklightCompensation.Value = pKsCameraControl->Value;
            m_BacklightCompensation.Flags =
                KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
        }
    }
    else if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_AUTO)
    {
        m_BacklightCompensation.Flags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

//  Get KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
NTSTATUS
CSensorSimulation::
GetBrightness(
    _Inout_ PKSPROPERTY_VIDEOPROCAMP_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    pKsCameraControl->Capabilities = m_Brightness.Capabilities;
    pKsCameraControl->Flags = m_Brightness.Flags;
    pKsCameraControl->Value = m_Brightness.Value;

    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
NTSTATUS
CSensorSimulation::
SetBrightness(
    _In_    PKSPROPERTY_VIDEOPROCAMP_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL)
    {
        //  Bounds check
        ntStatus = BoundsCheckSigned(pKsCameraControl->Value, BrightnessRangeAndStep[0]);
        if (NT_SUCCESS(ntStatus))
        {
            //  We report these changes in the next metadata payload.
            m_Brightness.Value = pKsCameraControl->Value;
            m_Brightness.Flags =
                KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
        }
    }
    else if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_AUTO)
    {
        m_Brightness.Flags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

//  Get KSPROPERTY_VIDEOPROCAMP_CONTRAST
NTSTATUS
CSensorSimulation::
GetContrast(
    _Inout_ PKSPROPERTY_VIDEOPROCAMP_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    pKsCameraControl->Capabilities = m_Contrast.Capabilities;
    pKsCameraControl->Flags = m_Contrast.Flags;
    pKsCameraControl->Value = m_Contrast.Value;

    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_VIDEOPROCAMP_CONTRAST
NTSTATUS
CSensorSimulation::
SetContrast(
    _In_    PKSPROPERTY_VIDEOPROCAMP_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL)
    {
        //  Bounds check
        ntStatus = BoundsCheckSigned(pKsCameraControl->Value, ContrastRangeAndStep[0]);
        if (NT_SUCCESS(ntStatus))
        {
            //  We report these changes in the next metadata payload.
            m_Contrast.Value = pKsCameraControl->Value;
            m_Contrast.Flags =
                KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
        }
    }
    else if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_AUTO)
    {
        m_Contrast.Flags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

//  Get KSPROPERTY_VIDEOPROCAMP_HUE
NTSTATUS
CSensorSimulation::
GetHue(
    _Inout_ PKSPROPERTY_VIDEOPROCAMP_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    pKsCameraControl->Capabilities = m_Hue.Capabilities;
    pKsCameraControl->Flags = m_Hue.Flags;
    pKsCameraControl->Value = m_Hue.Value;

    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_VIDEOPROCAMP_HUE
NTSTATUS
CSensorSimulation::
SetHue(
    _In_    PKSPROPERTY_VIDEOPROCAMP_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL)
    {
        //  Bounds check
        ntStatus = BoundsCheckSigned(pKsCameraControl->Value, HueRangeAndStep[0]);
        if (NT_SUCCESS(ntStatus))
        {
            //  We report these changes in the next metadata payload.
            m_Hue.Value = pKsCameraControl->Value;
            m_Hue.Flags =
                KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
        }
    }
    else if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_AUTO)
    {
        m_Hue.Flags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

//  Get KSPROPERTY_VIDEOPROCAMP_POWERLINE_FREQUENCY
NTSTATUS
CSensorSimulation::
GetPowerlineFreq(
    _Inout_ PKSPROPERTY_VIDEOPROCAMP_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    pKsCameraControl->Capabilities = m_PowerLineFreq.Capabilities;
    pKsCameraControl->Flags = m_PowerLineFreq.Flags;
    pKsCameraControl->Value = m_PowerLineFreq.Value;

    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_VIDEOPROCAMP_POWERLINE_FREQUENCY
NTSTATUS
CSensorSimulation::
SetPowerlineFreq(
    _In_    PKSPROPERTY_VIDEOPROCAMP_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL)
    {
        //  Bounds check
        ntStatus = BoundsCheckSigned(pKsCameraControl->Value, PLFRangeAndStep[0]);
        if (NT_SUCCESS(ntStatus))
        {
            //  We report these changes in the next metadata payload.
            m_PowerLineFreq.Value = pKsCameraControl->Value;
            m_PowerLineFreq.Flags =
                KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
        }
    }
    else if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_AUTO)
    {
        m_PowerLineFreq.Flags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
        m_PowerLineFreq.Value = 0;
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

//  Get [legacy] KSPROPERTY_VIDEOPROCAMP_WHITEBALANCE
NTSTATUS
CSensorSimulation::
GetWhiteBalance(
    _Inout_ PKSPROPERTY_VIDEOPROCAMP_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);

    pKsCameraControl->Capabilities = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO | KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    if (m_GlobalIspSettings.WhiteBalanceMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO)
    {
        pKsCameraControl->Flags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    }
    else
    {
        pKsCameraControl->Flags = KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    }

    ULONG temperature = m_GlobalIspSettings.WhiteBalanceSetting.VideoProc.Value.ul;
    if (m_GlobalIspSettings.WhiteBalanceSetting.Mode != KSCAMERA_EXTENDEDPROP_WHITEBALANCE_TEMPERATURE)
    {
        temperature = WhiteBalancePreset2Temperature(m_GlobalIspSettings.WhiteBalanceSetting.VideoProc.Value.ul);
    }
    pKsCameraControl->Value = (LONG)temperature;

    return STATUS_SUCCESS;
}

//  Set [legacy] KSPROPERTY_VIDEOPROCAMP_WHITEBALANCE
NTSTATUS
CSensorSimulation::
SetWhiteBalance(
    _In_    PKSPROPERTY_VIDEOPROCAMP_S pKsCameraControl
)
{
    PAGED_CODE();
    KScopedMutex    lock(m_SensorMutex);
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL)
    {
        //  We report these changes in the next metadata payload.
        m_GlobalIspSettings.WhiteBalanceSetting.VideoProc.Value.ul = pKsCameraControl->Value;
        m_GlobalIspSettings.WhiteBalanceMode =
            KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL;
        m_GlobalIspSettings.WhiteBalanceSetting.Mode = KSCAMERA_EXTENDEDPROP_WHITEBALANCE_TEMPERATURE;
    }
    else if (pKsCameraControl->Flags & KSPROPERTY_CAMERACONTROL_FLAGS_AUTO)
    {
        m_GlobalIspSettings.WhiteBalanceMode =
            KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_HISTOGRAM.
NTSTATUS
CSensorSimulation::
GetHistogram(
    _Inout_ CExtendedProperty *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    *pProperty = CExtendedProperty( m_Histogram );
    pProperty->PinId = m_PreviewIndex;
    pProperty->Capability = 0;
    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_HISTOGRAM.
NTSTATUS
CSensorSimulation::
SetHistogram(
    _In_    CExtendedProperty *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    m_Histogram = pProperty->Flags;

    //  Adjust the reported metadata buffer size for preview.
    m_MetadataInfo[m_PreviewIndex] =
        CExtendedMetadata::METADATA_MAX +
        (( m_Histogram == KSCAMERA_EXTENDEDPROP_HISTOGRAM_ON ) ? sizeof(CAMERA_METADATA_HISTOGRAM) : 0 ) ;

    return STATUS_SUCCESS;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_METADATA
NTSTATUS
CSensorSimulation::
GetMetadata(
    _Inout_ CExtendedMetadata *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    *pProperty = m_MetadataInfo[pProperty->PinId];
    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_METADATA
NTSTATUS
CSensorSimulation::
SetMetadata(
    _In_    CExtendedMetadata *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    *pProperty = m_MetadataInfo[pProperty->PinId];
    return STATUS_SUCCESS;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_OPTIMIZATIONHINT
NTSTATUS
CSensorSimulation::
GetOptimizationHint(
    _Inout_ CExtendedProperty *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    *pProperty = m_OptimizationHint;
    pProperty->Capability =
        KSCAMERA_EXTENDEDPROP_OPTIMIZATION_PHOTO    |
        KSCAMERA_EXTENDEDPROP_OPTIMIZATION_VIDEO    |
        KSCAMERA_EXTENDEDPROP_OPTIMIZATION_QUALITY  |
        KSCAMERA_EXTENDEDPROP_OPTIMIZATION_LATENCY  |
        KSCAMERA_EXTENDEDPROP_OPTIMIZATION_POWER    |
        KSCAMERA_EXTENDEDPROP_OPTIMIZATION_DEFAULT;

    DBG_TRACE( "Hint = 0x%016llX", m_OptimizationHint.Flags );

    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_OPTIMIZATIONHINT
NTSTATUS
CSensorSimulation::
SetOptimizationHint(
    _In_    CExtendedProperty *pProperty
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    m_OptimizationHint = *pProperty;

    DBG_TRACE( "Hint = 0x%016llX", m_OptimizationHint.Flags );

    return STATUS_SUCCESS;
}

//  Get for PROPSETID_VIDCAP_CAMERACONTROL_VIDEO_STABILIZATION:0
NTSTATUS
CSensorSimulation::
GetVideoStabMode(
    _Inout_ KSPROPERTY_CAMERACONTROL_VIDEOSTABILIZATION_MODE_S *pMode
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    pMode->Capabilities =
        KSPROPERTY_CAMERACONTROL_VIDEOSTABILIZATION_MODE_FLAGS_AUTO |
        KSPROPERTY_CAMERACONTROL_VIDEOSTABILIZATION_MODE_FLAGS_MANUAL ;
    pMode->VideoStabilizationMode = m_VideoStabMode;
    return STATUS_SUCCESS;
}

//  Set for PROPSETID_VIDCAP_CAMERACONTROL_VIDEO_STABILIZATION:0
NTSTATUS
CSensorSimulation::
SetVideoStabMode(
    _In_    KSPROPERTY_CAMERACONTROL_VIDEOSTABILIZATION_MODE_S *pMode
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    m_VideoStabMode = pMode->VideoStabilizationMode;
    return STATUS_SUCCESS;
}

//  Get for KSPROPERTY_CAMERACONTROL_FLASH_PROPERTY_ID
NTSTATUS
CSensorSimulation::
GetFlash(
    _Inout_ KSPROPERTY_CAMERACONTROL_FLASH_S *pFlash
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    pFlash->Capabilities =
        KSPROPERTY_CAMERACONTROL_FLASH_FLAGS_AUTO |
        KSPROPERTY_CAMERACONTROL_FLASH_FLAGS_MANUAL ;
    pFlash->Flash = m_Flash;
    return STATUS_SUCCESS;
}

//  Set for KSPROPERTY_CAMERACONTROL_FLASH_PROPERTY_ID
NTSTATUS
CSensorSimulation::
SetFlash(
    _In_    KSPROPERTY_CAMERACONTROL_FLASH_S *pFlash
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    m_Flash = pFlash->Flash;
    return STATUS_SUCCESS;
}

//  Get for KSPROPERTY_CAMERACONTROL_IMAGE_PIN_CAPABILITY_PROPERTY_ID
NTSTATUS
CSensorSimulation::
GetPinDependence(
    _Inout_ KSPROPERTY_CAMERACONTROL_IMAGE_PIN_CAPABILITY_S *pCaps
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    //  Limit capabilities to the valid set.
    pCaps->Capabilities &=
        (   KSPROPERTY_CAMERACONTROL_IMAGE_PIN_CAPABILITY_EXCLUSIVE_WITH_RECORD |
            KSPROPERTY_CAMERACONTROL_IMAGE_PIN_CAPABILITY_SEQUENCE_EXCLUSIVE_WITH_RECORD );

    return STATUS_SUCCESS;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOTRIGGERTIME
NTSTATUS
CSensorSimulation::
GetTriggerTime(
    _Inout_ CExtendedProperty *pQPC
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    *pQPC = CExtendedProperty(0);
    pQPC->PinId = GetStillIndex();
    NTSTATUS Status = GetQPC( &pQPC->m_Value.Value.ull );

    pQPC->Flags = ( pQPC->m_Value.Value.ull != 0 ) ?
                  KSPROPERTY_CAMERA_PHOTOTRIGGERTIME_SET :
                  KSPROPERTY_CAMERA_PHOTOTRIGGERTIME_CLEAR;
    return Status;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOTRIGGERTIME
NTSTATUS
CSensorSimulation::
SetTriggerTime(
    _In_    CExtendedProperty *pQPC
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    ULONGLONG   Time = ( pQPC->Flags & KSPROPERTY_CAMERA_PHOTOTRIGGERTIME_SET ) ? *pQPC : 0;
    return SetQPC( Time );
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_TORCHMODE
NTSTATUS
CSensorSimulation::
GetTorchMode(
    _Inout_ CExtendedProperty *pTorchMode
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    *pTorchMode = m_TorchMode;
    pTorchMode->Capability =
        KSCAMERA_EXTENDEDPROP_VIDEOTORCH_OFF |
        KSCAMERA_EXTENDEDPROP_VIDEOTORCH_ON  |
        KSCAMERA_EXTENDEDPROP_VIDEOTORCH_ON_ADJUSTABLEPOWER;

    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_TORCHMODE
NTSTATUS
CSensorSimulation::
SetTorchMode(
    _In_    CExtendedProperty *pTorchMode
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    m_TorchMode = *pTorchMode;
    return STATUS_SUCCESS;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMODE
NTSTATUS
CSensorSimulation::
GetPhotoMode(
    _Inout_ CExtendedPhotoMode  *pPhotoMode
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    *pPhotoMode = m_PhotoMode;
    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMODE
NTSTATUS
CSensorSimulation::
SetPhotoModeAsync(
    _In_    CExtendedPhotoMode  *pPhotoMode,
    _In_    CNotifier           *Notifier            // Use to notify the framework that the control is done.
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    DBG_ENTER("()");

    m_PhotoMode = *pPhotoMode;

    NTSTATUS Status =
        SetPinMode( pPhotoMode->Flags, pPhotoMode->RequestedHistoryFrames() );

    //  Tell the caller that we're done.
    Notifier->Set();

    DBG_LEAVE("()=0x%08X",Status);
    return Status;
}

/*
 *  KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMAXFRAMERATE Get Handler
 */
NTSTATUS
CSensorSimulation::
GetPhotoMaxFrameRate(
    _Inout_ CExtendedProperty   *pPhotoMaxFrameRate
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    *pPhotoMaxFrameRate = m_MaxFrameRate;
    pPhotoMaxFrameRate->Capability = KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL;
    pPhotoMaxFrameRate->PinId = GetStillIndex();
    return STATUS_SUCCESS;
}

/*
 *  KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMAXFRAMERATE Set Handler
 */
NTSTATUS
CSensorSimulation::
SetPhotoMaxFrameRateAsync(
    _In_    CExtendedProperty   *pPhotoMaxFrameRate,
    _In_    CNotifier           *Notifier            // Use to notify the framework that the control is done.
)
{
    PAGED_CODE();

    ULONGLONG       TimePerFrame = 0;
    {
        //  Note: Don't lock around SetPhotoFrameRate() to avoid a priority inversion.
        KScopedMutex    lock( m_SensorMutex );

        m_MaxFrameRate = *pPhotoMaxFrameRate;

        //  Try to apply the setting immediately.
        LARGE_INTEGER   PerformanceTime = { m_MaxFrameRate.m_Value.Value.ratio.LowPart };
        LONGLONG        Frequency = m_MaxFrameRate.m_Value.Value.ratio.HighPart;

        if( Frequency != 0 )
        {
            TimePerFrame = KSCONVERT_PERFORMANCE_TIME( Frequency, PerformanceTime );
        }
    }

    DBG_TRACE( "SetPhotoFrameRate(%lld)", TimePerFrame );

    //  Consume any error; we'll set the rate again when the pin starts.
    SetPhotoFrameRate( TimePerFrame );

    //  Tell the caller that we're done.
    Notifier->Set();

    return STATUS_SUCCESS;
}

/*
 *  KSPROPERTY_CAMERACONTROL_EXTENDED_WARMSTART
 */
NTSTATUS
CSensorSimulation::
GetWarmStart(
    _Inout_ CExtendedProperty   *pWarmStart
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    *pWarmStart = m_WarmStartEnabled[pWarmStart->PinId];
    pWarmStart->Capability =
        KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL |
        KSCAMERA_EXTENDEDPROP_WARMSTART_MODE_DISABLED |
        KSCAMERA_EXTENDEDPROP_WARMSTART_MODE_ENABLED ;
    return STATUS_SUCCESS;
}

/*
 *  KSPROPERTY_CAMERACONTROL_EXTENDED_WARMSTART
 */
NTSTATUS
CSensorSimulation::
SetWarmStartAsync(
    _In_    CExtendedProperty   *pWarmStart,
    _In_    CNotifier           *Notifier            // Use to notify the framework that the control is done.
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    m_WarmStartEnabled[pWarmStart->PinId] = *pWarmStart;

    //  Tell the caller that we're done.
    Notifier->Set();

    return STATUS_SUCCESS;
}

/*
 *  KSPROPERTY_CAMERACONTROL_EXTENDED_MAXVIDFPS_PHOTORES Get Handler
 */
NTSTATUS
CSensorSimulation::
GetMaxVideoFpsForPhotoRes(
    _Inout_ CExtendedMaxVideoFpsForPhotoRes *pPhotoRes
)
{
    PAGED_CODE();

    //  We're just a simulation and have no constraints - set 30FPS.
    pPhotoRes->PreviewFPSNum() =30;
    pPhotoRes->PreviewFPSDenom() = 1;
    pPhotoRes->CaptureFPSNum() = 30;
    pPhotoRes->CaptureFPSDenom() = 1;

    return STATUS_SUCCESS;
}

/*
 *  KSPROPERTY_CAMERACONTROL_EXTENDED_MAXVIDFPS_PHOTORES Set Handler
 */
NTSTATUS
CSensorSimulation::
SetMaxVideoFpsForPhotoRes(
    _In_    CExtendedMaxVideoFpsForPhotoRes *pPhotoRes
)
{
    PAGED_CODE();

    //  We don't really use this call.
    return STATUS_SUCCESS;
}

//  Get KSPROPERTY_CAMERACONTROL_EXTENDED_FIELDOFVIEW
NTSTATUS
CSensorSimulation::
GetFieldOfView(
    _Inout_ CExtendedFieldOfView    *pFieldOfView
)
{
    PAGED_CODE();

    //  Report dummy values.
    *pFieldOfView = CExtendedFieldOfView();
    pFieldOfView->NormalizedFocalLengthX() = 35;
    pFieldOfView->NormalizedFocalLengthY() = 35;

    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_CAMERACONTROL_EXTENDED_FIELDOFVIEW
NTSTATUS
CSensorSimulation::
GetCameraAngleOffset(
    _Inout_ CExtendedCameraAngleOffset  *pAngle
)
{
    PAGED_CODE();

    //  Just return zeros.  We're dead-on, no tilt.
    *pAngle = CExtendedCameraAngleOffset();

    return STATUS_SUCCESS;
}

//  KSPROPERTY_CAMERACONTROL_EXTENDED_ROI_CONFIGCAPS Get handler
NTSTATUS
CSensorSimulation::
GetRoiConfigCaps(
    _Inout_ CRoiConfig  *pCaps
)
{
    PAGED_CODE();
    //  No state accessed...
    //KScopedMutex    lock( m_SensorMutex );

    //  Our CRoiConfig ctor defines our capabilities.
    *pCaps = CRoiConfig();

    return STATUS_SUCCESS;
}

//  Per Frame Settings Caps
struct SOC_PFS_CAPS
{
    KSCAMERA_PERFRAMESETTING_CAP_HEADER         Hdr;
    SOC_CAP_WITH_STEPPING_LONGLONG              ExposureTime;
    KSCAMERA_PERFRAMESETTING_CAP_ITEM_HEADER    Flash;
    SOC_CAP_WITH_STEPPING                       EVCompensation;
    SOC_CAP_WITH_STEPPING                       Iso;
    SOC_CAP_WITH_STEPPING                       Focus;
    KSCAMERA_PERFRAMESETTING_CAP_ITEM_HEADER    StillConfirmation;
} ;

const ULONG SOC_PFS_NUMCAPS = 6;

//  Our predefined Variable Photo Sequence capabilities.
SOC_PFS_CAPS g_PFSCaps =
{
    //  KSCAMERA_PERFRAMESETTING_CAP_HEADER
    { sizeof(SOC_PFS_CAPS), SOC_PFS_NUMCAPS, 0 },               //Hdr
    //  ExposureTime
    {
        //  SOC_CAP_WITH_STEPPING_LONGLONG
        {
            //  KSCAMERA_PERFRAMESETTING_CAP_ITEM_HEADER
            sizeof(g_PFSCaps.ExposureTime),                     //Size
            KSCAMERA_PERFRAMESETTING_ITEM_EXPOSURE_TIME,        //Type
            KSCAMERA_PERFRAMESETTING_AUTO                       //Flags
            | KSCAMERA_PERFRAMESETTING_MANUAL
        },
        {
            //  KSPROPERTY_STEPPING_LONGLONG
            1000,                   //SteppingDelta (in us)
            {
                //  KSPROPERTY_BOUNDS_LONGLONG
                MIN_EXPOSURE_TIME,  //SignedMinimum
                MAX_EXPOSURE_TIME   //SignedMaximum (arbitrary - 1 hour)
            }
        }
    },
    //  Flash
    {
        //  KSCAMERA_PERFRAMESETTING_CAP_ITEM_HEADER
        sizeof(g_PFSCaps.Flash),
        KSCAMERA_PERFRAMESETTING_ITEM_FLASH,
        KSCAMERA_EXTENDEDPROP_FLASH_OFF
        | KSCAMERA_EXTENDEDPROP_FLASH_ON
        | KSCAMERA_EXTENDEDPROP_FLASH_ON_ADJUSTABLEPOWER
        | KSCAMERA_EXTENDEDPROP_FLASH_AUTO
        | KSCAMERA_EXTENDEDPROP_FLASH_AUTO_ADJUSTABLEPOWER
        | KSCAMERA_EXTENDEDPROP_FLASH_REDEYEREDUCTION
    },
    //  ExposureCompensation
    {
        //  SOC_CAP_WITH_STEPPING
        {
            //  KSCAMERA_PERFRAMESETTING_CAP_ITEM_HEADER
            sizeof(g_PFSCaps.EVCompensation),                       //Size
            KSCAMERA_PERFRAMESETTING_ITEM_EXPOSURE_COMPENSATION,    //Type
            KSCAMERA_PERFRAMESETTING_AUTO                           //Flags
            | KSCAMERA_EXTENDEDPROP_EVCOMP_SIXTHSTEP
            | KSCAMERA_EXTENDEDPROP_EVCOMP_QUARTERSTEP
            | KSCAMERA_EXTENDEDPROP_EVCOMP_THIRDSTEP
            | KSCAMERA_EXTENDEDPROP_EVCOMP_HALFSTEP
            | KSCAMERA_EXTENDEDPROP_EVCOMP_FULLSTEP
        },
        {
            //  KSPROPERTY_STEPPING_LONG
            1,                      //SteppingDelta
            0,                      //Reserved
            {
                //  KSPROPERTY_BOUNDS_LONG
                -2,                 //SignedMinimum
                2                   //SignedMaximum
            }
        }
    },
    //  Iso
    {
        //  SOC_CAP_WITH_STEPPING
        {
            //  KSCAMERA_PERFRAMESETTING_CAP_ITEM_HEADER
            sizeof(g_PFSCaps.Iso),
            KSCAMERA_PERFRAMESETTING_ITEM_ISO,
            KSCAMERA_EXTENDEDPROP_ISO_AUTO
            | KSCAMERA_EXTENDEDPROP_ISO_MANUAL
        },
        {
            //  KSPROPERTY_STEPPING_LONG
            50,                     //SteppingDelta
            0,                      //Reserved
            {
                //  KSPROPERTY_BOUNDS_LONG
                50,                 //SignedMinimum
                256000              //SignedMaximum
            }
        }
    },
    //  Focus
    {
        //  SOC_CAP_WITH_STEPPING
        {
            //  KSCAMERA_PERFRAMESETTING_CAP_ITEM_HEADER
            sizeof(g_PFSCaps.Focus),                            //Size
            KSCAMERA_PERFRAMESETTING_ITEM_FOCUS,                //Type
            KSCAMERA_PERFRAMESETTING_AUTO                       //Flags
            | KSCAMERA_PERFRAMESETTING_MANUAL
        },
        {
            //  KSPROPERTY_STEPPING_LONG
            10,                     //SteppingDelta (We'll use centimeters for kicks)
            0,                      //Reserved
            {
                //  KSPROPERTY_BOUNDS_LONG
                0,                  //SignedMinimum
                10000               //SignedMaximum (100 meters max)
            }
        }
    },
    //  StillConfirmation
    {
        //  KSCAMERA_PERFRAMESETTING_CAP_ITEM_HEADER
        sizeof(g_PFSCaps.StillConfirmation),
        KSCAMERA_PERFRAMESETTING_ITEM_PHOTOCONFIRMATION,
        0
    },
};

//  Get KSPROPERTY_CAMERACONTROL_PERFRAMESETTING_CAPABILITY
_Success_(return == 0)
NTSTATUS
CSensorSimulation::
GetPfsCaps(
    _Inout_opt_ KSCAMERA_PERFRAMESETTING_CAP_HEADER *Caps,
    _Inout_     ULONG                               *Size
)
{
    PAGED_CODE();

    if( Caps )
    {
        size_t  Length = sizeof(g_PFSCaps);
        if( Size && *Size < Length )
        {
            Length = *Size;
        }
        RtlCopyMemory( Caps, &g_PFSCaps, Length );
    }

    *Size = sizeof(g_PFSCaps);

    return STATUS_SUCCESS;
}

//  KSPROPERTY_CAMERACONTROL_EXTENDED_ROI_ISPCONTROL Get handler
NTSTATUS
CSensorSimulation::
GetRoi(
    _Inout_ CRoiProperty    *pRoi
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    //  *pRoiProperty = ...
    *pRoi = CRoiProperty();     // initialize it.

    //  Add control properties to it.
    pRoi->Add( &m_RoiWhiteBalance ) ;
    pRoi->Add( &m_RoiExposureMode ) ;
    pRoi->Add( &m_RoiFocusMode ) ;

    //  Log what we created.
    pRoi->Log();

    return STATUS_SUCCESS;
}

//  KSPROPERTY_CAMERACONTROL_EXTENDED_ROI_ISPCONTROL Set handler
NTSTATUS
CSensorSimulation::
SetRoiAsync(
    _In_    CRoiProperty    *pRoi,
    _In_    CNotifier       *Notifier
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    if( pRoi->GetControlCount() == 0 )
    {
        //  Clear all settings.
        DBG_TRACE("Clearing all settings...");
        m_RoiWhiteBalance = CWhiteBalanceRoiIspControl();
        m_RoiExposureMode = CExposureRoiIspControl();
        m_RoiFocusMode    = CFocusRoiIspControl();
    }
    else
    {
        //  Parse our settings.
        DBG_TRACE("Parsing new settings...");
        if( m_RoiWhiteBalance.
                Init( pRoi, KSPROPERTY_CAMERACONTROL_EXTENDED_WHITEBALANCEMODE ) )
        {
            m_GlobalIspSettings.WhiteBalanceMode = m_RoiWhiteBalance.GetFlags();
            m_RoiWhiteBalance.Log();
        }
        if( m_RoiExposureMode.
                Init( pRoi, KSPROPERTY_CAMERACONTROL_EXTENDED_EXPOSUREMODE ) )
        {
            m_GlobalIspSettings.ExposureMode = m_RoiExposureMode.GetFlags();
            m_RoiExposureMode.Log();
        }
        if( m_RoiFocusMode.
                Init( pRoi, KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSMODE ) )
        {
            ULONGLONG   Flags = m_RoiFocusMode.GetFlags();
            DBG_TRACE("Requesting FocusMode = 0x%016llX", Flags);

            if( Flags )
            {
                //  If we've been asked to unlock, remove any lock flags.
                if( Flags & KSCAMERA_EXTENDEDPROP_FOCUS_UNLOCK )
                {
                    //  Change our operation so we go back to the previous state.
                    Flags = m_GlobalIspSettings.FocusMode &
                            ~( KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK |
                               KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUSLOCK );
                }

                //  Handle the stand-alone lock case by setting the appropriate flags.
                if( ( Flags &
                        (KSCAMERA_EXTENDEDPROP_FOCUS_MODE_MASK |
                         KSCAMERA_EXTENDEDPROP_FOCUS_MODE_ADVANCED_MASK) ) ==
                        KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK )
                {
                    if( m_GlobalIspSettings.FocusMode & KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO )
                    {
                        Flags =
                            m_GlobalIspSettings.FocusMode | KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_LOCK;
                    }
                    if( m_GlobalIspSettings.FocusMode & KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUS )
                    {
                        Flags =
                            m_GlobalIspSettings.FocusMode |= KSCAMERA_EXTENDEDPROP_FOCUS_CONTINUOUSLOCK;
                    }
                }
                //  Note: We do not simulate deferred completion.
                m_FocusState = KSCAMERA_EXTENDEDPROP_FOCUSSTATE_FOCUSED;

                //  Record the Focus mode.
                DBG_TRACE("Recording FocusMode = 0x%016llX", Flags);
                m_GlobalIspSettings.FocusMode = Flags;
            }

            m_RoiFocusMode.Log();
        }
    }

    Notifier->Set();
    return STATUS_SUCCESS;
}

//  KSPROPERTY_CAMERACONTROL_EXTENDED_ROI_ISPCONTROL size handler
ULONG
CSensorSimulation::
SizeOfRoi()
{
    PAGED_CODE();

    return
        sizeof( CRoiProperty ) +
        m_RoiWhiteBalance.GetSize() +
        m_RoiExposureMode.GetSize() +
        m_RoiFocusMode.GetSize() ;
}


//  Get KSPROPERTY_VIDEOCONTROL_MODE.
NTSTATUS
CSensorSimulation::
GetVideoControlMode(
    _Inout_ KSPROPERTY_VIDEOCONTROL_MODE_S  *pMode
)
{
    PAGED_CODE();
    KScopedMutex    lock( m_SensorMutex );

    pMode->Mode =
        KS_VideoControlFlag_IndependentImagePin |
        KS_VideoControlFlag_Trigger |
        KS_VideoControlFlag_StartPhotoSequenceCapture |
        KS_VideoControlFlag_StopPhotoSequenceCapture;

    return STATUS_SUCCESS;
}

//  Set KSPROPERTY_VIDEOCONTROL_MODE.
NTSTATUS
CSensorSimulation::
SetVideoControlMode(
    _In_    KSPROPERTY_VIDEOCONTROL_MODE_S  *pMode
)
{
    PAGED_CODE();

    //  Note: Trigger will acquire a lock on the hwsim.
    return Trigger( pMode->StreamIndex, pMode->Mode );
}

