/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        filter.h

    Abstract:

        This file contains the filter level header for the base capture filter
        class.  It provides a wrapper on KSFILTER.  
        
        CCaptureFilter holds state information for the filter and contains
        implementations for standard camera controls.  These implementations
        validate arguments and capabilities of the control according to DDI
        definitions.  Once validated, control is passes to a CSensor object
        to manipulate the state of camera.

        CCaptureFilter holds a reference to any active CCapturePins that may 
        have been created on this filter.  The reference is used to obtain 
        the state of the pin and potentially modify the pin's allocator 
        framing.

        To add a new control, derive from CCaptureFilter and use one of the 
        DECLARE_PROPERTY_XXX() macros below to declare the property handlers.
        You can also use the DEFINE_PROP_ITEM_XXX() and DEFINE_STD_EVENT_ITEM()
        macros as aids in constructing a filter's automation table.

    History:

        created 3/12/2001

**************************************************************************/

extern KSPROPERTY_VALUES    ZoomRelativeValues;
extern KSPROPERTY_VALUES    ZoomValues;
extern KSPROPERTY_VALUES    PanValues;
extern KSPROPERTY_VALUES    RollValues;
extern KSPROPERTY_VALUES    TiltValues;
extern KSPROPERTY_VALUES    BacklightCompensationValues;
extern KSPROPERTY_VALUES    BrightnessValues;
extern KSPROPERTY_VALUES    ContrastValues;
extern KSPROPERTY_VALUES    HueValues;
extern KSPROPERTY_VALUES    PLFValues;
extern KSPROPERTY_VALUES    WhiteBalanceValues;
extern KSPROPERTY_VALUES    ExposureValues;
extern KSPROPERTY_VALUES    FocusValues;

class CCapturePin;
class CCaptureFilter
{
private:

    //
    // The AVStream filter object associated with this CCaptureFilter.
    //
    CCapturePin **m_pinArray;

protected:
    //
    //  Setup, memory allocation, etc.
    //
    NTSTATUS
    Initialize();

    //
    //  Cleanup
    //
    void
    Cleanup();

public:

    //
    //  setPin()
    //
    //  Attach a CCapturePin.
    //
    void
    setPin(
        _In_    CCapturePin *pPin,
        _In_    unsigned Id
    ) ;

    //
    //  getPin()
    //
    //  Query an attached CCapturePin.
    //
    CCapturePin *
    getPin(
        _In_    unsigned Id
    ) ;

    //
    //  CCaptureFilter()
    //
    //  Constructor.
    //
    CCaptureFilter (
        _In_    PKSFILTER Filter
    );

    //
    // ~CCaptureFilter():
    //
    // The capture filter destructor.
    //
    virtual
    ~CCaptureFilter ();

    //
    // DispatchCreate():
    //
    // This is the filter creation dispatch for the capture filter.  It
    // creates the CCaptureFilter object, associates it with the AVStream
    // object, and bags it for easy cleanup later.
    //
    static
    NTSTATUS
    DispatchCreate (
        _In_    PKSFILTER Filter,
        _In_    PIRP Irp
    );

    //
    // Close(): Called in response to IRP_MJ_CLOSE.
    //
    static
    NTSTATUS
    DispatchClose(
        _In_  PKSFILTER Filter,
        _In_  PIRP Irp
    );

    LARGE_INTEGER   m_StartTime;
    CCaptureDevice *m_pCaptureDevice;
    CSensor        *m_Sensor;
    PKSFILTER       m_pKSFilter;
    PKSCAMERA_PERFRAMESETTING_HEADER m_pPerFrameSettings;
    ULONG m_PFSSize;

    CNotifier   m_PhotoModeNotifier;
    CNotifier   m_PhotoMaxFrameRateNotifier;
    CNotifier   m_FocusNotifier;
    CNotifier   m_FocusRectNotifier;
    CNotifier   m_IsoNotifier;
    CNotifier   m_IsoAdvancedNotifier;
    CNotifier   m_EvCompensationNotifier;
    CNotifier   m_WhiteBalanceNotifier;
    CNotifier   m_ExposureNotifier;
    CNotifier   m_SceneModeNotifier;
    CNotifier   m_ThumbnailNotifier;
    CNotifier   m_WarmStartNotifier;
    CNotifier   m_RoiNotifier;
    CNotifier   m_ProfileNotifier;

    //
    //  The following static member template function is used to validate
    //  most KSPROPERTYs and thunk that call down to a handler that is a non-
    //  static member of CCaptureFilter (or a derived class).
    //
    //  Note: A separate instance of this thunking function is instantiated for 
    //        each control, so code space is not conserved.  However, performing 
    //        this logic in only one place ensures consistant behavior across 
    //        controls and reduces the chances for mistakes.  
    //
    template
    <typename OUTPUT,                       // type of data consumed by the handler.
     typename FILTER,
     NTSTATUS (FILTER::*pFun)(      // The local handling function.
         _Inout_ OUTPUT     *pOutput )>
    __declspec(code_seg("PAGE"))                // Put this function into paged code.
    static
    NTSTATUS
    Prop(
        _In_    PIRP pIrp,
        _In_    PKSPROPERTY pProperty,
        _Inout_ PVOID pData)
    {
        PAGED_CODE();
        UNREFERENCED_PARAMETER(pProperty);

        NTSTATUS Status = STATUS_SUCCESS;
        NT_ASSERT(pIrp);

        FILTER *pFilter = reinterpret_cast <FILTER *>(KsGetFilterFromIrp(pIrp)->Context);
        PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
        ULONG ulOutputBufferLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
        OUTPUT *pOutput = reinterpret_cast<OUTPUT *>(pData);

        if (ulOutputBufferLength == 0)
        {
            pIrp->IoStatus.Information = sizeof(OUTPUT);
            Status = STATUS_BUFFER_OVERFLOW;
        }
        else if (ulOutputBufferLength < sizeof(OUTPUT))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else if (pData && ulOutputBufferLength >= sizeof(OUTPUT))
        {
            Status = (pFilter->*pFun)( pOutput );
            if( NT_SUCCESS(Status) )
            {
                pIrp->IoStatus.Information = sizeof(OUTPUT);
            }
        }
        else
        {
            Status = STATUS_INVALID_PARAMETER;
        }

        return Status;
    }

    //  Get Per Frame Setting Caps
    static
    NTSTATUS
    GetPFSCaps(
        _In_    PIRP pIrp,
        _In_    PKSPROPERTY pProperty,
        _Inout_ PVOID pData
    );

    //  Get current Per Frame Settings
    static
    NTSTATUS
    GetPerFrameSettings(
        _In_    PIRP pIrp,
        _In_    PKSPROPERTY pProperty,
        _Inout_ PVOID pData
    );

    //  Set new Per Frame Settings
    static
    NTSTATUS
    SetPerFrameSettings(
        _In_    PIRP pIrp,
        _In_    PKSPROPERTY pProperty,
        _Inout_ PVOID pData
    );

    //  Clear Per Frame Settings
    static
    NTSTATUS
    ClearPerFrameSettings(
        _In_    PIRP pIrp,
        _In_    PKSPROPERTY pProperty,
        _Inout_ PVOID pData
    );

//
//  DECLARE_PROPERTY_XXX() macros.
//
//  The follow macros declare a property handler using a common naming 
//  convention.  This convention is unique to this sample code.  The 
//  convention is used again in the DEFINE_PROP_ITEM_XXX() macros during
//  construction of the automation tables.
//
//  Note: These definitions are non-static members of the filter.
//

//  Declare a property's GET handler.
#define DECLARE_PROPERTY_GET_HANDLER( type, name )      \
    NTSTATUS                                            \
    Get##name(                                          \
        _Inout_ type        *pOutput                    \
        );

//  Declare a property's SET handler.
#define DECLARE_PROPERTY_SET_HANDLER( type, name )      \
    NTSTATUS                                            \
    Set##name(                                          \
        _In_    type        *pOutput                    \
        );

//  Declare a property's GET and SET handlers.
#define DECLARE_PROPERTY_HANDLERS( type, name )         \
DECLARE_PROPERTY_GET_HANDLER( type, name )              \
DECLARE_PROPERTY_SET_HANDLER( type, name )

    //
    //  Declaration of all the standard control handlers that use fixed structures.
    //
    DECLARE_PROPERTY_HANDLERS( CExtendedPhotoMode, PhotoMode );
    DECLARE_PROPERTY_GET_HANDLER( CExtendedProperty, PhotoFrameRate );
    DECLARE_PROPERTY_HANDLERS( CExtendedProperty, PhotoMaxFrameRate );
    DECLARE_PROPERTY_HANDLERS( CExtendedProperty, WarmStart );
    DECLARE_PROPERTY_HANDLERS( CExtendedMaxVideoFpsForPhotoRes, MaxVideoFpsForPhotoRes );
    DECLARE_PROPERTY_GET_HANDLER( CExtendedFieldOfView, FieldOfView );
    DECLARE_PROPERTY_GET_HANDLER( CExtendedCameraAngleOffset, CameraAngleOffset );
    DECLARE_PROPERTY_HANDLERS( KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_S, FocusRect );
    DECLARE_PROPERTY_HANDLERS( CExtendedVidProcSetting, Focus );
    DECLARE_PROPERTY_GET_HANDLER( CExtendedProperty, FocusState );
    DECLARE_PROPERTY_HANDLERS( CExtendedProperty, FocusPriority );
    DECLARE_PROPERTY_HANDLERS( CExtendedProperty, ExtendedFlash )
    DECLARE_PROPERTY_HANDLERS( CExtendedProperty, Iso )
    DECLARE_PROPERTY_HANDLERS( CExtendedVidProcSetting, IsoAdvanced )
    DECLARE_PROPERTY_HANDLERS( CExtendedEvCompensation, EvCompensation )
    DECLARE_PROPERTY_HANDLERS( CExtendedVidProcSetting, WhiteBalance )
    DECLARE_PROPERTY_HANDLERS( CExtendedVidProcSetting, Exposure )
    DECLARE_PROPERTY_HANDLERS( CExtendedVidProcSetting, FaceDetection )
    DECLARE_PROPERTY_HANDLERS( CExtendedProperty, PhotoConfirmation )
    DECLARE_PROPERTY_HANDLERS( CExtendedProperty, SceneMode )
    DECLARE_PROPERTY_HANDLERS( CExtendedProperty, VFR )
    DECLARE_PROPERTY_HANDLERS( CExtendedProperty, VideoHDR )
    DECLARE_PROPERTY_HANDLERS( CExtendedVidProcSetting, Zoom )
    DECLARE_PROPERTY_HANDLERS( CExtendedProperty, VideoStabilization)
    DECLARE_PROPERTY_HANDLERS( CExtendedProperty, Histogram )
    DECLARE_PROPERTY_HANDLERS( CExtendedMetadata, Metadata )
    DECLARE_PROPERTY_HANDLERS( CExtendedProperty, OpticalImageStabilization )
    DECLARE_PROPERTY_HANDLERS( CExtendedProperty, OptimizationHint )
    DECLARE_PROPERTY_HANDLERS( CExtendedProperty, AdvancedPhoto )
    DECLARE_PROPERTY_HANDLERS( CExtendedProfile, CameraProfile )
    DECLARE_PROPERTY_HANDLERS( CExtendedProperty, Thumbnail )
    DECLARE_PROPERTY_HANDLERS( CExtendedProperty, TriggerTime )
    DECLARE_PROPERTY_HANDLERS( CExtendedProperty, TorchMode )

    DECLARE_PROPERTY_HANDLERS( KSPROPERTY_CAMERACONTROL_VIDEOSTABILIZATION_MODE_S, VideoStabMode )
    DECLARE_PROPERTY_HANDLERS( KSPROPERTY_CAMERACONTROL_FLASH_S, Flash )
    DECLARE_PROPERTY_GET_HANDLER( KSPROPERTY_CAMERACONTROL_IMAGE_PIN_CAPABILITY_S, PinDependence )

    DECLARE_PROPERTY_HANDLERS(KSPROPERTY_CAMERACONTROL_S, Exposure);
    DECLARE_PROPERTY_HANDLERS(KSPROPERTY_CAMERACONTROL_S, Focus);
    DECLARE_PROPERTY_HANDLERS(KSPROPERTY_CAMERACONTROL_S, Zoom);
    DECLARE_PROPERTY_HANDLERS(KSPROPERTY_CAMERACONTROL_S, ZoomRelative);
    DECLARE_PROPERTY_HANDLERS(KSPROPERTY_CAMERACONTROL_S, Pan);
    DECLARE_PROPERTY_HANDLERS(KSPROPERTY_CAMERACONTROL_S, Roll);
    DECLARE_PROPERTY_HANDLERS(KSPROPERTY_CAMERACONTROL_S, Tilt);
    DECLARE_PROPERTY_HANDLERS(KSPROPERTY_CAMERACONTROL_FOCAL_LENGTH_S, FocalLength);

    DECLARE_PROPERTY_HANDLERS(KSPROPERTY_VIDEOPROCAMP_S, BacklightCompensation);
    DECLARE_PROPERTY_HANDLERS(KSPROPERTY_VIDEOPROCAMP_S, Brightness);
    DECLARE_PROPERTY_HANDLERS(KSPROPERTY_VIDEOPROCAMP_S, Contrast);
    DECLARE_PROPERTY_HANDLERS(KSPROPERTY_VIDEOPROCAMP_S, Hue);
    DECLARE_PROPERTY_HANDLERS(KSPROPERTY_VIDEOPROCAMP_S, WhiteBalance);
    DECLARE_PROPERTY_HANDLERS(KSPROPERTY_VIDEOPROCAMP_S, PowerlineFreq);

    DECLARE_PROPERTY_GET_HANDLER( CRoiConfig, RoiConfigCaps );

    DECLARE_PROPERTY_HANDLERS(KSPROPERTY_VIDEOCONTROL_MODE_S, VideoControlMode);

    //
    //  The following are property handlers that use an atypical variable 
    //  length (not known at compile-time) parameter list.
    //
    //  Since we only have a few, declare them explicitly.
    //

    //  KSPROPERTY_CAMERACONTROL_EXTENDED_ROI_ISPCONTROL Get handler
    static
    NTSTATUS
    GetRoiIspControl(
        _In_    PIRP pIrp,
        _In_    PKSPROPERTY pProperty,
        _Inout_ PVOID pData
    );

    //  KSPROPERTY_CAMERACONTROL_EXTENDED_ROI_ISPCONTROL Set handler
    static
    NTSTATUS
    SetRoiIspControl(
        _In_    PIRP pIrp,
        _In_    PKSPROPERTY pProperty,
        _Inout_ PVOID pData);

    //  Make sure the PERFRAMESETTINGs passed to us are valid.
    _Success_(return == 0)
    NTSTATUS
    CCaptureFilter::ParsePFSBuffer(
        _In_reads_bytes_(BufferLimit)
        PKSCAMERA_PERFRAMESETTING_HEADER    pPFS,
        _In_    ULONG                               BufferLimit,
        _Outptr_opt_result_maybenull_
        ISP_FRAME_SETTINGS                **ppSettings );

    //  Helper function for finding our filter object.
    static
    CCaptureFilter *
    GetFilter(
        _In_    PKSPIN  Pin
    )
    {
        return reinterpret_cast <CCaptureFilter *>
               (KsPinGetParentFilter(Pin)->Context);
    }

    //  Accessor to convert a reference to a CCaptureFilter to a PKSFILTER.
    operator PKSFILTER()
    {
        return m_pKSFilter;
    }

    //  Get the name of the filter from the context structure passed in at initialization.
    LPCWSTR GetFilterName()
    {
        return m_pCaptureDevice->GetFilterName(m_pCaptureDevice->GetFilterIndex(m_pKSFilter));
    }

    //  Update the pin's allocator to a specific frame count.
    NTSTATUS
    UpdateAllocatorFraming( 
        _In_    ULONG PinId
    );

private:
    //  An arbitrary limit on Per-Frame settings.
    static
    const ULONG     MAX_FRAME_COUNT=512;

    // Filter level state information.
    KSCAMERA_EXTENDEDPROP_PROFILE   m_Profile;
};

//
//  Helper definitions for creating custom property sets
//
//  Note: Property automation tables require a statically defined accessor 
//        functions, however our properties functions are (mostly) implemented
//        as non-static member functions.  The following macros provide a
//        static member thunking function by invoking a member template 
//        function called Prop<>.  The Prop<> performs limited validation on
//        the buffer, provides back the size if the buffer size was 0, picks 
//        up a pointer to the filter through the IRP and calls the member
//        function that handles the property.
//

#define DEFINE_PROP_ITEM_NO_SET( T, ctrl, type, name )              \
    DEFINE_KSPROPERTY_ITEM(                                         \
        ctrl,                                                       \
        (&T::Prop<type, T, &T::Get##name>),                         \
        sizeof(KSPROPERTY),                                         \
        0,                                                          \
        NULL,                                                       \
        NULL, 0, NULL, NULL, 0                                      \
        )

#define DEFINE_PROP_ITEM_WITH_VALUES( T, ctrl, type, name, values )     \
    DEFINE_KSPROPERTY_ITEM(                                             \
        ctrl,                                                           \
        (&T::Prop<type, T, &T::Get##name>),                             \
        sizeof(KSPROPERTY),                                             \
        0,                                                              \
        (&T::Prop<type, T, &T::Set##name>),                             \
        values, 0, NULL, NULL, 0                                        \
        )

#define DEFINE_PROP_ITEM( T, ctrl, type, name )                         \
    DEFINE_PROP_ITEM_WITH_VALUES( T, ctrl, type, name, NULL )

//
//  A simplification for defining a KSEVENT.  We only need to specify one parameter.
//
#define DEFINE_STD_EVENT_ITEM(EVENTID)      \
    DEFINE_KSEVENT_ITEM                     \
    (                                       \
        EVENTID,                            \
        sizeof(KSEVENTDATA),                \
        0,                                  \
        NULL,                               \
        NULL,                               \
        NULL                                \
    )

//
//  A simplification for defining a KSPROPERTY_SET.  We only need to specify two parameters.
//
#define DEFINE_STD_PROPERTY_SET(SET, PROPERTIES)    \
    DEFINE_KSPROPERTY_SET(                          \
        &SET,                                       \
        SIZEOF_ARRAY(PROPERTIES),                   \
        PROPERTIES,                                 \
        0,                                          \
        NULL)

const LONGLONG MAX_EXPOSURE_TIME = 10000000ULL * 60 * 60;    // 1 min
const LONG MIN_EXPOSURE_TIME=10000;            // 1 ms
const LONG DEF_EXPOSURE_TIME=10000 * 30;       // 30 ms.
